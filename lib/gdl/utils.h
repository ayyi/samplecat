/*
 * Copyright (C) 2023-2025 Tim Orford
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <gtk/gtk.h>

/*
 *   Substitute for deprecated GObject.Parameter
 */
typedef struct {
   const gchar* name;
   GValue       value;
} DockParameter;

typedef void (*GtkCallback) (GtkWidget*, gpointer data);

GList* gtk_widget_get_children (GtkWidget*);

gchar* gdl_remove_extension_from_path (const gchar*);

#define set_str(P, NAME) ({ if (P) g_free(P); P = NAME; })
#define DOCK_NEW(T, ...) ({T* obj = g_new0(T, 1); *obj = (T){__VA_ARGS__}; obj;})
