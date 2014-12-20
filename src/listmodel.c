/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2007-2014 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#include "config.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <gtk/gtk.h>

#include "debug/debug.h"
#ifdef USE_AYYI
#include "ayyi/ayyi.h"
#endif

#include "typedefs.h"
#include "utils/pixmaps.h"
#include "file_manager/mimetype.h"
#include "db/db.h"
#include "support.h"
#include "main.h"
#include "application.h"
#include "sample.h"
#include "overview.h"
#include "list_store.h"
#include "listmodel.h"

static void  listmodel__update              ();
static bool _listmodel__update_by_rowref    (GtkTreeRowReference*, int, void*);
static void _listmodel__update_by_tree_iter (GtkTreeIter*, int, void*);
static void  listmodel__set_overview        (GtkTreeRowReference*, GdkPixbuf*);
static void  listmodel__set_peaklevel       (GtkTreeRowReference*, float);


GtkListStore*
listmodel__new()
{
	void icon_theme_changed(Application* application, char* theme, gpointer data){ listmodel__update(); }
	g_signal_connect((gpointer)app, "icon-theme", G_CALLBACK(icon_theme_changed), NULL);

	void listmodel__sample_changed(SamplecatModel* m, Sample* sample, int prop, void* val, gpointer user_data)
	{
		dbg(1, "prop=%i %s", prop, samplecat_model_print_col_name(prop));
		switch(prop){
			case COL_ICON: // online/offline, mtime
				{
					GdkPixbuf* iconbuf = NULL;
					if (sample->online) {
						MIME_type* mime_type = mime_type_lookup(sample->mimetype);
						type_to_icon(mime_type);
						if (!mime_type->image) dbg(0, "no icon.");
						iconbuf = mime_type->image->sm_pixbuf;
					}
					//gtk_list_store_set(app->store, iter, COL_ICON, iconbuf, -1);
					_listmodel__update_by_rowref (sample->row_ref, prop, iconbuf);
				}
				break;
			case COL_KEYWORDS:
				if(sample->row_ref){
					_listmodel__update_by_rowref (sample->row_ref, prop, val);
					//gtk_list_store_set(app->store, iter, COL_KEYWORDS, (char*)val, -1);
				}
				break;
			case COL_OVERVIEW:
				if (sample->row_ref)
					listmodel__set_overview(sample->row_ref, sample->overview);
				break;
			case COL_COLOUR:
				_listmodel__update_by_rowref (sample->row_ref, prop, val);
				break;
			case COL_PEAKLEVEL:
				if (sample->row_ref) listmodel__set_peaklevel(sample->row_ref, sample->peaklevel);
				break;
			case COL_X_EBUR:
			case COL_X_NOTES:
				// nothing to do.
				break;
			case -1:
				{
					// TODO set other props too ?

					//char* metadata = sample_get_metadata_str(s);

					GtkTreePath* path = gtk_tree_row_reference_get_path(sample->row_ref);
					g_return_if_fail(path);

					GtkTreeIter iter;
					gtk_tree_model_get_iter(GTK_TREE_MODEL(app->store), &iter, path);
					gtk_tree_path_free(path);

					char samplerate_s[32]; samplerate_format(samplerate_s, sample->sample_rate);
					char length_s[64]; smpte_format(length_s, sample->length);
					gtk_list_store_set((GtkListStore*)app->store, &iter, 
							COL_CHANNELS, sample->channels,
							COL_SAMPLERATE, samplerate_s,
							COL_LENGTH, length_s,
							-1);
					dbg(1, "file info updated.");
					//if(metadata) g_free(metadata);
				}
				break;
			default:
				dbg(0, "property not handled");
				break;
		}
	}
	g_signal_connect((gpointer)app->model, "sample-changed", G_CALLBACK(listmodel__sample_changed), NULL);

	return (GtkListStore*)samplecat_list_store_new();
}


static void
listmodel__update()
{
	application_search();
}


