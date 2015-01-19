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
#include "listview.h"

typedef struct
{
	DBusGProxy*    proxy;
} AyyiConnection;

#define APPLICATION_SERVICE_NAME "org.ayyi.Auditioner.Daemon"
#define DBUS_APP_PATH            "/org/ayyi/auditioner/daemon"
#define DBUS_INTERFACE           "org.ayyi.auditioner.Daemon"

static AyyiConnection* adbus;

static int  auditioner_check      ();
static void auditioner_connect    (Callback, gpointer);
static void auditioner_disconnect ();
static bool auditioner_play       (Sample*);
static void auditioner_stop       ();
static void auditioner_play_all   ();

const Auditioner a_ayyidbus = {
	&auditioner_check,
	&auditioner_connect,
	&auditioner_disconnect,
	&auditioner_play,
	&auditioner_play_all,
	&auditioner_stop,
	NULL,
	NULL,
	NULL
};


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
		dbus_g_proxy_add_signal(adbus->proxy, "Position", G_TYPE_INT, G_TYPE_INVALID);                                                             

		void on_stopped(DBusGProxy* proxy, gpointer user_data)
		{
			if(!app->play.queue){
				application_on_play_finished();
			}
		}
		dbus_g_proxy_connect_signal (adbus->proxy, "PlaybackStopped", G_CALLBACK(on_stopped), NULL, NULL);

		void on_position(DBusGProxy* proxy, int position, gpointer user_data)
		{
			application_set_position(position);
		}
		dbus_g_proxy_connect_signal (adbus->proxy, "Position", G_CALLBACK(on_position), NULL, NULL);

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


static int
auditioner_check()
{
	// It is not possible to check the service without starting it so we return OK
	return 0;
}


bool
auditioner_play(Sample* sample)
{
	dbg(1, "%s", sample->full_path);
	dbus_g_proxy_call_no_reply(adbus->proxy, "StartPlayback", G_TYPE_STRING, sample->full_path, G_TYPE_INVALID);
	return true;
}


void
auditioner_stop()
{
	dbg(1, "...");
	dbus_g_proxy_call_no_reply(adbus->proxy, "StopPlayback", G_TYPE_STRING, "", G_TYPE_INVALID);
}


void
auditioner_play_all()
{
	static void (*stop)(DBusGProxy*, gpointer) = NULL;

	void play_next()
	{
		if(!app->play.queue){
			dbus_g_proxy_disconnect_signal(adbus->proxy, "PlaybackStopped", G_CALLBACK(stop), NULL);
		}
		application_play_next();
	}

	void playall__on_stopped(DBusGProxy* proxy, gpointer user_data)
	{
		play_next();
	}
	stop = playall__on_stopped;

	dbus_g_proxy_connect_signal (adbus->proxy, "PlaybackStopped", G_CALLBACK(playall__on_stopped), NULL, NULL);
}


