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
#include <string.h>
#include "debug/debug.h"
#include "dh_link.h"
#include "widgets/dh_tree.h"
#include "support.h"
#include "application.h"
#include "model.h"
#include "dir_list.h"
#include "directories.h"

static bool on_dir_tree_link_selected (GObject*, DhLink*, gpointer);


GtkWidget*
dir_panel_new ()
{
	GtkWidget* _dir_tree_new ()
	{
		dir_list_update(); // because this is slow, it is not done until a consumer needs it.

		return ((Application*)app)->dir_treeview = dh_book_tree_new(&samplecat.model->dir_tree);
	}

	GtkWidget* tree = _dir_tree_new();
	GtkWidget* widget = scrolled_window_new();
	gtk_scrolled_window_set_child ((GtkScrolledWindow*)widget, tree);
	g_signal_connect(tree, "link-selected", G_CALLBACK(on_dir_tree_link_selected), NULL);

	/* The dir tree shows ALL directories and is not affected by the current filter setting.
	void on_dir_filter_changed(GObject* _filter, gpointer _entry)
	{
		SamplecatFilter* filter = (SamplecatFilter*)_filter;
	}
	g_signal_connect(samplecat.model->filters.dir, "changed", G_CALLBACK(on_dir_filter_changed), NULL);
	*/

	void on_dir_list_changed (GObject* _model, gpointer user_data)
	{
		/*
		 * Refresh the directory tree.
		 *
		 * TODO make updates more efficient so the whole node tree doesnt have to be recreated
		 */
		dh_book_tree_reload((DhBookTree*)((Application*)app)->dir_treeview);
	}
	g_signal_connect(samplecat.model, "dir-list-changed", G_CALLBACK(on_dir_list_changed), NULL);

	void dir_on_theme_changed (Application* a, char* theme, gpointer _tree)
	{
		GtkWidget* container = gtk_widget_get_parent(a->dir_treeview);
		gtk_widget_unparent(a->dir_treeview);
		a->dir_treeview = _dir_tree_new();
		gtk_scrolled_window_set_child((GtkScrolledWindow*)container, a->dir_treeview);
	}
	g_signal_connect(app, "icon-theme", G_CALLBACK(dir_on_theme_changed), &tree);

	return widget;
}


static bool
on_dir_tree_link_selected (GObject* _, DhLink* link, gpointer data)
{
	g_return_val_if_fail(link, false);

	dbg(1, "dir=%s", link->uri);

	observable_string_set(samplecat.model->filters2.dir, g_strdup(link->uri));

	return false;
}

