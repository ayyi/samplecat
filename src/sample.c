#include "../config.h"
#define __USE_GNU
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <jack/jack.h>
#include "file_manager/file_manager.h"
#include "gqview_view_dir_tree.h"
#include "typedefs.h"
#include "support.h"
#include "main.h"
#if (defined HAVE_JACK && !(defined USE_DBUS || defined USE_GAUDITION))
#include "jack_player.h"
#endif
#include "overview.h"
#include "listview.h"
#include "sample.h"


extern struct _app app;
extern Filer filer;
extern unsigned debug;


Sample*
sample_new()
{
	Sample* sample = g_new0(struct _sample, 1);

	sample->bg_colour = app.bg_colour;

	return sample;
}


Sample*
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

	Sample* sample = g_new0(struct _sample, 1);
	sample->id     = id;
	sample->length = atoi(length);
	sample->sample_rate = atoi(samplerate);
	sample->pixbuf = NULL;
	sample->channels = channels;
	sample->row_ref = gtk_tree_row_reference_new(GTK_TREE_MODEL(app.store), path);
	snprintf(sample->filename, 255, "%s/%s", fpath, fname);
	return sample;
}


//FIXME this is an internal detail of file_view. Use an accessor instead.
#define FILE_VIEW_COL_LEAF 0

Sample*
sample_new_from_fileview(GtkTreeModel* model, GtkTreeIter* iter)
{
	gchar* fname;
	gtk_tree_model_get(model, iter, FILE_VIEW_COL_LEAF, &fname, -1);

	Sample* sample = g_new0(struct _sample, 1);
	snprintf(sample->filename, 255, "%s/%s", filer.real_path, fname);
	return sample;
}


Sample*
sample_new_from_result(Sample* r)
{
	Sample* s      = g_new0(struct _sample, 1);
	s->id          = r->idx;
	s->row_ref     = r->row_ref;
	snprintf(s->filename, 255, "%s/%s", r->dir, r->sample_name);
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
sample_ref(Sample* sample)
{
	sample->ref_count++;
}


void
sample_unref(Sample* sample)
{
	sample->ref_count--;
	if(sample->ref_count < 1) sample_free(sample);
}


void
sample_free(Sample* sample)
{
	if(sample->row_ref) gtk_tree_row_reference_free(sample->row_ref);
	g_free(sample);
}

#include "audio_decoder/ad.h"
#include "audio_analysis/ebumeter/ebur128.h"
gboolean
sample_get_file_info(Sample* sample)
{
	struct adinfo nfo;
	if (!ad_finfo(sample->filename, &nfo)) {
		return false;
	}
	sample->channels    = nfo.channels;
	sample->sample_rate = nfo.sample_rate;
	sample->length      = nfo.length;
	return true;
}

gboolean
result_get_file_info(Sample* sample)
{
	struct adinfo nfo;
	if (!ad_finfo(sample->sample_name, &nfo)) {
		return false;
	}
	sample->channels    = nfo.channels;
	sample->sample_rate = nfo.sample_rate;
	sample->length      = nfo.length;

	struct ebur128 ebur;
	ebur128analyse(sample->sample_name, &ebur);
	free(sample->misc);
	sample->misc      = g_strdup_printf(
		"Integrated loudness:   %6.1lf LU%s\n"
		"Loudness range:        %6.1lf LU\n"
		"Integrated threshold:  %6.1lf LU%s\n"
		"Range threshold:       %6.1lf LU%s\n"
		"Range min:             %6.1lf LU%s\n"
		"Range max:             %6.1lf LU%s\n"
		"Momentary max:         %6.1lf LU%s\n"
		"Short term max:        %6.1lf LU%s\n"
		, ebur.integrated , ebur.lufs?"FS":""
		, ebur.range
		, ebur.integ_thr  , ebur.lufs?"FS":""
		, ebur.range_thr  , ebur.lufs?"FS":""
		, ebur.range_min  , ebur.lufs?"FS":""
		, ebur.range_max  , ebur.lufs?"FS":""
		, ebur.maxloudn_M , ebur.lufs?"FS":""
		, ebur.maxloudn_S , ebur.lufs?"FS":""
		);
	return true;
}

Sample*
result_new_from_model(GtkTreePath* path)
{
	GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(app.view));
	GtkTreeIter iter;
	gtk_tree_model_get_iter(model, &iter, path);

	Sample* sample = g_new0(Sample, 1);

	gchar* mimetype, *length, *notes, *misc, *dir_path; int id; unsigned colour_index;
	gchar* samplerate;
	int channels;
	float peak_level;
	GdkPixbuf* overview;
	gtk_tree_model_get(model, &iter, COL_FNAME, &sample->dir, COL_NAME, &sample->sample_name, COL_FNAME, &dir_path, COL_IDX, &id, COL_LENGTH, &length, COL_SAMPLERATE, &samplerate, COL_CHANNELS, &channels, COL_KEYWORDS, &sample->keywords, COL_MIMETYPE, &mimetype, COL_PEAKLEVEL, &peak_level, COL_COLOUR, &colour_index, COL_OVERVIEW, &overview, COL_NOTES, &notes, COL_MISC, &misc, -1);
	g_return_val_if_fail(mimetype, NULL);

	float sample_rate = atof(samplerate) * 1000;

	sample->idx    = id;
	sample->length = atof(length) * 1000;
	sample->sample_rate = sample_rate;
	sample->overview = NULL;
	sample->channels = channels;
	sample->row_ref = gtk_tree_row_reference_new(GTK_TREE_MODEL(app.store), path);
	sample->mimetype = mimetype;
	sample->peak_level = peak_level;
	sample->misc = misc;
	sample->overview = overview;
	sample->notes = notes;
	sample->full_path = g_strdup_printf("%s/%s", dir_path, sample->sample_name);
	return sample;
}


void
result_free(Sample* result)
{
	if(result->row_ref) gtk_tree_row_reference_free(result->row_ref);

	if(result->sample_name) g_free(result->sample_name);
	if(result->full_path) g_free(result->full_path);
	g_free(result);
}


