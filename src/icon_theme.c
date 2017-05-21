/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2007-2017 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/

#include "config.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <sys/param.h>
#include <fnmatch.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <gtk/gtk.h>
#include "debug/debug.h"
#include "file_manager/file_manager.h"
#include "support.h"
#include "application.h"
#include "icon_theme.h"

GList* themes = NULL;
#if 0
char theme_name[64] = {0,};
#else
extern char theme_name[];
#endif
extern GtkIconTheme* icon_theme;
extern Application* application;

static void get_theme_names      (GPtrArray*);
static void icon_theme_on_select (GtkMenuItem*, gpointer);
#ifdef DEBUG
static void print_icon_list      ();
#endif


GList*
icon_theme_init()
{
	//-build a menu list of available themes.

	icon_theme_set_theme(NULL);

	if(_debug_){
		gint n_elements;
		gchar** path[64];
		gtk_icon_theme_get_search_path(icon_theme, path, &n_elements);
		int i;
		for(i=0;i<n_elements;i++){
			dbg(2, "icon_theme_path=%s", path[0][i]);
		}
		g_strfreev(*path);
	}

	GtkWidget* menu = gtk_menu_new();

	GPtrArray* names = g_ptr_array_new();
	get_theme_names(names);

	int i; for (i = 0; i < names->len; i++) {
		char* name = names->pdata[i];
		dbg(2, "name=%s", name);

		GtkWidget* item = gtk_menu_item_new_with_label(name);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

		g_object_set_data(G_OBJECT(item), "theme", g_strdup(name)); //make sure this string is free'd when menu is updated.

		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(icon_theme_on_select), NULL);

		g_free(name);
	}
	gtk_widget_show_all(menu);

	g_ptr_array_free(names, TRUE);

	return themes = g_list_append(NULL, menu);
}


static void
get_theme_names(GPtrArray* names)
{
	void add_themes_from_dir(GPtrArray* names, const char* dir)
	{
		if (access(dir, F_OK) != 0)	return;

		GPtrArray* list = list_dir((guchar*)dir);
		g_return_if_fail(list != NULL);

		int i;
		for (i = 0; i < list->len; i++){
			char* index_path = g_build_filename(dir, list->pdata[i], "index.theme", NULL);
			
			if (access(index_path, F_OK) == 0){
				g_ptr_array_add(names, list->pdata[i]);
			}
			else g_free(list->pdata[i]);

			g_free(index_path);
		}

		g_ptr_array_free(list, TRUE);
	}

	gint n_dirs = 0;
	gchar** theme_dirs = NULL;
	gtk_icon_theme_get_search_path(icon_theme, &theme_dirs, &n_dirs); // dir list is derived from XDG_DATA_DIRS
	int i; for (i = 0; i < n_dirs; i++) add_themes_from_dir(names, theme_dirs[i]);
	g_strfreev(theme_dirs);

	g_ptr_array_sort(names, strcmp2);
}


static void
icon_theme_on_select(GtkMenuItem* menuitem, gpointer user_data)
{
	g_return_if_fail(menuitem);

	const gchar* name = g_object_get_data(G_OBJECT(menuitem), "theme");
	dbg(1, "theme=%s", name);

#ifdef DEBUG
	if(_debug_) print_icon_list();
#endif
	icon_theme_set_theme(name);
	application_emit_icon_theme_changed(app, name);
}


bool
check_default_theme(gpointer data)
{
	// The default gtk icon theme "hi-color" does not contain any mimetype icons.

	static char* names[] = {"audio-x-wav", "audio-x-generic", "gnome-mime-audio"};

	if(!theme_name[0]){
		GtkIconInfo* info;
		int i = 0;
		while(i++ < G_N_ELEMENTS(names) && !(info = gtk_icon_theme_lookup_icon(icon_theme, names[i], ICON_HEIGHT, 0)));
		if(info){
			gtk_icon_info_free(info);
		}else{
			warnprintf("default icon theme appears not to contain audio mime-type icons\n");

			// TODO use a random fallback theme
		}
	}
	return G_SOURCE_REMOVE;
}


void
icon_theme_set_theme(const char* name)
{
	mime_type_clear();

	dbg(1, "setting theme: %s.", theme_name);

	if(name && name[0]){
		if(!*theme_name) icon_theme = gtk_icon_theme_new(); // the old icon theme cannot be updated
		g_strlcpy(theme_name, name, 64);
		gtk_icon_theme_set_custom_theme(icon_theme, theme_name);
	}

	if(!strlen(theme_name))
		g_idle_add(check_default_theme, NULL);

#if 0 // test is disabled as it is not reliable
	while (1)
	{
		GtkIconInfo* info = gtk_icon_theme_lookup_icon(icon_theme, "mime-application:postscript", ICON_HEIGHT, 0);
		if (!info)
		{
			//dbg(1, "looking up test icon...");
			info = gtk_icon_theme_lookup_icon(icon_theme, "gnome-mime-application-postscript", ICON_HEIGHT, 0);
		}
		if (info)
		{
			dbg(0, "got test icon ok. Using theme '%s'", theme_name);
			//print_icon_list();
			g_object_unref(info);
			return;
		}

		if (strcmp(theme_name, DEFAULT_THEME) == 0) break;

		warnprintf("Icon theme '%s' does not contain MIME icons. Using default theme instead.\n", theme_name);
		
		strcpy(theme_name, DEFAULT_THEME);
	}
#endif

#if 0
	const char *home_dir = g_get_home_dir();
	char *icon_home = g_build_filename(home_dir, ".icons", NULL);
	if (!file_exists(icon_home)) mkdir(icon_home, 0755);
	g_free(icon_home);
#endif

	//icon_home = g_build_filename(home_dir, ".icons", "ROX", NULL);
	//if (symlink(make_path(app_dir, "ROX"), icon_home))
	//	errprintf("Failed to create symlink '%s':\n%s\n\n"
	//	"(this may mean that the ROX theme already exists there, but "
	//	"the 'mime-application:postscript' icon couldn't be loaded for "
	//	"some reason)", icon_home, g_strerror(errno));
	//g_free(icon_home);

	gtk_icon_theme_rescan_if_needed(icon_theme);
}


#ifdef DEBUG
static void
print_icon_list()
{
	GList* icon_list = gtk_icon_theme_list_icons(icon_theme, "MimeTypes");
	if(icon_list){
		dbg(0, "%s----------------------------------", theme_name);
		for(;icon_list;icon_list=icon_list->next){
			char* icon = icon_list->data;
			printf("%s\n", icon);
			g_free(icon);
		}
		g_list_free(icon_list);
		printf("-------------------------------------------------\n");
	}
	else warnprintf("icon_theme has no mimetype icons?\n");

}
#endif


