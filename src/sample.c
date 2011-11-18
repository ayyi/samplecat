#include "../config.h"
#define __USE_GNU
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include "file_manager/file_manager.h"
#include "file_manager/rox/rox_support.h" // to_utf8()
#include "gqview_view_dir_tree.h"
#include "typedefs.h"
#include "support.h"
#include "main.h"

#include "mimetype.h"
#include "overview.h"
#include "listview.h"
#include "sample.h"


extern struct _app app;


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
sample_new_from_filename(char *path, gboolean path_alloced)
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
	sample->mimetype=g_strdup_printf("%s/%s", mime_type->media_type, mime_type->subtype);

	if(mimetype_is_unsupported(mime_type, sample->mimetype)){
		dbg(0, "file type \"%s\" not supported.\n", sample->mimetype);
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
	if(sample->ref_count >0) {
		dbg(0, "WARNING: freeing sample w/ refcount:%d", sample->ref_count);
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

// TODO move to audio_analysis/analyzers.c ?! or overview.c
#include "audio_decoder/ad.h"
gboolean
sample_get_file_info(Sample* sample)
{
	struct adinfo nfo;
	if (!ad_finfo(sample->full_path, &nfo)) {
		return false;
	}
	sample->channels    = nfo.channels;
	sample->sample_rate = nfo.sample_rate;
	sample->length      = nfo.length;
	sample->bit_rate    = nfo.bit_rate;
	sample->bit_depth   = nfo.bit_depth;
	sample->meta_data   = nfo.meta_data;
	//ad_free_nfo(&nfo); // no! retain allocated meta_data 
	return true;
}

Sample*
sample_get_by_tree_iter(GtkTreeIter* iter)
{
	Sample *sample;
	gtk_tree_model_get(GTK_TREE_MODEL(app.store), iter, COL_SAMPLEPTR, &sample, -1);
	if(sample) sample_ref(sample);
	return sample;
}

Sample*
sample_get_from_model(GtkTreePath* path)
{
#if 1
	GtkTreeModel* model = GTK_TREE_MODEL(app.store);
#else
	GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(app.view));
#endif
	GtkTreeIter iter;
	gtk_tree_model_get_iter(model, &iter, path);
	Sample* sample = sample_get_by_tree_iter(&iter);
	if (sample && !sample->row_ref) sample->row_ref = gtk_tree_row_reference_new(GTK_TREE_MODEL(app.store), path);
	return sample;
}

Sample*
sample_get_by_row_ref(GtkTreeRowReference* ref)
{
	GtkTreePath *path;
	if (!ref || !gtk_tree_row_reference_valid(ref)) return NULL;
	if(!(path = gtk_tree_row_reference_get_path(ref))) return NULL;
	Sample* sample = sample_get_from_model(path);
	gtk_tree_path_free(path);
	return sample;
}

Sample*
sample_get_by_filename(char *abspath) {
	Sample *rv = NULL;
	gboolean find_id (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data) {
		Sample *s = sample_get_by_tree_iter(iter);
		if (!strcmp(s->full_path, abspath)) {
			rv=s;
			return TRUE;
		}
		sample_unref(s);
		return FALSE;
	}
	GtkTreeModel* model = GTK_TREE_MODEL(app.store);
	gtk_tree_model_foreach(model, &find_id, NULL);
	return rv;
}
