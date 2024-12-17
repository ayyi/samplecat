/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. https://ayyi.github.io/samplecat/    |
 | copyright (C) 2024-2024 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define TYPE_DOCK_LAYOUT       (dock_layout_get_type ())
#define TYPE_DOCK_LAYOUT_CHILD (dock_layout_child_get_type ())

/* DockLayout */

G_DECLARE_FINAL_TYPE (DockLayout, dock_layout, , DOCK_LAYOUT, GtkLayoutManager)

/* DockLayoutChild */

G_DECLARE_FINAL_TYPE (DockLayoutChild, dock_layout_child, , DOCK_LAYOUT_CHILD, GtkLayoutChild)

void        dock_layout_child_set_measure      (DockLayoutChild*, gboolean measure);
gboolean    dock_layout_child_get_measure      (DockLayoutChild*);
void        dock_layout_child_set_clip_overlay (DockLayoutChild*, gboolean clip_overlay);
gboolean    dock_layout_child_get_clip_overlay (DockLayoutChild*);

G_END_DECLS