void
listmodel__clear()
{
	PF;
	GtkTreeIter iter;
	while(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(app->store), &iter)){
		GdkPixbuf* pixbuf = NULL;
		Sample* sample = NULL;
		gtk_tree_model_get(GTK_TREE_MODEL(app->store), &iter, COL_OVERVIEW, &pixbuf, -1);
		gtk_tree_model_get(GTK_TREE_MODEL(app->store), &iter, COL_SAMPLEPTR, &sample, -1);
		gtk_list_store_remove(app->store, &iter);
		if(pixbuf) g_object_unref(pixbuf);
		if(sample) sample_unref(sample);
	}

	((SamplecatListStore*)app->store)->row_count = 0;
}


void
listmodel__add_result(Sample* sample)
{
	//TODO finish moving this into ListStore.add()

	if(!app->store) return;
	g_return_if_fail(sample);

#if 1
	/* these has actualy been checked _before_ here
	 * but backend may 'inject' mime types. ?!
	 */
	if(!sample->mimetype){
		dbg(0,"no mimetype given -- this should NOT happen: fix backend");
		return;
	}
	if(mimestring_is_unsupported(sample->mimetype)){
		dbg(0, "unsupported MIME type: %s", sample->mimetype);
		return;
	}
#endif

	if(!sample->sample_rate){
		// needed w/ tracker backend.
		sample_get_file_info(sample);
	}

	char samplerate_s[32]; samplerate_format(samplerate_s, sample->sample_rate);
	char length_s[64]; smpte_format(length_s, sample->length);

#ifdef USE_AYYI
	GdkPixbuf* ayyi_icon = NULL;

	//is the file loaded in the current Ayyi song?
	if(ayyi.got_shm){
		gchar* fullpath = g_build_filename(sample->sample_dir, sample->sample_name, NULL);
		if(ayyi_song__have_file(fullpath)){
			dbg(1, "sample is used in current project TODO set icon");
		} else dbg(2, "sample not used in current project");
		g_free(fullpath);
	}
#endif

#define NSTR(X) (X?X:"")
	//icon (only shown if the sound file is currently available):
	GdkPixbuf* iconbuf = sample->online ? get_iconbuf_from_mimetype(sample->mimetype) : NULL;

	GtkTreeIter iter;
	gtk_list_store_append(app->store, &iter);
	gtk_list_store_set(app->store, &iter,
			COL_ICON,       iconbuf,
			COL_NAME,       sample->sample_name,
			COL_FNAME,      sample->sample_dir,
			COL_IDX,        sample->id,
			COL_MIMETYPE,   sample->mimetype,
			COL_KEYWORDS,   NSTR(sample->keywords),
			COL_PEAKLEVEL,  sample->peaklevel,
			COL_OVERVIEW,   sample->overview,
			COL_LENGTH,     length_s,
			COL_SAMPLERATE, samplerate_s,
			COL_CHANNELS,   sample->channels, 
			COL_COLOUR,     sample->colour_index,
#ifdef USE_AYYI
			COL_AYYI_ICON,  ayyi_icon,
#endif
			COL_SAMPLEPTR,  sample,
			-1);

	GtkTreePath* treepath;
	if((treepath = gtk_tree_model_get_path(GTK_TREE_MODEL(app->store), &iter))){
		sample->row_ref = gtk_tree_row_reference_new(GTK_TREE_MODEL(app->store), treepath);
		gtk_tree_path_free(treepath);
	}

	if(sample->peaklevel==0 && sample->row_ref){
		dbg(1, "recalculate peak");
		request_peaklevel(sample);
	}
	if(!sample->overview && sample->row_ref){
		dbg(1, "regenerate overview");
		request_overview(sample);
	}
}


static void
_listmodel__update_by_tree_iter(GtkTreeIter* iter, int what, void* data)
{
	switch (what) {
		case COL_ICON:
			gtk_list_store_set(app->store, iter, COL_ICON, data, -1);
			break;
		case COL_KEYWORDS:
			dbg(1, "val=%s", (char*)data);
			gtk_list_store_set(app->store, iter, COL_KEYWORDS, (char*)data, -1);
			break;
		case COL_COLOUR:
			gtk_list_store_set(app->store, iter, what, *((guint*)data), -1);
			break;
		default:
			dbg(0, "update for this type is not yet implemented"); 
			break;
	}
}


static gboolean
_listmodel__update_by_rowref(GtkTreeRowReference* row_ref, int what, void* data)
{
	GtkTreePath* path = gtk_tree_row_reference_get_path(row_ref);
	g_return_val_if_fail(path, false);

	GtkTreeIter iter;
	gtk_tree_model_get_iter(GTK_TREE_MODEL(app->store), &iter, path);
	gtk_tree_path_free(path);
	_listmodel__update_by_tree_iter(&iter, what, data);

	return true;
}


