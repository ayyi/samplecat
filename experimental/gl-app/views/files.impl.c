/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2017-2017 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include "debug/debug.h"
#include "file_manager/file_manager.h"
#include "agl/utils.h"
#include "views/files.impl.h"

#define _g_object_unref0(var) ((var == NULL) ? NULL : (var = (g_object_unref (var), NULL)))

#define TYPE_DIRECTORY_VIEW            (directory_view_get_type ())
#define DIRECTORY_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_DIRECTORY_VIEW, DirectoryView))
#define DIRECTORY_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  TYPE_DIRECTORY_VIEW, DirectoryViewClass))
#define IS_DIRECTORY_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_DIRECTORY_VIEW))
#define IS_DIRECTORY_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  TYPE_DIRECTORY_VIEW))
#define DIRECTORY_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  TYPE_DIRECTORY_VIEW, DirectoryViewClass))

typedef struct _DirectoryViewClass DirectoryViewClass;

struct _DirectoryViewClass {
	GObjectClass parent_class;
};

struct _DirectoryViewPrivate {
	gboolean needs_update;
    int      cursor_base;      // Cursor when minibuffer opened
};


static gpointer directory_view_parent_class = NULL;

GType directory_view_get_type (void) G_GNUC_CONST;
#define DIRECTORY_VIEW_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_DIRECTORY_VIEW, DirectoryViewPrivate))
enum  {
	DIRECTORY_VIEW_DUMMY_PROPERTY
};

static void     directory_view_finalize     (GObject*);
static gboolean directory_view_get_selected (ViewIface*, ViewIter*);


static gboolean
is_selected (DirectoryView* dv, int i)
{
	return directory_view_get_selected((ViewIface*)dv, &(ViewIter){.i = i});
}


static gboolean
_is_selected(DirectoryView* view, int i)
{
	return i == view->selection;
}


static DirItem*
iter_next (ViewIter* iter)
{
	DirectoryView* dv = (DirectoryView*)iter->view;
	int n = dv->items->len;
	int i = iter->i;

	g_return_val_if_fail(iter->n_remaining >= 0, NULL);

	// i is the last item returned (always valid)

	g_return_val_if_fail(i >= 0 && i < n, NULL);

	while (iter->n_remaining) {
		i++;
		iter->n_remaining--;

		if (i == n)
			i = 0;

		g_return_val_if_fail(i >= 0 && i < n, NULL);

		if (iter->flags & VIEW_ITER_SELECTED && !is_selected(dv, i))
			continue;

		iter->i = i;
		return ((ViewItem*)dv->items->pdata[i])->item;
	}

	iter->i = -1;
	return NULL;
}


static DirItem*
iter_prev (ViewIter* iter)
{
	DirectoryView* dv = (DirectoryView*)iter->view;
	int n = dv->items->len;
	int i = iter->i;

	g_return_val_if_fail(iter->n_remaining >= 0, NULL);

	// i is the last item returned (always valid)

	g_return_val_if_fail(i >= 0 && i < n, NULL);

	while (iter->n_remaining) {
		i--;
		iter->n_remaining--;

		if (i == -1)
			i = n - 1;

		g_return_val_if_fail(i >= 0 && i < n, NULL);

		if (iter->flags & VIEW_ITER_SELECTED && !is_selected(dv, i))
			continue;

		iter->i = i;
		return ((ViewItem*)dv->items->pdata[i])->item;
	}

	iter->i = -1;
	return NULL;
}


static DirItem*
iter_peek (ViewIter* iter)
{
	DirectoryView* dv = (DirectoryView*)iter->view;
	int n = dv->items->len;
	int i = iter->i;

	if (i == -1) return NULL;

	g_return_val_if_fail(i >= 0 && i < n, NULL);

	return ((ViewItem*)dv->items->pdata[i])->item;
}


