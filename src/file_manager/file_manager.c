/*
 * Copyright (C) 2007-2009, Tim Orford
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

#include "file_manager/file_manager.h"
#include "rox/view_iface.h"
#include "file_view.h"
#include "dir.h"
#include "diritem.h"
#include "rox/rox_support.h"
#include "mimetype.h"

#include "src/typedefs.h"
#include "src/support.h"


Filer filer;
GList* all_filer_windows = NULL;
static AyyiLibfilemanager* new_file_manager = NULL;
static gboolean initialised = FALSE;
extern char theme_name[];

Filer*
file_manager__init()
{
	filer.filter               = FILER_SHOW_ALL;
	filer.filter_string        = NULL;
	filer.regexp               = NULL;
	filer.filter_directories   = FALSE;
	filer.display_style_wanted = SMALL_ICONS;
	filer.sort_type            = SORT_NAME;

	new_file_manager = ayyi_libfilemanager_new(&filer);

	return &filer;
	//return &new_file_manager->file_window;
}


GtkWidget*
file_manager__new_window(const char* path)
{
	GtkWidget* file_view = view_details_new(&filer);
	filer.view = (ViewIface*)file_view;

	all_filer_windows = g_list_prepend(all_filer_windows, &filer);

	filer_change_to(&filer, path, NULL);

	return file_view;
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


//removed. call ayyi_filemanager_set_icon_theme(char*) instead.
/*
void
file_manager__set_icon_theme(const char* name)
{
	strncpy(theme_name, name, 63);
	_set_icon_theme();

	ayyi_filemanager_set_icon_theme(new_file_manager, name);
	//ayyi_filemanager_emit_theme_changed(new_file_manager);
}
*/


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


