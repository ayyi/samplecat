/*
 * $Id: type.c,v 1.169 2005/07/24 10:19:31 tal197 Exp $
 *
 * ROX-Filer, filer for the ROX desktop project, v2.3
 * Copyright (C) 2005, the ROX-Filer team.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
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
#include "observer.h"
#include "application.h"
#include "icon_theme.h"

GList* themes = NULL; 
char theme_name[64] = "Amaranth";
extern unsigned debug;
extern GtkIconTheme* icon_theme;
extern Application* application;

static void add_themes_from_dir (GPtrArray* names, const char* dir);
static void on_theme_select     (GtkMenuItem*, gpointer);
static void print_icon_list     ();


/*static */GList*
icon_theme_init()
{
	//-build a menu list of available themes.

	icon_theme = gtk_icon_theme_new();
	//icon_theme = gtk_icon_theme_get_default();
	set_icon_theme();

	if(debug){
		gint n_elements;
		gchar** path[64];
		gtk_icon_theme_get_search_path(icon_theme, path, &n_elements);
		int i;
		for(i=0;i<n_elements;i++){
			dbg(2, "icon_theme_path=%s", path[0][i]);
		}
		g_strfreev(*path);
	}

	gchar **theme_dirs = NULL;
	gint n_dirs = 0;
	int i;

	GtkWidget* menu = gtk_menu_new();

	gtk_icon_theme_get_search_path(icon_theme, &theme_dirs, &n_dirs);
	GPtrArray* names = g_ptr_array_new();
	for (i = 0; i < n_dirs; i++) add_themes_from_dir(names, theme_dirs[i]);
	g_strfreev(theme_dirs);

	g_ptr_array_sort(names, strcmp2);

	for (i = 0; i < names->len; i++) {
		char *name = names->pdata[i];
		dbg(2, "name=%s", name);

		GtkWidget* item = gtk_menu_item_new_with_label(name);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		gtk_widget_show_all(item);

		g_object_set_data(G_OBJECT(item), "theme", g_strdup(name)); //make sure this string is free'd when menu is updated.

		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(on_theme_select), NULL);

		g_free(name);
	}

	g_ptr_array_free(names, TRUE);

	return themes  = g_list_append(NULL, menu);
}


static void
add_themes_from_dir(GPtrArray* names, const char* dir)
{
	if (access(dir, F_OK) != 0)	return;

	GPtrArray* list = list_dir((guchar*)dir);
	g_return_if_fail(list != NULL);

	int i;
	for (i = 0; i < list->len; i++){
		char *index_path;

		index_path = g_build_filename(dir, list->pdata[i], "index.theme", NULL);
		
		if (access(index_path, F_OK) == 0){
			g_ptr_array_add(names, list->pdata[i]);
		}
		else g_free(list->pdata[i]);

		g_free(index_path);
	}

	g_ptr_array_free(list, TRUE);
}


static void
on_theme_select(GtkMenuItem* menuitem, gpointer user_data)
{
	g_return_if_fail(menuitem);

	gchar* name = g_object_get_data(G_OBJECT(menuitem), "theme");
	dbg(0, "theme=%s", name);

	if(name && strlen(name)) strncpy(theme_name, name, 64);

	if(debug) print_icon_list();
	set_icon_theme();
	application_emit_icon_theme_changed(application, name);
}


void
set_icon_theme()
{
	//const char *home_dir = g_get_home_dir();
	GtkIconInfo *info;
	//char *icon_home;

	//if (!theme_name || !*theme_name) theme_name = "ROX";
	if (/*!theme_name ||*/ !*theme_name) strcpy(theme_name, "ROX");

	//g_hash_table_remove_all(type_hash);
	reset_type_hash();

	while (1)
	{
		dbg(1, "setting theme: %s.", theme_name);
		gtk_icon_theme_set_custom_theme(icon_theme, theme_name);
		return;

		//the test below is disabled as its not reliable

		info = gtk_icon_theme_lookup_icon(icon_theme, "mime-application:postscript", ICON_HEIGHT, 0);
		if (!info)
		{
			//dbg(1, "looking up test icon...");
			info = gtk_icon_theme_lookup_icon(icon_theme, "gnome-mime-application-postscript", ICON_HEIGHT, 0);
		}
		if (info)
		{
			dbg(0, "got test icon ok. Using theme '%s'", theme_name);
			//print_icon_list();
			gtk_icon_info_free(info);
			return;
		}

		if (strcmp(theme_name, "ROX") == 0) break;

		warnprintf("Icon theme '%s' does not contain MIME icons. Using ROX default theme instead.\n", theme_name);
		
		//theme_name = "ROX";
		strcpy(theme_name, "ROX");
	}

#if 0
	icon_home = g_build_filename(home_dir, ".icons", NULL);
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