static DirItem*
iter_init (ViewIter* iter)
{
	DirectoryView* dv = (DirectoryView*)iter->view;
	int n = dv->items->len;
	int flags = iter->flags;

	iter->peek = iter_peek;

	if (iter->n_remaining == 0) return NULL;

	int i = -1;
	if (flags & VIEW_ITER_FROM_CURSOR) {
		GtkTreePath* path;
		gtk_tree_view_get_cursor((GtkTreeView*)dv, &path, NULL);
		if (!path) return NULL;	// no cursor
		i = gtk_tree_path_get_indices(path)[0];
		gtk_tree_path_free(path);
	}
	else if (flags & VIEW_ITER_FROM_BASE) i = dv->priv->cursor_base;

	if (i < 0 || i >= n)
	{
		/* Either a normal iteration, or an iteration from an
		 * invalid starting point.
		 */
		if (flags & VIEW_ITER_BACKWARDS)
			i = n - 1;
		else
			i = 0;
	}

	if (i < 0 || i >= n) return NULL;	// no items at all!

	iter->next = flags & VIEW_ITER_BACKWARDS ? iter_prev : iter_next;
	iter->n_remaining--;
	iter->i = i;

	if (flags & VIEW_ITER_SELECTED && !is_selected(dv, i)) return iter->next(iter);
	return iter->peek(iter);
}


static void
make_iter (DirectoryView* dv, ViewIter* iter, IterFlags flags)
{
	iter->view = (ViewIface*)dv;
	iter->next = iter_init;
	iter->peek = NULL;
	iter->i = -1;
	iter->flags = flags;

	if (flags & VIEW_ITER_ONE_ONLY) {
		iter->n_remaining = 1;
		iter->next(iter);
	} else {
		iter->n_remaining = dv->items->len;
	}
}


/*
 *   Set the iterator to return 'i' on the next peek().
 *   If i is -1, returns NULL on next peek().
 */
static void
make_item_iter (DirectoryView* dv, ViewIter* iter, int i)
{
	make_iter(dv, iter, 0);

	g_return_if_fail(i >= -1 && i < (int)dv->items->len);

	iter->i = i;
	iter->next = iter_next;
	iter->peek = iter_peek;
	iter->n_remaining = 0;
}


static void
free_view_item (ViewItem *view_item)
{
	if (view_item->image) g_object_unref(G_OBJECT(view_item->image));
	g_free(view_item->utf8_name);
	g_free(view_item);
}


// ------------------------ start iface implementation -------------------

static void
directory_view_sort (ViewIface* view)
{
	//resort((DirectoryView *)view);
}


static void
directory_view_style_changed(ViewIface *view, int flags)
{
	DirectoryView* dv = (DirectoryView*)view;
	ViewItem** items = (ViewItem**)dv->items->pdata;
	int n = dv->items->len;

	int i; for (i = 0; i < n; i++) {
		ViewItem* item = items[i];
		_g_object_unref0(item->image);
	}

	agl_actor__invalidate((AGlActor*)dv->view);

	//if (flags & VIEW_UPDATE_HEADERS) details_update_header_visibility(dv);
}


static void
directory_view_add_items (ViewIface* _view, GPtrArray* new_items)
{
	GtkTreePath* details_get_path (GtkTreeModel* tree_model, GtkTreeIter* iter)
	{
		GtkTreePath* retval = gtk_tree_path_new();
		gtk_tree_path_append_index(retval, GPOINTER_TO_INT(iter->user_data));

		return retval;
	}

	DirectoryView* view = (DirectoryView*)_view;
	GPtrArray* items = view->items;
	GtkTreeModel* model = (GtkTreeModel*)view->model;

	//find the last row in the tree model
	GtkTreeIter iter = {.user_data = GINT_TO_POINTER(items->len)};

	GtkTreePath* path = details_get_path(model, &iter);

	int i; for (i = 0; i < new_items->len; i++) {
		DirItem* item = (DirItem*)new_items->pdata[i];
		char* leafname = item->leafname;

		if(!vm_directory_match_filter(view->model, item)) continue;
		if (leafname[0] == '.') {
			if (leafname[1] == '\0')
				continue; // never show '.'

			if (leafname[1] == '.' && leafname[2] == '\0')
				continue; // never show '..'
		}

		ViewItem* vitem = AGL_NEW(ViewItem,
			.item = item,
			.utf8_name = g_utf8_validate(leafname, -1, NULL)
				? NULL
				: to_utf8(leafname)
		);
		dbg(2, "leaf=%20s owner=%3i size=%i", leafname, item->uid, item->size);

		g_ptr_array_add(items, vitem); // !!! model just points to the garray entry???? !!!

		iter.user_data = GINT_TO_POINTER(items->len - 1);
		gtk_tree_model_row_inserted(model, path, &iter); // emit "row-inserted"
		gtk_tree_path_next(path);
	}

	gtk_tree_path_free(path);

//	resort(dv);
}


