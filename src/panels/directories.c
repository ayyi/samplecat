/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
 | copyright (C) 2007-2025 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#include "config.h"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <gtk/gtk.h>
#pragma GCC diagnostic warning "-Wdeprecated-declarations"
#include <string.h>
#include "debug/debug.h"
#include "dh_link.h"
#include "dh_tree.h"
#include "typedefs.h"
#include "support.h"
#include "application.h"
#include "model.h"
#include "dir_list.h"
#include "directories.h"

static bool on_dir_tree_link_selected (GObject*, DhLink*, gpointer);


GtkWidget*
dir_panel_new ()
{
	GtkWidget* tree = app->dir_treeview = dh_book_tree_new(&samplecat.model->dir_tree);
	GtkWidget* widget = scrolled_window_new();
	gtk_container_add((GtkContainer*)widget, tree);
	g_signal_connect(tree, "link-selected", G_CALLBACK(on_dir_tree_link_selected), NULL);

	/* The dir tree shows ALL directories and is not affected by the current filter setting.
	void on_dir_filter_changed(GObject* _filter, gpointer _entry)
	{
		SamplecatFilter* filter = (SamplecatFilter*)_filter;
	}
	g_signal_connect(samplecat.model->filters.dir, "changed", G_CALLBACK(on_dir_filter_changed), NULL);
	*/

	void on_dir_list_changed (GObject* _model, void* _tree, gpointer tree)
	{
		/*
		 * Refresh the directory tree.
		 */
		dh_book_tree_reload((DhBookTree*)app->dir_treeview);
	}
	dir_list_register(on_dir_list_changed, tree);

	void dirs_on_finalize (gpointer tree, GObject* was)
	{
		dir_list_unregister (tree);
	}
	g_object_weak_ref(G_OBJECT(tree), dirs_on_finalize, tree);

	void dir_on_theme_changed (Application* a, char* theme, gpointer _tree)
	{
		GtkWidget* container = gtk_widget_get_parent(app->dir_treeview);
		gtk_widget_destroy(app->dir_treeview);
		app->dir_treeview = dh_book_tree_new(&samplecat.model->dir_tree);
		gtk_container_add((GtkContainer*)container, app->dir_treeview);
		gtk_widget_show(app->dir_treeview);
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

