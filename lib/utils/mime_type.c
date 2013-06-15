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
#include "utils/ayyi_utils.h"
#include "utils/fscache.h"
#include "mime_type.h"

GtkIconTheme* icon_theme = NULL;


GdkPixbuf*
mime_type_get_pixbuf(MIME_type* mime_type)
{
	g_return_val_if_fail(mime_type, NULL);

	type_to_icon(mime_type);
	if ( mime_type->image == NULL ) dbg(0, "no icon.\n");
	return mime_type->image->sm_pixbuf;
}


/* Return the image for this type, loading it if needed.
 * Places to check are: (eg type="text_plain", base="text")
 * 1. <Choices>/MIME-icons/base_subtype
 * 2. Icon theme 'mime-base:subtype'
 * 3. Icon theme 'mime-base'
 * 4. Unknown type icon.
 *
 * Note: You must g_object_unref() the image afterwards.
 */
MaskedPixmap*
type_to_icon(MIME_type* type)
{
	GtkIconInfo* full;
	char* type_name, *path;
	char icon_height = 16; //HUGE_HEIGHT 
	time_t now;

	if (!type){	g_object_ref(im_unknown); return im_unknown; }

	now = time(NULL);
	// already got an image?
	if (type->image) {
		// Yes - don't recheck too often
		if (abs(now - type->image_time) < 2)
		{
			g_object_ref(type->image);
			return type->image;
		}
		g_object_unref(type->image);
		type->image = NULL;
	}

	//gboolean is_audio = !strcmp(type->media_type, "audio");

	type_name = g_strconcat(type->media_type, "_", type->subtype, ".png", NULL);
	//path = choices_find_xdg_path_load(type_name, "MIME-icons", SITE);
	path = NULL; //apparently NULL means use default.
	g_free(type_name);
	if (path) {
		//type->image = g_fscache_lookup(pixmap_cache, path);
		//g_free(path);
	}

	if (type->image) goto out;

	type_name = g_strconcat("mime-", type->media_type, ":", type->subtype, NULL);
	dbg(2, "%s", type_name);
	//full = gtk_icon_theme_load_icon(icon_theme, type_name, icon_height, 0, NULL);
	full = gtk_icon_theme_lookup_icon(icon_theme, type_name, icon_height, 0);
	if(full) dbg(0, "found");
	g_free(type_name);
	if (!full) {
		//printf("type_to_icon(): gtk_icon_theme_load_icon() failed. mimetype=%s/%s\n", type->media_type, type->subtype);
		// Ugly hack... try for a GNOME icon
		if (type == inode_directory) type_name = g_strdup("gnome-fs-directory");
		else type_name = g_strconcat("gnome-mime-", type->media_type, "-", type->subtype, NULL);
		//full = gtk_icon_theme_load_icon(icon_theme, type_name, icon_height, 0, NULL);
		full = gtk_icon_theme_lookup_icon(icon_theme, type_name, icon_height, 0);
		//if (full) dbg(0, "found icon! iconname=%s", type_name);
		g_free(type_name);
	}
	if (!full) {
		// try for a media type:
		type_name = g_strconcat("mime-", type->media_type, NULL);
		//full = gtk_icon_theme_load_icon(icon_theme,	type_name, icon_height, 0, NULL);
		full = gtk_icon_theme_lookup_icon(icon_theme, type_name, icon_height, 0);
		//if (full) dbg(0, "gtk_icon_theme_load_icon() found icon! iconname=%s", type_name);
		//else dbg(2, "not found: %s", type_name);
		g_free(type_name);
	}
	if (!full) {
		// Ugly hack... try for a GNOME default media icon
		type_name = g_strconcat("gnome-mime-", type->media_type, NULL);

		full = gtk_icon_theme_lookup_icon(icon_theme, type_name, icon_height, 0);
		//if(!full) dbg(0, "not found: %s", type_name);
 		g_free(type_name);
 	}
	if (!full/* && is_audio*/) {
		// try any old non-standard rubbish:
		type_name = g_strconcat(type->media_type, "-x-generic", NULL);
		//full = gtk_icon_theme_load_icon(icon_theme,	type_name, icon_height, 0, NULL);
		full = gtk_icon_theme_lookup_icon(icon_theme, type_name, icon_height, 0);
		//if (full) dbg(0, "gtk_icon_theme_load_icon() found icon! iconname=%s", type_name);
		//else dbg(0, "not found: %s", type_name);
		g_free(type_name);
	}
	if (!full) {
		// try any old non-standard rubbish:
		type_name = g_strconcat("gnome-fs-regular", NULL);
		full = gtk_icon_theme_lookup_icon(icon_theme, type_name, icon_height, 0);
		//if (full) dbg(0, "gtk_icon_theme_load_icon() found icon! iconname=%s", type_name);
		//else dbg(0, "not found: %s", type_name);
		g_free(type_name);
	}
	if (full) {
		const char *icon_path;
		/* Get the actual icon through our cache, not through GTK, because
		 * GTK doesn't cache icons.
		 */
		icon_path = gtk_icon_info_get_filename(full);
		if (icon_path) type->image = g_fscache_lookup(pixmap_cache, icon_path);
		//if (icon_path) type->image = masked_pixmap_new(full);
		/* else shouldn't happen, because we didn't use
		 * GTK_ICON_LOOKUP_USE_BUILTIN.
		 */
		gtk_icon_info_free(full);
	}

out:
	if (!type->image)
	{
		dbg(2, "failed! using im_unknown.");
		/* One ref from the type structure, one returned */
		type->image = im_unknown;
		g_object_ref(im_unknown);
	}

	type->image_time = now;
	
	g_object_ref(type->image);
	return type->image;
}

