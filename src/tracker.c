/*

Copyright (C) Tim Orford 2007-2008

This software is licensed under the GPL. See accompanying file COPYING.

*/
#ifdef USE_TRACKER
#include "config.h"
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


void
tracker_search(TrackerClient* tc)
{
	gchar* search = g_strdup("test");
	gint limit = 10;

	ServiceType type = SERVICE_FILES;
	GError* error = NULL;
	char **p_strarray;
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
}

#endif //USE_TRACKER
