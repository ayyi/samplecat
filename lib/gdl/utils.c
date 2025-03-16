/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. https://ayyi.github.io/samplecat/    |
 | copyright (C) 2023-2025 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#include <gtk/gtk.h>


GList*
gtk_widget_get_children (GtkWidget* widget)
{
	GtkWidget* child = gtk_widget_get_first_child (widget);
	GList* l = g_list_prepend(NULL, child);

	while ((child = gtk_widget_get_next_sibling (child))) {
		l = g_list_append(l, child);
	}

	return l;
}


gchar*
gdl_remove_extension_from_path (const gchar* path)
{
	const gchar* ptr = path;

	if (!path) return NULL;
	if (strlen(path) < 2) return g_strdup(path);

	int p = strlen(path) - 1;
	while (ptr[p] != '.' && p > 0) p--;
	if (p == 0) p = strlen(path) - 1;
	return g_strndup(path, (guint)p);
}
