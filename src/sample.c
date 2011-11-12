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
//#if (defined HAVE_JACK && !(defined USE_DBUS || defined USE_GAUDITION))
//#include "jack_player.h"
//#endif

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
	sample->colour_index = 0; // XXX colour_index of app.bg_colour // app.config.colour[colour_index]
	sample_ref(sample); // TODO: CHECK using functions!
	return sample;
}

Sample*
sample_new_from_filename(char *path, gboolean path_alloced)
{
	if (!file_exists(path)) {
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

	if(!sample->sample_name){
		sample->sample_name= g_path_get_basename(sample->full_path);
	}
	if(!sample->dir){
		sample->dir = g_path_get_dirname(sample->full_path);
	}
	return sample;
}


Sample*
sample_dup(Sample* s)
{
	Sample* r = g_new0(struct _sample, 1);
	memcpy(r,s, sizeof(Sample));
	// XXX: duplicate all string texts !!! -> free static in db ..
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
	if(sample->misc) g_free(sample->misc);
	if(sample->notes) g_free(sample->notes);
	if(sample->updated) g_free(sample->updated);
	if(sample->dir) g_free(sample->dir);
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


// TODO move to audio_analysis/analyzers.c ?!
#include "audio_decoder/ad.h"
#include "audio_analysis/ebumeter/ebur128.h"
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

#if 0
	// TODO -- analyze in background when creating overview.
	struct ebur128 ebur;
	if (!ebur128analyse(sample->full_path, &ebur)){
		if (sample->misc) free(sample->misc);
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
	}
#endif
	return true;
}

Sample*
sample_new_from_model(GtkTreePath* path)
{
	GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(app.view));
	GtkTreeIter iter;
	gtk_tree_model_get_iter(model, &iter, path);
	// XXX TODO store pointer to sample in model!
#if 0
	Sample* sample;
	gtk_tree_model_get(model, &iter, COL_SAMPLEPTR, &sample, -1);
	//if (!sample->row_ref) sample->row_ref = gtk_tree_row_reference_new(GTK_TREE_MODEL(app.store), path);
	sample_ref(sample);
	return sample;
#else
	Sample* sample = g_new0(Sample, 1);

	gchar* mimetype, *length, *notes, *misc, *dir_path; int id; unsigned colour_index;
	gchar* samplerate;
	int channels;
	float peak_level;
	GdkPixbuf* overview;
	gtk_tree_model_get(model, &iter, COL_FNAME, &sample->dir, COL_NAME, &sample->sample_name, COL_FNAME, &dir_path, COL_IDX, &id, COL_LENGTH, &length, COL_SAMPLERATE, &samplerate, COL_CHANNELS, &channels, COL_KEYWORDS, &sample->keywords, COL_MIMETYPE, &mimetype, COL_PEAKLEVEL, &peak_level, COL_COLOUR, &colour_index, COL_OVERVIEW, &overview, COL_NOTES, &notes, COL_MISC, &misc, -1);
	g_return_val_if_fail(mimetype, NULL);

	float sample_rate = atof(samplerate) * 1000;

	sample->id     = id;
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
	sample_ref(sample);
	return sample;
#endif
}
