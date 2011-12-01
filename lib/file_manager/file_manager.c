/*
 * Copyright (C) 2007-2011, Tim Orford
 * Copyright (C) 2006, Thomas Leonard and others (see changelog for details).
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; version 3.
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

/* file_manager.c - provides single-instance backend for filesystem views */

#include "config.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "utils/ayyi_utils.h"
#include "utils/mime_type.h"
#include "file_manager.h"

Filer filer;
GList* all_filer_windows = NULL;
static AyyiLibfilemanager* new_file_manager = NULL;
static gboolean initialised = FALSE;
extern char theme_name[];

GSList* plugins = NULL;

typedef	FileTypePluginPtr (*infoFunc)();
enum {
    PLUGIN_TYPE_1 = 1,
    PLUGIN_TYPE_MAX
};
static void file_manager__load_plugins();


static Filer*
file_manager__init()
{
	filer.filter               = FILER_SHOW_ALL;
	filer.filter_string        = NULL;
	filer.regexp               = NULL;
	filer.filter_directories   = FALSE;
	filer.display_style_wanted = SMALL_ICONS;
	filer.sort_type            = SORT_NAME;

	new_file_manager = ayyi_libfilemanager_new(&filer);

	//type_init();
	pixmaps_init();
	dir_init();

	gboolean _load_plugins(gpointer user_data){ file_manager__load_plugins(); return IDLE_STOP; }
	g_idle_add(_load_plugins, NULL);

	initialised = TRUE;
	return &filer;
	//return &new_file_manager->file_window;
}


GtkWidget*
file_manager__new_window(const char* path)
{
	if(!initialised) file_manager__init();

	GtkWidget* file_view = view_details_new(&filer);
	filer.view = (ViewIface*)file_view;

	all_filer_windows = g_list_prepend(all_filer_windows, &filer);

	filer_change_to(&filer, path, NULL);

	return file_view;
}


Filer*
file_manager__get()
{
	if(!initialised) file_manager__init();
	return &filer;
}


void
file_manager__update_all(void)
{
	GList *next = all_filer_windows;

	while (next)
	{
		Filer *filer_window = (Filer*) next->data;

		/* Updating directory may remove it from list -- stop sending
		 * patches to move this line!
		 */
		next = next->next;

		/* Don't trigger a refresh if we're already scanning.
		 * Otherwise, two views of a single directory will trigger
		 * two scans.
		 */
		if (filer_window->directory &&
				!filer_window->directory->scanning)
			filer_update_dir(filer_window, TRUE);
	}
}


void
file_manager__on_dir_changed()
{
	ayyi_libfilemanager_emit_dir_changed(new_file_manager);
}


AyyiLibfilemanager*
file_manager__get_signaller()
{
	return new_file_manager;
}


static FileTypePluginPtr
file_manager__plugin_load(const gchar* filepath)
{
	FileTypePluginPtr plugin = NULL;
	gboolean success = FALSE;

#if GLIB_CHECK_VERSION(2,3,3)
	GModule* handle = g_module_open(filepath, G_MODULE_BIND_LOCAL);
#else
	GModule* handle = g_module_open(filepath, 0);
#endif

	if(!handle) {
		gwarn("cannot open %s (%s)!", filepath, g_module_error());
		return NULL;
	}

	infoFunc plugin_get_info;
	if(g_module_symbol(handle, "plugin_get_info", (void*)&plugin_get_info)) {
		// load generic plugin info
		if(NULL != (plugin = (*plugin_get_info)())) {
			// check plugin version
			if(FILETYPE_PLUGIN_API_VERSION != plugin->api_version){
				dbg(0, "API version mismatch: \"%s\" (%s, type=%d) has version %d should be %d", plugin->name, filepath, plugin->type, plugin->api_version, FILETYPE_PLUGIN_API_VERSION);
			}

			/* check if all mandatory symbols are provided */
			if (!(plugin->plugin_init &&
				  plugin->plugin_deinit)) {
				dbg(0, "'%s': mandatory symbols missing.", plugin->name);
				return FALSE;
			}
			success = TRUE;

			// try to load specific plugin type symbols
			switch(plugin->type) {
				default:
					if(plugin->type >= PLUGIN_TYPE_MAX) {
						dbg(0, "Unknown or unsupported plugin type: %s (%s, type=%d)", plugin->name, filepath, plugin->type);
					} else {
						dbg(0, "name='%s'", plugin->name);
					}
					break;
			}
		}
	} else {
		gwarn("File '%s' is not a valid Filtype plugin", filepath);
	}
	
	if(!success) {
		g_module_close(handle);
		return NULL;
	}
		
	return plugin;
}


#define plugin_path "/usr/lib/samplecat/:/usr/local/lib/samplecat/"
static void file_manager__load_plugins()
{
	FileTypePluginPtr plugin = NULL;
	GError* error = NULL;

	if(!g_module_supported()) g_error("Modules not supported! (%s)", g_module_error());

	int found = 0;

	gchar** paths = g_strsplit(plugin_path, ":", 0);
	char* path;
	int i = 0;
	while((path = paths[i++])){
		if(!g_file_test(path, G_FILE_TEST_EXISTS)) continue;

		dbg(1, "scanning for plugins (%s) ...", path);
		GDir* dir = g_dir_open(path, 0, &error);
		if (!error) {
			const gchar* filename = g_dir_read_name(dir);
			while (filename) {
				dbg(1, "testing %s...", filename);
				gchar* filepath = g_build_filename(path, filename, NULL);
				// filter files with correct library suffix
				if(!strncmp(G_MODULE_SUFFIX, filename + strlen(filename) - strlen(G_MODULE_SUFFIX), strlen(G_MODULE_SUFFIX))) {
					// If we find one, try to load plugin info and if this was successful try to invoke the specific plugin
					// type loader. If the second loading went well add the plugin to the plugin list.
					if (!(plugin = file_manager__plugin_load(filepath))) {
						dbg(0, "'%s' failed to load.", filename);
					} else {
						found++;
						plugins = g_slist_append(plugins, plugin);
					}
				} else {
					dbg(2, "-> no library suffix");
				}
				g_free(filepath);
				filename = g_dir_read_name(dir);
			}
			g_dir_close(dir);
		} else {
			gwarn("dir='%s' failed. %s", path, error->message);
			g_error_free(error);
			error = NULL;
		}
	}
	g_strfreev(paths);

	dbg(1, "filetype plugins loaded: %i.", found);
}


