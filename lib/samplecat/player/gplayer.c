/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#include "config.h"
#undef USE_GTK
#include <stdio.h>
#include <stdlib.h>
#define __USE_GNU
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "debug/debug.h"
#include "samplecat/support.h"
#include "samplecat/sample.h"
#include "player.h"

static int  gplayer_check      ();
static void gplayer_connect    (ErrorCallback, gpointer);
static void gplayer_disconnect ();
static bool gplayer_play       (Sample*);
static void gplayer_stop       ();

const Auditioner a_gplayer = {
	&gplayer_check,
	&gplayer_connect,
	&gplayer_disconnect,
	&gplayer_play,
	NULL,
	&gplayer_stop,
	NULL,
	NULL,
	NULL
};


static int
gplayer_check()
{
	char *c;
	if (
		(c = g_find_program_in_path ("afplay")) ||
		(c = g_find_program_in_path ("mplayer")) ||
		(c = g_find_program_in_path ("gst-launch-0.10")) ||
		(c = g_find_program_in_path ("totem-audio-preview"))
	) {
		g_free(c);
		return 0;
	}
	return -1; /* FAIL */
}


static char**
get_preview_argv (const char *path)
{
	char *command;
	char **argv;
	int i;

	/* OSX: afplay - Audio File Play */
	command = g_find_program_in_path ("afplay");
	if (command) {
		argv = g_new (char *, 3);
		argv[0] = command;
		argv[1] = g_strdup (path);
		argv[2] = NULL;

		return argv;
	}

	if((command = g_find_program_in_path ("mplayer"))){
		argv = g_new (char *, 3);
		argv[0] = command;
		argv[1] = g_strdup (path);
		argv[2] = NULL;

		return argv;
	}

	command = g_find_program_in_path ("gst-launch-0.10");
	if (command) {
		char *uri;
		uri = g_filename_to_uri (path, NULL, NULL);
		argv = g_new (char *, 10);
		i = 0;
		argv[i++] = command;
		argv[i++] = g_strdup ("playbin");
		argv[i++] = g_strconcat ("uri=", uri, NULL);
		/* do not display videos */
		argv[i++] = g_strdup ("current-video=-1");
		argv[i++] = NULL;
		g_free (uri);

		return argv;
	}

	command = g_find_program_in_path ("totem-audio-preview");
	if (command) {
		char *uri;
		uri = g_filename_to_uri (path, NULL, NULL);
		argv = g_new (char *, 3);
		argv[0] = command;
		argv[1] = g_strdup (uri);
		argv[2] = NULL;
		g_free (uri);

		return argv;
	}

	return NULL;
}


static int audio_preview_child_watch = 0;
static GPid audio_preview_child_pid = 0;

static void
stop_playback ()
{
	if (audio_preview_child_pid == 0)  return;

	kill (audio_preview_child_pid, SIGTERM);
	g_source_remove(audio_preview_child_watch);
	waitpid(audio_preview_child_pid, NULL, 0);
	audio_preview_child_pid = 0;
}


static void
audio_child_died (GPid pid, gint status, gpointer data)
{
	dbg(1, "pid:%d status:%d", pid, status);
	audio_preview_child_watch = 0;
	audio_preview_child_pid = 0;

	if(play->queue)
		play->next();
	else
		player_on_play_finished();
}


static bool
play_file (const char * path)
{
	GPid child_pid;
	char **argv;
	GError *error = NULL;

	if(!(argv = get_preview_argv (path))){
		gwarn("audio preview is unavailable: install either of afplay, mplayer, totem-audio-preview or gstreamer");
		return FALSE;
	}

	if (!g_spawn_async_with_pipes (NULL,
				       argv,
				       NULL, 
				       G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL | G_SPAWN_DO_NOT_REAP_CHILD ,
				       NULL,
				       NULL /* user_data */,
				       &child_pid,
				       NULL, NULL, NULL,
				       &error)) {
		g_strfreev (argv);
		g_warning("Error spawning sound preview: %s\n", error->message);
		g_error_free (error);
		return FALSE;
	}
	g_strfreev (argv);

	audio_preview_child_watch = g_child_watch_add (child_pid, audio_child_died, NULL);
	audio_preview_child_pid = child_pid;
	
	return FALSE;
}


static void
gplayer_play_path (const char* path)
{
	dbg(1, "%s", path);
	stop_playback();
	play_file(path);
}

/* public API */

#if 0
static void gplayer_toggle(Sample* sample) {
	dbg(1, "%s", sample->full_path);
	if (audio_preview_child_pid) gplayer_stop();
	else gplayer_play_path(sample->full_path);
}
#endif

static bool
gplayer_play (Sample* sample)
{
	dbg(1, "%s", sample->full_path);
	gplayer_play_path(sample->full_path);
	return true;
}


static void
gplayer_connect (ErrorCallback callback, gpointer user_data)
{
	callback(NULL, user_data);
}

static void gplayer_disconnect() {;}

static void
gplayer_stop ()
{
	dbg(1, "stop audition..");
	stop_playback();
}
