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
* This file is partially based on vala/application.vala but is manually synced.
*
*/
#include "config.h"
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <glib-object.h>
#include "debug/debug.h"
#include <sample.h>
#include "support.h"
#include "model.h"
#include "db/db.h"
#include "list_store.h"
#include "application.h"

#ifdef HAVE_AYYIDBUS
  #include "auditioner.h"
#endif
#ifdef HAVE_JACK
  #include "jack_player.h"
#endif
#ifdef HAVE_GPLAYER
  #include "gplayer.h"
#endif

extern void colour_box_init();

static gpointer application_parent_class = NULL;

enum  {
	APPLICATION_DUMMY_PROPERTY
};
static GObject* application_constructor    (GType, guint n_construct_properties, GObjectConstructParam*);
static void     application_finalize       (GObject*);
static void     application_set_auditioner ();


Application*
application_construct (GType object_type)
{
	Application* app = g_object_new (object_type, NULL);
	app->state = 1;
	app->cache_dir = g_build_filename (g_get_home_dir(), ".config", PACKAGE, "cache", NULL);
	app->config_dir = g_build_filename (g_get_home_dir(), ".config", PACKAGE, NULL);
	return app;
}


Application*
application_new ()
{
	Application* app = application_construct (TYPE_APPLICATION);

	colour_box_init();

	memset(app->config.colour, 0, PALETTE_SIZE * 8);

	app->config_filename = g_strdup_printf("%s/.config/" PACKAGE "/" PACKAGE, g_get_home_dir());

#if (defined HAVE_JACK)
	app->enable_effect = true;
	app->link_speed_pitch = true;
	app->effect_param[0] = 0.0; /* cent transpose [-100 .. 100] */
	app->effect_param[1] = 0.0; /* semitone transpose [-12 .. 12] */
	app->effect_param[2] = 0.0; /* octave [-3 .. 3] */
	app->playback_speed = 1.0;
#endif

	app->model = samplecat_model_new();

	samplecat_model_add_filter (app->model, app->model->filters.search   = samplecat_filter_new("search"));
	samplecat_model_add_filter (app->model, app->model->filters.dir      = samplecat_filter_new("directory"));
	samplecat_model_add_filter (app->model, app->model->filters.category = samplecat_filter_new("category"));

	void on_filter_changed(GObject* _filter, gpointer user_data)
	{
		//SamplecatFilter* filter = (SamplecatFilter*)_filter;
		extern void do_search(); // TODO
		do_search();
	}

	GList* l = app->model->filters_;
	for(;l;l=l->next){
		g_signal_connect((SamplecatFilter*)l->data, "changed", G_CALLBACK(on_filter_changed), NULL);
	}

	application_set_auditioner();

	return app;
}


void
application_emit_icon_theme_changed (Application* self, const gchar* s)
{
	g_return_if_fail (self != NULL);
	g_return_if_fail (s != NULL);
	g_signal_emit_by_name (self, "icon-theme", s);
}


static GObject*
application_constructor (GType type, guint n_construct_properties, GObjectConstructParam* construct_properties)
{
	GObjectClass* parent_class = G_OBJECT_CLASS (application_parent_class);
	GObject* obj = parent_class->constructor (type, n_construct_properties, construct_properties);
	return obj;
}