static void
directory_view_update_items (ViewIface* view, GPtrArray* items)
{
#if 0
	DirectoryView	*dv = (DirectoryView *) view;
	GtkTreeModel *model = (GtkTreeModel *) dv;

	g_return_if_fail(items->len > 0);

	/* The item data has already been modified, so this gives the
	 * final sort order...
	 */
	resort(dv);

	int i;
	for (i = 0; i < items->len; i++)
	{
		DirItem *item = (DirItem *) items->pdata[i];
		const gchar *leafname = item->leafname;

		if (!fm__match_filter(dv->filer_window2, item)) continue;

		int j = details_find_item(dv, item);

		if (j < 0) g_warning("Failed to find '%s'\n", leafname);
		else
		{
			GtkTreeIter iter;
			ViewItem *view_item = dv->items->pdata[j];
			if (view_item->image)
			{
				g_object_unref(G_OBJECT(view_item->image));
				view_item->image = NULL;
			}
			GtkTreePath *path = gtk_tree_path_new();
			gtk_tree_path_append_index(path, j);
			iter.user_data = GINT_TO_POINTER(j);
			gtk_tree_model_row_changed(model, path, &iter);
		}
	}
#endif
}


static void
directory_view_delete_if (ViewIface* view, gboolean (*test)(gpointer item, gpointer data), gpointer data)
{
	DirectoryView* dv = (DirectoryView*)view;
	GPtrArray* items = dv->items;

	int i = 0;
	while (i < items->len) {
		ViewItem* item = items->pdata[i];

		if (test(item->item, data)) {
			free_view_item(items->pdata[i]);
			g_ptr_array_remove_index(items, i);
			agl_actor__invalidate((AGlActor*)dv->view);
		} else {
			i++;
		}
	}
}


static void
directory_view_clear (ViewIface* view)
{
	GPtrArray* items = ((DirectoryView*)view)->items;
	g_ptr_array_set_size(items, 0);
	agl_actor__invalidate((AGlActor*)((DirectoryView*)view)->view);
}


static void
directory_view_select_all (ViewIface* view)
{
#if 0
	DirectoryView *dv = (DirectoryView *) view;

	dv->can_change_selection++;
	gtk_tree_selection_select_all(dv->selection);
	dv->can_change_selection--;
#endif
}


static void
directory_view_clear_selection (ViewIface* view)
{
#if 0
	DirectoryView *dv = (DirectoryView *) view;

	dv->can_change_selection++;
	gtk_tree_selection_unselect_all(dv->selection);
	dv->can_change_selection--;
#endif
}


static int
directory_view_count_items (ViewIface* view)
{
	return ((DirectoryView*)view)->items->len;
}


static int
directory_view_count_selected(ViewIface* view)
{
	return 1;
}


static void
directory_view_show_cursor(ViewIface* view)
{
}


static void
directory_view_get_iter(ViewIface* view, ViewIter* iter, IterFlags flags)
{
	make_iter((DirectoryView*)view, iter, flags);
}


