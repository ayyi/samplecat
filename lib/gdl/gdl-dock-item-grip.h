/*
 * gdl-dock-item-grip.h
 *
 * Author: Michael Meeks Copyright (C) 2002 Sun Microsystems, Inc.
 *
 * Based on BonoboDockItemGrip.  Original copyright notice follows.
 *
 * Copyright (C) 1998 Ettore Perazzoli
 * Copyright (C) 1998 Elliot Lee
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * All rights reserved.
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
#include <gdl/gdl-dock-item.h>

G_BEGIN_DECLS

#define GDL_TYPE_DOCK_ITEM_GRIP            (gdl_dock_item_grip_get_type())
#define GDL_DOCK_ITEM_GRIP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GDL_TYPE_DOCK_ITEM_GRIP, GdlDockItemGrip))
#define GDL_DOCK_ITEM_GRIP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GDL_TYPE_DOCK_ITEM_GRIP, GdlDockItemGripClass))
#define GDL_IS_DOCK_ITEM_GRIP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GDL_TYPE_DOCK_ITEM_GRIP))
#define GDL_IS_DOCK_ITEM_GRIP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GDL_TYPE_DOCK_ITEM_GRIP))
#define GDL_DOCK_ITEM_GRIP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GDL_TYPE_DOCK_ITEM_GRIP, GdlDockItemGripClass))

typedef struct _GdlDockItemGrip        GdlDockItemGrip;
typedef struct _GdlDockItemGripClass   GdlDockItemGripClass;
typedef struct _GdlDockItemGripPrivate GdlDockItemGripPrivate;
typedef struct _GdlDockItemGripClassPrivate GdlDockItemGripClassPrivate;

struct _GdlDockItemGrip {
    GtkWidget parent;

    GdlDockItemGripPrivate *priv;
};

struct _GdlDockItemGripClass {
    GtkWidgetClass parent_class;
    GdlDockItemGripClassPrivate *priv;
};

GType      gdl_dock_item_grip_get_type    (void);
GtkWidget *gdl_dock_item_grip_new         (GdlDockItem *item);
void       gdl_dock_item_grip_set_label   (GdlDockItemGrip *grip,
                                           GtkWidget *label);
void       gdl_dock_item_grip_hide_handle (GdlDockItemGrip *grip);
void       gdl_dock_item_grip_show_handle (GdlDockItemGrip *grip);

void       gdl_dock_item_grip_set_cursor  (GdlDockItemGrip *grip,
                                           gboolean in_drag);

gboolean   gdl_dock_item_grip_has_event   (GdlDockItemGrip *grip,
                                           GdkEvent *event);
void       gdl_dock_item_grip_remove      (GtkWidget *container,
                                           GtkWidget *widget);

G_END_DECLS
