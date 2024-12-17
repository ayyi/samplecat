/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. https://ayyi.github.io/samplecat/    |
 | copyright (C) 2023-2024 Tim Orford <tim@orford.org>                  |
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


/*
 *  Similar to connecting to a gobject notify signal but with an additional callback for the current value
 */
void
behaviour_subject_connect (GObject* object, const char* prop, void (*callback)(GObject*, GParamSpec*, gpointer), gpointer user_data)
{
	GParamSpec* pspec = g_object_class_find_property (G_OBJECT_GET_CLASS(object), prop);
	callback (object, pspec, user_data);

	g_autofree char* signal = g_strdup_printf("notify::%s", prop);
	g_signal_connect (object, signal, G_CALLBACK(callback), user_data);
}
