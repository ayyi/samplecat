/* This file is part of the GNOME Devtools Libraries.
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

#include <glib-object.h>
#include <gdl/master.h>
#include <gdl/gdl-dock.h>

G_BEGIN_DECLS

#define GDL_DOCK_YAML
#define GDL_DOCK_XML_FALLBACK

/* standard macros */
#define GDL_TYPE_DOCK_LAYOUT              (gdl_dock_layout_get_type ())
#define GDL_DOCK_LAYOUT(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GDL_TYPE_DOCK_LAYOUT, GdlDockLayout))
#define GDL_DOCK_LAYOUT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GDL_TYPE_DOCK_LAYOUT, GdlDockLayoutClass))
#define GDL_IS_DOCK_LAYOUT(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GDL_TYPE_DOCK_LAYOUT))
#define GDL_IS_DOCK_LAYOUT_CLASS(klass)	  (G_TYPE_CHECK_CLASS_TYPE ((klass), GDL_TYPE_DOCK_LAYOUT))
#define GDL_DOCK_LAYOUT_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), GDL_TYPE_DOCK_LAYOUT, GdlDockLayoutClass))

/* data types & structures */
typedef struct _GdlDockLayout GdlDockLayout;
typedef struct _GdlDockLayoutPrivate GdlDockLayoutPrivate;
typedef struct _GdlDockLayoutClass GdlDockLayoutClass;

#define N_LAYOUT_DIRS 3

/**
 * GdlDockLayout:
 *
 * The GdlDockLayout struct contains only private fields
 * and should not be directly accessed.
 */
struct _GdlDockLayout {
    GObject               g_object;

    const char*           dirs[N_LAYOUT_DIRS];
    /*< private >*/
#ifndef GDL_DISABLE_DEPRECATED
    gboolean              deprecated_dirty;
    GdlDockMaster        *deprecated_master;
#endif
    GdlDockLayoutPrivate *priv;
};

struct _GdlDockLayoutClass {
    GObjectClass  g_object_class;
};


/* public interface */

GType            gdl_dock_layout_get_type       (void);

GdlDockLayout   *gdl_dock_layout_new            (GObject       *master);

void             gdl_dock_layout_set_master     (GdlDockLayout *layout,
                                                 GObject       *master);
GObject         *gdl_dock_layout_get_master     (GdlDockLayout *layout);

gboolean         gdl_dock_layout_load_layout    (GdlDockLayout *layout,
                                                 const gchar   *name);

void             gdl_dock_layout_save_layout_xml(GdlDockLayout *layout,
                                                 const gchar   *name);

void             gdl_dock_layout_delete_layout  (GdlDockLayout *layout,
                                                 const gchar   *name);

GList           *gdl_dock_layout_get_xml_layouts(GdlDockLayout *layout,
                                                 gboolean       include_default);
void             gdl_dock_layout_get_yaml_layouts(GdlDockLayout *layout,
                                                  void (*foreach)(const char*, gpointer), gpointer user_data);

gboolean         gdl_dock_layout_load_from_xml_file (GdlDockLayout *layout,
                                                 const gchar   *filename);
bool             gdl_dock_layout_load_from_file (GdlDockLayout*, const gchar*);
gboolean         gdl_dock_layout_load_from_string (GdlDockLayout *layout,
                                                 const gchar   *str);

gboolean         gdl_dock_layout_save_to_file   (GdlDockLayout *layout,
                                                 const gchar   *filename);

gboolean         gdl_dock_layout_is_dirty       (GdlDockLayout *layout);

#ifndef GDL_DISABLE_DEPRECATED
void             gdl_dock_layout_attach         (GdlDockLayout *layout,
                                                 GdlDockMaster *master);
#endif

G_END_DECLS
