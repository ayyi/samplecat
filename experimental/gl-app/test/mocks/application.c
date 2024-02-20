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
#include <glib-object.h>
#include "debug/debug.h"
#include "support.h"
#include "model.h"
#include "db/db.h"
#include "samplecat.h"
#include "behaviours/panel.h"
#include "views/panel.h"
#include "views/context_menu.h"
#include "application.h"

extern GHashTable* agl_actor_registry;

static gpointer application_parent_class = NULL;

enum  {
	APPLICATION_DUMMY_PROPERTY
};
static GObject* application_constructor (GType, guint n_construct_properties, GObjectConstructParam*);
static void     application_finalize    (GObject*);
static void     application_search      ();

#ifdef HAVE_AYYIDBUS
extern Auditioner a_ayyidbus;
#endif


Application*
application_construct (GType object_type)
{
	Application* app = g_object_new (object_type, NULL);
	app->config_ctx.filename = g_strdup_printf("%s/.config/" PACKAGE "/" PACKAGE, g_get_home_dir());

	return app;
}


Application*
application_new ()
{
	app = application_construct (TYPE_APPLICATION);

	samplecat_init();

	{
		ConfigContext* ctx = &app->config_ctx;
		ctx->options = g_malloc0(CONFIG_MAX * sizeof(ConfigOption*));

		void get_theme_name (ConfigOption* option)
		{
			g_value_set_string(&option->val, theme_name);
		}
		ctx->options[CONFIG_ICON_THEME] = config_option_new_string("icon_theme", get_theme_name);
	}

	void on_filter_changed (Observable* filter, AGlVal value, gpointer user_data)
	{
		application_search();
	}
	for (int i = 0; i < N_FILTERS; i++) {
		agl_observable_subscribe_with_state (samplecat.model->filters3[i], on_filter_changed, NULL);
	}

	return app;
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

	g_signal_new ("actor-added", TYPE_APPLICATION, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1, G_TYPE_POINTER);
}


static void
application_instance_init (Application* self)
{
	play = player_new();
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
	if (g_once_init_enter ((gsize*)&application_type_id__volatile)) {
		static const GTypeInfo g_define_type_info = { sizeof (ApplicationClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) application_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (Application), 0, (GInstanceInitFunc) application_instance_init, NULL };
		GType application_type_id;
		application_type_id = g_type_register_static (G_TYPE_OBJECT, "Application", &g_define_type_info, 0);
		g_once_init_leave (&application_type_id__volatile, application_type_id);
	}
	return application_type_id__volatile;
}


void
application_quit (Application* app)
{
	PF;
	g_signal_emit_by_name (app, "on-quit");
}


/**
 * fill the display with the results matching the current set of filters.
 */
static void
application_search ()
{
	PF;
	if(BACKEND_IS_NULL) return;

	samplecat_list_store_do_search((SamplecatListStore*)samplecat.store);
}


static void
action_play (gpointer _)
{
	application_play_selected();
}


static void
action_delete (gpointer _)
{
}


Menu menu = {
	12,
	{
		{"Play", "media-playback-start", {'p'}, action_play},
		{"Delete", "delete", {0}, action_delete},
		{},
		{},
		{},
		{},
		{},
		{},
		{},
		{},
		{},
		{},
	}
};


void
application_menu_init ()
{
}


void
application_set_auditioner ()
{
#ifdef HAVE_AYYIDBUS
	play->auditioner = &a_ayyidbus;
#endif

	gboolean set_auditioner_on_idle (gpointer data)
	{
		//_set_auditioner();

		player_connect(NULL, data);

		return G_SOURCE_REMOVE;
	}
	g_idle_add_full(G_PRIORITY_LOW, set_auditioner_on_idle, NULL, NULL);

	void application_on_player_ready(gpointer user_data, gpointer _)
	{
		dbg(1, "player ready");
	}
	am_promise_add_callback(play->ready, application_on_player_ready, NULL);
}


void
application_play (Sample* sample)
{
	if (play->status == PLAY_PAUSED) {
		if (play->auditioner->playpause) {
			play->auditioner->playpause(false);
		}
		play->status = PLAY_PLAYING;
		return;
	}

	if (sample) dbg(1, "%s", sample->name);
}


void
application_play_selected ()
{
	Sample* sample = samplecat.model->selection;
	if (!sample) {
		return;
	}
	dbg(1, "PLAY %s", sample->full_path);

	application_play(sample);
}
