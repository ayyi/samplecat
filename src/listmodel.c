#include "config.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <gtk/gtk.h>

#ifdef USE_AYYI
#include "ayyi/ayyi.h"
#endif

#include "typedefs.h"
#include "file_manager/mimetype.h"
#include "support.h"
#include "main.h"
#include "sample.h"
#include "dnd.h"
#include "pixmaps.h"
#include "overview.h"
#include "listmodel.h"

extern struct _app app;
extern unsigned debug;


GtkListStore*
listmodel__new()
{
	return gtk_list_store_new(NUM_COLS, GDK_TYPE_PIXBUF,
	                               #ifdef USE_AYYI
	                               GDK_TYPE_PIXBUF,
	                               #endif
	                               G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING, G_TYPE_FLOAT, G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING);
}


void
listmodel__update()
{
	do_search(NULL, NULL);
}


void
listmodel__clear()
{
	PF;

	//gtk_list_store_clear(app.store);

	GtkTreeIter iter;
	GdkPixbuf* pixbuf = NULL;

	while(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(app.store), &iter)){

		gtk_tree_model_get(GTK_TREE_MODEL(app.store), &iter, COL_OVERVIEW, &pixbuf, -1);

		gtk_list_store_remove(app.store, &iter);

		if(pixbuf) g_object_unref(pixbuf);
	}
}


void
listmodel__add_result(Sample* result)
{
	if(!app.store) return;
	g_return_if_fail(result);

#if 1
	/* this has actualy been checked _before_ here
	 * but backend may 'inject' mime types..
	 */
	if(!result->mimetype){
		dbg(0,"no mimetype given -- this should NOT happen: fix backend");
		return;
	}
	if(mimestring_is_unsupported(result->mimetype)){
		dbg(0, "unsupported MIME type: %s", result->mimetype);
		return;
	}
#endif

	if(!result->sample_rate){
		// needed w/ tracker backend.
		sample_get_file_info(result);
	}
#if 0
	if(!result->sample_name){
		result->sample_name= g_path_get_basename(result->full_path);
	}
	if(!result->dir){
		result->dir = g_path_get_dirname(result->full_path);
	}
#endif

	char samplerate_s[32]; samplerate_format(samplerate_s, result->sample_rate);
	char* keywords = result->keywords ? result->keywords : "";
	char length[64]; format_time_int(length, result->length);

#ifdef USE_AYYI
	GdkPixbuf* ayyi_icon = NULL;
#endif

#if 0
//#ifdef USE_AYYI
	//is the file loaded in the current Ayyi song?
	if(ayyi.got_song){
		gchar* fullpath = g_build_filename(row[MYSQL_DIR], sample_name, NULL);
		if(pool__file_exists(fullpath)) dbg(0, "exists"); else dbg(0, "doesnt exist");
		g_free(fullpath);
	}
//#endif
#endif
#define NSTR(X) (X?X:"")
	//icon (only shown if the sound file is currently available):
	GdkPixbuf* iconbuf = result->online ? get_iconbuf_from_mimetype(result->mimetype) : NULL;

	GtkTreeIter iter;
	gtk_list_store_append(app.store, &iter);
	gtk_list_store_set(app.store, &iter,
			COL_ICON,       iconbuf,
			COL_NAME,       result->sample_name,
			COL_FNAME,      result->dir,
			COL_IDX,        result->id,
			COL_MIMETYPE,   result->mimetype,
			COL_KEYWORDS,   NSTR(keywords), 
			COL_PEAKLEVEL,  result->peak_level,
			COL_OVERVIEW,   result->overview,
			COL_LENGTH,     length,
			COL_SAMPLERATE, samplerate_s,
			COL_CHANNELS,   result->channels, 
			COL_NOTES,      NSTR(result->notes),
			COL_COLOUR,     result->colour_index,
			COL_MISC,       NSTR(result->misc),
#ifdef USE_AYYI
			COL_AYYI_ICON,  ayyi_icon,
#endif
			-1);

	// XXX only create a row_reference IFF !result->overview
	GtkTreePath* treepath;
	if((treepath = gtk_tree_model_get_path(GTK_TREE_MODEL(app.store), &iter))){
		result->row_ref = gtk_tree_row_reference_new(GTK_TREE_MODEL(app.store), treepath);
		gtk_tree_path_free(treepath);
	}

	if(result->peak_level==0 && result->row_ref){
		dbg(0, "recalucale peak");
		request_peaklevel(result);
	}
	if(!result->overview && result->row_ref){
		dbg(0, "regenerate overview");
		request_overview(result);
	}
	/* gtk_list_store_set will only "copy"  G_TYPE_STRING or G_TYPE_BOXED.
	 * references to char* need to kept allocated!
	 */
	sample_ref(result);
}

#ifdef NEVER
/*
 * @return: result must be free'd by caller.
 */
char*
listmodel__get_filename_from_id(int id)
{
	GtkTreeIter iter;
	gtk_tree_model_get_iter_first(GTK_TREE_MODEL(app.store), &iter);
	int i;
	char* fname;
	char* path;
	int row = 0;
	do {
		gtk_tree_model_get(GTK_TREE_MODEL(app.store), &iter, COL_IDX, &i, COL_NAME, &fname, COL_FNAME, &path, -1);
		if(i == id){
			dbg(2, "found %s/%s", path, fname);
			return g_strdup_printf("%s/%s", path, fname);
		}
		row++;
	} while (gtk_tree_model_iter_next(GTK_TREE_MODEL(app.store), &iter));

	gwarn("not found. %i\n", id);
	return NULL;
}
#endif


void
listmodel__set_overview(GtkTreeRowReference* row_ref, GdkPixbuf* pixbuf)
{
	GtkTreePath* treepath;
	if((treepath = gtk_tree_row_reference_get_path(row_ref))){ //it's possible the row may no longer be visible.
		GtkTreeIter iter;
		if(gtk_tree_model_get_iter(GTK_TREE_MODEL(app.store), &iter, treepath)){
			if(GDK_IS_PIXBUF(pixbuf)){
				gtk_list_store_set(app.store, &iter, COL_OVERVIEW, pixbuf, -1);
			}else perr("pixbuf is not a pixbuf.\n");

		}else perr("failed to get row iter. row_ref=%p\n", row_ref);
		gtk_tree_path_free(treepath);
	} else dbg(2, "no path for rowref. row_ref=%p\n", row_ref); //this is not an error. The row may not be part of the current search.
}


void
listmodel__set_peaklevel(GtkTreeRowReference* row_ref, float level)
{
	GtkTreePath* treepath;
	if((treepath = gtk_tree_row_reference_get_path(row_ref))){ //it's possible the row may no longer be visible.
		GtkTreeIter iter;
		if(gtk_tree_model_get_iter(GTK_TREE_MODEL(app.store), &iter, treepath)){
			gtk_list_store_set(app.store, &iter, COL_PEAKLEVEL, level, -1);

		}else perr("failed to get row iter. row_ref=%p\n", row_ref);
		gtk_tree_path_free(treepath);
	} else dbg(2, "no path for rowref. row_ref=%p\n", row_ref); //this is not an error. The row may not be part of the current search.
}


