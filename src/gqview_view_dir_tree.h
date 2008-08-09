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

#ifndef VIEW_DIR_TREE_H
#define VIEW_DIR_TREE_H

#include <gqview2/typedefs.h>

ViewDirTree *vdtree_new(const gchar *path, gint expand);

void vdtree_set_select_func(ViewDirTree *vdt,
			    void (*func)(ViewDirTree *vdt, const gchar *path, gpointer data), gpointer data);

void vdtree_set_layout(ViewDirTree *vdt, LayoutWindow *layout);

gint vdtree_set_path(ViewDirTree *vdt, const gchar *path);
void vdtree_refresh(ViewDirTree *vdt);

const gchar *vdtree_row_get_path(ViewDirTree *vdt, gint row);

void vdtree_on_icon_theme_changed(ViewDirTree *vdt);

void dir_on_select(ViewDirTree *vdt, const gchar *path, gpointer data);

#endif

