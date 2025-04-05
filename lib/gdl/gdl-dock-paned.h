/*
 * gdl-dock-paned.h
 *
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

#include <gdl/gdl-dock-item.h>

G_BEGIN_DECLS

#define GDL_TYPE_DOCK_PANED              (gdl_dock_paned_get_type ())
#define GDL_DOCK_PANED(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GDL_TYPE_DOCK_PANED, GdlDockPaned))
#define GDL_DOCK_PANED_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GDL_TYPE_DOCK_PANED, GdlDockPanedClass))
#define GDL_IS_DOCK_PANED(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GDL_TYPE_DOCK_PANED))
#define GDL_IS_DOCK_PANED_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GDL_TYPE_DOCK_PANED))
#define GDL_DOCK_PANED_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GDL_TYE_DOCK_PANED, GdlDockPanedClass))

typedef struct _GdlDockPaned        GdlDockPaned;
typedef struct _GdlDockPanedClass   GdlDockPanedClass;
typedef struct _GdlDockPanedPrivate GdlDockPanedPrivate;

struct _GdlDockPaned {
    GdlDockItem  dock_item;

    GdlDockPanedPrivate *priv;
};

struct _GdlDockPanedClass {
    GdlDockItemClass parent_class;
};

GType      gdl_dock_paned_get_type        (void);

void       gdl_dock_paned_add             (GdlDockPaned* container, GtkWidget*);

G_END_DECLS
