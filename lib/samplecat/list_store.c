/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2007-2017 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <file_manager/mimetype.h>
#include <file_manager/pixmaps.h>
#include <debug/debug.h>
#include <db/db.h>
#include <samplecat.h>
#include <list_store.h>

#define SAMPLECAT_TYPE_LIST_STORE (samplecat_list_store_get_type ())
#define SAMPLECAT_LIST_STORE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SAMPLECAT_TYPE_LIST_STORE, SamplecatListStore))
#define SAMPLECAT_LIST_STORE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), SAMPLECAT_TYPE_LIST_STORE, SamplecatListStoreClass))
#define SAMPLECAT_IS_LIST_STORE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SAMPLECAT_TYPE_LIST_STORE))
#define SAMPLECAT_IS_LIST_STORE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SAMPLECAT_TYPE_LIST_STORE))
#define SAMPLECAT_LIST_STORE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), SAMPLECAT_TYPE_LIST_STORE, SamplecatListStoreClass))

#define _gtk_tree_row_reference_free0(var) ((var == NULL) ? NULL : (var = (gtk_tree_row_reference_free (var), NULL)))
#define _gtk_tree_path_free0(var) ((var == NULL) ? NULL : (var = (gtk_tree_path_free (var), NULL)))

static gpointer samplecat_list_store_parent_class = NULL;

enum  {
	SAMPLECAT_LIST_STORE_DUMMY_PROPERTY
};

static void     samplecat_list_store_finalize (GObject*);


SamplecatListStore*
samplecat_list_store_construct (GType object_type)
{
	GType _tmp0_[14] = {0};
	GType types[14];
	SamplecatListStore* self = (SamplecatListStore*) g_object_new (object_type, NULL);
	_tmp0_[0] = GDK_TYPE_PIXBUF;
	_tmp0_[1] = G_TYPE_INT;
	_tmp0_[2] = G_TYPE_STRING;
	_tmp0_[3] = G_TYPE_STRING;
	_tmp0_[4] = G_TYPE_STRING;
	_tmp0_[5] = GDK_TYPE_PIXBUF;
	_tmp0_[6] = G_TYPE_STRING;
	_tmp0_[7] = G_TYPE_STRING;
	_tmp0_[8] = G_TYPE_INT;
	_tmp0_[9] = G_TYPE_STRING;
	_tmp0_[10] = G_TYPE_FLOAT;
	_tmp0_[11] = G_TYPE_INT;
	_tmp0_[12] = G_TYPE_POINTER;
	_tmp0_[13] = G_TYPE_INT64;
	memcpy (types, _tmp0_, 14 * sizeof (GType));
	gtk_list_store_set_column_types ((GtkListStore*) self, 14, types);
	return self;
}


SamplecatListStore*
samplecat_list_store_new (void)
{
	return samplecat_list_store_construct (SAMPLECAT_TYPE_LIST_STORE);
}


void
samplecat_list_store_clear_ (SamplecatListStore* self)
{
	PF;
	GtkTreeIter iter;
	while(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(self), &iter)){
		GdkPixbuf* pixbuf = NULL;
		Sample* sample = NULL;
		gtk_tree_model_get(GTK_TREE_MODEL(self), &iter, COL_OVERVIEW, &pixbuf, COL_SAMPLEPTR, &sample, -1);
		gtk_list_store_remove((GtkListStore*)self, &iter);
		if(pixbuf) g_object_unref(pixbuf);
		if(sample) sample_unref(sample);
	}

	self->row_count = 0;
}


void
samplecat_list_store_add (SamplecatListStore* self, Sample* sample)
{

	if(!samplecat.store) return;
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
	char length_s[64]; format_smpte(length_s, sample->length);

#ifdef USE_AYYI
	GdkPixbuf* ayyi_icon = NULL;

	//is the file loaded in the current Ayyi song?
	if(ayyi.got_shm){
		gchar* fullpath = g_build_filename(sample->sample_dir, sample->name, NULL);
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
	gtk_list_store_append(samplecat.store, &iter);
	gtk_list_store_set(samplecat.store, &iter,
			COL_ICON,       iconbuf,
			COL_NAME,       sample->name,
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
			COL_LEN,        sample->length,
			-1);

	GtkTreePath* treepath;
	if((treepath = gtk_tree_model_get_path(GTK_TREE_MODEL(samplecat.store), &iter))){
		sample->row_ref = gtk_tree_row_reference_new(GTK_TREE_MODEL(samplecat.store), treepath);
		gtk_tree_path_free(treepath);
	}

	Sample* _tmp13_;
	g_return_if_fail (self != NULL);
	if(sample->row_ref && sample->online){
		request_analysis(sample);
	}

	_tmp13_ = sample;
	sample_ref (_tmp13_);
}


void
samplecat_list_store_on_sample_changed (SamplecatListStore* self, Sample* sample, gint prop, void* val)
{
	g_return_if_fail (self != NULL);
	g_return_if_fail (sample->row_ref);

	GtkTreePath* path = gtk_tree_row_reference_get_path(sample->row_ref);
	if(!path) return;
	GtkTreeIter iter;
	gtk_tree_model_get_iter(GTK_TREE_MODEL(samplecat.store), &iter, path);
	gtk_tree_path_free(path);


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
				gtk_list_store_set(samplecat.store, &iter, COL_ICON, iconbuf, -1);
			}
			break;
		case COL_KEYWORDS:
			gtk_list_store_set(samplecat.store, &iter, COL_KEYWORDS, (char*)val, -1);
			break;
		case COL_OVERVIEW:
			gtk_list_store_set((GtkListStore*)self, &iter, COL_OVERVIEW, sample->overview, -1);
			break;
		case COL_COLOUR:
			gtk_list_store_set(samplecat.store, &iter, prop, *((guint*)val), -1);
			break;
		case COL_PEAKLEVEL:
			gtk_list_store_set((GtkListStore*)self, &iter, COL_PEAKLEVEL, sample->peaklevel, -1);
			break;
		case COL_FNAME:
			gtk_list_store_set(samplecat.store, &iter, COL_FNAME, sample->sample_dir, -1);
			break;
		case COL_X_EBUR:
		case COL_X_NOTES:
			// nothing to do.
			break;
		case COL_ALL:
		case -1: // deprecated
			{
				//char* metadata = sample_get_metadata_str(s);

				char samplerate_s[32]; samplerate_format(samplerate_s, sample->sample_rate);
				char length_s[64]; format_smpte(length_s, sample->length);
				gtk_list_store_set((GtkListStore*)samplecat.store, &iter,
						COL_CHANNELS, sample->channels,
						COL_SAMPLERATE, samplerate_s,
						COL_LENGTH, length_s,
						COL_PEAKLEVEL, sample->peaklevel,
						COL_OVERVIEW, sample->overview,
						-1);
				dbg(1, "file info updated.");
				//if(metadata) g_free(metadata);
			}
			break;
		default:
			dbg(0, "property not handled %i", prop);
			break;
	}
}


