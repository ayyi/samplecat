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

#include <gtk/gtk.h>

typedef struct _ViewDetailsClass ViewDetailsClass;

/*
 *  ViewDetails inherits from GtkTreeView and implements the GtkTreeModel interface.
 */
typedef struct _ViewDetails {
    GtkTreeView       treeview;
    GtkTreeSelection* selection;
    GtkWidget*        scroll_win;

    AyyiFilemanager*  filer_window;     // Used for styles, etc

    GPtrArray*        items;            // ViewItem

    GtkCellRenderer*  cell_leaf;
	
    int               (*sort_fn)(const void*, const void*);

    int               cursor_base;      // Cursor when minibuffer opened

    int               wink_item;        // -1 => not winking
    gint              wink_timeout;
    int               wink_step;

    int               can_change_selection;

    GtkRequisition    desired_size;

    gboolean          lasso_box;
    int               lasso_start_index;
    int               drag_box_x[2];	// Index 0 is the fixed corner
    int               drag_box_y[2];

    gboolean          use_alt_colours;
#ifdef GTK4_TODO
    GdkColor          alt_bg;
    GdkColor          alt_fg;
#endif
} ViewDetails;


#define VIEW_DETAILS(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), view_details_get_type(), ViewDetails))

GtkWidget* view_details_new      (AyyiFilemanager* filer_window);
GType      view_details_get_type ();

#ifdef GTK4_TODO
void       view_details_dnd_get  (GtkWidget*, GdkDragContext*, GtkSelectionData*, guint info, guint time, gpointer data);

void       view_details_set_alt_colours(ViewDetails*, GdkColor* bg, GdkColor* fg);
#endif
