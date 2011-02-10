/*

Samplecat

Copyright (C) Tim Orford 2007-2011

This software is licensed under the GPL. See accompanying file COPYING.

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

#include "typedefs.h"
#include "support.h"
#include "sample.h"
#include "main.h"
#include "auditioner.h"

#define APPLICATION_SERVICE_NAME "org.ayyi.Auditioner.Daemon"
#define DBUS_APP_PATH            "/org/ayyi/auditioner/daemon"
#define DBUS_INTERFACE           "org.ayyi.auditioner.Daemon"

extern struct _app app;

void
auditioner_connect()
{
	app.auditioner = g_new0(Auditioner, 1);

	gboolean _auditioner_connect(gpointer _)
	{
		GError* error = NULL;
		DBusGConnection* auditioner = dbus_g_bus_get(DBUS_BUS_SESSION, &error);
		if(!auditioner){ 
			errprintf("failed to get dbus connection\n");
			return FALSE;
		}   
		if(!(app.auditioner->proxy = dbus_g_proxy_new_for_name (auditioner, APPLICATION_SERVICE_NAME, DBUS_APP_PATH, DBUS_INTERFACE))){
			errprintf("failed to get Auditioner\n");
			return FALSE;
		}

		dbus_g_proxy_add_signal(app.auditioner->proxy, "PlaybackStopped", G_TYPE_INVALID);                                                             

		return IDLE_STOP;
	}
	g_idle_add(_auditioner_connect, NULL);
}


void
auditioner_play(Sample* sample)
{
	dbg(1, "%s", sample->filename);
	dbus_g_proxy_call_no_reply(app.auditioner->proxy, "StartPlayback", G_TYPE_STRING, sample->filename, G_TYPE_INVALID);
}


void
auditioner_play_result(Result* result)
{
	dbg(1, "%s", result->full_path);
	dbus_g_proxy_call_no_reply(app.auditioner->proxy, "StartPlayback", G_TYPE_STRING, result->full_path, G_TYPE_INVALID);
}


void
auditioner_play_path(const char* path)
{
	g_return_if_fail(path);
	dbus_g_proxy_call_no_reply(app.auditioner->proxy, "StartPlayback", G_TYPE_STRING, path, G_TYPE_INVALID);
}


void
auditioner_stop(Sample* sample)
{
	dbg(1, "...");
	dbus_g_proxy_call_no_reply(app.auditioner->proxy, "StopPlayback", G_TYPE_STRING, sample->filename, G_TYPE_INVALID);
}


void
auditioner_toggle(Sample* sample)
{
	dbg(1, "%s", sample->filename);
	dbus_g_proxy_call_no_reply(app.auditioner->proxy, "TogglePlayback", G_TYPE_STRING, sample->filename, G_TYPE_INVALID);
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
			Result* result = play_queue->data;
			play_queue = g_list_remove(play_queue, result);
			auditioner_play_result(result);
			result_free(result);
		}else{
			dbg(1, "play_all finished. disconnecting...");
			dbus_g_proxy_disconnect_signal(app.auditioner->proxy, "PlaybackStopped", G_CALLBACK(stop), NULL);
		}
	}

	void playall__on_stopped(DBusGProxy* proxy, gpointer user_data)
	{
		play_next();
	}
	stop = playall__on_stopped;

	dbus_g_proxy_connect_signal (app.auditioner->proxy, "PlaybackStopped", G_CALLBACK(playall__on_stopped), NULL, NULL);

	gboolean foreach_func(GtkTreeModel* model, GtkTreePath* path, GtkTreeIter* iter, gpointer user_data)
	{
		Result* result = result_new_from_model(path);
		play_queue = g_list_append(play_queue, result);
		dbg(2, "%s", result->sample_name);
		return FALSE; //continue
	}

	gtk_tree_model_foreach(GTK_TREE_MODEL(app.store), foreach_func, NULL);

	if(play_queue) play_next();
}


