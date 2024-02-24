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
#include <gio/gio.h>
#include <glib-object.h>
#include <glib.h>
#include "debug/debug.h"
#include "file_manager/pixmaps.h"
#include "types.h"
#include "samplecat/samplecat.h"
#include "application-nogui.h"

enum  {
	NO_GUI_APPLICATION_0_PROPERTY,
	NO_GUI_APPLICATION_NUM_PROPERTIES
};

enum  {
	NO_GUI_APPLICATION_NUM_SIGNALS
};

static gpointer no_gui_application_parent_class = NULL;

static void     no_gui_application_startup       (GApplication*);
static void     no_gui_application_activate      (GApplication*);
static void     no_gui_application_finalize      (GObject*);
static GType    no_gui_application_get_type_once (void);

#if 0
static void     console__show_result_header ();
#endif
static void     console__show_result        (Sample*);
static void     console__show_result_footer (int row_count);


NoGuiApplication*
no_gui_application_construct (GType object_type)
{
	return (NoGuiApplication*) g_object_new (object_type, "application-id", "org.ayyi.samplecat", "flags", G_APPLICATION_NON_UNIQUE, NULL);
}


NoGuiApplication*
no_gui_application_new (void)
{
	return no_gui_application_construct (TYPE_NO_GUI_APPLICATION);
}


static gboolean
no_gui_application_dbus_register (GApplication* application, GDBusConnection* connection, const gchar* object_path, GError** error)
{
	return true;
}

static void
no_gui_application_startup (GApplication* base)
{
	G_APPLICATION_CLASS (no_gui_application_parent_class)->startup(base);
}


static void
no_gui_application_activate (GApplication* base)
{
	G_APPLICATION_CLASS (no_gui_application_parent_class)->activate(base);

	samplecat_list_store_do_search((SamplecatListStore*)samplecat.store);
}


static void
no_gui_application_class_init (NoGuiApplicationClass* klass, gpointer klass_data)
{
	no_gui_application_parent_class = g_type_class_peek_parent (klass);

	((GApplicationClass*) klass)->startup = no_gui_application_startup;
	((GApplicationClass*) klass)->activate = no_gui_application_activate;
	((GApplicationClass*) klass)->dbus_register = no_gui_application_dbus_register;
	G_OBJECT_CLASS (klass)->finalize = no_gui_application_finalize;
}

static void
no_gui_application_instance_init (NoGuiApplication* self, gpointer klass)
{
	((SamplecatApplication*)self)->temp_view = true;

	play = player_new();
	pixmaps_init();

	void store_content_changed (GtkListStore* store, gpointer self)
	{
		PF;
		GtkTreeIter iter;
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
		if (!gtk_tree_model_get_iter_first((GtkTreeModel*)store, &iter)) { gerr ("cannot get iter."); return; }
#pragma GCC diagnostic warning "-Wdeprecated-declarations"
		int row_count = 0;
		do {
			if (++row_count < 100) {
				Sample* sample = samplecat_list_store_get_sample_by_iter(&iter);
				if (sample) {
					console__show_result(sample);
					sample_unref(sample);
				}
			}
		} while (gtk_tree_model_iter_next((GtkTreeModel*)store, &iter));

		console__show_result_footer(row_count);
		g_application_release(self);
	}

	if (!((SamplecatApplication*)self)->args.add) {
		g_application_hold((GApplication*)self);
		g_signal_connect(G_OBJECT(samplecat.store), "content-changed", G_CALLBACK(store_content_changed), self);
	}

	void log_message (GObject* o, char* message, gpointer _)
	{
		puts(message);
	}
	g_signal_connect(samplecat.logger, "message", G_CALLBACK(log_message), NULL);
}

static void
no_gui_application_finalize (GObject* obj)
{
	G_OBJECT_CLASS (no_gui_application_parent_class)->finalize (obj);
}

static GType
no_gui_application_get_type_once (void)
{
	static const GTypeInfo g_define_type_info = { sizeof (NoGuiApplicationClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) no_gui_application_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (NoGuiApplication), 0, (GInstanceInitFunc) no_gui_application_instance_init, NULL };
	return g_type_register_static (samplecat_application_get_type (), "NoGuiApplication", &g_define_type_info, 0);
}

GType
no_gui_application_get_type (void)
{
	static volatile gsize no_gui_application_type_id__once = 0;
	if (g_once_init_enter ((gsize*)&no_gui_application_type_id__once)) {
		GType no_gui_application_type_id = no_gui_application_get_type_once ();
		g_once_init_leave (&no_gui_application_type_id__once, no_gui_application_type_id);
	}
	return no_gui_application_type_id__once;
}

#if 0
static void
console__show_result_header ()
{
	printf("filters: text='%s' dir=%s\n", samplecat.model->filters2.search->value.c, strlen(samplecat.model->filters2.dir->value.c) ? samplecat.model->filters2.dir->value.c : "<all directories>");

	printf("  name                 directory                            length ch rate mimetype\n");
}
#endif


static void
console__show_result (Sample* result)
{
	int w = get_terminal_width();

	#define DIR_MAX (35)
	char dir[DIR_MAX];
	g_strlcpy(dir, dir_format(result->sample_dir), DIR_MAX);

	#define SNAME_MAX (20)
	int max = SNAME_MAX + (w > 100 ? 10 : 0);
	char name[max];
	g_strlcpy(name, result->name, max);

	char format[256] = {0,};
	snprintf(format, 255, "  %%-%is %%-35s %%7"PRIi64" %%d %%5d %%s\n", max);

	printf(format, name, dir, result->length, result->channels, result->sample_rate, result->mimetype);
}


static void
console__show_result_footer (int row_count)
{
	printf("total %i samples found.\n", row_count);
}

