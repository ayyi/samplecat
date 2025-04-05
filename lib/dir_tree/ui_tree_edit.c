/*
 * (SLIK) SimpLIstic sKin functions
 * (C) 2004 John Ellis
 *
 * Author: John Ellis
 *
 * This software is released under the GNU General Public License (GNU GPL).
 * Please read the included file COPYING for more information.
 * This software comes with no warranty of any kind, use at your own risk!
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "ui_tree_edit.h"

/*
 *-------------------------------------------------------------------
 * cell popup editor
 *-------------------------------------------------------------------
 */

#ifdef GTK4_TODO
static void tree_edit_close(TreeEditData *ted)
{
	gtk_grab_remove(ted->window);
	gdk_keyboard_ungrab(GDK_CURRENT_TIME);
	gdk_pointer_ungrab(GDK_CURRENT_TIME);

	gtk_widget_destroy(ted->window);

	g_free(ted->old_name);
	g_free(ted->new_name);
	gtk_tree_path_free(ted->path);

	g_free(ted);
}

static void tree_edit_do(TreeEditData *ted)
{
	ted->new_name = g_strdup(gtk_entry_get_text(GTK_ENTRY(ted->entry)));

	if (strcmp(ted->new_name, ted->old_name) != 0)
		{
		if (ted->edit_func)
			{
			if (ted->edit_func(ted, ted->old_name, ted->new_name, ted->edit_data))
				{
				/* hmm, should the caller be required to set text instead ? */
				}
			}
		}
}

static gint tree_edit_click_end_cb(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	TreeEditData *ted = data;

	tree_edit_do(ted);
	tree_edit_close(ted);

	return TRUE;
}

static gint tree_edit_click_cb(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	TreeEditData *ted = data;

	gint x, y;
	gint w, h;

	gint xr, yr;

	xr = (gint)event->x_root;
	yr = (gint)event->y_root;

	gdk_window_get_origin(ted->window->window, &x, &y);
	gdk_drawable_get_size(ted->window->window, &w, &h);

	if (xr < x || yr < y || xr > x + w || yr > y + h)
		{
		/* gobble the release event, so it does not propgate to an underlying widget */
		g_signal_connect(G_OBJECT(ted->window), "button_release_event",
				 G_CALLBACK(tree_edit_click_end_cb), ted);
		return TRUE;
		}
	return FALSE;
}

static gint tree_edit_key_press_cb(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	TreeEditData *ted = data;

	switch (event->keyval)
		{
		case GDK_Return:
		case GDK_KP_Enter:
		case GDK_Tab: 		/* ok, we are going to intercept the focus change
					   from keyboard and act like return was hit */
		case GDK_ISO_Left_Tab:
		case GDK_Up:
		case GDK_Down:
		case GDK_KP_Up:
		case GDK_KP_Down:
		case GDK_KP_Left:
		case GDK_KP_Right:
			tree_edit_do(ted);
			tree_edit_close(ted);
			break;
		case GDK_Escape:
			tree_edit_close(ted);
			break;
		default:
			break;
		}

	return FALSE;
}

static gboolean tree_edit_by_path_idle_cb(gpointer data)
{
	TreeEditData *ted = data;
	GdkRectangle rect;
	gint x, y, w, h;	/* geometry of cell within tree */
	gint wx, wy;		/* geometry of tree from root window */
	gint sx, sw;

	gtk_tree_view_get_cell_area(ted->tree, ted->path, ted->column, &rect);

	x = rect.x;
	y = rect.y;
	w = rect.width + 4;
	h = rect.height + 4;

	if (gtk_tree_view_column_cell_get_position(ted->column, ted->cell, &sx, &sw))
		{
		x += sx;
		w = MAX(w - sx, sw);
		}

	gdk_window_get_origin(gtk_tree_view_get_bin_window(ted->tree), &wx, &wy);

	x += wx - 2; /* the -val is to 'fix' alignment of entry position */
	y += wy - 2;

	/* now show it */
	gtk_widget_set_size_request(ted->window, w, h);
	gtk_widget_realize(ted->window);
	gtk_window_move(GTK_WINDOW(ted->window), x, y);
	gtk_window_resize(GTK_WINDOW(ted->window), w, h);
	gtk_widget_show(ted->window);

	/* grab it */
	gtk_widget_grab_focus(ted->entry);
	/* explicitely set the focus flag for the entry, for some reason on popup windows this
	 * is not set, and causes no edit cursor to appear ( popups not allowed focus? )
	 */
	GTK_WIDGET_SET_FLAGS(ted->entry, GTK_HAS_FOCUS);
	gtk_grab_add(ted->window);
	gdk_pointer_grab(ted->window->window, TRUE,
			 GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_BUTTON_MOTION_MASK,
			 NULL, NULL, GDK_CURRENT_TIME);
	gdk_keyboard_grab(ted->window->window, TRUE, GDK_CURRENT_TIME);

	return FALSE;
}

