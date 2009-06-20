/*

Copyright (C) Tim Orford 2007-2009

This software is licensed under the GPL. See accompanying file COPYING.

*/
#include "config.h"
#ifdef USE_TRACKER
#include <stdio.h>
#include <stdlib.h>
#define __USE_GNU
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <sndfile.h>
#include <gtk/gtk.h>

#include "typedefs.h"
#include "mysql/mysql.h"
#include "support.h"
#include "tracker.h"
#include <src/tracker_.h>

TrackerClient* tc = NULL;


gboolean
tracker__init(gpointer data)
{
	//note: trackerd doesnt have to be running - it will be auto-started.
	if((tc = tracker_connect(TRUE))){
		tracker__search(tc);
		tracker_disconnect (tc);
	}
	else warnprintf("cant connect to tracker daemon.");
	return IDLE_STOP;
}


void
tracker__search()
{
	char **p_strarray;
#if 0
	gchar* search = g_strdup("test");
	gint limit = 10;

	ServiceType type = SERVICE_FILES;
	GError* error = NULL;
	dbg(0, "doing tracker search...");
	//FIXME this is a synchronous call, so app will be blocked if no quick reply.
	gchar** result = tracker_search_text (tc, -1, type, search, 0, limit, &error);
	dbg(0, "got tracker result.");
	g_free (search);
	if (!error) {
		if (!result) dbg(0, "no tracker results");
		if (!*result) dbg(0, "no tracker results");
		for (p_strarray = result; *p_strarray; p_strarray++) {
			char *s = g_locale_from_utf8 (*p_strarray, -1, NULL, NULL, NULL);
			if (!s) continue;
			g_print ("  %s\n", s);
			g_free (s);
		}

	} else {
		warnprintf("internal tracker error: %s\n", error->message);
		g_error_free (error);
	}

	g_strfreev (result);
	g_free (search);
#endif

	//--------------------------------------------------------------

	void wav_reply(char** result, GError* error, gpointer user_data)
	{
		dbg(0, "reply!");
		if (error) {
			warnprintf("internal tracker error: %s\n", error->message);
			g_error_free (error);
			return;
		}
	}
	char* mimes[4];
	mimes[0] = g_strdup("audio/x-wav");
	mimes[1] = NULL;
	GError* error = NULL;
	//tracker_files_get_by_mime_type_async(tc, 1, mimes, 0, 100, wav_reply, NULL);
	gchar** result = tracker_files_get_by_mime_type(tc, 1, mimes, 0, 100, &error);

	dbg(0, "got tracker result.");
	if (!error) {
		if (!result) dbg(0, "no tracker results");
		if (!*result) dbg(0, "no items found.");
		for (p_strarray = result; *p_strarray; p_strarray++) {
			char *s = g_locale_from_utf8 (*p_strarray, -1, NULL, NULL, NULL);
			if (!s) continue;
			g_print ("  %s\n", s);
			g_free (s);
		}

	} else {
		warnprintf("internal tracker error: %s\n", error->message);
		g_error_free (error);
	}

	g_strfreev (result);
}


gboolean
tracker__search_iter_new(char* search, char* dir)
{
	return false;
}


void
tracker__search_iter_free()
{
}


#endif //USE_TRACKER
