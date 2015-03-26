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

	if (type->image) goto out;

	GtkIconInfo* get_icon(char* type_name)
	{
		GtkIconInfo* info = gtk_icon_theme_lookup_icon(icon_theme, type_name, icon_height, 0);
		g_free(type_name);
		return info;
	}

	GtkIconInfo* info;
	{
		info = get_icon(g_strconcat(type->media_type, "-", type->subtype, NULL));
	}
	if (!info)
		info = get_icon(g_strconcat("mime-", type->media_type, ":", type->subtype, NULL));
	if (!info) {
		// Ugly hack... try for a GNOME icon
		info = get_icon(type == inode_directory
			? g_strdup("gnome-fs-directory")
			: g_strconcat("gnome-mime-", type->media_type, "-", type->subtype, NULL)
		);
	}
	if (!info) {
		// try for a media type:
		info = get_icon(g_strconcat("mime-", type->media_type, NULL));
	}
	if (!info) {
		// Ugly hack... try for a GNOME default media icon
		info = get_icon(g_strconcat("gnome-mime-", type->media_type, NULL));
 	}
	if (!info) {
		// try any old non-standard rubbish:
		info = get_icon(g_strconcat(type->media_type, "-x-generic", NULL));
	}
	if (!info) {
		// try any old non-standard rubbish:
		info = get_icon(g_strconcat("gnome-fs-regular", NULL));
	}

	if (info) {
		const char *icon_path;
		/* Get the actual icon through our cache, not through GTK, because
		 * GTK doesn't cache icons.
		 */
		icon_path = gtk_icon_info_get_filename(info);
		if (icon_path) type->image = g_fscache_lookup(pixmap_cache, icon_path);
		//if (icon_path) type->image = masked_pixmap_new(info);
		/* else shouldn't happen, because we didn't use
		 * GTK_ICON_LOOKUP_USE_BUILTIN.
		 */
		gtk_icon_info_free(info);
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

