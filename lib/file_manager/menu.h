/*
 +----------------------------------------------------------------------+
 | This file is part of the Ayyi project. http://ayyi.org               |
 | copyright (C) 2011-2023 Tim Orford <tim@orford.org>                  |
 | copyright (C) 2006, Thomas Leonard and others                        |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#pragma once

GtkWidget*      fm__make_context_menu   ();
#ifdef GTK4_TODO
void            fm__add_menu_item       (GtkAction*);
#endif
void            fm__add_submenu         (GtkWidget*);

// private
void            fm__menu_on_view_change (GtkWidget*);
