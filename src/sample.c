#include "../config.h"
#define __USE_GNU
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <jack/jack.h>
#include <sndfile.h>
#include "file_manager/file_manager.h"
#include "gqview_view_dir_tree.h"
#include "typedefs.h"
#include "types.h"
#include "dh-link.h"
#include "support.h"
#include "main.h"
#include "audio.h"
#include "overview.h"
#include "listview.h"
#include "sample.h"


extern struct _app app;
extern Filer filer;


sample*
sample_new()
{
	sample* sample = g_new0(struct _sample, 1);

	sample->bg_colour = app.bg_colour;

	return sample;
}


sample*
sample_new_from_model(GtkTreePath *path)
{
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(app.view));
	GtkTreeIter iter;
	gtk_tree_model_get_iter(model, &iter, path);

	gchar *fpath, *fname, *mimetype; int id; unsigned colour_index;
	gchar *length;
	gchar *samplerate;
	int channels;
	gtk_tree_model_get(model, &iter, COL_FNAME, &fpath, COL_NAME, &fname, COL_IDX, &id, COL_LENGTH, &length, COL_SAMPLERATE, &samplerate, COL_CHANNELS, &channels, COL_MIMETYPE, &mimetype, COL_COLOUR, &colour_index, -1);
	g_return_val_if_fail(mimetype, NULL);

	sample* sample = g_new0(struct _sample, 1);
	sample->id     = id;
	sample->length = atoi(length);
	sample->sample_rate = atoi(samplerate);
	sample->pixbuf = NULL;
	sample->channels = channels;
	sample->row_ref = gtk_tree_row_reference_new(GTK_TREE_MODEL(app.store), path);
	snprintf(sample->filename, 255, "%s/%s", fpath, fname);
	if(!strcmp(mimetype, "audio/x-flac")) sample->filetype = TYPE_FLAC; else sample->filetype = TYPE_SNDFILE; 
	return sample;
}


//FIXME this is an internal detail of file_view. Use an accessor instead.
#define FILE_VIEW_COL_LEAF 0

sample*
sample_new_from_fileview(GtkTreeModel* model, GtkTreeIter* iter)
{
	gchar* fname;
	gtk_tree_model_get(model, iter, FILE_VIEW_COL_LEAF, &fname, -1);

	sample* sample = g_new0(struct _sample, 1);
	snprintf(sample->filename, 255, "%s/%s", filer.real_path, fname);
	return sample;
}


sample*
sample_new_from_result(SamplecatResult* r)
{
	sample* s      = g_new0(struct _sample, 1);
	s->id          = r->idx;
	s->row_ref     = r->row_ref;
	snprintf(s->filename, 255, "%s/%s", r->dir, r->sample_name);
	s->filetype    = TYPE_SNDFILE;
	s->sample_rate = r->sample_rate;
	s->length      = r->length;
	s->frames      = 0;
	s->channels    = r->channels;
	s->bitdepth    = 0;
	s->pixbuf      = r->overview;
	color_rgba_to_gdk(&s->bg_colour, r->colour);
	return s;
}


void
sample_free(sample* sample)
{
	if(sample->row_ref) gtk_tree_row_reference_free(sample->row_ref);
	g_free(sample);
}


void
sample_set_type_from_mime_string(sample* sample, char* mime_string)
{
	if(!strcmp(mime_string, "audio/x-flac")) sample->filetype = TYPE_FLAC;
	dbg(2, "mimetype: %s type=%i", mime_string, sample->filetype);
}


