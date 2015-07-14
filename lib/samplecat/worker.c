/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2007-2015 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>
#include <glib.h>
#include "debug/debug.h"
#include "typedefs.h"
#include "src/sample.h"

typedef struct
{
    Sample*        sample;
    SampleCallback work;
    SampleCallback done;
	gpointer       user_data;
} Message;

static GAsyncQueue*  msg_queue = NULL;
static GList*        msg_list  = NULL;
static GList*        clients   = NULL;

static gpointer worker_thread (gpointer data);
static bool     send_progress (gpointer);

void
worker_thread_init()
{
	dbg(3, "creating overview thread...");

	msg_queue = g_async_queue_new();

	if(!g_thread_new("worker", worker_thread, NULL)){
		perr("failed to create worker thread");
	}
}


static gpointer
worker_thread(gpointer data)
{
	//TODO consider replacing the main loop with a blocking call on the async queue,
	//(g_async_queue_pop) waiting for messages.

	dbg(1, "new worker thread.");

	g_async_queue_ref(msg_queue);

	bool done(gpointer _message)
	{
		Message* message = _message;
		message->done(message->sample, message->user_data);

		send_progress(GINT_TO_POINTER(g_list_length(msg_list)));

		sample_unref(message->sample);
		g_free(message);
		return G_SOURCE_REMOVE;
	}

	bool worker_timeout(gpointer data)
	{
		// check for new work
		while(g_async_queue_length(msg_queue)){
			Message* message = g_async_queue_pop(msg_queue); // blocks
			msg_list = g_list_append(msg_list, message);
			dbg(2, "new message! %p", message);
		}

		while(msg_list){
			Message* message = g_list_first(msg_list)->data;
			msg_list = g_list_remove(msg_list, message);

			Sample* sample = message->sample;
			message->work(sample, message->user_data);
			g_idle_add(done, message);

			g_usleep(1000); // may possibly help prevent tree freezes if too many jobs complete in quick succession.
		}

		return G_SOURCE_CONTINUE;
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
worker_add_job(Sample* sample, SampleCallback work, SampleCallback done, gpointer user_data)
{
	if(!msg_queue) return;

	Message* message = g_new0(Message, 1);
	*message = (Message){
		.sample = sample_ref(sample),
		.work = work,
		.done = done,
		.user_data = user_data
	};

	g_async_queue_push(msg_queue, message);
}


void
worker_register(Callback callback)
{
	clients = g_list_prepend(clients, callback);
}


static bool
send_progress(gpointer user_data)
{
	GList* l = clients;
	for(;l;l=l->next){
		((Callback)l->data)(user_data);
	}

	return G_SOURCE_REMOVE;
}


