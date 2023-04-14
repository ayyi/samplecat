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
#include <GL/gl.h>
#include "debug/debug.h"

#include "file_manager/mimetype.h" // for mime_type_clear


void
set_icon_theme (const char* name)
{
	mime_type_clear();

	dbg(1, "setting theme: '%s'", theme_name);

	if (name && name[0]) {
		if (!*theme_name)
			icon_theme = gtk_icon_theme_new(); // the old icon theme cannot be updated

		//ensure_valid_themes (icon_theme);
		gtk_icon_theme_has_icon (icon_theme, "image-jpg");

		g_strlcpy(theme_name, name, 64);
		gtk_icon_theme_set_theme_name(icon_theme, theme_name);
#if 0
		gtk_icon_theme_list_icons (icon_theme, NULL);
#endif
	}

#if 0
	if(!strlen(theme_name))
		g_idle_add(check_default_theme, NULL);
#endif
}


static GHashTable* icon_textures = NULL;
#define MAKE_KEY(name, size) g_strdup_printf("%s-%i", name, size)

static guint
texture_from_pixbuf (const char* name, GdkPixbuf* pixbuf)
{
	guint
	create_icon (const char* name, GdkPixbuf* pixbuf)
	{
		g_return_val_if_fail(pixbuf, 0);

		guint textures[1];
		glGenTextures(1, textures);

		dbg(2, "icon: pixbuf=%ix%i %ibytes/px", gdk_pixbuf_get_width(pixbuf), gdk_pixbuf_get_height(pixbuf), gdk_pixbuf_get_n_channels(pixbuf));
		glBindTexture   (GL_TEXTURE_2D, textures[0]);
		glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D    (GL_TEXTURE_2D, 0, GL_RGBA, gdk_pixbuf_get_width(pixbuf), gdk_pixbuf_get_height(pixbuf), 0, GL_RGBA, GL_UNSIGNED_BYTE, gdk_pixbuf_get_pixels(pixbuf));
		//gl_warn("texture bind");

		return textures[0];
	}

	guint t = create_icon(name, pixbuf);
	g_hash_table_insert(icon_textures, (gpointer)MAKE_KEY(name, gdk_pixbuf_get_width(pixbuf)), GINT_TO_POINTER(t));

#if 0 // no, this is not safe
	g_object_unref(pixbuf);
#endif
	return t;
}


guint
get_icon_texture_by_name (const char* name, int size)
{
	if (!icon_textures) icon_textures = g_hash_table_new(NULL, NULL);

	char* key = MAKE_KEY(name, size);
	guint t = GPOINTER_TO_INT(g_hash_table_lookup(icon_textures, key));
	g_free(key);
	if (t) {
		return t;
	}

	g_autoptr(GIcon) icon_name = g_content_type_get_icon (name);
	GtkIconPaintable* paintable = gtk_icon_theme_lookup_by_gicon(icon_theme, icon_name, size, 1, GTK_TEXT_DIR_NONE, 0);
	GFile* file = gtk_icon_paintable_get_file (paintable);
	if (file) {
		char* path = g_file_get_path (file);
		g_object_unref(file);
		if (path) {
			GdkPixbuf* pixbuf = gdk_pixbuf_new_from_file (path, NULL);
			g_free(path);
			if (pixbuf)
				return texture_from_pixbuf(name, pixbuf);
		} else {
			pwarn("icon not found for %s", name);
		}
	}

#ifdef GTK4_TODO
	GdkTexture* icon = NULL;

	GdkSnapshot* snapshot = NULL;
	gdk_paintable_snapshot (paintable, snapshot, size, size);
	g_object_unref(paintable);

	guchar* data[4 * size * size];
	gsize stride = 4 * size;
	gdk_texture_download (icon, data, stride);
#endif

	return 0;
}


guint
get_icon_texture_by_mimetype (MIME_type* mime_type)
{
	if (!icon_textures) icon_textures = g_hash_table_new(NULL, NULL);

	GdkPixbuf* pixbuf = mime_type_get_pixbuf(mime_type);
	return texture_from_pixbuf(mime_type->subtype, pixbuf);
}
