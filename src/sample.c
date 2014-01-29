/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://samplecat.orford.org          |
* | copyright (C) 2007-2013 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#include "config.h"
#define __USE_GNU
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include "debug/debug.h"
#include "file_manager/file_manager.h"
#include "file_manager/rox_support.h" // to_utf8()
#include "types.h"
#include "support.h"
#include "audio_decoder/ad.h"
#include "main.h"

#include "mimetype.h"
#include "overview.h"
#include "listview.h"
#include "sample.h"


Sample*
sample_new()
{
	Sample* sample = g_new0(struct _sample, 1);
	sample->id = -1;
	sample->colour_index = 0;
	sample_ref(sample);
	return sample;
}


Sample*
sample_new_from_filename(char* path, gboolean path_alloced)
{
	if (!file_exists(path)) {
		perr("file not found: %s\n", path);
		if (path_alloced) free(path);
		return NULL;
	}

	Sample* sample = sample_new();
	sample->full_path = path_alloced?path:g_strdup(path);

	MIME_type* mime_type = type_from_path(path);
	if (!mime_type) {
		perr("can not resolve mime-type of file\n");
		sample_unref(sample);
		return NULL;
	}
	sample->mimetype = g_strdup_printf("%s/%s", mime_type->media_type, mime_type->subtype);

	if(mimetype_is_unsupported(mime_type, sample->mimetype)){
		dbg(1, "file type \"%s\" not supported.", sample->mimetype);
		sample_unref(sample);
		return NULL;
	}

	sample->mtime = file_mtime(path);

	if(!sample->sample_name){
		gchar *bn =g_path_get_basename(sample->full_path);
		sample->sample_name= to_utf8(bn);
		g_free(bn);
	}
	if(!sample->sample_dir){
		gchar *dn = g_path_get_dirname(sample->full_path);
		sample->sample_dir = to_utf8(dn);
		g_free(dn);
	}
	return sample;
}


Sample*
sample_dup(Sample* s)
{
	Sample* r = g_new0(struct _sample, 1);
	memset(r,0, sizeof(Sample));

  r->id           = s->id;
  r->ref_count    = 0;
	r->sample_rate  = s->sample_rate;
	r->length       = s->length;
	r->frames       = s->frames;
	r->channels     = s->channels;
	r->bit_depth    = s->bit_depth;
	r->bit_rate     = s->bit_rate;
	r->peaklevel    = s->peaklevel;
	r->colour_index = s->colour_index;
	r->online       = s->online;
	r->mtime        = s->mtime;
#define DUPSTR(P) if (s->P) r->P=strdup(s->P);
	DUPSTR(sample_dir)
	DUPSTR(sample_name)
	DUPSTR(full_path)
	DUPSTR(keywords)
	DUPSTR(ebur)
	DUPSTR(notes)
	DUPSTR(mimetype)
	DUPSTR(meta_data)

	if (s->overview) r->overview=gdk_pixbuf_copy(s->overview);
	sample_ref(r);
	return r;
}


void
sample_free(Sample* sample)
{
	if(sample->ref_count > 0) {
		gwarn("freeing sample with refcount: %d", sample->ref_count);
	}
	if(sample->row_ref) gtk_tree_row_reference_free(sample->row_ref);
	if(sample->sample_name) g_free(sample->sample_name);
	if(sample->full_path) g_free(sample->full_path);
	if(sample->mimetype) g_free(sample->mimetype);
	if(sample->ebur) g_free(sample->ebur);
	if(sample->notes) g_free(sample->notes);
	if(sample->sample_dir) g_free(sample->sample_dir);
	if(sample->meta_data) g_free(sample->meta_data);
	//if(sample->overview) g_free(sample->overview); // check how to free that!
	g_free(sample);
}


void
sample_ref(Sample* sample)
{
	sample->ref_count++;
}


void
sample_unref(Sample* sample)
{
	if (!sample) return;
	sample->ref_count--;
	if(sample->ref_count < 1) sample_free(sample);
}


void
sample_refresh(Sample* sample, gboolean force_update)
{
	time_t mtime = file_mtime(sample->full_path);
	if(mtime > 0){
		if (sample->mtime < mtime || force_update) {
			/* file may have changed - FULL UPDATE */
			dbg(0, "file modified: full update: %s", sample->full_path);

			// re-check mime-type
			Sample* test = sample_new_from_filename(sample->full_path, false);
			if (test) sample_unref(test);
			else return;

			if (sample_get_file_info(sample)) {
				g_signal_emit_by_name (app->model, "sample-changed", sample, -1, NULL);
				sample->mtime = mtime;
				request_peaklevel(sample);
				request_overview(sample);
				request_ebur128(sample);
			} else {
				dbg(0, "full update - reading file info failed!");
			}
		}
		sample->mtime = mtime;
		sample->online = 1;
	}else{
		/* file does not exist */
		sample->online = 0;
	}
	g_signal_emit_by_name (app->model, "sample-changed", sample, COL_ICON, NULL);
}


gboolean
sample_get_file_info(Sample* sample)
{
	if(!file_exists(sample->full_path)){
		if(sample->online){
			listmodel__update_sample(sample, COL_ICON, (void*)false);
		}
		return false;
	}

	struct adinfo nfo;
	if (!ad_finfo(sample->full_path, &nfo)) {
		return false;
	}
	sample->channels    = nfo.channels;
	sample->sample_rate = nfo.sample_rate;
	sample->length      = nfo.length;
	sample->frames      = nfo.frames;
	sample->bit_rate    = nfo.bit_rate;
	sample->bit_depth   = nfo.bit_depth;
	sample->meta_data   = nfo.meta_data;
	//ad_free_nfo(&nfo); // no, retain allocated meta_data 
	return true;
}


Sample*
sample_get_by_tree_iter(GtkTreeIter* iter)
{
	Sample* sample;
	gtk_tree_model_get(GTK_TREE_MODEL(app->store), iter, COL_SAMPLEPTR, &sample, -1);
	if(sample) sample_ref(sample);
	return sample;
}


Sample*
sample_get_from_model(GtkTreePath* path)
{
	GtkTreeModel* model = GTK_TREE_MODEL(app->store);
	GtkTreeIter iter;
	if(!gtk_tree_model_get_iter(model, &iter, path)) return NULL;
	Sample* sample = sample_get_by_tree_iter(&iter);
	if (sample && !sample->row_ref) sample->row_ref = gtk_tree_row_reference_new(GTK_TREE_MODEL(app->store), path);
	return sample;
}

Sample*
sample_get_by_row_ref(GtkTreeRowReference* ref)
{
	GtkTreePath* path;
	if (!ref || !gtk_tree_row_reference_valid(ref)) return NULL;
	if(!(path = gtk_tree_row_reference_get_path(ref))) return NULL;
	Sample* sample = sample_get_from_model(path);
	gtk_tree_path_free(path);
	return sample;
}

struct find_sample {
	Sample *rv;
	const char *abspath;
};

gboolean filter_sample (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
	struct find_sample *fs = (struct find_sample*) data;
	Sample *s = sample_get_by_tree_iter(iter);
	if (!strcmp(s->full_path, fs->abspath)) {
		fs->rv=s;
		return TRUE;
	}
	sample_unref(s);
	return FALSE;
}

Sample*
sample_get_by_filename(const char* abspath)
{
	struct find_sample fs;
	fs.rv = NULL; fs.abspath = abspath;
	GtkTreeModel* model = GTK_TREE_MODEL(app->store);
	gtk_tree_model_foreach(model, &filter_sample, &fs);
	return fs.rv;
}

