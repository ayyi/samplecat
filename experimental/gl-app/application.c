/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2007-2019 Tim Orford <tim@orford.org>                  |
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
#include <glib-object.h>
#include "debug/debug.h"
#include "support.h"
#include "model.h"
#include "db/db.h"
#include "samplecat.h"
#include "list_store.h"
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

	//app->cache_dir = g_build_filename (g_get_home_dir(), ".config", PACKAGE, "cache", NULL);
	//app->configctx.dir = g_build_filename (g_get_home_dir(), ".config", PACKAGE, NULL);

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

		void get_theme_name(ConfigOption* option)
		{
			g_value_set_string(&option->val, theme_name);
		}
		ctx->options[CONFIG_ICON_THEME] = config_option_new_string("icon_theme", get_theme_name);
	}

	void on_filter_changed(GObject* _filter, gpointer user_data)
	{
		application_search();
	}

	GList* l = samplecat.model->filters_;
	for(;l;l=l->next){
		g_signal_connect((SamplecatFilter*)l->data, "changed", G_CALLBACK(on_filter_changed), NULL);
	}

	/*
	void listmodel__sample_changed(SamplecatModel* m, Sample* sample, int prop, void* val, gpointer _app)
	{
		samplecat_list_store_on_sample_changed((SamplecatListStore*)samplecat.store, sample, prop, val);
	}
	g_signal_connect((gpointer)samplecat.model, "sample-changed", G_CALLBACK(listmodel__sample_changed), app);

	void log_message(GObject* o, char* message, gpointer _)
	{
		dbg(1, "---> %s", message);
		statusbar_print(1, PACKAGE_NAME". Version "PACKAGE_VERSION);
	}
	g_signal_connect(samplecat.logger, "message", G_CALLBACK(log_message), NULL);
	*/

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
	/*
	g_signal_new ("config_loaded", TYPE_APPLICATION, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	g_signal_new ("icon_theme", TYPE_APPLICATION, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__STRING, G_TYPE_NONE, 1, G_TYPE_STRING);
	g_signal_new ("on_quit", TYPE_APPLICATION, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	g_signal_new ("theme_changed", TYPE_APPLICATION, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	g_signal_new ("layout_changed", TYPE_APPLICATION, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	g_signal_new ("audio_ready", TYPE_APPLICATION, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	*/
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
	if (g_once_init_enter (&application_type_id__volatile)) {
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
	void panel_select (gpointer user_data)
	{
		AGlActorClass* k = (AGlActorClass*)user_data;
		AGlActor* existing = agl_actor__find_by_class((AGlActor*)app->scene, k);
		if(existing){
			application_remove_panel(k);
		}else{
			application_add_panel(k);
		}
	}

	bool show_icon (gpointer user_data)
	{
		return !!agl_actor__find_by_class((AGlActor*)app->scene, (AGlActorClass*)user_data);
	}

	typedef struct
	{
		int i;
	} A;

	void foreach (gpointer key, gpointer value, gpointer user_data)
	{
		AGlActorClass* k = (AGlActorClass*)value;
		A* a = user_data;

		for(int i = 0; i < AGL_ACTOR_N_BEHAVIOURS; i++){
			AGlBehaviourClass* ki = k->behaviour_classes[i];
			if(ki == panel_get_class()){
				if(a->i < menu.len){
					menu.items[a->i] = (MenuItem){
						(char*)key,
						"checkmark",
						{0,},
						panel_select,
						show_icon,
						k
					};
					a->i++;
				}
			}
		}
	}
	g_hash_table_foreach(agl_actor_registry, foreach, &(A){.i = 3});
}


void
application_add_panel (AGlActorClass* klass)
{
	float height = 80;

	AGlActor* parent = agl_actor__find_by_name((AGlActor*)app->scene, "Dock H");
	if(parent){
		if((parent = agl_actor__find_by_name(parent, "Left"))){
			AGlActor* panel = panel_view_get_class()->new(NULL);
			panel->region.y2 = height;
			((PanelView*)panel)->size_req.min = (AGliPt){-1, 24};
			AGlActor* actor = klass->new(NULL);
			agl_actor__add_child(panel, actor);

			for(GList* l = parent->children; l; l = l->next){
				((AGlActor*)l->data)->region.y2 += height;
			}
			agl_actor__insert_child(parent, panel, 0);
			agl_actor__set_size(parent);
		}
	}
	else gwarn("parent not found");
}


void
application_remove_panel (AGlActorClass* klass)
{
	AGlActor* actor = agl_actor__find_by_class((AGlActor*)app->scene, klass);
	if(actor){
		actor = actor->parent;
		agl_actor__remove_child(actor->parent, actor);
		agl_actor__set_size(actor->parent);
	}
}


void
application_set_auditioner ()
{
	play->auditioner = &a_ayyidbus;

	bool set_auditioner_on_idle(gpointer data)
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
	if(play->status == PLAY_PAUSED){
		if(play->auditioner->playpause){
			play->auditioner->playpause(false);
		}
		play->status = PLAY_PLAYING;
		return;
	}

	if(sample) dbg(1, "%s", sample->name);

	if(player_play(sample)){
#if 0
		if(app->play.queue)
			statusbar_print(1, "playing 1 of %i ...", g_list_length(app->play.queue));
		else
			statusbar_print(1, "");
	}else{
		statusbar_print(1, "File not playable");
#endif
	}
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

