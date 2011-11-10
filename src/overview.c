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


static double calc_signal_max (Sample* sample) {
	return ad_maxsignal(sample->filename);
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
				dbg(1, "queuing for overview: filename=%s.", sample->filename);
				make_overview(sample);
				g_idle_add(on_overview_done, sample);
			}
			else if(message->type == MSG_TYPE_PEAKLEVEL){
				sample->peak_level = calc_signal_max(message->sample);
				g_idle_add(on_peaklevel_done, sample);
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

	dbg(2, "sending message: sample=%p filename=%s (%p)", sample, sample->filename, sample->filename);
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

	dbg(2, "sending message: sample=%p filename=%s (%p)", sample, sample->filename, sample->filename);
	sample_ref(sample);
	g_async_queue_push(app.msg_queue, message);
}