static void
listmodel__set_overview(GtkTreeRowReference* row_ref, GdkPixbuf* pixbuf)
{
	GtkTreePath* treepath;
	if((treepath = gtk_tree_row_reference_get_path(row_ref))){ //it's possible the row may no longer be visible.
		GtkTreeIter iter;
		if(gtk_tree_model_get_iter(GTK_TREE_MODEL(app->store), &iter, treepath)){
			if(GDK_IS_PIXBUF(pixbuf)){
				gtk_list_store_set(app->store, &iter, COL_OVERVIEW, pixbuf, -1);
			}else perr("pixbuf is not a pixbuf.\n");

		}else perr("failed to get row iter. row_ref=%p\n", row_ref);
		gtk_tree_path_free(treepath);
	} else dbg(2, "no path for rowref. row_ref=%p\n", row_ref); //this is not an error. The row may not be part of the current search.
}


static void
listmodel__set_peaklevel(GtkTreeRowReference* row_ref, float level)
{
	GtkTreePath* treepath;
	if((treepath = gtk_tree_row_reference_get_path(row_ref))){ //it's possible the row may no longer be visible.
		GtkTreeIter iter;
		if(gtk_tree_model_get_iter(GTK_TREE_MODEL(app->store), &iter, treepath)){
			gtk_list_store_set(app->store, &iter, COL_PEAKLEVEL, level, -1);

		}else perr("failed to get row iter. row_ref=%p\n", row_ref);
		gtk_tree_path_free(treepath);
	} else dbg(2, "no path for rowref. row_ref=%p\n", row_ref); //this is not an error. The row may not be part of the current search.
}


void
listmodel__move_files(GList* list, const gchar* dest_path)
{
	/* This is called _before_ the files are actually moved!
	 * file_util_move_simple() which is called afterwards 
	 * will free the list and dest_path.
	 */
	if (!list) return;
	if (!dest_path) return;

	GList* l = list;
	do {
		const gchar* src_path=l->data;

		gchar* bn = g_path_get_basename(src_path);
		gchar* full_path = g_strdup_printf("%s%c%s", dest_path, G_DIR_SEPARATOR, bn);
		g_free(bn);

		if (src_path) {
			dbg(0,"move %s -> %s | NEW: %s", src_path, dest_path, full_path);
			int id = -1;
			if(backend.file_exists(src_path, &id) && id > 0){
				backend.update_string(id, "filedir", dest_path);
				backend.update_string(id, "full_path", full_path);
			}

			Sample* s = sample_get_by_filename(src_path);
			if (s) {
				GtkTreePath* treepath;
				free(s->full_path);
				free(s->sample_dir);
				s->full_path  = strdup(full_path);
				s->sample_dir = strdup(dest_path);
				//it's possible the row may no longer be visible:
				if((treepath = gtk_tree_row_reference_get_path(s->row_ref))){
					GtkTreeIter iter;
					if(gtk_tree_model_get_iter(GTK_TREE_MODEL(app->store), &iter, treepath)){
						gtk_list_store_set(app->store, &iter, COL_FNAME, s->sample_dir, -1);
					}
					gtk_tree_path_free(treepath);
				}
			}
		}
		g_free(full_path);
		l = l->next;
	} while (l);
}


struct find_filename {
	int id;
	char *rv;
};


static bool
filter_id (GtkTreeModel* model, GtkTreePath* path, GtkTreeIter* iter, gpointer data) {
	struct find_filename* ff = (struct find_filename*) data;
	Sample* s = sample_get_by_tree_iter(iter);
	if (s->id == ff->id) {
		ff->rv = strdup(s->full_path);
		sample_unref(s);
		return true;
	}
	sample_unref(s);
	return false;
}


char*
listmodel__get_filename_from_id(int id)
{
	struct find_filename ff;
	ff.id = id;
	ff.rv = NULL;
	GtkTreeModel* model = GTK_TREE_MODEL(app->store);
	gtk_tree_model_foreach(model, &filter_id, &ff);
	return ff.rv;
}
