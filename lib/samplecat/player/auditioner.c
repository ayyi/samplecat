/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. https://ayyi.github.io/samplecat/    |
 | copyright (C) 2007-2024 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#include "config.h"
#undef USE_GTK
#include <glib.h>
#include <dbus/dbus-glib-bindings.h>

#include "debug/debug.h"
#include "player.h"
#include "samplecat/support.h"
#include "samplecat/sample.h"

typedef struct
{
	DBusGProxy* proxy;
} AyyiConnection;

#define APPLICATION_SERVICE_NAME "org.ayyi.Auditioner.Daemon"
#define DBUS_APP_PATH            "/org/ayyi/auditioner/daemon"
#define DBUS_INTERFACE           "org.ayyi.auditioner.Daemon"

typedef void (*StatusCallback) (char*, int, GError*, gpointer);

static AyyiConnection* dbus;

static int  auditioner_check      ();
static void auditioner_connect    (ErrorCallback, gpointer);
static void auditioner_disconnect ();
static bool auditioner_play       (Sample*);
static void auditioner_stop       ();
static void auditioner_play_all   ();
static void auditioner_status     (StatusCallback, gpointer);
static void auditioner_set_level  (double);

const Auditioner a_ayyidbus = {
	&auditioner_check,
	&auditioner_connect,
	&auditioner_disconnect,
	&auditioner_play,
	&auditioner_play_all,
	&auditioner_stop,
	.set_level = auditioner_set_level,
};


typedef struct {
	ErrorCallback callback;
	gpointer user_data;
} C;


static void
on_really_connected (char* status, int len, GError* error, gpointer user_data)
{
	C* c = user_data;

	if (!error) {
		dbus_g_proxy_add_signal(dbus->proxy, "PlaybackStopped", G_TYPE_INVALID);
		dbus_g_proxy_add_signal(dbus->proxy, "PlaybackStatus", G_TYPE_INT, G_TYPE_INVALID);
		dbus_g_proxy_add_signal(dbus->proxy, "Position", G_TYPE_DOUBLE, G_TYPE_DOUBLE, G_TYPE_INVALID);

		void on_status (DBusGProxy* proxy, int status, gpointer user_data)
		{
			if (status) {
				player_on_play();
			} else {
				player_on_play_finished();
			}
		}
		dbus_g_proxy_connect_signal (dbus->proxy, "PlaybackStatus", G_CALLBACK(on_status), NULL, NULL);

		void on_position (DBusGProxy* proxy, double seconds, double length, gpointer user_data)
		{
			player_set_position_seconds(seconds);
		}
		dbus_g_proxy_connect_signal (dbus->proxy, "Position", G_CALLBACK(on_position), NULL, NULL);

#if 0
		// Samplecat has its own queue, it does not use the auditioner queue

		dbus_g_proxy_add_signal(dbus->proxy, "QueueChange", G_TYPE_INT, G_TYPE_INVALID);
		void on_queue_change (DBusGProxy* proxy, int len, gpointer user_data) {}
		dbus_g_proxy_connect_signal (dbus->proxy, "QueueChange", G_CALLBACK(on_queue_change), NULL, NULL);
#endif
	}

	if (c->callback) c->callback(error, c->user_data);

	g_free(c);
}


void
auditioner_connect (ErrorCallback callback, gpointer user_data)
{
	dbus = g_new0(AyyiConnection, 1);

	gboolean _auditioner_connect (gpointer user_data)
	{
		C* c = user_data;
		GError* error = NULL;

		DBusGConnection* bus = dbus_g_bus_get(DBUS_BUS_SESSION, &error);
		if (!bus) {
			error = g_error_new_literal(g_quark_from_static_string(AUDITIONER_DOMAIN), 1, "failed to get dbus connection");
			goto error;
		}
		if (!(dbus->proxy = dbus_g_proxy_new_for_name (bus, APPLICATION_SERVICE_NAME, DBUS_APP_PATH, DBUS_INTERFACE))) {
			error = g_error_new_literal(g_quark_from_static_string(AUDITIONER_DOMAIN), 1, "failed to get Auditioner");
			goto error;
		}

		// Even if we get here, it doesn't mean the service is running.
		// In order to be able to report problems, an actual service
		// call is made.
		auditioner_status(on_really_connected, c);

		return G_SOURCE_REMOVE;

	error:
		if (c->callback) c->callback(error, c->user_data);
		g_error_free(error);
		return G_SOURCE_REMOVE;
	}

	g_idle_add(_auditioner_connect, SC_NEW(C,
		.callback = callback,
		.user_data = user_data
	));
}


void
auditioner_disconnect ()
{
}


static int
auditioner_check ()
{
	// It is not possible to check the service without starting it so we return OK
	return 0;
}


bool
auditioner_play (Sample* sample)
{
	dbg(1, "%s", sample->full_path);
	dbus_g_proxy_call_no_reply(dbus->proxy, "StartPlayback", G_TYPE_STRING, sample->full_path, G_TYPE_INVALID);
	return true;
}


static void
auditioner_stop ()
{
	dbg(1, "...");
	dbus_g_proxy_call_no_reply(dbus->proxy, "StopPlayback", G_TYPE_STRING, "", G_TYPE_INVALID);
}


void
auditioner_play_all ()
{
	static void (*stop)(DBusGProxy*, gpointer) = NULL;

	void play_next ()
	{
		if (!play->queue) {
			dbus_g_proxy_disconnect_signal(dbus->proxy, "PlaybackStopped", G_CALLBACK(stop), NULL);
		}
		play->next();
	}

	void playall__on_stopped (DBusGProxy* proxy, gpointer user_data)
	{
		play_next();
	}
	stop = playall__on_stopped;

	dbus_g_proxy_connect_signal (dbus->proxy, "PlaybackStopped", G_CALLBACK(playall__on_stopped), NULL, NULL);
}


static void
auditioner_set_level (double level)
{
	GError* error = NULL;
	dbus_g_proxy_call(dbus->proxy, "Level", &error, G_TYPE_DOUBLE, level, G_TYPE_INVALID, G_TYPE_INVALID);
	if (error) {
		pwarn("error: %s", error->message);
		g_error_free(error);
	}
}


typedef struct {
    StatusCallback callback;
    gpointer user_data;
} C2;


static void
audtioner_status_reply (DBusGProxy* proxy, DBusGProxyCall* call, gpointer data)
{
	C2* c = data;

	char* status;
	int queue_size;
	GError* error = NULL;
	dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_STRING, &status, G_TYPE_INT, &queue_size, G_TYPE_INVALID);

	c->callback(status, queue_size, error, c->user_data);

	if (error) {
		printf("%s\n", error->message);
		g_error_free(error);
	} else {
		g_free(status);
	}
	g_free(c);
}


static void
auditioner_status (StatusCallback callback, gpointer user_data)
{
	dbus_g_proxy_begin_call(dbus->proxy, "getStatus", audtioner_status_reply, SC_NEW(C2, .callback=callback, .user_data=user_data), NULL, G_TYPE_INVALID);
}