static void
directory_view_get_iter_at_point(ViewIface* view, ViewIter* iter, GdkWindow* src, int x, int y)
{
	DirectoryView* dv = (DirectoryView*)view;

	int i = files_view_row_at_coord(dv->view, x, y);

	make_item_iter(dv, iter, i);
}


static void
directory_view_cursor_to_iter(ViewIface *view, ViewIter *iter)
{
#if 0
	DirectoryView *dv = (DirectoryView *) view;
	if(!GTK_WIDGET_REALIZED(view)) return;

	GtkTreePath *path = gtk_tree_path_new();

	if (iter)
		gtk_tree_path_append_index(path, iter->i);
	else
	{
		/* Using depth zero or index -1 gives an error, but this
		 * is OK!
		 */
		gtk_tree_path_append_index(path, dv->items->len);
	}

	if(GTK_WIDGET_REALIZED((GtkWidget*)view)){
		gtk_tree_view_set_cursor((GtkTreeView *) view, path, NULL, FALSE);
	}
	gtk_tree_path_free(path);
#endif
}


static void
directory_view_set_selected (ViewIface* view, ViewIter* iter, gboolean selected)
{
	void set_selected(DirectoryView* dv, int i, gboolean selected)
	{
		if (selected){
			dv->selection = i;
		}

		agl_actor__invalidate((AGlActor*)dv->view);
	}

	set_selected((DirectoryView*)view, iter->i, selected);
}


static gboolean
directory_view_get_selected (ViewIface* view, ViewIter* iter)
{
	return _is_selected((DirectoryView*)view, iter->i);
}


static void
directory_view_select_only (ViewIface* view, ViewIter* iter)
{
	DirectoryView* dv = (DirectoryView *) view;

	dv->selection = iter->i;
}


static void
directory_view_set_frozen (ViewIface* view, gboolean frozen)
{
}


static gboolean
directory_view_cursor_visible(ViewIface* view)
{
	return TRUE;
}


static void
directory_view_set_base(ViewIface* view, ViewIter* iter)
{
	DirectoryView *dv = (DirectoryView *) view;

	dv->priv->cursor_base = iter->i;
}


static void
directory_view_start_lasso_box(ViewIface *view, GdkEventButton *event)
{
#if 0
	DirectoryView *dv = (DirectoryView *) view;
	GtkAdjustment *adj;

	adj = gtk_tree_view_get_vadjustment((GtkTreeView *) dv);

	dv->lasso_start_index = get_lasso_index(dv, event->y);

	//filer_set_autoscroll(dv->filer_window, TRUE);

	dv->drag_box_x[0] = dv->drag_box_x[1] = event->x;
	dv->drag_box_y[0] = dv->drag_box_y[1] = event->y + adj->value;
	dv->lasso_box = TRUE;
#endif
}


static void
directory_view_extend_tip(ViewIface *view, ViewIter *iter, GString *tip)
{
}


static gboolean
directory_view_auto_scroll_callback (ViewIface* view)
{
#if 0
	DirectoryView* dv = (DirectoryView*)view;
	GtkTreeView	*tree = (GtkTreeView *) view;
	FilerWindow	*filer_window = dv->filer_window;
	GtkRange	*scrollbar = (GtkRange *) filer_window->scrollbar;
	GdkWindow	*window;
	gint		x, y, w, h;
	GdkModifierType	mask;
	int		diff = 0;

	window = gtk_tree_view_get_bin_window(tree);

	gdk_window_get_pointer(window, &x, &y, &mask);
	gdk_drawable_get_size(window, &w, NULL);

	GtkAdjustment* adj = gtk_range_get_adjustment(scrollbar);
	h = adj->page_size;

	if ((x < 0 || x > w || y < 0 || y > h) && !dv->lasso_box)
		return FALSE;		/* Out of window - stop */

	if (y < AUTOSCROLL_STEP)
		diff = y - AUTOSCROLL_STEP;
	else if (y > h - AUTOSCROLL_STEP)
		diff = AUTOSCROLL_STEP + y - h;

	if (diff)
	{
		int	old = adj->value;
		int	value = old + diff;

		value = CLAMP(value, 0, adj->upper - adj->page_size);
		gtk_adjustment_set_value(adj, value);

		if (adj->value != old)
			dnd_spring_abort();
	}
#endif
	return TRUE;
}


