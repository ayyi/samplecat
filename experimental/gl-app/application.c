/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2007-2018 Tim Orford <tim@orford.org>                  |
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
#include "samplecat.h"
#include "list_store.h"
#ifndef USE_GDL
#include "window.h"
#endif
#include "application.h"


static gpointer application_parent_class = NULL;

enum  {
	APPLICATION_DUMMY_PROPERTY
};
static GObject* application_constructor    (GType, guint n_construct_properties, GObjectConstructParam*);
static void     application_finalize       (GObject*);
static void     application_search         ();


Application*
application_construct (GType object_type)
{
	Application* app = g_object_new (object_type, NULL);
	app->config_ctx.filename = g_strdup_printf("%s/.config/" PACKAGE "/" PACKAGE, g_get_home_dir());

	//app->cache_dir = g_build_filename (g_get_home_dir(), ".config", PACKAGE, "cache", NULL);
	//app->configctx.dir = g_build_filename (g_get_home_dir(), ".config", PACKAGE, NULL);

	app->style = (Style){
		.bg = 0x000000ff,
		.bg_alt = 0x181818ff,
		.bg_selected = 0x777777ff,
		.fg = 0x66aaffff,
		.text = 0xbbbbbbff,
		.selection = 0x6677ff77,
	};

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
	g_signal_new ("play_start", TYPE_APPLICATION, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	g_signal_new ("play_stop", TYPE_APPLICATION, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	g_signal_new ("play_position", TYPE_APPLICATION, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	*/
	g_signal_new ("actor-added", TYPE_APPLICATION, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1, G_TYPE_POINTER);
}


static void
application_instance_init (Application* self)
{
	//self->state = NONE;
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
	PF;
	g_signal_emit_by_name (app, "on-quit");
}


/**
 * fill the display with the results matching the current set of filters.
 */
static void
application_search()
{
	PF;
	if(BACKEND_IS_NULL) return;

	samplecat_list_store_do_search((SamplecatListStore*)samplecat.store);
}


void
application_play(Sample* sample)
{
	if(sample) dbg(1, "%s", sample->name);
}


void
application_play_selected()
{
	Sample* sample = samplecat.model->selection;
	if (!sample) {
		return;
	}
	dbg(1, "PLAY %s", sample->full_path);

	application_play(sample);
}

