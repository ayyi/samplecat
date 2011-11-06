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
#include "support.h"
#include "main.h"
#include "audio.h"
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
	if(!strcmp(mimetype, "audio/x-flac")) sample->filetype = TYPE_FLAC; else sample->filetype = TYPE_SNDFILE; 
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
sample_new_from_result(SamplecatResult* r)
{
	Sample* s      = g_new0(struct _sample, 1);
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


gboolean
sample_get_file_sndfile_info(Sample* sample)
{
	char *filename = sample->filename;

	SF_INFO        sfinfo;   //the libsndfile struct pointer
	SNDFILE        *sffile;
	sfinfo.format  = 0;

	if(!(sffile = sf_open(filename, SFM_READ, &sfinfo))){
		dbg(0, "not able to open input file %s.", filename);
		puts(sf_strerror(NULL));    // print the error message from libsndfile:
		int e = sf_error(NULL);
		dbg(0, "error=%i", e);
		return false;
	}

	char chanwidstr[64];
	if     (sfinfo.channels==1) snprintf(chanwidstr, 64, "mono");
	else if(sfinfo.channels==2) snprintf(chanwidstr, 64, "stereo");
	else                        snprintf(chanwidstr, 64, "channels=%i", sfinfo.channels);
	if(debug) printf("%iHz %s frames=%i\n", sfinfo.samplerate, chanwidstr, (int)sfinfo.frames);
	sample->channels    = sfinfo.channels;
	sample->sample_rate = sfinfo.samplerate;
	sample->length      = sfinfo.samplerate ? (sfinfo.frames * 1000) / sfinfo.samplerate : 0;

	if(sample->channels<1 || sample->channels>100){ dbg(0, "bad channel count: %i", sample->channels); return false; }
	if(sf_close(sffile)) perr("bad file close.\n");

	return true;
}


gboolean
sample_get_file_info(Sample* sample)
{
#ifdef HAVE_FLAC_1_1_1
	if(sample->filetype==TYPE_FLAC) return get_file_info_flac(sample);
#else
	if(0){}
#endif
	else                            return sample_get_file_sndfile_info(sample);
}


gboolean
result_get_file_sndfile_info(Result* sample)
{
	char *filename = sample->sample_name;

	SF_INFO        sfinfo;   //the libsndfile struct pointer
	SNDFILE        *sffile;
	sfinfo.format  = 0;

	if(!(sffile = sf_open(filename, SFM_READ, &sfinfo))){
		dbg(0, "not able to open input file %s.", filename);
		puts(sf_strerror(NULL));    // print the error message from libsndfile:
		int e = sf_error(NULL);
		dbg(0, "error=%i", e);
		return false;
	}

	char chanwidstr[64];
	if     (sfinfo.channels==1) snprintf(chanwidstr, 64, "mono");
	else if(sfinfo.channels==2) snprintf(chanwidstr, 64, "stereo");
	else                        snprintf(chanwidstr, 64, "channels=%i", sfinfo.channels);
	if(debug) printf("%iHz %s frames=%i\n", sfinfo.samplerate, chanwidstr, (int)sfinfo.frames);
	sample->channels    = sfinfo.channels;
	sample->sample_rate = sfinfo.samplerate;
	sample->length      = (sfinfo.frames * 1000) / sfinfo.samplerate;

	if(sample->channels<1 || sample->channels>100){ dbg(0, "bad channel count: %i", sample->channels); return false; }
	if(sf_close(sffile)) perr("bad file close.\n");

	return true;
}


void
sample_set_type_from_mime_string(Sample* sample, char* mime_string)
{
	if(!strcmp(mime_string, "audio/x-flac")) sample->filetype = TYPE_FLAC;
	dbg(2, "mimetype: %s type=%i", mime_string, sample->filetype);
}


Result*
result_new_from_model(GtkTreePath* path)
{
	GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(app.view));
	GtkTreeIter iter;
	gtk_tree_model_get_iter(model, &iter, path);

	Result* sample = g_new0(Result, 1);

	gchar* mimetype, *length, *notes, *dir_path; int id; unsigned colour_index;
	gchar* samplerate;
	int channels;
	float peak_level;
	GdkPixbuf* overview;
	gtk_tree_model_get(model, &iter, COL_FNAME, &sample->dir, COL_NAME, &sample->sample_name, COL_FNAME, &dir_path, COL_IDX, &id, COL_LENGTH, &length, COL_SAMPLERATE, &samplerate, COL_CHANNELS, &channels, COL_KEYWORDS, &sample->keywords, COL_MIMETYPE, &mimetype, COL_PEAKLEVEL, &peak_level, COL_COLOUR, &colour_index, COL_OVERVIEW, &overview, COL_NOTES, &notes, -1);
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
	sample->overview = overview;
	sample->notes = notes;
	sample->full_path = g_strdup_printf("%s/%s", dir_path, sample->sample_name);
	return sample;
}


void
result_free(Result* result)
{
	if(result->row_ref) gtk_tree_row_reference_free(result->row_ref);

	if(result->sample_name) g_free(result->sample_name);
	if(result->full_path) g_free(result->full_path);
	g_free(result);
}