// ------------------------ end iface implementation -------------------


DirectoryView*
directory_view_construct (GType object_type)
{
	DirectoryView* self = (DirectoryView*)g_object_new(object_type, NULL);
	return self;
}


DirectoryView*
directory_view_new (VMDirectory* model, FilesView* view)
{
	DirectoryView* dv = directory_view_construct (TYPE_DIRECTORY_VIEW);
	dv->model = model;
	dv->view = view;
	return dv;
}


static void
directory_view_class_init (DirectoryViewClass * klass)
{
	directory_view_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (DirectoryViewPrivate));
	G_OBJECT_CLASS (klass)->finalize = directory_view_finalize;
}


static void
directory_view_interface_init (gpointer giface, gpointer iface_data)
{
	ViewIfaceClass* iface = giface;

	g_assert(G_TYPE_FROM_INTERFACE(iface) == VIEW_TYPE_IFACE);

	iface->sort              = directory_view_sort;
	iface->style_changed     = directory_view_style_changed;
	iface->add_items         = directory_view_add_items;
	iface->update_items      = directory_view_update_items;
	iface->delete_if         = directory_view_delete_if;
	iface->clear             = directory_view_clear;
	iface->select_all        = directory_view_select_all;
	iface->clear_selection   = directory_view_clear_selection;
	iface->count_items       = directory_view_count_items;
	iface->count_selected    = directory_view_count_selected;
	iface->show_cursor       = directory_view_show_cursor;
	iface->get_iter          = directory_view_get_iter;
	iface->get_iter_at_point = directory_view_get_iter_at_point;
	iface->cursor_to_iter    = directory_view_cursor_to_iter;
	iface->set_selected      = directory_view_set_selected;
	iface->get_selected      = directory_view_get_selected;
	iface->set_frozen        = directory_view_set_frozen;
	iface->select_only       = directory_view_select_only;
	iface->cursor_visible    = directory_view_cursor_visible;
	iface->set_base          = directory_view_set_base;
	iface->start_lasso_box   = directory_view_start_lasso_box;
	iface->extend_tip        = directory_view_extend_tip;
	iface->auto_scroll_callback = directory_view_auto_scroll_callback;
}


static void
directory_view_instance_init (DirectoryView* self)
{
	self->priv = DIRECTORY_VIEW_GET_PRIVATE (self);
	self->items = g_ptr_array_new();
}


static void
directory_view_finalize (GObject* obj)
{
	G_OBJECT_CLASS (directory_view_parent_class)->finalize (obj);
}


GType
directory_view_get_type ()
{
	static volatile gsize directory_type_id__volatile = 0;
	if (g_once_init_enter (&directory_type_id__volatile)) {
		static const GTypeInfo g_define_type_info = { sizeof(DirectoryViewClass), (GBaseInitFunc)NULL, (GBaseFinalizeFunc)NULL, (GClassInitFunc)directory_view_class_init, (GClassFinalizeFunc)NULL, NULL, sizeof(DirectoryView), 0, (GInstanceInitFunc)directory_view_instance_init, NULL};
		static const GInterfaceInfo view_iface_info = { directory_view_interface_init, (GInterfaceFinalizeFunc)NULL, NULL};
		GType directory_view_type_id = g_type_register_static (G_TYPE_OBJECT, "DirectoryView", &g_define_type_info, 0);
		g_type_add_interface_static (directory_view_type_id, VIEW_TYPE_IFACE, &view_iface_info);
		g_once_init_leave (&directory_type_id__volatile, directory_view_type_id);
	}
	return directory_type_id__volatile;
}

