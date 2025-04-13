/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. https://ayyi.github.io/samplecat/    |
 | copyright (C) 2007-2025 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#pragma once

#include <gtk/gtk.h>
#include "gdl/gdl-dock-item.h"

GtkWidget*  window_new             (GtkApplication*, gpointer);

GdlDockItem* find_panel            (const char* name);

#if 0
GtkWidget*  message_panel__add_msg (const gchar* msg, const gchar* stock_id);
#endif
