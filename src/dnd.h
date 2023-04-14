/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
 | copyright (C) 2007-2023 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#pragma once

#include <gtk/gtk.h>
#include "dnd/dnd.h"

void     dnd_setup     ();
#ifdef GTK4_TODO
gint     drag_received (GtkWidget*, GdkDragContext*, gint x, gint y, GtkSelectionData*, guint info, guint time, gpointer user_data);
gint     drag_motion   (GtkWidget*, GdkDragContext*, gint x, gint y, guint time, gpointer user_data);
#endif
