/*
 * This file is part of the GNOME Devtools Libraries.
 *
 * Copyright (C) 2002 Gustavo Giráldez <gustavo.giraldez@gmx.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#pragma once

#include <gtk/gtk.h>
#include <gdl/gdl-dock-object.h>
#include <gdl/gdl-dock-item.h>
#include <gdl/gdl-dock-placeholder.h>

G_BEGIN_DECLS

#define GDL_TYPE_DOCK            (gdl_dock_get_type ())
#define GDL_DOCK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GDL_TYPE_DOCK, GdlDock))
#define GDL_DOCK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GDL_TYPE_DOCK, GdlDockClass))
#define GDL_IS_DOCK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GDL_TYPE_DOCK))
#define GDL_IS_DOCK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GDL_TYPE_DOCK))
#define GDL_DOCK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_DOCK, GdlDockClass))

typedef void (*GtkCallback) (GtkWidget *widget, gpointer data);

/* data types & structures */
typedef struct _GdlDock        GdlDock;
typedef struct _GdlDockClass   GdlDockClass;
typedef struct _GdlDockPrivate GdlDockPrivate;

struct _GdlDock {
    GdlDockObject    object;

    GdlDockPrivate  *priv;
};

struct _GdlDockClass {
    /*< private >*/
    GdlDockObjectClass parent_class;

    void  (* layout_changed)  (GdlDock *dock);    /* proxy signal for the master */
};

/* public interface */

GtkWidget     *gdl_dock_new               (void);

GtkWidget     *gdl_dock_new_from          (GdlDock          *original,
                                           gboolean          floating);

GType          gdl_dock_get_type          (void);

GdlDockItem*   gdl_dock_add_item          (GdlDock          *dock,
                                           GdlDockItem      *item,
                                           GdlDockPlacement  placement);
void           gdl_dock_just_add_item     (GdlDock          *dock,
                                           GdlDockItem      *item);

void           gdl_dock_add               (GdlDock          *container,
                                           GtkWidget        *widget);
void           gdl_dock_forall            (GdlDock          *container,
                                           gboolean          include_internals,
                                           GtkCallback       callback,
                                           gpointer          callback_data);

void           gdl_dock_add_floating_item (GdlDock          *dock,
                                           GdlDockItem      *item,
                                           gint              x,
                                           gint              y,
                                           gint              width,
                                           gint              height);

GdlDockItem   *gdl_dock_get_item_by_name  (GdlDock     *dock,
                                           const gchar *name);

GList         *gdl_dock_get_named_items   (GdlDock    *dock);

GdlDock       *gdl_dock_object_get_toplevel (GdlDockObject *object);
GdlDockObject *gdl_dock_get_root            (GdlDock       *dock);

void           gdl_dock_show_preview        (GdlDock       *dock,
                                             GdkRectangle  *rect);
void           gdl_dock_hide_preview        (GdlDock       *dock);
void           gdl_dock_show_overlay        (GdlDock       *dock, GtkWidget* overlay);
void           gdl_dock_hide_overlay        (GdlDock       *dock);

void           gdl_dock_set_skip_taskbar    (GdlDock       *dock,
                                             gboolean       skip);
#ifndef GDL_DISABLE_DEPRECATED
GdlDockPlaceholder *gdl_dock_get_placeholder_by_name (GdlDock     *dock,
                                                      const gchar *name);
#endif

G_END_DECLS
