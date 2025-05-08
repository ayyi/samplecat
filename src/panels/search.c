/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2007-2020 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/

#include "config.h"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <gtk/gtk.h>
#pragma GCC diagnostic warning "-Wdeprecated-declarations"
#include "debug/debug.h"
#include "support.h"
#include "application.h"

static struct {
	GtkWidget* search;
	GtkWidget* toolbar;
} window;

static void tagshow_selector_new ();

/*
 *  Search box and tagging
 */
GtkWidget*
search_new ()
{
	PF;

	g_return_val_if_fail(app->window, FALSE);

	GtkWidget* filter_vbox = gtk_vbox_new(NON_HOMOGENOUS, 0);

	GtkWidget* row1 = window.toolbar = gtk_hbox_new(NON_HOMOGENOUS, 0);
	gtk_box_pack_start(GTK_BOX(filter_vbox), row1, EXPAND_FALSE, FILL_FALSE, 0);

	gboolean on_focus_out (GtkWidget* widget, GdkEventFocus* event, gpointer user_data)
	{
		PF;
		const gchar* text = gtk_entry_get_text(GTK_ENTRY(window.search));
		observable_string_set(samplecat.model->filters2.search, g_strdup(text));
		return NOT_HANDLED;
	}

	Observable* filter = samplecat.model->filters2.search;
	GtkWidget* entry = window.search = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(entry), 64);
	gtk_entry_set_icon_from_stock (GTK_ENTRY(entry), GTK_ENTRY_ICON_PRIMARY, "gtk-find");
	if(filter->value.c) gtk_entry_set_text(GTK_ENTRY(entry), filter->value.c);
	gtk_widget_set_name(entry, "search-entry");
	gtk_box_pack_start(GTK_BOX(row1), entry, EXPAND_TRUE, TRUE, 0);
	g_signal_connect(G_OBJECT(entry), "focus-out-event", G_CALLBACK(on_focus_out), NULL);
	gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
	g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(on_focus_out), NULL);

	void on_search_filter_changed (Observable* _filter, AGlVal value, gpointer _entry)
	{
		gtk_entry_set_text(GTK_ENTRY(_entry), value.c);
	}
	agl_observable_subscribe (filter, on_search_filter_changed, entry);

	tagshow_selector_new();

	return filter_vbox;
}


static void
tagshow_selector_new ()
{
	#define ALL_CATEGORIES "All categories"

	GtkWidget* combo = gtk_combo_box_new_text();
	GtkComboBox* combo_ = GTK_COMBO_BOX(combo);
	gtk_combo_box_append_text(combo_, ALL_CATEGORIES);
	for(int i=0;i<G_N_ELEMENTS(samplecat.model->categories);i++){
		gtk_combo_box_append_text(combo_, samplecat.model->categories[i]);
	}
	gtk_combo_box_set_active(combo_, 0);
	gtk_box_pack_start(GTK_BOX(window.toolbar), combo, EXPAND_FALSE, FALSE, 0);

	void on_view_category_changed (GtkComboBox* widget, gpointer user_data)
	{
		// update the sample list with the new view-category
		PF;

		char* category = gtk_combo_box_get_active_text(GTK_COMBO_BOX(widget));
		if (!strcmp(category, ALL_CATEGORIES)) g_clear_pointer(&category, g_free);
		observable_string_set(samplecat.model->filters2.category, category);
	}
	g_signal_connect(combo, "changed", G_CALLBACK(on_view_category_changed), NULL);

	void on_category_filter_changed (Observable* filter, AGlVal value, gpointer user_data)
	{
		GtkComboBox* combo = user_data;

		if(!filter->value.c || !strlen(filter->value.c)){
			gtk_combo_box_set_active(combo, 0);
		}
	}

	agl_observable_subscribe (samplecat.model->filters2.category, on_category_filter_changed, combo);
}
