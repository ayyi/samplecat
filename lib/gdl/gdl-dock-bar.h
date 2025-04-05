/*
 * This file is part of the GNOME Devtools Libraries.
 *
 * Copyright (C) 2003 Jeroen Zwartepoorte <jeroen@xs4all.nl>
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
#include <gdl/gdl-dock.h>

G_BEGIN_DECLS

/* standard macros */
#define GDL_TYPE_DOCK_BAR            (gdl_dock_bar_get_type ())
#define GDL_DOCK_BAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GDL_TYPE_DOCK_BAR, GdlDockBar))
#define GDL_DOCK_BAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GDL_TYPE_DOCK_BAR, GdlDockBarClass))
#define GDL_IS_DOCK_BAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GDL_TYPE_DOCK_BAR))
#define GDL_IS_DOCK_BAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GDL_TYPE_DOCK_BAR))
#define GDL_DOCK_BAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_DOCK_BAR, GdlDockBarClass))

/* data types & structures */
typedef struct _GdlDockBar        GdlDockBar;
typedef struct _GdlDockBarClass   GdlDockBarClass;
typedef struct _GdlDockBarPrivate GdlDockBarPrivate;

/**
 * GdlDockBarStyle:
 * @GDL_DOCK_BAR_ICONS: Buttons display only icons in the dockbar.
 * @GDL_DOCK_BAR_TEXT: Buttons display only text labels in the dockbar.
 * @GDL_DOCK_BAR_BOTH: Buttons display text and icons in the dockbar.
 * @GDL_DOCK_BAR_AUTO: identical to @GDL_DOCK_BAR_BOTH.
 *
 * Used to customize the appearance of a #GdlDockBar.
 *
 **/
typedef enum {
    GDL_DOCK_BAR_ICONS,
    GDL_DOCK_BAR_TEXT,
    GDL_DOCK_BAR_BOTH,
    GDL_DOCK_BAR_AUTO
} GdlDockBarStyle;

struct _GdlDockBar {
    GtkBox parent;

    /*< private >*/
#ifndef GDL_DISABLE_DEPRECATED
    GdlDock *deprecated_dock;
#endif
    GdlDockBarPrivate *priv;
};

struct _GdlDockBarClass {
    GtkBoxClass parent_class;
};

GType           gdl_dock_bar_get_type       (void);

GtkWidget      *gdl_dock_bar_new            (GObject           *master);

void            gdl_dock_bar_set_style      (GdlDockBar        *dockbar,
                                             GdlDockBarStyle    style);
GdlDockBarStyle gdl_dock_bar_get_style      (GdlDockBar        *dockbar);

#ifndef GDL_DISABLE_DEPRECATED
GtkOrientation gdl_dock_bar_get_orientation (GdlDockBar        *dockbar) G_GNUC_DEPRECATED_FOR(gtk_orientable_get_orientation);
void           gdl_dock_bar_set_orientation (GdlDockBar        *dockbar,
                                             GtkOrientation     orientation) G_GNUC_DEPRECATED_FOR(gtk_orientable_set_orientation);
#endif

G_END_DECLS
