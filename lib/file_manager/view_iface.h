/**
* +----------------------------------------------------------------------+
* | This file is part of the Ayyi project. http://ayyi.org               |
* | copyright (C) 2011-2017 Tim Orford <tim@orford.org>                  |
* | copyright (C) 2006, Thomas Leonard and others                        |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#ifndef __view_iface_h__
#define __view_iface_h__

#define AUTOSCROLL_STEP 20

#include <glib-object.h>
#include <gdk/gdk.h>
#include "file_manager/typedefs.h"

typedef enum {
	/* iter->next moves to selected items only */
	VIEW_ITER_SELECTED	= 1 << 0,

	/* iteration starts from cursor (first call to next() returns
	 * iter AFTER cursor). If there is no cursor, flag is ignored
	 * (will iterate over everything).
	 */
	VIEW_ITER_FROM_CURSOR	= 1 << 1,

	/* next() moves backwards */
	VIEW_ITER_BACKWARDS	= 1 << 2,

	/* next() always returns NULL and has no effect */
	VIEW_ITER_ONE_ONLY	= 1 << 3,

	/* Like FROM_CURSOR, but using the base position. The base is set
	 * from the cursor position when the path minibuffer is opened.
	 */
	VIEW_ITER_FROM_BASE	= 1 << 4,
} IterFlags;

typedef struct _ViewIfaceClass	ViewIfaceClass;

/* A viewport containing a Collection which also handles redraw.
 * This is the Collection-based implementation of the View interface.
 */
typedef struct _ViewCollection ViewCollection;

struct _ViewIter {
	// Returns the value last returned by next()
    DirItem*   (*peek)(ViewIter *iter);

    DirItem*   (*next)(ViewIter *iter);

    // private fields
    ViewIface* view;
    int        i, n_remaining;
    int        flags;
};

struct _ViewIfaceClass {
    GTypeInterface base_iface;

    void     (*sort)                 (ViewIface*);
    void     (*style_changed)        (ViewIface*, int flags);
    void     (*add_items)            (ViewIface*, GPtrArray*);
    void     (*update_items)         (ViewIface*, GPtrArray*);
    void     (*delete_if)            (ViewIface*, gboolean (*test)(gpointer item, gpointer data), gpointer data);
    void     (*clear)                (ViewIface*);
    void     (*select_all)           (ViewIface*);
    void     (*clear_selection)      (ViewIface*);
    int      (*count_items)          (ViewIface*);
    int      (*count_selected)       (ViewIface*);
    void     (*show_cursor)          (ViewIface*);

    void     (*get_iter)             (ViewIface*, ViewIter*, IterFlags);
    void     (*get_iter_at_point)    (ViewIface*, ViewIter*, GdkWindow* src, int x, int y);
    void     (*cursor_to_iter)       (ViewIface*, ViewIter*);

    void     (*set_selected)         (ViewIface*, ViewIter*, gboolean selected);
    gboolean (*get_selected)         (ViewIface*, ViewIter*);
    void     (*set_frozen)           (ViewIface*, gboolean frozen);
    void     (*select_only)          (ViewIface*, ViewIter*);
    gboolean (*cursor_visible)       (ViewIface*);
    void     (*set_base)             (ViewIface*, ViewIter*);
    void     (*start_lasso_box)      (ViewIface*, GdkEventButton*);
    void     (*extend_tip)           (ViewIface*, ViewIter*, GString* tip);
    gboolean (*auto_scroll_callback) (ViewIface*);
};

#define VIEW_TYPE_IFACE           (view_iface_get_type())
#define VIEW(obj)                 (G_TYPE_CHECK_INSTANCE_CAST((obj), VIEW_TYPE_IFACE, ViewIface))
#define VIEW_IS_IFACE(obj)        (G_TYPE_CHECK_INSTANCE_TYPE((obj), VIEW_TYPE_IFACE))
#define VIEW_IFACE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_INTERFACE((obj), VIEW_TYPE_IFACE, ViewIfaceClass))

// Flags for view_style_changed()
enum {
    VIEW_UPDATE_VIEWDATA = 1 << 0,
    VIEW_UPDATE_NAME     = 1 << 1,
    VIEW_UPDATE_HEADERS  = 1 << 2,
};

GType    view_iface_get_type    (void);
void     view_sort              (ViewIface*);
void     view_style_changed     (ViewIface*, int flags);
gboolean view_autoselect        (ViewIface*, const gchar* leaf);
void     view_add_items         (ViewIface*, GPtrArray* items);
void     view_update_items      (ViewIface*, GPtrArray* items);
void     view_delete_if         (ViewIface*, gboolean (*test)(gpointer item, gpointer data), gpointer data);
void     view_clear             (ViewIface*);
void     view_select_all        (ViewIface*);
void     view_clear_selection   (ViewIface*);
int      view_count_items       (ViewIface*);
int      view_count_selected    (ViewIface*);
void     view_show_cursor       (ViewIface*);

void     view_get_iter          (ViewIface*, ViewIter*, IterFlags flags);
void     view_get_iter_at_point (ViewIface*, ViewIter*, GdkWindow* src, int x, int y);
void     view_get_cursor        (ViewIface*, ViewIter*);
void     view_cursor_to_iter    (ViewIface*, ViewIter*);

void     view_set_selected      (ViewIface*, ViewIter*, gboolean selected);
gboolean view_get_selected      (ViewIface*, ViewIter*);
void     view_select_only       (ViewIface*, ViewIter*);
void     view_freeze            (ViewIface*);
void     view_thaw              (ViewIface*);
void     view_select_if         (ViewIface*, gboolean (*test)(ViewIter*, gpointer data), gpointer data);

gboolean view_cursor_visible    (ViewIface*);
void     view_set_base          (ViewIface*, ViewIter*);
void     view_start_lasso_box   (ViewIface*, GdkEventButton*);
void     view_extend_tip        (ViewIface*, ViewIter*, GString* tip);
gboolean view_auto_scroll_callback(ViewIface*);

#endif
