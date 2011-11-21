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
#include "inspector.h"

extern struct _app app;
extern unsigned debug;


GtkListStore*
listmodel__new()
{
	return gtk_list_store_new(NUM_COLS, 
	 GDK_TYPE_PIXBUF,  // COL_ICON
 #ifdef USE_AYYI
	 GDK_TYPE_PIXBUF,  // COL_AYYI_ICON
 #endif
	 G_TYPE_INT,       // COL_IDX
	 G_TYPE_STRING,    // COL_NAME
	 G_TYPE_STRING,    // COL_FNAME
	 G_TYPE_STRING,    // COL_KEYWORDS
	 GDK_TYPE_PIXBUF,  // COL_OVERVIEW
	 G_TYPE_STRING,    // COL_LENGTH,
	 G_TYPE_STRING,    // COL_SAMPLERATE
	 G_TYPE_INT,       // COL_CHANNELS
	 G_TYPE_STRING,    // COL_MIMETYPE
	 G_TYPE_FLOAT,     // COL_PEAKLEVEL
	 G_TYPE_INT,       // COL_COLOUR
	 G_TYPE_POINTER    // COL_SAMPLEPTR
	 );
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
	GtkTreeIter iter;
	while(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(app.store), &iter)){
		GdkPixbuf* pixbuf = NULL;
		Sample* sample = NULL;
		gtk_tree_model_get(GTK_TREE_MODEL(app.store), &iter, COL_OVERVIEW, &pixbuf, -1);
		gtk_tree_model_get(GTK_TREE_MODEL(app.store), &iter, COL_SAMPLEPTR, &sample, -1);
		gtk_list_store_remove(app.store, &iter);
		if(pixbuf) g_object_unref(pixbuf);
		if(sample) sample_unref(sample);
	}
}

