/*
 * GQview
 * (C) 2004 John Ellis
 *
 * Author: John Ellis
 *
 * This software is released under the GNU General Public License (GNU GPL).
 * Please read the included file COPYING for more information.
 * This software comes with no warranty of any kind, use at your own risk!
 */

#pragma once

#include <dir-tree/typedefs.h>

ViewDirTree* vdtree_new                  (const gchar* path, gint expand);
void         vdtree_free                 (ViewDirTree*);
void         vdtree_set_select_func      (ViewDirTree*, void (*func)(ViewDirTree*, const gchar* path, gpointer data), gpointer data);
void         vdtree_set_layout           (ViewDirTree*, LayoutWindow*);
gint         vdtree_set_path             (ViewDirTree*, const gchar *path);
void         vdtree_refresh              (ViewDirTree*);
const gchar* vdtree_row_get_path         (ViewDirTree*, gint row);

const char*  vdtree_get_selected         (ViewDirTree*);

void         vdtree_on_icon_theme_changed(ViewDirTree*);
void         vdtree_add_menu_item        (GMenuModel*, GAction*);