gint tree_edit_by_path(GtkTreeView *tree, GtkTreePath *tpath, gint column, const gchar *text,
		       gint (*edit_func)(TreeEditData *, const gchar *, const gchar *, gpointer), gpointer data)
{
	TreeEditData *ted;
	GtkTreeViewColumn *tcolumn;
	GtkCellRenderer *cell = NULL;
	GList *list;
	GList *work;

	if (!edit_func) return FALSE;
	if (!GTK_WIDGET_VISIBLE(tree)) return FALSE;

	tcolumn = gtk_tree_view_get_column(tree, column);
	if (!tcolumn) return FALSE;

	list = gtk_tree_view_column_get_cell_renderers(tcolumn);
	work = list;
	while (work && !cell)
		{
		cell = work->data;
		if (!GTK_IS_CELL_RENDERER_TEXT(cell))
			{
			cell = NULL;
			}
		work = work->next;
		}

	g_list_free(list);
	if (!cell) return FALSE;

	if (!text) text = "";

	ted = g_new0(TreeEditData, 1);

	ted->old_name = g_strdup(text);
	ted->new_name = NULL;

	ted->edit_func = edit_func;
	ted->edit_data = data;

	ted->tree = tree;
	ted->path = gtk_tree_path_copy(tpath);
	ted->column = tcolumn;
	ted->cell = cell;

	gtk_tree_view_scroll_to_cell(ted->tree, ted->path, ted->column, FALSE, 0.0, 0.0);

	/* create the window */

	ted->window = gtk_window_new(GTK_WINDOW_POPUP);
	gtk_window_set_resizable(GTK_WINDOW(ted->window), FALSE);
	g_signal_connect(G_OBJECT(ted->window), "button_press_event",
			 G_CALLBACK(tree_edit_click_cb), ted);
	g_signal_connect(G_OBJECT(ted->window), "key_press_event",
			 G_CALLBACK(tree_edit_key_press_cb), ted);

	ted->entry = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(ted->entry), ted->old_name);
	gtk_editable_select_region(GTK_EDITABLE(ted->entry), 0, strlen(ted->old_name));
	gtk_container_add(GTK_CONTAINER(ted->window), ted->entry);
	gtk_widget_show(ted->entry);

	/* due to the fact that gtktreeview scrolls in an idle loop, we cannot
	 * reliably get the cell position until those scroll priority signals are processed
	 */
	g_idle_add_full(G_PRIORITY_DEFAULT_IDLE - 2, tree_edit_by_path_idle_cb, ted, NULL);

	return TRUE;
}

/*
 *-------------------------------------------------------------------
 * tree cell position retrieval
 *-------------------------------------------------------------------
 */

