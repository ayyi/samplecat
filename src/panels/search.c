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
#include <gtk/gtk.h>
#include "debug/debug.h"
#include "gdl/gdl-dock-item.h"
#include "support.h"
#include "application.h"
#include "widgets/suggestion_entry.h"

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
	GtkStringList* recent;
} window;

static void tagshow_selector_new ();


static void
search_class_init (SearchClass * klass, gpointer klass_data)
{
	search_parent_class = g_type_class_peek_parent (klass);

	window.recent = gtk_string_list_new(NULL);

	G_OBJECT_CLASS (klass)->constructor = search_constructor;
}

static GObject*
search_constructor (GType type, guint n_construct_properties, GObjectConstructParam* construct_properties)
{
	GObjectClass* parent_class = G_OBJECT_CLASS (search_parent_class);
	GObject* obj = parent_class->constructor (type, n_construct_properties, construct_properties);
	g_object_set(obj, "expand", false, NULL);

	gtk_widget_set_size_request((GtkWidget*)obj, -1, 60);

	return obj;
}

static void
search_instance_init (Search* self, gpointer klass)
{
	Observable* filter = samplecat.model->filters2.search;

	GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
	gdl_dock_item_set_child(GDL_DOCK_ITEM(self), vbox);

	GtkWidget* row1 = window.toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_append(GTK_BOX(vbox), row1);

	{
		GtkWidget* entry = suggestion_entry_new();
		gtk_widget_set_hexpand (entry, TRUE);
		g_object_set (entry, "placeholder-text", "Search", NULL);
		suggestion_entry_set_model(SUGGESTION_ENTRY (entry), G_LIST_MODEL (window.recent));
		g_object_unref(window.recent);

		gtk_box_append(GTK_BOX(row1), entry);

		void on_search_filter_changed (Observable* _filter, AGlVal value, gpointer entry)
		{
			if (strcmp(value.c, gtk_editable_get_text(GTK_EDITABLE(entry))))
				gtk_editable_set_text (GTK_EDITABLE(entry), value.c);
		}
		agl_observable_subscribe_with_state (filter, on_search_filter_changed, entry);

		void on_text_changed (GtkEditable* editable, gpointer user_data)
		{
			const char* text = gtk_editable_get_text(GTK_EDITABLE(editable));
			observable_string_set(samplecat.model->filters2.search, g_strdup(text));

		}
		g_signal_connect(entry, "search-changed", (void*)on_text_changed, NULL);

		void on_activate (SuggestionEntry* self, gpointer user_data)
		{
			if (g_list_model_get_n_items((GListModel*)window.recent) >= 20) {
				gtk_string_list_remove(window.recent, 0);
			}
			gtk_string_list_append(window.recent, gtk_editable_get_text(gtk_editable_get_delegate((GtkEditable*)self)));
		}
		g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(on_activate), NULL);
	}

	tagshow_selector_new();
}


static void
tagshow_selector_new ()
{
	#define ALL_CATEGORIES "All categories"

	GtkStringList* list = gtk_string_list_new((const char* const*)samplecat.model->categories);
	gtk_string_list_splice(list, 0, 0, (const char* const[]){ALL_CATEGORIES, NULL});
	GtkWidget* dropdown = gtk_drop_down_new(G_LIST_MODEL(list), NULL);
	gtk_box_append(GTK_BOX(window.toolbar), dropdown);

	void on_view_category_changed (GtkDropDown* dropdown, GParamSpec* pspec, GtkStringList* list)
	{
		// update the sample-list with the new view-category
		PF;

  		int row = gtk_drop_down_get_selected(dropdown);
		const char* category = gtk_string_list_get_string(list, row);
		observable_string_set(samplecat.model->filters2.category, strdup(category));
	}
	g_signal_connect(dropdown, "notify::selected", G_CALLBACK(on_view_category_changed), list);

	void on_category_filter_changed (Observable* filter, AGlVal value, gpointer dropdown)
	{
		if (!filter->value.c || !strlen(filter->value.c)) {
			gtk_drop_down_set_selected((GtkDropDown*)dropdown, 0);
		}
	}

	agl_observable_subscribe(samplecat.model->filters2.category, on_category_filter_changed, dropdown);
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
