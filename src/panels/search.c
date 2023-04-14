/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
 | copyright (C) 2007-2023 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#include "config.h"
#include <gtk/gtk.h>
#include "debug/debug.h"
#include "gdl/gdl-dock-item.h"
#include "support.h"
#include "application.h"
													#include "gtk/utils.h"

#define TYPE_SEARCH            (search_get_type ())
#define SEARCH(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_SEARCH, Search))
#define SEARCH_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_SEARCH, SearchClass))
#define IS_SEARCH(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_SEARCH))
#define IS_SEARCH_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_SEARCH))
#define SEARCH_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_SEARCH, SearchClass))

typedef struct _Search Search;
typedef struct _SearchClass SearchClass;

struct _Search {
	GdlDockItem parent_instance;
};

struct _SearchClass {
	GdlDockItemClass parent_class;
};

static gpointer search_parent_class = NULL;

GType           search_get_type    (void);
static GObject* search_constructor (GType type, guint n_construct_properties, GObjectConstructParam* construct_properties);

static struct {
	GtkWidget* search;
	GtkWidget* toolbar;
} window;

static void tagshow_selector_new ();


static void
search_class_init (SearchClass * klass, gpointer klass_data)
{
	search_parent_class = g_type_class_peek_parent (klass);

	G_OBJECT_CLASS (klass)->constructor = search_constructor;
}

static GObject*
search_constructor (GType type, guint n_construct_properties, GObjectConstructParam* construct_properties)
{
	GObjectClass* parent_class = G_OBJECT_CLASS (search_parent_class);
	GObject* obj = parent_class->constructor (type, n_construct_properties, construct_properties);
	g_object_set(obj, "expand", false, NULL);
	return obj;
}

static void
search_instance_init (Search* self, gpointer klass)
{
	GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
	gdl_dock_item_set_child(GDL_DOCK_ITEM(self), vbox);

	GtkWidget* row1 = window.toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_append(GTK_BOX(vbox), row1);

	void on_focus_out (GtkEventControllerFocus* self, gpointer user_data)
	{
		PF;
		const gchar* text = gtk_entry_buffer_get_text(gtk_entry_get_buffer(GTK_ENTRY(window.search)));
		observable_string_set(samplecat.model->filters2.search, g_strdup(text));
	}

	Observable* filter = samplecat.model->filters2.search;
	GtkWidget* entry = window.search = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(entry), 64);
	gtk_entry_set_icon_from_icon_name (GTK_ENTRY(entry), GTK_ENTRY_ICON_PRIMARY, "system-search-symbolic");
	GtkEntryBuffer* text = gtk_entry_get_buffer(GTK_ENTRY(entry));
	if (filter->value.c) gtk_entry_buffer_set_text(text, filter->value.c, -1);
	gtk_widget_set_name(entry, "search-entry");
	gtk_widget_set_hexpand(entry, true);
	gtk_box_append(GTK_BOX(row1), entry);

	GtkEventController* focus = gtk_event_controller_focus_new();
	g_signal_connect(focus, "leave", G_CALLBACK(on_focus_out), NULL);

	gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
	g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(on_focus_out), NULL);

	void on_search_filter_changed (Observable* _filter, AGlVal value, gpointer _entry)
	{
		gtk_entry_buffer_set_text(gtk_entry_get_buffer(GTK_ENTRY(_entry)), value.c, -1);
	}
	agl_observable_subscribe (filter, on_search_filter_changed, entry);

	tagshow_selector_new();
}


static void
tagshow_selector_new ()
{
	#define ALL_CATEGORIES "All categories"

	GtkWidget* combo = gtk_combo_box_text_new();
	GtkComboBox* combo_ = GTK_COMBO_BOX(combo);
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), ALL_CATEGORIES);
	for (int i=0;i<G_N_ELEMENTS(samplecat.model->categories);i++) {
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), samplecat.model->categories[i]);
	}
	gtk_combo_box_set_active(combo_, 0);
	gtk_box_append(GTK_BOX(window.toolbar), combo);

	void on_view_category_changed (GtkComboBox* widget, gpointer user_data)
	{
		// update the sample list with the new view-category
		PF;

		char* category = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget));
		if (!strcmp(category, ALL_CATEGORIES)) g_free0(category);
		observable_string_set(samplecat.model->filters2.category, category);
	}
	g_signal_connect(combo, "changed", G_CALLBACK(on_view_category_changed), NULL);

	void on_category_filter_changed (Observable* filter, AGlVal value, gpointer combo)
	{
		if (!filter->value.c || !strlen(filter->value.c)) {
			gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);
		}
	}

	agl_observable_subscribe (samplecat.model->filters2.category, on_category_filter_changed, combo);
}


static GType
search_get_type_once (void)
{
	static const GTypeInfo g_define_type_info = { sizeof (SearchClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) search_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (Search), 0, (GInstanceInitFunc) search_instance_init, NULL };
	return g_type_register_static (gdl_dock_item_get_type (), "Search", &g_define_type_info, 0);
}

GType
search_get_type (void)
{
	static volatile gsize search_type_id__once = 0;
	if (g_once_init_enter ((gsize*)&search_type_id__once)) {
		GType search_type_id;
		search_type_id = search_get_type_once ();
		g_once_init_leave (&search_type_id__once, search_type_id);
	}
	return search_type_id__once;
}

