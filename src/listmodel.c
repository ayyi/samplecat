
#include "config.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <gtk/gtk.h>
#ifdef USE_AYYI
#include "ayyi/ayyi.h"
#endif
#include "file_manager/support.h"

#include "typedefs.h"
#include "src/types.h"
#include "mimetype.h"
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
	                               G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING, G_TYPE_FLOAT, G_TYPE_STRING, G_TYPE_INT);
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
listmodel__add_result(SamplecatResult* result)
{
	g_return_if_fail(result);

	Sample* sample = NULL;
	if(!result->sample_rate){
		sample = sample_new_from_result(result);
		sample_get_file_sndfile_info(sample);
		result->sample_rate = sample->sample_rate;
		result->length = sample->length;
		result->channels = sample->channels;
	}

	char samplerate_s[32]; float samplerate = result->sample_rate; samplerate_format(samplerate_s, samplerate);
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

	GdkPixbuf* get_iconbuf_from_mimetype(char* mimetype)
	{
		GdkPixbuf* iconbuf = NULL;
		MIME_type* mime_type = mime_type_lookup(mimetype);
		if(mime_type){
			type_to_icon(mime_type);
			if (!mime_type->image) dbg(0, "no icon.");
			iconbuf = mime_type->image->sm_pixbuf;
		}
		return iconbuf;
	}

	//icon (only shown if the sound file is currently available):
	GdkPixbuf* iconbuf = result->online ? get_iconbuf_from_mimetype(result->mimetype) : NULL;

	GtkTreeIter iter;
	gtk_list_store_append(app.store, &iter);
	gtk_list_store_set(app.store, &iter, COL_ICON, iconbuf,
	                   COL_NAME, result->sample_name,
	                   COL_FNAME, result->dir,
	                   COL_IDX, result->idx,
	                   COL_MIMETYPE, result->mimetype,
	                   COL_KEYWORDS, keywords, 
	                   COL_PEAKLEVEL, result->peak_level,
	                   COL_OVERVIEW, result->overview, COL_LENGTH, length, COL_SAMPLERATE, samplerate_s, COL_CHANNELS, result->channels, 
	                   COL_NOTES, result->notes, COL_COLOUR, result->colour,
#ifdef USE_AYYI
	                   COL_AYYI_ICON, ayyi_icon,
#endif
	                   -1);

	GtkTreePath* treepath;
	if((treepath = gtk_tree_model_get_path(GTK_TREE_MODEL(app.store), &iter))){
		GtkTreeRowReference* row_ref = gtk_tree_row_reference_new(GTK_TREE_MODEL(app.store), treepath);
		gtk_tree_path_free(treepath);
		result->row_ref = row_ref;
		if(sample) sample->row_ref = result->row_ref;
	}

	if(!result->overview){
		if(result->row_ref){
			if(!mimestring_is_unsupported(result->mimetype)){
				if(!sample) sample = sample_new_from_result(result);
				dbg(2, "no overview: sending request: filename=%s", result->sample_name);

				request_overview(sample);
			}
		}
		else pwarn("cannot request overview without row_ref.");
	}
}


char*
listmodel__get_filename_from_id(int id)
{
	//result must be free'd by caller.

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

	gwarn("not found. %i", id);
	return NULL;
}


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