void
listmodel__add_result(Sample* result)
{
	if(!app.store) return;
	g_return_if_fail(result);

#if 1
	/* these has actualy been checked _before_ here
	 * but backend may 'inject' mime types. ?!
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

	char samplerate_s[32]; samplerate_format(samplerate_s, result->sample_rate);
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
			COL_FNAME,      result->sample_dir,
			COL_IDX,        result->id,
			COL_MIMETYPE,   result->mimetype,
			COL_KEYWORDS,   NSTR(result->keywords), 
			COL_PEAKLEVEL,  result->peaklevel,
			COL_OVERVIEW,   result->overview,
			COL_LENGTH,     length,
			COL_SAMPLERATE, samplerate_s,
			COL_CHANNELS,   result->channels, 
			COL_COLOUR,     result->colour_index,
#ifdef USE_AYYI
			COL_AYYI_ICON,  ayyi_icon,
#endif
			COL_SAMPLEPTR,  result,
			-1);

	GtkTreePath* treepath;
	if((treepath = gtk_tree_model_get_path(GTK_TREE_MODEL(app.store), &iter))){
		result->row_ref = gtk_tree_row_reference_new(GTK_TREE_MODEL(app.store), treepath);
		gtk_tree_path_free(treepath);
	}

	if(result->peaklevel==0 && result->row_ref){
		dbg(1, "recalucale peak");
		request_peaklevel(result);
	}
	if(!result->overview && result->row_ref){
		dbg(1, "regenerate overview");
		request_overview(result);
	}
	if((!result->ebur || !strlen(result->ebur)) && result->row_ref){
		dbg(1, "regenerate ebur128");
		request_ebur128(result);
	}
	sample_ref(result);
}

/* used to update information that was generated in
 * a backgound analysis process */
void
listmodel__update_result(Sample* sample, int what)
{
	if(!sample->row_ref || !gtk_tree_row_reference_valid(sample->row_ref)){
		/* this should never happen  --
		 * it may if the file is removed again before
		 * the background-process(es) completed
		 */
		dbg(0,"rowref not set");
	}

	switch (what) {
		case COL_OVERVIEW:
			backend.update_pixbuf(sample);
			//if (sample->pixbuf) backend.update_blob(sample->id, "pixbuf", pixbuf_to_blob(sample->pixbuf));
			if (sample->row_ref)
				listmodel__set_overview(sample->row_ref, sample->overview);
			break;
		case COL_PEAKLEVEL:
			backend.update_peaklevel(sample->id, sample->peaklevel);
			if (sample->row_ref)
				listmodel__set_peaklevel(sample->row_ref, sample->peaklevel);
			break;
		case COLX_EBUR:
			backend.update_ebur(sample->id, sample->ebur);
			break;
		default:
			dbg(0,"update for this type is not yet implemented"); 
			break;
	}

	if (sample->id == app.inspector->row_id) {
		inspector_set_labels(sample);
	}
}

/* used when user interactively changes meta-data listview.c/window.c,
 * or when modifying meta-data in the inspector.c (wrapped by
 * listmodel__update_by_rowref() below.
 * Also used  when user invokes an 'update' command - main.c */
gboolean
listmodel__update_by_ref(GtkTreeIter *iter, int what, void *data)
{
	gboolean rv = true;
	Sample *s=NULL;
	gtk_tree_model_get(GTK_TREE_MODEL(app.store), iter, COL_SAMPLEPTR, &s, -1);
	if (!s) {
		// THIS SHOULD NEVER HAPPEN!
		dbg(0, "FIXME: no sample data in list model!");
		return false;
	}
	sample_ref(s);

	switch (what) {
		case COL_ICON: // online/offline, mtime
			{
				gboolean online = s->online;
				backend.update_online(s->id, online, s->mtime);
				GdkPixbuf* iconbuf = NULL;
				if (online) {
					MIME_type* mime_type = mime_type_lookup(s->mimetype);
					type_to_icon(mime_type);
					if (!mime_type->image) dbg(0, "no icon.");
					iconbuf = mime_type->image->sm_pixbuf;
				}
				gtk_list_store_set(app.store, iter, COL_ICON, iconbuf, -1);
			}
			break;
		case COL_KEYWORDS:
			{
				if(!backend.update_keywords(s->id, (char*)data)) rv=false;
				else {
					gtk_list_store_set(app.store, iter, COL_KEYWORDS, (char*)data, -1);
					if (s->keywords) free(s->keywords);
					s->keywords=strdup((char*)data);
				}
			}
			break;
		case COLX_NOTES:
				if (!backend.update_notes(s->id, (char*)data)) rv=false;
				else {
					if (s->notes) free(s->notes);
					s->notes = strdup((char*)data);
				}
			break;
		case COL_COLOUR:
			{
				unsigned int colour_index = *((unsigned int*)data);
				if(!backend.update_colour(s->id, colour_index)) rv=false;
				else {
					gtk_list_store_set(app.store, iter, COL_COLOUR, colour_index, -1);
					s->colour_index=colour_index;
				}
			}
			break;
		default:
			dbg(0,"update for this type is not yet implemented"); 
			break;
	}

	if (s->id == app.inspector->row_id) {
		inspector_set_labels(s);
	}
	sample_unref(s);
	return rv;
}

gboolean
listmodel__update_by_rowref(GtkTreeRowReference *row_ref, int what, void *data)
{
	// used by the inspector..
	GtkTreePath *path;
	if(!(path = gtk_tree_row_reference_get_path(row_ref))) {
		/* this SHOULD never happen:
		 * it was possible before the inspector hid 
		 * internal meta-data for non sample, file-system files.
		 * The latter do not have a Sample -> row_ref.
		 * code may still buggy wrt that.
		 */
		perr("FIXME: cannot get row by refernce\n");
		return false;
	}

	GtkTreeIter iter;
	gtk_tree_model_get_iter(GTK_TREE_MODEL(app.store), &iter, path);
	gtk_tree_path_free(path);
	return listmodel__update_by_ref(&iter, what, data);
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


struct find_filename {
	int id;
	char *rv;
};

gboolean
filter_id (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data) {
	struct find_filename *ff = (struct find_filename*) data;
	Sample *s = sample_get_by_tree_iter(iter);
	if (s->id == ff->id) {
		ff->rv=strdup(s->full_path);
		sample_unref(s);
		return TRUE;
	}
	sample_unref(s);
	return FALSE;
}

char*
listmodel__get_filename_from_id(int id)
{
	struct find_filename ff;
	ff.id=id;
	ff.rv=NULL;
	GtkTreeModel* model = GTK_TREE_MODEL(app.store);
	gtk_tree_model_foreach(model, &filter_id, &ff);
	return ff.rv;
}
