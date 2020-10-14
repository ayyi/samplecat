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
#include <math.h>
#include <sys/time.h>
#include <glib.h>
#include "debug/debug.h"
#include "typedefs.h"
#include "samplecat/sample.h"
#include "samplecat/support.h"

bool worker_go_slow = false;

typedef struct
{
    Sample*        sample;
    SampleCallback work;
    SampleCallback done;
	gpointer       user_data;
} Message;

static GAsyncQueue*  msg_queue = NULL;
static GList*        clients   = NULL;

static gpointer worker_thread (gpointer data);
static bool     send_progress (gpointer);

void
worker_thread_init ()
{
	dbg(3, "creating overview thread...");

	msg_queue = g_async_queue_new();

	if(!g_thread_new("worker", worker_thread, NULL)){
		perr("failed to create worker thread");
	}
}


static gpointer
worker_thread (gpointer data)
{
	dbg(1, "new worker thread.");

	g_async_queue_ref(msg_queue);

	gboolean done (gpointer _message)
	{
		Message* message = _message;
		message->done(message->sample, message->user_data);

		send_progress(GINT_TO_POINTER(g_async_queue_length(msg_queue)));

		sample_unref(message->sample);
		g_free(message);

		return G_SOURCE_REMOVE;
	}

	while(true){
		Message* message = g_async_queue_pop(msg_queue); // blocks
		dbg(2, "new message! %p", message);

		message->work(message->sample, message->user_data);
		g_idle_add(done, message);

		if(worker_go_slow)
			/*
			 *  Scan in progress - because of disc contention, analysis tasks cannot run in parallel
			 */
			g_usleep(5000 * 1000);
		else
			/*
			 *  A long sleep is needed here to allow foreground to update,
			 */
			g_usleep(500 * 1000);
	}

	return NULL;
}


void
worker_add_job (Sample* sample, SampleCallback work, SampleCallback done, gpointer user_data)
{
	if(!msg_queue) return;

	g_async_queue_push(msg_queue, SC_NEW(Message,
		.sample = sample_ref(sample),
		.work = work,
		.done = done,
		.user_data = user_data
	));
}


void
worker_register (Callback callback)
{
	clients = g_list_prepend(clients, callback);
}


static bool
send_progress (gpointer user_data)
{
	GList* l = clients;
	for(;l;l=l->next){
		((Callback)l->data)(user_data);
	}

	return G_SOURCE_REMOVE;
}


