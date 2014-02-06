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
#include <sample.h>
#include "application.h"

extern void colour_box_init();

static gpointer application_parent_class = NULL;

enum  {
	APPLICATION_DUMMY_PROPERTY
};
static GObject* application_constructor (GType, guint n_construct_properties, GObjectConstructParam*);
static void     application_finalize    (GObject*);


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
	app->colourbox_dirty = true;

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
	g_signal_new ("icon_theme", TYPE_APPLICATION, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__STRING, G_TYPE_NONE, 1, G_TYPE_STRING);
	g_signal_new ("on_quit", TYPE_APPLICATION, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	g_signal_new ("theme_changed", TYPE_APPLICATION, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	g_signal_new ("layout_changed", TYPE_APPLICATION, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
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


