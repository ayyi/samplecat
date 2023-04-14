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
#include <math.h>
#include <gtk/gtk.h>
#include "debug/debug.h"
#include "application.h"
#include "file_manager/file_manager.h"
#include "file_manager/menu.h"
#include "file_manager/pixmaps.h"

static void window_on_fileview_row_selected (GtkTreeView*, gpointer);


GtkWidget*
fileview_new ()
{
	GtkWidget* fman_hpaned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_paned_set_position(GTK_PANED(fman_hpaned), 210);

	void fman_left (const char* initial_folder)
	{
		void dir_on_select (ViewDirTree* vdt, const gchar* path, gpointer data)
		{
			PF;
			fm__change_to(file_manager__get(), path, NULL);
		}

		gint expand = TRUE;
		ViewDirTree* dir_list = ((Application*)app)->dir_treeview2 = vdtree_new(initial_folder, expand); 
		vdtree_set_select_func(dir_list, dir_on_select, NULL); //callback
		gtk_paned_set_start_child(GTK_PANED(fman_hpaned), dir_list->widget);

		void icon_theme_changed (Application* a, char* theme, gpointer _dir_tree)
		{
			vdtree_on_icon_theme_changed((ViewDirTree*)a->dir_treeview2);
		}
		g_signal_connect((gpointer)app, "icon-theme", G_CALLBACK(icon_theme_changed), dir_list);

#ifdef GTK4_TODO
		make_menu_actions(fm_tree_keys, G_N_ELEMENTS(fm_tree_keys), vdtree_add_menu_item);
#endif
	}

	void fman_right (const char* initial_folder)
	{
		GtkWidget* file_view = ((Application*)app)->fm_view = file_manager__new_window(initial_folder);
		AyyiFilemanager* fm = file_manager__get();
		gtk_paned_set_end_child(GTK_PANED(fman_hpaned), file_view);
		g_signal_connect(G_OBJECT(fm->view), "cursor-changed", G_CALLBACK(window_on_fileview_row_selected), NULL);

		void window_on_dir_changed (GtkWidget* widget, gpointer data)
		{
			PF;
		}
		g_signal_connect(G_OBJECT(file_manager__get()), "dir_changed", G_CALLBACK(window_on_dir_changed), NULL);

		void icon_theme_changed (Application* a, char* theme, gpointer _dir_tree)
		{
			file_manager__update_all();
		}
		g_signal_connect((gpointer)app, "icon-theme", G_CALLBACK(icon_theme_changed), file_view);

#ifdef GTK4_TODO
		make_menu_actions(menu_keys, G_N_ELEMENTS(menu_keys), fm__add_menu_item);
#endif

		//set up fileview as dnd source:
#ifdef GTK4_TODO
		gtk_drag_source_set(file_view, GDK_BUTTON1_MASK | GDK_BUTTON2_MASK, dnd_file_drag_types, dnd_file_drag_types_count, GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_ASK);
		g_signal_connect(G_OBJECT(file_view), "drag_data_get", G_CALLBACK(view_details_dnd_get), NULL);
#endif
	}

	const char* dir = (app->config.browse_dir[0] && g_file_test(app->config.browse_dir, G_FILE_TEST_IS_DIR))
		? app->config.browse_dir
		: g_get_home_dir();
	fman_left(dir);
	fman_right(dir);

	return fman_hpaned;
}


static void
window_on_fileview_row_selected (GtkTreeView* treeview, gpointer user_data)
{
	//a filesystem file has been clicked on.
	PF;

	AyyiFilemanager* fm = file_manager__get();

	gchar* full_path = NULL;
	DirItem* item;
	ViewIter iter;
	view_get_iter(fm->view, &iter, 0);
	while ((item = iter.next(&iter))) {
		if (view_get_selected(fm->view, &iter)) {
			full_path = g_build_filename(fm->real_path, item->leafname, NULL);
			break;
		}
	}
	if (!full_path) return;

	dbg(1, "%s", full_path);

	/* TODO: do nothing if directory selected 
	 * 
	 * this happens when a dir is selected in the left tree-browser
	 * while some file was previously selected in the right file-list
	 * -> we get the new dir + old filename
	 *
	 * event-handling in window.c should use 
	 *   gtk_tree_selection_set_select_function()
	 * or block file-list events during updates after the
	 * dir-tree brower selection changed.
	 */

	Sample* s = sample_new_from_filename(full_path, true);
	if (s) {
		s->online = true;
		samplecat_model_set_selection (samplecat.model, s);
		sample_unref(s);
	}
}