void
samplecat_list_store_do_search (SamplecatListStore* self)
{
	g_return_if_fail (self);

	samplecat_list_store_clear_(self);

	if(!samplecat.model->backend.search_iter_new(NULL)) {
		return;
	}

	int row_count = 0;
	unsigned long* lengths;
	Sample* result;
	while((result = samplecat.model->backend.search_iter_next(&lengths)) && row_count < LIST_STORE_MAX_ROWS){
		Sample* s = sample_dup(result);
		samplecat_list_store_add(self, s);
		sample_unref(s);
		row_count++;
	}

	samplecat.model->backend.search_iter_free();

	((SamplecatListStore*)samplecat.store)->row_count = row_count;

	g_signal_emit_by_name (self, "content-changed");
}


static void
samplecat_list_store_class_init (SamplecatListStoreClass * klass)
{
	samplecat_list_store_parent_class = g_type_class_peek_parent (klass);
	G_OBJECT_CLASS (klass)->finalize = samplecat_list_store_finalize;
	g_signal_new ("content_changed", SAMPLECAT_TYPE_LIST_STORE, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}


static void
samplecat_list_store_instance_init (SamplecatListStore * self)
{
	self->row_count = 0;
}


static void
samplecat_list_store_finalize (GObject* obj)
{
	SamplecatListStore* self = G_TYPE_CHECK_INSTANCE_CAST (obj, SAMPLECAT_TYPE_LIST_STORE, SamplecatListStore);
	_gtk_tree_row_reference_free0 (self->playing);
	G_OBJECT_CLASS (samplecat_list_store_parent_class)->finalize (obj);
}


GType
samplecat_list_store_get_type (void)
{
	static volatile gsize samplecat_list_store_type_id__volatile = 0;
	if (g_once_init_enter (&samplecat_list_store_type_id__volatile)) {
		static const GTypeInfo g_define_type_info = { sizeof (SamplecatListStoreClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) samplecat_list_store_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (SamplecatListStore), 0, (GInstanceInitFunc) samplecat_list_store_instance_init, NULL };
		GType samplecat_list_store_type_id;
		samplecat_list_store_type_id = g_type_register_static (GTK_TYPE_LIST_STORE, "SamplecatListStore", &g_define_type_info, 0);
		g_once_init_leave (&samplecat_list_store_type_id__volatile, samplecat_list_store_type_id);
	}
	return samplecat_list_store_type_id__volatile;
}



/** samplecat_list_store_get_sample_by_iter returns a pointer to
 * the sample struct in the data model or NULL if not found.
 * @return needs to be sample_unref();
 */
Sample*
samplecat_list_store_get_sample_by_iter(GtkTreeIter* iter)
{
	Sample* sample;
	gtk_tree_model_get(GTK_TREE_MODEL(samplecat.store), iter, COL_SAMPLEPTR, &sample, -1);
	if(sample) sample_ref(sample);
	return sample;
}


/**
 * @return needs to be sample_unref();
 */
Sample*
samplecat_list_store_get_sample_by_row_index (int row)
{
	GtkTreePath* path = gtk_tree_path_new_from_indices (row, -1);
	if(path){
		Sample* sample = sample_get_from_model(path);
		gtk_tree_path_free(path);
		return sample;
	}

	return NULL;
}


/** samplecat_list_store_get_sample_by_row_ref returns a pointer to
 * the sample struct in the data model or NULL if not found.
 * @return needs to be sample_unref();
 */
Sample*
samplecat_list_store_get_sample_by_row_ref(GtkTreeRowReference* ref)
{
	GtkTreePath* path;
	if (!ref || !gtk_tree_row_reference_valid(ref)) return NULL;
	if(!(path = gtk_tree_row_reference_get_path(ref))) return NULL;
	Sample* sample = sample_get_from_model(path);
	gtk_tree_path_free(path);
	return sample;
}


