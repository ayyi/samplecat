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

GtkWidget*   find_widget_by_type_wide  (GtkWidget*, GType);
GtkWidget*   find_widget_by_type_deep  (GtkWidget*, GType);

#ifdef DEBUG
void         print_widget_tree         (GtkWidget*);
#endif
