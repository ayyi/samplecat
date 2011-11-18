#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>
#include <gtk/gtk.h>

#include "file_manager.h"
#include "typedefs.h"
#include "support.h"
#include "main.h"
#include "overview.h"
#include "sample.h"
#include "cellrenderer_hypertext.h"

#include "mimetype.h"
#include "pixmaps.h"

#include "audio_decoder/ad.h"
#include "audio_analysis/waveform/waveform.h"

extern struct _app app;
extern unsigned debug;

static GList* msg_list = NULL;

// TODO move to audio_analysis/analyzers.c ?!
#include "audio_analysis/meter/peak.h"
static double calc_signal_max (Sample* sample) {
	return ad_maxsignal(sample->full_path);
}

#include "audio_decoder/ad.h"
#include "audio_analysis/ebumeter/ebur128.h"
static void
calc_ebur128(Sample* sample)
{
	struct ebur128 ebur;
	if (!ebur128analyse(sample->full_path, &ebur)){
		if (sample->ebur) free(sample->ebur);
		sample->ebur      = g_strdup_printf(
			"<small><tt>"
			"Integrated loudness:   %6.1lf LU%s\n"
			"Loudness range:        %6.1lf LU\n"
			"Integrated threshold:  %6.1lf LU%s\n"
			"Range threshold:       %6.1lf LU%s\n"
			"Range min:             %6.1lf LU%s\n"
			"Range max:             %6.1lf LU%s\n"
			"Momentary max:         %6.1lf LU%s\n"
			"Short term max:        %6.1lf LU%s\n"
			"</tt></small>"
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
}

gpointer
overview_thread(gpointer data)
{
	//TODO consider replacing the main loop with a blocking call on the async queue,
	//(g_async_queue_pop) waiting for messages.

	dbg(1, "new overview thread.");

	if(!app.msg_queue){ perr("no msg_queue!\n"); return NULL; }

	g_async_queue_ref(app.msg_queue);

	gboolean worker_timeout(gpointer data)
	{

		//check for new work
		while(g_async_queue_length(app.msg_queue)){
			struct _message* message = g_async_queue_pop(app.msg_queue); // blocks
			msg_list = g_list_append(msg_list, message);
			dbg(2, "new message! %p", message);
		}

		while(msg_list != NULL){
			struct _message* message = g_list_first(msg_list)->data;
			msg_list = g_list_remove(msg_list, message);

			Sample* sample = message->sample;
			if(message->type == MSG_TYPE_OVERVIEW){
				dbg(1, "queuing for overview: filename=%s.", sample->full_path);
				make_overview(sample);
				g_idle_add(on_overview_done, sample);
			}
			else if(message->type == MSG_TYPE_PEAKLEVEL){
				sample->peaklevel = calc_signal_max(message->sample);
				g_idle_add(on_peaklevel_done, sample);
			}
			else if(message->type == MSG_TYPE_EBUR128){
				calc_ebur128(message->sample);
				g_idle_add(on_ebur128_done, sample);
			}

			g_free(message);
		}

		return TIMER_CONTINUE;
	}

	GMainContext* context = g_main_context_new();
	GSource* source = g_timeout_source_new(1000);
	gpointer _data = NULL;
	g_source_set_callback(source, worker_timeout, _data, NULL);
	g_source_attach(source, context);

	g_main_loop_run (g_main_loop_new (context, 0));
	return NULL;
}


void
request_overview(Sample* sample)
{
	if(!app.msg_queue) return;

	struct _message* message = g_new0(struct _message, 1);
	message->type = MSG_TYPE_OVERVIEW;
	message->sample = sample;

	dbg(2, "sending message: sample=%p filename=%s", sample, sample->full_path);
	sample_ref(sample);
	g_async_queue_push(app.msg_queue, message); //notify the overview thread.
}

void
request_peaklevel(Sample* sample)
{
	if(!app.msg_queue) return;

	struct _message* message = g_new0(struct _message, 1);
	message->type = MSG_TYPE_PEAKLEVEL;
	message->sample = sample;

	dbg(2, "sending message: sample=%p filename=%s", sample, sample->full_path);
	sample_ref(sample);
	g_async_queue_push(app.msg_queue, message);
}

void
request_ebur128(Sample* sample)
{
	if(!app.msg_queue) return;

	struct _message* message = g_new0(struct _message, 1);
	message->type = MSG_TYPE_EBUR128;
	message->sample = sample;

	dbg(2, "sending message: sample=%p filename=%s", sample, sample->full_path);
	sample_ref(sample);
	g_async_queue_push(app.msg_queue, message);
}
