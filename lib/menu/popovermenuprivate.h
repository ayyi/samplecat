/* GTK - The GIMP Toolkit
 * Copyright © 2019 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "popovermenu.h"

G_BEGIN_DECLS

GtkWidget* ayyi_popover_menu_get_active_item  (AyyiPopoverMenu*);
void       ayyi_popover_menu_set_active_item  (AyyiPopoverMenu*, GtkWidget* item);
void       ayyi_popover_menu_set_open_submenu (AyyiPopoverMenu*, GtkWidget* submenu);
void       ayyi_popover_menu_close_submenus   (AyyiPopoverMenu*);

GtkWidget* ayyi_popover_menu_get_parent_menu  (AyyiPopoverMenu*);
void       ayyi_popover_menu_set_parent_menu  (AyyiPopoverMenu*, GtkWidget* parent);

GtkWidget* ayyi_popover_menu_new              (void);

void       ayyi_popover_menu_add_submenu      (AyyiPopoverMenu*, GtkWidget* submenu, const char* name);
void       ayyi_popover_menu_open_submenu     (AyyiPopoverMenu*, const char* name);

GtkWidget* ayyi_popover_menu_get_stack        (AyyiPopoverMenu*);

G_END_DECLS

