/* GTK - The GIMP Toolkit
 * Copyright © 2014 Red Hat, Inc.
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

#include <gtk/gtkpopover.h>

G_BEGIN_DECLS

#define AYYI_TYPE_POPOVER_MENU   (ayyi_popover_menu_get_type ())
#define AYYI_POPOVER_MENU(o)     (G_TYPE_CHECK_INSTANCE_CAST ((o), AYYI_TYPE_POPOVER_MENU, AyyiPopoverMenu))
#define AYYI_IS_POPOVER_MENU(o)  (G_TYPE_CHECK_INSTANCE_TYPE ((o), AYYI_TYPE_POPOVER_MENU))

typedef struct _AyyiPopoverMenu AyyiPopoverMenu;

struct _AyyiPopoverMenu
{
  GtkPopover parent_instance;

  GtkWidget*          active_item;
  GtkWidget*          open_submenu;
  GtkWidget*          parent_menu;
  GMenuModel*         model;
  GtkPopoverMenuFlags flags;
};

GType               ayyi_popover_menu_get_type       (void) G_GNUC_CONST;

GtkWidget*          ayyi_popover_menu_new_from_model (GMenuModel*);

GtkWidget*          ayyi_popover_menu_new_from_model_full (GMenuModel*, GtkPopoverMenuFlags);

void                ayyi_popover_menu_set_menu_model (AyyiPopoverMenu *popover, GMenuModel*);
GMenuModel*         ayyi_popover_menu_get_menu_model (AyyiPopoverMenu *popover);

void                ayyi_popover_menu_set_flags      (AyyiPopoverMenu*, GtkPopoverMenuFlags);
GtkPopoverMenuFlags ayyi_popover_menu_get_flags      (AyyiPopoverMenu*);

gboolean            ayyi_popover_menu_add_child      (AyyiPopoverMenu*, GtkWidget* child, const char* id);

gboolean            ayyi_popover_menu_remove_child   (AyyiPopoverMenu*, GtkWidget* child);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(AyyiPopoverMenu, g_object_unref)

G_END_DECLS
