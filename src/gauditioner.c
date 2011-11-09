#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#define __USE_GNU
#include <string.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "typedefs.h"
#include "support.h"
#include "sample.h"
#include "main.h"
#include "auditioner.h"

static char ** get_preview_argv (const char *path) {
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


static int audio_preview_child_watch =0;
static GPid audio_preview_child_pid =0;
static GList* play_queue = NULL;

static void stop_playback () {
	if (audio_preview_child_pid == 0)  return;

	kill (audio_preview_child_pid, SIGTERM);
	g_source_remove(audio_preview_child_watch);
	waitpid(audio_preview_child_pid, NULL, 0);
	audio_preview_child_pid = 0;
}

static void play_next() {
	if(play_queue){
		Result* result = play_queue->data;
		play_queue = g_list_remove(play_queue, result);
		dbg(1, "%s", result->full_path);
		/* TODO highlight current in app.store - see listview.c */
		auditioner_play_result(result);
		result_free(result);
	}else{
		dbg(1, "play_all finished. disconnecting...");
		stop_playback();
	}
}


static void audio_child_died (GPid pid, gint status, gpointer data) {
	dbg(1, "pid:%d status:%d", pid, status);
	audio_preview_child_watch = 0;
	audio_preview_child_pid = 0;
	if(play_queue) play_next();
}

static gboolean play_file (const char * path) {
	GPid child_pid;
	char **argv;
	GError *error;

	argv = get_preview_argv (path);
	if (argv == NULL) {
		gwarn("audio preview is unavailable: install either of afplay, totem-audio-preview or gstreamer");
		return FALSE;
	}

	error = NULL;
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


/* public API */

void auditioner_play_path(const char* path) {
	dbg(1, "%s", path);
	stop_playback();
	play_file(path);
}

void auditioner_toggle(Sample* sample) {
	dbg(1, "%s", sample->filename);
	auditioner_play_path(sample->filename);
}

void auditioner_play(Sample* sample) {
	dbg(1, "%s", sample->filename);
	auditioner_play_path(sample->filename);
}

void auditioner_play_all() {
	dbg(1, "...");
	if(play_queue){
		pwarn("already playing");
		return;
	}

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

/* unused public API */
void auditioner_connect() {;}

void auditioner_play_result(Result* result) {
	dbg(1, "%s", result->full_path);
	auditioner_play_path(result->full_path);
}

void auditioner_stop(Sample* sample) {
	dbg(1, "stop audition..");
	if (play_queue) {
		g_list_free(play_queue);
		play_queue=NULL;
	}
	stop_playback();
}
