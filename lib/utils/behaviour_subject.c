/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. https://ayyi.github.io/samplecat/    |
 | copyright (C) 2024-2025 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#include <glib-object.h>


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
