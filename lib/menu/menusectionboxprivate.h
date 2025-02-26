/*
 * Copyright © 2014 Codethink Limited
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the licence, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Ryan Lortie <desrt@desrt.ca>
 */

#pragma once

#include "popovermenu.h"

G_BEGIN_DECLS

#define AYYI_TYPE_MENU_SECTION_BOX             (ayyi_menu_section_box_get_type ())
#define AYYI_MENU_SECTION_BOX(inst)            (G_TYPE_CHECK_INSTANCE_CAST ((inst), AYYI_TYPE_MENU_SECTION_BOX, AyyiMenuSectionBox))
#define AYYI_MENU_SECTION_BOX_CLASS(class)     (G_TYPE_CHECK_CLASS_CAST ((class), AYYI_TYPE_MENU_SECTION_BOX, AyyiMenuSectionBoxClass))
#define AYYI_IS_MENU_SECTION_BOX(inst)         (G_TYPE_CHECK_INSTANCE_TYPE ((inst), AYYI_TYPE_MENU_SECTION_BOX))
#define AYYI_IS_MENU_SECTION_BOX_CLASS(class)  (G_TYPE_CHECK_CLASS_TYPE ((class), AYYI_TYPE_MENU_SECTION_BOX))
#define AYYI_MENU_SECTION_BOX_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), AYYI_TYPE_MENU_SECTION_BOX, AyyiMenuSectionBoxClass))

typedef struct _AyyiMenuSectionBox AyyiMenuSectionBox;

GType     ayyi_menu_section_box_get_type       (void) G_GNUC_CONST;
void      ayyi_menu_section_box_new_toplevel   (AyyiPopoverMenu*, GMenuModel*, GtkPopoverMenuFlags);

gboolean  ayyi_menu_section_box_add_custom     (AyyiPopoverMenu*, GtkWidget* child, const char* id);
gboolean  ayyi_menu_section_box_remove_custom  (AyyiPopoverMenu*, GtkWidget* child);

G_END_DECLS