gint tree_view_get_cell_origin(GtkTreeView *widget, GtkTreePath *tpath, gint column, gint text_cell_only,
			       gint *x, gint *y, gint *width, gint *height)
{
	gint x_origin, y_origin;
	gint x_offset, y_offset;
	gint header_size;
	GtkTreeViewColumn *tv_column;
	GdkRectangle rect;

	tv_column = gtk_tree_view_get_column(widget, column);
	if (!tv_column || !tpath) return FALSE;

	/* hmm, appears the rect will not account for X scroll, but does for Y scroll
	 * use x_offset instead for X scroll (sigh)
	 */
	gtk_tree_view_get_cell_area(widget, tpath, tv_column, &rect);
	gtk_tree_view_tree_to_widget_coords(widget, 0, 0, &x_offset, &y_offset);
	gdk_window_get_origin(GTK_WIDGET(widget)->window, &x_origin, &y_origin);

	if (gtk_tree_view_get_headers_visible(widget))
		{
		header_size = tv_column->button->allocation.height;
		}
	else
		{
		header_size = 0;
		}

	if (text_cell_only)
		{
		GtkCellRenderer *cell = NULL;
		GList *renderers;
		GList *work;
		gint cell_x;
		gint cell_width;

		renderers = gtk_tree_view_column_get_cell_renderers(tv_column);
		work = renderers;
		while (work && !cell)
			{
			cell = work->data;
			work = work->next;
			if (!GTK_IS_CELL_RENDERER_TEXT(cell)) cell = NULL;
			}
		g_list_free(renderers);

		if (!cell) return FALSE;
		
		if (!gtk_tree_view_column_cell_get_position(tv_column, cell, &cell_x, &cell_width))
			{
			cell_x = 0;
			cell_width = rect.width;
			}
		*x = x_origin + x_offset + rect.x + cell_x;
		*width = cell_width;
		}
	else
		{
		*x = x_origin + x_offset + rect.x;
		*width = rect.width;
		}
	*y = y_origin + rect.y + header_size;
	*height = rect.height;
	return TRUE;
}

void tree_view_get_cell_clamped(GtkTreeView *widget, GtkTreePath *tpath, gint column, gint text_cell_only,
				gint *x, gint *y, gint *width, gint *height)
{
	gint wx, wy, ww, wh;
	GdkWindow *window;

	window = GTK_WIDGET(widget)->window;
	gdk_window_get_origin(window, &wx, &wy);
	gdk_drawable_get_size(window, &ww, &wh);
	if (!tree_view_get_cell_origin(widget, tpath, column, text_cell_only, x,  y, width, height))
		{
		*x = wx;
		*y = wy;
		*width = ww;
		*height = wh;
		return;
		}

	*width = MIN(*width, ww);
	*x = CLAMP(*x, wx, wx + ww - (*width));
	*y = CLAMP(*y, wy, wy + wh);
	*height = MIN(*height, wy + wh - (*y));
}

gint tree_view_row_get_visibility(GtkTreeView *widget, GtkTreeIter *iter, gint fully_visible)
{
	GtkTreeModel *store;
	GtkTreePath *tpath;	
	gint cx, cy;
                                                                                                                               
	GdkRectangle vrect;
	GdkRectangle crect;

	if (!GTK_WIDGET_REALIZED(GTK_WIDGET(widget))) return 0;

	store = gtk_tree_view_get_model(widget);
	tpath = gtk_tree_model_get_path(store, iter);

	gtk_tree_view_get_visible_rect(widget, &vrect);
	gtk_tree_view_get_cell_area(widget, tpath, NULL, &crect);
	gtk_tree_path_free(tpath);

	gtk_tree_view_widget_to_tree_coords(widget, crect.x, crect.y, &cx, &cy);

	if (fully_visible)
		{
		if (cy < vrect.y) return -1;
		if (cy + crect.height > vrect.y + vrect.height) return 1;
		return 0;
		}

	if (cy + crect.height < vrect.y) return -1;
	if (cy > vrect.y + vrect.height) return 1;
	return 0;
}

gint tree_view_row_make_visible(GtkTreeView *widget, GtkTreeIter *iter, gint center)
{
	GtkTreePath *tpath;
	gint vis;

	vis = tree_view_row_get_visibility(widget, iter, TRUE);

	tpath = gtk_tree_model_get_path(gtk_tree_view_get_model(widget), iter);
	if (center && vis != 0)
		{
		gtk_tree_view_scroll_to_cell(widget, tpath, NULL, TRUE, 0.5, 0.0);
		}
	else if (vis < 0)
		{
		gtk_tree_view_scroll_to_cell(widget, tpath, NULL, TRUE, 0.0, 0.0);
		}
	else if (vis > 0)
		{
		gtk_tree_view_scroll_to_cell(widget, tpath, NULL, TRUE, 1.0, 0.0);
		}
	gtk_tree_path_free(tpath);

	return vis;
}

