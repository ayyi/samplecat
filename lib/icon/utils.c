/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2007-2020 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#include "config.h"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <gtk/gtk.h>
#pragma GCC diagnostic warning "-Wdeprecated-declarations"
#include <GL/gl.h>
#include "debug/debug.h"

#include "file_manager/mimetype.h" // for mime_type_clear


const char*
find_icon_theme (const char* themes[])
{
	gboolean exists (const char* theme, char** paths)
	{
		gboolean found = FALSE;
		char* p;
		for(int i=0;(p=paths[i]);i++){
			char* dir = g_build_filename(p, theme, NULL);
			found = g_file_test (dir, G_FILE_TEST_EXISTS);
			g_free(dir);
			if(found){
				break;
			}
		}
		return found;
	}

	gchar** paths = NULL;
	int n_elements = 0;
	gtk_icon_theme_get_search_path(icon_theme, &paths, &n_elements);
	if(paths){
		const char* theme = NULL;

		for(int t = 0; themes[t]; t++){
			if(exists(themes[t], paths)){
				theme = themes[t];
				break;
			}
		}

		g_strfreev(paths);
		return theme;
	}
	return NULL;
}


void
set_icon_theme (const char* name)
{
	mime_type_clear();

	dbg(1, "setting theme: '%s'", theme_name);

	if(name && name[0]){
		if(!*theme_name) icon_theme = gtk_icon_theme_new(); // the old icon theme cannot be updated
		//ensure_valid_themes (icon_theme);
		gtk_icon_theme_has_icon (icon_theme, "image-jpg");

		g_strlcpy(theme_name, name, 64);
		gtk_icon_theme_set_custom_theme(icon_theme, theme_name);
#if 0
		gtk_icon_theme_list_icons (icon_theme, NULL);
#endif
	}

#if 0
	if(!strlen(theme_name))
		g_idle_add(check_default_theme, NULL);
#endif

	gtk_icon_theme_rescan_if_needed(icon_theme);
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

	g_object_unref(pixbuf);
	return t;
}


guint
get_icon_texture_by_name (const char* name, int size)
{
	if(!icon_textures) icon_textures = g_hash_table_new(NULL, NULL);

	char* key = MAKE_KEY(name, size);
	guint t = GPOINTER_TO_INT(g_hash_table_lookup(icon_textures, key));
	g_free(key);
	if(t){
		return t;
	}

	GError* error = NULL;
	GdkPixbuf* pixbuf = gtk_icon_theme_load_icon(icon_theme, name, size, 0, &error);
	if(error){
		gwarn("%s", error->message);
		g_error_free(error);
		return 0;
	}
	return texture_from_pixbuf(name, pixbuf);
}


guint
get_icon_texture_by_mimetype (MIME_type* mime_type)
{
	if(!icon_textures) icon_textures = g_hash_table_new(NULL, NULL);

	GdkPixbuf* pixbuf = mime_type_get_pixbuf(mime_type);
	return texture_from_pixbuf(mime_type->subtype, pixbuf);
}
