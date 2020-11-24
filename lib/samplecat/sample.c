/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2007-2020 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#include "config.h"
#define __USE_GNU
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <gtk/gtk.h>
#pragma GCC diagnostic warning "-Wdeprecated-declarations"
#include "debug/debug.h"
#include "decoder/ad.h"
#include "file_manager/support.h" // to_utf8()
#include "file_manager/mimetype.h"
#include "samplecat/support.h"
#include "support.h"
#include "model.h"
#include "worker.h"
#include "list_store.h"
#include "sample.h"

#undef DEBUG_REFCOUNTS

static void sample_free (Sample*);


Sample*
sample_new ()
{
	Sample* sample = g_new0(Sample, 1);
	sample->id = -1;
	sample->colour_index = 0;
	sample_ref(sample);
	return sample;
}


Sample*
sample_new_from_filename (char* path, bool path_alloced)
{
	if (!file_exists(path)) {
		perr("file not found: %s\n", path);
		if (path_alloced) g_free(path);
		return NULL;
	}

	Sample* sample = sample_new();
	sample->full_path = path_alloced ? path : g_strdup(path);

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

	if(!sample->name){
		gchar* bn = g_path_get_basename(sample->full_path);
		sample->name= to_utf8(bn);
		g_free(bn);
	}

	if(!sample->sample_dir){
		gchar* dn = g_path_get_dirname(sample->full_path);
		sample->sample_dir = to_utf8(dn);
		g_free(dn);
	}

	return sample;
}


Sample*
sample_dup (Sample* s)
{
	Sample* r = g_new0(Sample, 1);

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
#define DUPSTR(P) if (s->P) r->P = strdup(s->P);
	DUPSTR(sample_dir)
	DUPSTR(name)
	DUPSTR(full_path)
	DUPSTR(keywords)
	DUPSTR(ebur)
	DUPSTR(notes)
	DUPSTR(mimetype)
	r->meta_data = s->meta_data ? g_ptr_array_ref(s->meta_data) : NULL;

	if (s->overview) r->overview = gdk_pixbuf_copy(s->overview);
	sample_ref(r);
	return r;
}


static void
sample_free (Sample* sample)
{
	if(sample->ref_count > 0) {
		gwarn("freeing sample with refcount: %d", sample->ref_count);
	}
	if(sample->row_ref) gtk_tree_row_reference_free(sample->row_ref);
	if(sample->name) g_free(sample->name);
	if(sample->full_path) g_free(sample->full_path);
	if(sample->mimetype) g_free(sample->mimetype);
	if(sample->ebur) g_free(sample->ebur);
	if(sample->notes) g_free(sample->notes);
	if(sample->sample_dir) g_free(sample->sample_dir);
	if(sample->meta_data) g_ptr_array_unref(sample->meta_data);
	//if(sample->overview) g_free(sample->overview); // check how to free that!
	g_free(sample);
}


Sample*
sample_ref (Sample* sample)
{
#ifdef DEBUG_REFCOUNTS
	if(sample->name && !strcmp(sample->name, "test.wav")) printf("+ %i --> %i\n", sample->ref_count, sample->ref_count+1);
#endif
	sample->ref_count++;

	return sample;
}


void
sample_unref (Sample* sample)
{
	if (!sample) return;
#ifdef DEBUG_REFCOUNTS
	if(sample->name && !strcmp(sample->name, "test.wav")) printf("- %i --> %i\n", sample->ref_count, sample->ref_count-1);
#endif

	sample->ref_count--;

#ifdef DEBUG_REFCOUNTS
	g_return_if_fail(sample->ref_count >= 0);
	if(sample->ref_count < 1) gwarn("freeing sample... %s", sample->name ? sample->name : "");
#endif

	if(sample->ref_count < 1) sample_free(sample);
}


bool
sample_get_file_info (Sample* sample)
{
	// ownership of allocated memory (ie the meta_data) is transferred to the caller.

	if(!file_exists(sample->full_path)){
		if(sample->online){
			samplecat_model_update_sample (samplecat.model, sample, COL_ICON, NULL);
		}
		return false;
	}

	WfAudioInfo nfo;
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

	return true;
}


Sample*
sample_get_by_filename (const char* abspath)
{
	struct find_sample {
		Sample*     rv;
		const char* abspath;
	};

	gboolean filter_sample (GtkTreeModel* model, GtkTreePath* path, GtkTreeIter* iter, gpointer data)
	{
		struct find_sample* fs = (struct find_sample*) data;
		Sample* s = samplecat_list_store_get_sample_by_iter(iter);
		if (!strcmp(s->full_path, fs->abspath)) {
			fs->rv = s;
			return true;
		}
		sample_unref(s);
		return false;
	}

	struct find_sample fs = {
		.abspath = abspath
	};

	gtk_tree_model_foreach(GTK_TREE_MODEL(samplecat.store), &filter_sample, &fs);

	return fs.rv;
}


/*
 *   Result must be g_free'd.
 */
char*
sample_get_metadata_str (Sample* sample)
{
	if(!sample->meta_data) return NULL;

	GPtrArray* array = g_ptr_array_new();
	char** data = (char**)sample->meta_data->pdata;
	int i; for(i=0;i<sample->meta_data->len;i+=2){
		g_ptr_array_add(array, g_strdup_printf("%s:%s", data[i], data[i+1]));
	}
	g_ptr_array_add(array, NULL);

	gchar** items = (gchar**)g_ptr_array_free(array, false);
	char* str = g_strjoinv("\n", items);
	g_strfreev(items);
	return str;
}


void
sample_set_metadata (Sample* sample, const char* str)
{
	if(sample->meta_data) g_ptr_array_unref(sample->meta_data);
	sample->meta_data = g_ptr_array_new_full(32, g_free);

	gchar** items = g_strsplit(str, "\n", 32);
	int i; for(i=0;i<g_strv_length(items);i++){
		gchar** split = g_strsplit(items[i], ":", 2);
		g_ptr_array_add(sample->meta_data, g_strdup(split[0]));
		g_ptr_array_add(sample->meta_data, g_strdup(split[1]));
		g_strfreev(split);
	}
	g_strfreev(items);
}