gint tree_view_move_cursor_away(GtkTreeView *widget, GtkTreeIter *iter, gint only_selected)
{
	GtkTreeModel *store;
	GtkTreePath *tpath;
	GtkTreePath *fpath;
	gint move = FALSE;

	if (!iter) return FALSE;

	store = gtk_tree_view_get_model(widget);
	tpath = gtk_tree_model_get_path(store, iter);
	gtk_tree_view_get_cursor(widget, &fpath, NULL);

	if (fpath && gtk_tree_path_compare(tpath, fpath) == 0)
		{
		GtkTreeSelection *selection;

		selection = gtk_tree_view_get_selection(widget);

		if (!only_selected ||
		    gtk_tree_selection_path_is_selected(selection, tpath))
			{
			GtkTreeIter current;

			current = *iter;
			if (gtk_tree_model_iter_next(store, &current))
				{
				gtk_tree_path_next(tpath);
				move = TRUE;
				}
			else if (gtk_tree_path_prev(tpath) &&
				 gtk_tree_model_get_iter(store, &current, tpath))
				{
				move = TRUE;
				}

			if (move)
				{
				gtk_tree_view_set_cursor(widget, tpath, NULL, FALSE);
				}
			}
		}

	gtk_tree_path_free(tpath);
	if (fpath) gtk_tree_path_free(fpath);

	return move;
}

gint tree_path_to_row(GtkTreePath *tpath)
{
	gint *indices;
                                                                                                                    
	indices = gtk_tree_path_get_indices(tpath);
	if (indices) return indices[0];
                                                                                                                    
	return -1;
}


/*
 *-------------------------------------------------------------------
 * color utilities
 *-------------------------------------------------------------------
 */

void shift_color(GdkColor *src, gshort val, gint direction)
{
	gshort cs;

	if (val == -1)
		{
		val = STYLE_SHIFT_STANDARD;
		}
	else
		{
		val = CLAMP(val, 1, 100);
		}
	cs = 0xffff / 100 * val;

	/* up or down ? */
	if (direction < 0 ||
	    (direction == 0 &&((gint)src->red + (gint)src->green + (gint)src->blue) / 3 > 0xffff / 2))
		{
		src->red = MAX(0 , src->red - cs);
		src->green = MAX(0 , src->green - cs);
		src->blue = MAX(0 , src->blue - cs);
		}
	else
		{
		src->red = MIN(0xffff, src->red + cs);
		src->green = MIN(0xffff, src->green + cs);
		src->blue = MIN(0xffff, src->blue + cs);
		}
}

/* darkens or lightens a style's color for given state
 * esp. useful for alternating dark/light in (c)lists
 */
void style_shift_color(GtkStyle *style, GtkStateType type, gshort shift_value, gint direction)
{
	if (!style) return;

	shift_color(&style->base[type], shift_value, direction);
	shift_color(&style->bg[type], shift_value, direction);
}

/*
 *-------------------------------------------------------------------
 * auto scroll by mouse position
 *-------------------------------------------------------------------
 */

#define AUTO_SCROLL_DEFAULT_SPEED 100
#define AUTO_SCROLL_DEFAULT_REGION 20

typedef struct _AutoScrollData AutoScrollData;
struct _AutoScrollData
{
	gint timer_id;
	gint region_size;
	GtkWidget *widget;
	GtkAdjustment *adj;
	gint max_step;

	gint (*notify_func)(GtkWidget *, gint, gint, gpointer);
	gpointer notify_data;
};

void widget_auto_scroll_stop(GtkWidget *widget)
{
	AutoScrollData *sd;

	sd = g_object_get_data(G_OBJECT(widget), "autoscroll");
	if (!sd) return;
	g_object_set_data(G_OBJECT(widget), "autoscroll", NULL);

	if (sd->timer_id != -1) g_source_remove(sd->timer_id);
	g_free(sd);
}

