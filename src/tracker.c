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
#include "src/types.h"
#include "tracker.h"
#include <src/tracker_.h>

extern int debug;
//extern struct _app app;
void set_backend(BackendType);

static TrackerClient* tc = NULL;
static SamplecatResult result;
struct _iter
{
	gchar** result;
	char**  p_strarray;
	int     idx;
};
struct _iter iter = {};


gboolean
tracker__init(gpointer data)
{
	PF;
	//note: trackerd doesnt have to be running - it will be auto-started.
	if((tc = tracker_connect(TRUE))){
		set_backend(BACKEND_TRACKER);

		//temporary:
		void do_search(char* search, char *dir);
		do_search(NULL, NULL);

		//tracker_disconnect (tc); //FIXME we dont disconnect!
	}
	else warnprintf("cant connect to tracker daemon.");
	return IDLE_STOP;
}


#if 0
void
tracker__search()
{
	PF;
	char **p_strarray;

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
}
#endif


gboolean
tracker__search_iter_new(char* search, char* dir)
{
	PF;

	char* mimes[4];
	mimes[0] = g_strdup("audio/x-wav");
	mimes[1] = NULL;
	GError* error = NULL;

	gint limit = 100; //FIXME
	ServiceType type = SERVICE_FILES;

	iter.idx = 0;
	//warning! this is a synchronous call, so app will be blocked if no quick reply.
	//iter.result = tracker_files_get_by_mime_type(tc, 1, mimes, 0, limit, &error);
	iter.result = tracker_search_text (tc, -1, type, search, 0, limit, &error);
	iter.p_strarray = iter.result;
	if(iter.result && !error){
		return true;
	}

	if(error){
		warnprintf("internal tracker error: %s\n", error->message);
		g_error_free (error);
	}
	return false;
}


SamplecatResult*
tracker__search_iter_next()
{
	iter.p_strarray++;
	iter.idx++;
	if(!iter.p_strarray) return NULL;
	if(!*iter.p_strarray) return NULL;

	memset(&result, 0, sizeof(SamplecatResult));

	char* s = g_locale_from_utf8 (*iter.p_strarray, -1, NULL, NULL, NULL);
	if (s) {
		g_print ("  %i: %s\n", iter.idx, s);
		result.sample_name = g_path_get_basename(s);
		result.dir = g_path_get_dirname(s);
		result.idx = iter.idx; //TODO this idx is pretty meaningless.
		result.mime_type = "audio/x-wav";
		g_free (s);
	}
	else return NULL;

	return &result;
}


void
tracker__search_iter_free()
{
	g_strfreev (iter.result);
}


#endif //USE_TRACKER
