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
#include "file_manager/file_manager.h"
#include "icon_theme.h"

GList* themes = NULL;

static void get_theme_names      (GPtrArray*);
#ifdef GTK4_TODO
static void icon_theme_on_select (GtkMenuItem*, gpointer);
#ifdef DEBUG
static void print_icon_list      ();
#endif
#endif


GList*
icon_theme_init ()
{
	// Build a menu list of available themes.

	icon_theme = gtk_icon_theme_new(); // the default icon theme cannot be updated
	icon_theme_set_theme(NULL);

#ifdef DEBUG
	if (_debug_ > 1) {
		gchar** paths = gtk_icon_theme_get_search_path(icon_theme);
		gchar* path = paths[0];
		for (int i=0;path;path=paths[++i]) {
			dbg(2, "icon_theme_path=%s", path);
		}
		g_strfreev(paths);
	}
#endif

#ifdef GTK4_TODO
	GtkWidget* menu = gtk_menu_new();
#endif

	GPtrArray* names = g_ptr_array_new();
	get_theme_names(names);

	for (int i = 0; i < names->len; i++) {
		char* name = names->pdata[i];
		dbg(2, "name=%s", name);

#ifdef GTK4_TODO
		GtkWidget* item = gtk_menu_item_new_with_label(name);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

		g_object_set_data(G_OBJECT(item), "theme", g_strdup(name)); //make sure this string is free'd when menu is updated.

		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(icon_theme_on_select), NULL);
#endif

		g_free(name);
	}
#ifdef GTK4_TODO
	gtk_widget_show(menu);
#endif

	g_ptr_array_free(names, TRUE);

#ifdef GTK4_TODO
	return themes = g_list_append(NULL, menu);
#else
	return NULL;
#endif
}


const char*
find_icon_theme (const char* themes[])
{
	gboolean exists (const char* theme, char** paths)
	{
		gboolean found = FALSE;
		char* p;
		for (int i=0;(p=paths[i]);i++) {
			char* dir = g_build_filename(p, theme, NULL);
			found = g_file_test (dir, G_FILE_TEST_EXISTS);
			g_free(dir);
			if (found) {
				break;
			}
		}
		return found;
	}

	gchar** paths = gtk_icon_theme_get_search_path(icon_theme);
	if (paths) {
		const char* theme = NULL;

		for (int t = 0; themes[t]; t++) {
			if (exists(themes[t], paths)) {
				theme = themes[t];
				break;
			}
		}

		g_strfreev(paths);
		return theme;
	}
	return NULL;
}


static void
get_theme_names (GPtrArray* names)
{
	void add_themes_from_dir (GPtrArray* names, const char* dir)
	{
		if (access(dir, F_OK) != 0)	return;

		GPtrArray* list = list_dir((guchar*)dir);
		g_return_if_fail(list);

		for (int i = 0; i < list->len; i++) {
			char* index_path = g_build_filename(dir, list->pdata[i], "index.theme", NULL);
			
			if (access(index_path, F_OK) == 0){
				g_ptr_array_add(names, list->pdata[i]);
			}
			else g_free(list->pdata[i]);

			g_free(index_path);
		}

		g_ptr_array_free(list, TRUE);
	}

	gchar** theme_dirs = gtk_icon_theme_get_search_path(icon_theme); // dir list is derived from XDG_DATA_DIRS
	if (theme_dirs) {
		for (struct {gchar* dir; int i;} loop = {theme_dirs[0]}; loop.dir; loop.dir = theme_dirs[++loop.i])
			add_themes_from_dir(names, loop.dir);
		g_strfreev(theme_dirs);
	}

	g_ptr_array_sort(names, strcmp2);
}


#ifdef GTK4_TODO
static void
icon_theme_on_select (GtkMenuItem* menuitem, gpointer user_data)
{
	g_return_if_fail(menuitem);

	const gchar* name = g_object_get_data(G_OBJECT(menuitem), "theme");
	dbg(1, "theme=%s", name);

#ifdef DEBUG
	if(_debug_) print_icon_list();
#endif
	icon_theme_set_theme(name);
}
#endif


gboolean
check_default_theme (gpointer data)
{
	static char* names[] = {"audio-x-wav", "audio-x-generic", "gnome-mime-audio"};

	if (!theme_name[0]) {
		gboolean found = false;
		int i = 0;
		while (i++ < G_N_ELEMENTS(names) && !(found = gtk_icon_theme_has_icon (icon_theme, names[i])));

		if (!found)
			warnprintf("default icon theme appears not to contain audio mime-type icons\n");
	}
	return G_SOURCE_REMOVE;
}


void
icon_theme_set_theme (const char* name)
{
	if (name == theme_name)
		return;

	if (name && strcmp(theme_name, name))
		mime_type_clear();

	dbg(1, "setting theme: '%s'", name);

	if (name && name[0]) {
		g_strlcpy(theme_name, name, 64);

		gtk_icon_theme_set_theme_name(icon_theme, theme_name);
		g_signal_emit_by_name(icon_theme, "changed", NULL);
	} else {
		gtk_icon_theme_set_theme_name(icon_theme, NULL);
	}

	if (!strlen(theme_name))
		g_idle_add(check_default_theme, NULL);
}


#ifdef GTK4_TODO
#ifdef DEBUG
static void
print_icon_list ()
{
	char** icons = gtk_icon_theme_get_icon_names(icon_theme);
	if (icons) {
		dbg(0, "%s----------------------------------", theme_name);
		char* icon = icons[0];
		for (int i=0;icon;icon=icons[++i]) {
			printf("%s\n", icon);
		}
		g_strfreev(icons);
		printf("-------------------------------------------------\n");
	}
	else warnprintf("icon_theme has no mimetype icons?\n");

}
#endif
#endif