static gint widget_auto_scroll_cb(gpointer data)
{
	AutoScrollData *sd = data;
	GdkWindow *window;
	gint x, y;
	gint w, h;
	gint amt = 0;

	if (sd->max_step < sd->region_size)
		{
		sd->max_step = MIN(sd->region_size, sd->max_step + 2);
		}

	window = sd->widget->window;
	gdk_window_get_pointer(window, &x, &y, NULL);
	gdk_drawable_get_size(window, &w, &h);

	if (x < 0 || x >= w || y < 0 || y >= h)
		{
		sd->timer_id = -1;
		widget_auto_scroll_stop(sd->widget);
		return FALSE;
		}

	if (h < sd->region_size * 3)
		{
		/* height is cramped, nicely divide into three equal regions */
		if (y < h / 3 || y > h / 3 * 2)
			{
			amt = (y < h / 2) ? 0 - ((h / 2) - y) : y - (h / 2);
			}
		}
	else if (y < sd->region_size)
		{
		amt = 0 - (sd->region_size - y);
		}
	else if (y >= h - sd->region_size)
		{
		amt = y - (h - sd->region_size);
		}

	if (amt != 0)
		{
		amt = CLAMP(amt, 0 - sd->max_step, sd->max_step);

		if (sd->adj->value != CLAMP(sd->adj->value + amt, sd->adj->lower, sd->adj->upper - sd->adj->page_size))
			{
			/* only notify when scrolling is needed */
			if (sd->notify_func && !sd->notify_func(sd->widget, x, y, sd->notify_data))
				{
				sd->timer_id = -1;
				widget_auto_scroll_stop(sd->widget);
				return FALSE;
				}

			gtk_adjustment_set_value(sd->adj,
				CLAMP(sd->adj->value + amt, sd->adj->lower, sd->adj->upper - sd->adj->page_size));
			}
		}

	return TRUE;
}

gint widget_auto_scroll_start(GtkWidget *widget, GtkAdjustment *v_adj, gint scroll_speed, gint region_size,
			      gint (*notify_func)(GtkWidget *widget, gint x, gint y, gpointer data), gpointer notify_data)
{
	AutoScrollData *sd;

	if (!widget || !v_adj) return 0;
	if (g_object_get_data(G_OBJECT(widget), "autoscroll")) return 0;
	if (scroll_speed < 1) scroll_speed = AUTO_SCROLL_DEFAULT_SPEED;
	if (region_size < 1) region_size = AUTO_SCROLL_DEFAULT_REGION;

	sd = g_new0(AutoScrollData, 1);
	sd->widget = widget;
	sd->adj = v_adj;
	sd->region_size = region_size;
	sd->max_step = 1;
	sd->timer_id = g_timeout_add(scroll_speed, widget_auto_scroll_cb, sd);

	sd->notify_func = notify_func;
	sd->notify_data = notify_data;

	g_object_set_data(G_OBJECT(widget), "autoscroll", sd);

	return scroll_speed;
}


/*
 *-------------------------------------------------------------------
 * GList utils
 *-------------------------------------------------------------------
 */

GList *uig_list_insert_link(GList *list, GList *link, gpointer data)
{
	GList *new_list;

	if (!list || link == list) return g_list_prepend(list, data);
	if (!link) return g_list_append(list, data);

	new_list = g_list_alloc ();
	new_list->data = data;

	if (link->prev)
		{
		link->prev->next = new_list;
		new_list->prev = link->prev;
		}
	else
		{
		list = new_list;
		}
	link->prev = new_list;
	new_list->next = link;

	return list;
}

GList *uig_list_insert_list(GList *parent, GList *insert_link, GList *list)
{
	GList *end;

	if (!insert_link) return g_list_concat(parent, list);
	if (insert_link == parent) return g_list_concat(list, parent);
	if (!parent) return list;
	if (!list) return parent;

	end  = g_list_last(list);

	if (insert_link->prev) insert_link->prev->next = list;
	list->prev = insert_link->prev;
	insert_link->prev = end;
	end->next = insert_link;

	return parent;
}
#endif
