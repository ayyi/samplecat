/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. https://ayyi.github.io/samplecat/    |
 | copyright (C) 2007-2026 Tim Orford <tim@orford.org>                  |
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
#include "gtk/menu.h"
#include "debug/debug.h"
#include "application.h"
#include "support.h"
#include "file_manager/file_manager.h"
#include "file_manager/menu.h"
#include "file_manager/pixmaps.h"

static void fileview_on_row_selected (GtkTreeView*, AyyiFilemanager*);
static void add_to_db_on_activate    (GSimpleAction*, GVariant*, gpointer);
static void play_on_activate         (GSimpleAction*, GVariant*, gpointer);
#ifdef GTK4_TODO
static void menu__add_dir_to_db      (GSimpleAction*, GVariant*, gpointer);

Accel fm_tree_keys[] = {
	{"Add to database", NULL, {{(char)'y',}, {0,}}, menu__add_dir_to_db,   NULL},
};
#endif

Accel menu_keys[] = {
	{"Add to database", NULL, {{(char)'a',}, {0,}}, add_to_db_on_activate, GINT_TO_POINTER(0)},
	{"Play"           , NULL, {{(char)'p',}, {0,}}, play_on_activate,      NULL              },
};


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

			if (strcmp(path, vdt->path))
				fm__change_to(file_manager__get(), path, NULL);
		}

		#define expand TRUE
		ViewDirTree* dir_tree = vdtree_new(initial_folder, expand);
		vdtree_set_select_func(dir_tree, dir_on_select, NULL);
		gtk_paned_set_start_child(GTK_PANED(fman_hpaned), dir_tree->widget);

		void dir_tree_on_dir_changed (GtkWidget* widget, const char* dir, gpointer dir_tree)
		{
			PF;
			vdtree_set_path((ViewDirTree*)dir_tree, dir);
		}
		g_signal_connect(G_OBJECT(file_manager__get()), "dir-changed", G_CALLBACK(dir_tree_on_dir_changed), dir_tree);

		void icon_theme_changed (Application* a, char* theme, gpointer dir_tree)
		{
			vdtree_on_icon_theme_changed((ViewDirTree*)dir_tree);
		}
		g_signal_connect((gpointer)app, "icon-theme", G_CALLBACK(icon_theme_changed), dir_tree);

		void dir_tree_on_layout_changed (GObject* object, gpointer user_data)
		{
			ViewDirTree* dir_tree = user_data;

			// scroll to position
			vdtree_set_path(dir_tree, dir_tree->path);
		}

		Idle* idle = idle_new(dir_tree_on_layout_changed, dir_tree);
		g_signal_connect_data(app, "layout-changed", (GCallback)idle->run, idle, (GClosureNotify)g_free, 0);

		void files_on_dir_tree_finalize (gpointer idle, GObject* was)
		{
			g_signal_handlers_disconnect_matched(app, G_SIGNAL_MATCH_DATA, 0, 0, 0, NULL, ((Idle*)idle)->user_data);
		}
		g_object_weak_ref(G_OBJECT(dir_tree->widget), files_on_dir_tree_finalize, idle);

#ifdef GTK4_TODO
		// TODO menu is created dynanically on demand, so we can't yet access the menu or the model
		GMenuModel* section = (GMenuModel*)g_menu_new ();
		GMenuModel* model = gtk_popover_menu_get_menu_model (GTK_POPOVER_MENU(dir_tree->popup));
		g_menu_append_section (G_MENU(model), NULL, section);
		make_menu_actions(GTK_WIDGET(dir_tree), fm_tree_keys, G_N_ELEMENTS(fm_tree_keys), vdtree_add_menu_item, section);
#endif
	}

	void fman_right (const char* initial_folder)
	{
		GtkWidget* file_view = file_manager__new_window(initial_folder);
		AyyiFilemanager* fm = file_manager__get();
		gtk_paned_set_end_child(GTK_PANED(fman_hpaned), file_view);
		g_signal_connect(G_OBJECT(fm->view), "cursor-changed", G_CALLBACK(fileview_on_row_selected), fm);

		void file_view_on_dir_changed (GtkWidget* widget, gpointer data)
		{
			PF;
		}
		g_signal_connect(G_OBJECT(fm), "dir-changed", G_CALLBACK(file_view_on_dir_changed), NULL);

		void icon_theme_changed (Application* a, char* theme, gpointer _fm)
		{
			AyyiFilemanager* fm = _fm;

			if (fm->directory && !fm->directory->scanning) {
				fm__update_dir(fm, true);
			}
		}
		g_signal_connect((gpointer)app, "icon-theme", G_CALLBACK(icon_theme_changed), fm);

		fm->menu.user = (GMenuModel*)g_menu_new ();
		make_menu_actions(file_view, menu_keys, G_N_ELEMENTS(menu_keys), fm__add_menu_item, fm->menu.user);

		// set up fileview as dnd source
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
fileview_on_row_selected (GtkTreeView* treeview, AyyiFilemanager* fm)
{
	PF;

	if (fm->directory->needs_update) {
		return;
	}

	ViewIter iter;
	view_get_iter(fm->view, &iter, 0);

	DirItem* item;
	while ((item = iter.next(&iter))) {
		if (view_get_selected(fm->view, &iter)) {
			gchar* full_path = g_build_filename(fm->real_path, item->leafname, NULL);
			dbg(1, "%s", full_path);

			Sample* s = sample_new_from_filename(full_path, true);
			if (s) {
				s->online = true;
				samplecat_model_set_selection (samplecat.model, s);
				sample_unref(s);
			}

			break;
		}
	}
}


static void
add_to_db_on_activate (GSimpleAction* action, GVariant* parameter, gpointer app)
{
	PF;
	AyyiFilemanager* fm = file_manager__get();

	DirItem* item;
	ViewIter iter;
	view_get_iter(fm->view, &iter, 0);
	while ((item = iter.next(&iter))) {
		if (view_get_selected(fm->view, &iter)) {
			gchar* filepath = g_build_filename(fm->real_path, item->leafname, NULL);
#ifdef GTK4_TODO
			if (do_progress(0, 0)) break;
#endif
			ScanResults results = {0,};
			samplecat_application_add_file(filepath, &results);
			if (results.n_added) statusbar_print(1, "file added");
			g_free(filepath);
		}
	}
#ifdef GTK4_TODO
	hide_progress();
#endif
}


static void
play_on_activate (GSimpleAction* action, GVariant* parameter, gpointer app)
{
	PF;
#ifdef GTK4_TODO
	AyyiFilemanager* fm = file_manager__get();

	GList* selected = fm__selected_items(fm);
	GList* l = selected;
	for (;l;l=l->next) {
		char* item = l->data;
		dbg(1, "%s", item);
		application_play_path(item);
		g_free(item);
	}
	g_list_free(selected);
#endif
}


#ifdef GTK4_TODO
static void
menu__add_dir_to_db (GSimpleAction* action, GVariant* parameter, gpointer user_data)
{
	PF;
	const char* path = vdtree_get_selected(((Application*)app)->dir_treeview2);
	dbg(1, "path=%s", path);
	if (path) {
		ScanResults results = {0,};
		samplecat_application_scan(path, &results);
	}
}
#endif
