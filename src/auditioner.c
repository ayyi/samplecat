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

		return IDLE_STOP;
	}
	g_idle_add(_auditioner_connect, NULL);
}


void
auditioner_play(Sample* sample)
{
	dbg(0, "%s", sample->filename);
	dbus_g_proxy_call_no_reply(app.auditioner->proxy, "StartPlayback", G_TYPE_STRING, sample->filename, G_TYPE_INVALID);
}


void
auditioner_stop(Sample* sample)
{
	dbg(0, "...");
	dbus_g_proxy_call_no_reply(app.auditioner->proxy, "StopPlayback", G_TYPE_STRING, sample->filename, G_TYPE_INVALID);
}


void
toggle_playback(Sample* sample)
{
	dbg(0, "...");
	dbus_g_proxy_call_no_reply(app.auditioner->proxy, "TogglePlayback", G_TYPE_STRING, sample->filename, G_TYPE_INVALID);
}