static void
application_class_init (ApplicationClass* klass)
{
	application_parent_class = g_type_class_peek_parent (klass);
	G_OBJECT_CLASS (klass)->constructor = application_constructor;
	G_OBJECT_CLASS (klass)->finalize = application_finalize;
	g_signal_new ("config_loaded", TYPE_APPLICATION, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	g_signal_new ("icon_theme", TYPE_APPLICATION, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__STRING, G_TYPE_NONE, 1, G_TYPE_STRING);
	g_signal_new ("on_quit", TYPE_APPLICATION, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	g_signal_new ("theme_changed", TYPE_APPLICATION, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	g_signal_new ("layout_changed", TYPE_APPLICATION, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	g_signal_new ("audio_ready", TYPE_APPLICATION, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}


static void
application_instance_init (Application* self)
{
	self->state = 0;
}


static void
application_finalize (GObject* obj)
{
	G_OBJECT_CLASS (application_parent_class)->finalize (obj);
}


GType
application_get_type ()
{
	static volatile gsize application_type_id__volatile = 0;
	if (g_once_init_enter (&application_type_id__volatile)) {
		static const GTypeInfo g_define_type_info = { sizeof (ApplicationClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) application_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (Application), 0, (GInstanceInitFunc) application_instance_init, NULL };
		GType application_type_id;
		application_type_id = g_type_register_static (G_TYPE_OBJECT, "Application", &g_define_type_info, 0);
		g_once_init_leave (&application_type_id__volatile, application_type_id);
	}
	return application_type_id__volatile;
}


void
application_quit(Application* app)
{
	g_signal_emit_by_name (app, "on-quit");
}


int  auditioner_nullC() {return 0;}
void auditioner_null() {;}
void auditioner_nullP(const char* p) {;}
void auditioner_nullS(Sample* s) {;}

static void
_set_auditioner() /* tentative - WIP */
{
	printf("auditioner backend: "); fflush(stdout);
	const static Auditioner a_null = {
		&auditioner_nullC,
		&auditioner_null,
		&auditioner_null,
		&auditioner_nullP,
		&auditioner_nullS,
		&auditioner_nullS,
		&auditioner_null,
		&auditioner_null,
		NULL, NULL, NULL, NULL
	};
#ifdef HAVE_JACK
  const static Auditioner a_jack = {
		&jplay__check,
		&jplay__connect,
		&jplay__disconnect,
		&jplay__play_path,
		&jplay__play,
		&jplay__toggle,
		&jplay__play_all,
		&jplay__stop,
		&jplay__play_selected,
		&jplay__pause,
		&jplay__seek,
		&jplay__getposition
	};
#endif
#ifdef HAVE_AYYIDBUS
	const static Auditioner a_ayyidbus = {
		&auditioner_check,
		&auditioner_connect,
		&auditioner_disconnect,
		&auditioner_play_path,
		&auditioner_play,
		&auditioner_toggle,
		&auditioner_play_all,
		&auditioner_stop,
		NULL,
		NULL,
		NULL,
		NULL
	};
#endif
#ifdef HAVE_GPLAYER
	const static Auditioner a_gplayer = {
		&gplayer_check,
		&gplayer_connect,
		&gplayer_disconnect,
		&gplayer_play_path,
		&gplayer_play,
		&gplayer_toggle,
		&gplayer_play_all,
		&gplayer_stop,
		NULL,
		NULL,
		NULL,
		NULL
	};
#endif

	gboolean connected = false;
#ifdef HAVE_JACK
	if(!connected && can_use(app->players, "jack")){
		app->auditioner = & a_jack;
		if (!app->auditioner->check()) {
			connected = true;
			printf("JACK playback.\n");
		}
	}
#endif
#ifdef HAVE_AYYIDBUS
	if(!connected && can_use(app->players, "ayyi")){
		app->auditioner = & a_ayyidbus;
		if (!app->auditioner->check()) {
			connected = true;
			printf("ayyi_audition.\n");
		}
	}
#endif
#ifdef HAVE_GPLAYER
	if(!connected && can_use(app->players, "cli")){
		app->auditioner = & a_gplayer;
		if (!app->auditioner->check()) {
			connected = true;
			printf("using CLI player.\n");
		}
	}
#endif
	if (!connected) {
		printf("no playback support.\n");
		app->auditioner = & a_null;
	}
}


static void
application_set_auditioner()
{
	// The gui is allowed to load before connecting the audio.
	// Connecting the audio can sometimes be very slow.

	// TODO starting jack blocks the gui so this needs to be moved to another thread.

	void set_auditioner_on_connected(gpointer _)
	{
		g_signal_emit_by_name (app, "audio-ready");
	}

	bool set_auditioner_on_idle(gpointer data)
	{
		_set_auditioner();

		app->auditioner->connect(set_auditioner_on_connected, data);

		return IDLE_STOP;
	}
	g_idle_add_full(G_PRIORITY_LOW, set_auditioner_on_idle, NULL, NULL);
}


gboolean
application_add_file(const char* path)
{
	/*
	 *  uri must be "unescaped" before calling this fn. Method string must be removed.
	 */

#if 1
	/* check if file already exists in the store
	 * -> don't add it again
	 */
	if(backend.file_exists(path, NULL)) {
		statusbar_print(1, "duplicate: not re-adding a file already in db.");
		gwarn("duplicate file: %s", path);
		//TODO ask what to do
		Sample* s = sample_get_by_filename(path);
		if (s) {
			//sample_refresh(s, false);
			samplecat_model_refresh_sample (app->model, s, false);
			sample_unref(s);
		} else {
			dbg(1, "sample found in db but not in model.");
		}
		return false;
	}
#endif

	dbg(1, "%s", path);
	if(BACKEND_IS_NULL) return false;

	Sample* sample = sample_new_from_filename((char*)path, false);
	if (!sample) {
		if (_debug_) gwarn("cannot add file: file-type is not supported");
		statusbar_print(1, "cannot add file: file-type is not supported");
		return false;
	}

	if(!sample_get_file_info(sample)){
		gwarn("cannot add file: reading file info failed");
		statusbar_print(1, "cannot add file: reading file info failed");
		sample_unref(sample);
		return false;
	}
#if 1
	/* check if /same/ file already exists w/ different path */
	GList* existing;
	if((existing = backend.filter_by_audio(sample))) {
		GList* l = existing; int i;
#ifdef INTERACTIVE_IMPORT
		GString* note = g_string_new("Similar or identical file(s) already present in database:\n");
#endif
		for(i=1;l;l=l->next, i++){
			/* TODO :prompt user: ask to delete one of the files
			 * - import/update the remaining file(s)
			 */
			dbg(0, "found similar or identical file: %s", l->data);
#ifdef INTERACTIVE_IMPORT
			if (i < 10)
				g_string_append_printf(note, "%d: '%s'\n", i, (char*) l->data);
#endif
		}
#ifdef INTERACTIVE_IMPORT
		if (i > 9)
			g_string_append_printf(note, "..\n and %d more.", i - 9);
		g_string_append_printf(note, "Add this file: '%s' ?", sample->full_path);
		if (do_progress_question(note->str) != 1) {
			// 0, aborted: -> whole add_file loop is aborted on next do_progress() call.
			// 1, OK
			// 2, cancled: -> only this file is skipped
			sample_unref(sample);
			g_string_free(note, true);
			return false;
		}
		g_string_free(note, true);
		g_list_foreach(existing, (GFunc)g_free, NULL);
		g_list_free(existing);
#endif /* END interactive import */
	}
#endif /* END check for similar files on import */

	sample->online = 1;
	sample->id = backend.insert(sample);
	if (sample->id < 0) {
		sample_unref(sample);
		return false;
	}

	samplecat_list_store_add((SamplecatListStore*)app->store, sample);

	samplecat_model_add(app->model);

	sample_unref(sample);
	return true;
}


