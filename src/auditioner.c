/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2007-2014 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#define __audtioner_c__
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#define __USE_GNU
#include <string.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <dbus/dbus-glib-bindings.h>

#include "debug/debug.h"
#include "typedefs.h"
#include "support.h"
#include "sample.h"
#include "application.h"
#include "auditioner.h"
#include "listview.h"

typedef struct
{
	DBusGProxy*    proxy;
} AyyiConnection;

#define APPLICATION_SERVICE_NAME "org.ayyi.Auditioner.Daemon"
#define DBUS_APP_PATH            "/org/ayyi/auditioner/daemon"
#define DBUS_INTERFACE           "org.ayyi.auditioner.Daemon"

static AyyiConnection *adbus;


void
auditioner_connect(Callback callback, gpointer user_data)
{
	adbus = g_new0(AyyiConnection, 1);

	typedef struct {
		Callback callback;
		gpointer user_data;
	} C;
	C* c = g_new(C, 1);
	c->callback = callback;
	c->user_data = user_data;

	gboolean _auditioner_connect(gpointer user_data)
	{
		C* c = user_data;

		GError* error = NULL;
		DBusGConnection* auditioner = dbus_g_bus_get(DBUS_BUS_SESSION, &error);
		if(!auditioner){ 
			errprintf("failed to get dbus connection\n");
			return FALSE;
		}   
		if(!(adbus->proxy = dbus_g_proxy_new_for_name (auditioner, APPLICATION_SERVICE_NAME, DBUS_APP_PATH, DBUS_INTERFACE))){
			errprintf("failed to get Auditioner\n");
			return FALSE;
		}

		dbus_g_proxy_add_signal(adbus->proxy, "PlaybackStopped", G_TYPE_INVALID);                                                             

		c->callback(c->user_data);

		g_free(c);
		return G_SOURCE_REMOVE;
	}
	g_idle_add(_auditioner_connect, c);
}


void
auditioner_disconnect()
{
}


int auditioner_check() {
	// TODO: check if service is available.
	// start `ayyi_auditioner` if possible and re-check.
	return 0; /*OK*/
}


void
auditioner_play(Sample* sample)
{
	dbg(1, "%s", sample->full_path);
	dbus_g_proxy_call_no_reply(adbus->proxy, "StartPlayback", G_TYPE_STRING, sample->full_path, G_TYPE_INVALID);
}


void
auditioner_play_path(const char* path)
{
	g_return_if_fail(path);
	dbus_g_proxy_call_no_reply(adbus->proxy, "StartPlayback", G_TYPE_STRING, path, G_TYPE_INVALID);
}


void
auditioner_stop()
{
	dbg(1, "...");
	dbus_g_proxy_call_no_reply(adbus->proxy, "StopPlayback", G_TYPE_STRING, "", G_TYPE_INVALID);
}


void
auditioner_toggle(Sample* sample)
{
	dbg(1, "%s", sample->full_path);
	dbus_g_proxy_call_no_reply(adbus->proxy, "TogglePlayback", G_TYPE_STRING, sample->full_path, G_TYPE_INVALID);
}


void
auditioner_play_all()
{
	//maybe add queue() fn to auditioner?

	static GList* play_queue = NULL;
	if(play_queue){
		pwarn("already playing");
		return;
	}

	static void (*stop)(DBusGProxy*, gpointer) = NULL;

	void play_next()
	{
		if(play_queue){
			Sample* result = play_queue->data;
			play_queue = g_list_remove(play_queue, result);
			highlight_playing_by_ref(result->row_ref);
			auditioner_play(result);
			sample_unref(result);
		}else{
			dbg(1, "play_all finished. disconnecting...");
			dbus_g_proxy_disconnect_signal(adbus->proxy, "PlaybackStopped", G_CALLBACK(stop), NULL);
		}
	}

	void playall__on_stopped(DBusGProxy* proxy, gpointer user_data)
	{
		play_next();
	}
	stop = playall__on_stopped;

	dbus_g_proxy_connect_signal (adbus->proxy, "PlaybackStopped", G_CALLBACK(playall__on_stopped), NULL, NULL);

	gboolean foreach_func(GtkTreeModel* model, GtkTreePath* path, GtkTreeIter* iter, gpointer user_data)
	{
		Sample* result = sample_get_from_model(path);
		play_queue = g_list_append(play_queue, result);
		dbg(2, "%s", result->sample_name);
		return FALSE; //continue
	}

	gtk_tree_model_foreach(GTK_TREE_MODEL(app->store), foreach_func, NULL);

	if(play_queue) play_next();
}


