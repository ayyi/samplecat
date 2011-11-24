/*
 * taken form gqview 2.0.1
 *
 * GQview
 * (C) 2004 John Ellis
 *
 * Author: John Ellis
 *
 * This software is released under the GNU General Public License (GNU GPL).
 * Please read the included file COPYING for more information.
 * This software comes with no warranty of any kind, use at your own risk!
 */
#include <gtk/gtk.h>
#include "file_manager/file_manager.h"
#include "file_manager/support.h"

#include "gqview.h"
#include "view_dir_tree.h"

#include "typedefs.h"
#include "support.h"
#include "listmodel.h"

#include "filelist.h"
#include "layout_util.h"
#include "utilops.h"
#include "ui_fileops.h"
#include "ui_menu.h"
#include "ui_tree_edit.h"

#include <gdk/gdkkeysyms.h> /* for keyboard values */

#include "dnd.h"
GdkPixbuf* mime_type_get_pixbuf(MIME_type*);


#define VDTREE_INDENT 14
#define VDTREE_PAD 4

extern MIME_type *inode_directory;

enum {
	DIR_COLUMN_POINTER = 0,
	DIR_COLUMN_ICON,
	DIR_COLUMN_NAME,
	DIR_COLUMN_COLOR,
	DIR_COLUMN_COUNT
};

gint tree_descend_subdirs = FALSE;
gint show_dot_files = FALSE;
gint file_filter_disable = FALSE;


typedef struct _PathData PathData;
struct _PathData
{
	gchar *name;
	FileData *node;
};

typedef struct _NodeData NodeData;
struct _NodeData
{
	FileData *fd;
	gint expanded;
	time_t last_update;
};


static gint vdtree_populate_path_by_iter(ViewDirTree *vdt, GtkTreeIter *iter, gint force, const gchar *target_path);
static FileData *vdtree_populate_path(ViewDirTree *vdt, const gchar *path, gint expand, gint force);

static GList* menu_items = NULL;

/*
 *----------------------------------------------------------------------------
 * utils
 *----------------------------------------------------------------------------
 */

static void set_cursor(GtkWidget *widget, GdkCursorType cursor_type)
{
	GdkCursor *cursor = NULL;

	if (!widget || !widget->window) return;

	if (cursor_type > -1) cursor = gdk_cursor_new (cursor_type);
	gdk_window_set_cursor (widget->window, cursor);
	if (cursor) gdk_cursor_unref(cursor);
	gdk_flush();
}

static void vdtree_busy_push(ViewDirTree *vdt)
{
	if (vdt->busy_ref == 0) set_cursor(vdt->treeview, GDK_WATCH);
	vdt->busy_ref++;
}

static void vdtree_busy_pop(ViewDirTree *vdt)
{
	if (vdt->busy_ref == 1) set_cursor(vdt->treeview, -1);
	if (vdt->busy_ref > 0) vdt->busy_ref--;
}

static gint vdtree_find_row(ViewDirTree *vdt, FileData *fd, GtkTreeIter *iter, GtkTreeIter *parent)
{
	GtkTreeModel *store;
	gint valid;

	store = gtk_tree_view_get_model(GTK_TREE_VIEW(vdt->treeview));
	if (parent)
		{
		valid = gtk_tree_model_iter_children(store, iter, parent);
		}
	else
		{
		valid = gtk_tree_model_get_iter_first(store, iter);
		}
	while (valid)
		{
		NodeData *nd;
		GtkTreeIter found;

		gtk_tree_model_get(GTK_TREE_MODEL(store), iter, DIR_COLUMN_POINTER, &nd, -1);
		if (nd->fd == fd) return TRUE;

		if (vdtree_find_row(vdt, fd, &found, iter))
			{
			memcpy(iter, &found, sizeof(found));
			return TRUE;
			}

		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(store), iter);
		}

	return FALSE;
}

static void
vdtree_icon_set_by_iter(ViewDirTree *vdt, GtkTreeIter *iter, GdkPixbuf *pixbuf)
{
	GtkTreeModel *store;
	GdkPixbuf *old;

	store = gtk_tree_view_get_model(GTK_TREE_VIEW(vdt->treeview));
	gtk_tree_model_get(store, iter, DIR_COLUMN_ICON, &old, -1);
	if (old != vdt->pf->deny)
		{
		gtk_tree_store_set(GTK_TREE_STORE(store), iter, DIR_COLUMN_ICON, pixbuf, -1);
		}
}

static void
vdtree_expand_by_iter(ViewDirTree *vdt, GtkTreeIter *iter, gint expand)
{
	GtkTreeModel *store;
	GtkTreePath *tpath;

	store = gtk_tree_view_get_model(GTK_TREE_VIEW(vdt->treeview));
	tpath = gtk_tree_model_get_path(store, iter);
	if (expand)
		{
		gtk_tree_view_expand_row(GTK_TREE_VIEW(vdt->treeview), tpath, FALSE);
		vdtree_icon_set_by_iter(vdt, iter, vdt->pf->open);
		}
	else
		{
		gtk_tree_view_collapse_row(GTK_TREE_VIEW(vdt->treeview), tpath);
		}
	gtk_tree_path_free(tpath);
}

static void vdtree_expand_by_data(ViewDirTree *vdt, FileData *fd, gint expand)
{
	GtkTreeIter iter;

	if (vdtree_find_row(vdt, fd, &iter, NULL))
		{
		vdtree_expand_by_iter(vdt, &iter, expand);
		}
}

static void vdtree_color_set(ViewDirTree *vdt, FileData *fd, gint color_set)
{
	GtkTreeModel *store;
	GtkTreeIter iter;

	if (!vdtree_find_row(vdt, fd, &iter, NULL)) return;
	store = gtk_tree_view_get_model(GTK_TREE_VIEW(vdt->treeview));
	gtk_tree_store_set(GTK_TREE_STORE(store), &iter, DIR_COLUMN_COLOR, color_set, -1);
}

#ifdef LATER
static gint vdtree_rename_row_cb(TreeEditData *td, const gchar *old, const gchar *new, gpointer data)
{
	ViewDirTree *vdt = data;
	GtkTreeModel *store;
	GtkTreeIter iter;
	NodeData *nd;
	gchar *old_path;
	gchar *new_path;
	gchar *base;

	store = gtk_tree_view_get_model(GTK_TREE_VIEW(vdt->treeview));
	if (!gtk_tree_model_get_iter(store, &iter, td->path)) return FALSE;
	gtk_tree_model_get(store, &iter, DIR_COLUMN_POINTER, &nd, -1);
	if (!nd) return FALSE;

	old_path = g_strdup(nd->fd->path);

	base = remove_level_from_path(old_path);
	new_path = concat_dir_and_file(base, new);
	g_free(base);

	if (!rename_file(old_path, new_path))
		{
		gchar *buf;

		buf = g_strdup_printf(_("Failed to rename %s to %s."), old, new);
		file_util_warning_dialog("Rename failed", buf, GTK_STOCK_DIALOG_ERROR, vdt->treeview);
		g_free(buf);
		}
	else
		{
		vdtree_populate_path(vdt, new_path, TRUE, TRUE);

		if (vdt->layout && strcmp(vdt->path, old_path) == 0)
			{
			layout_set_path(vdt->layout, new_path);
			}
		}

	g_free(old_path);
	g_free(new_path);

	return FALSE;
}

static void vdtree_rename_by_data(ViewDirTree *vdt, FileData *fd)
{
	GtkTreeModel *store;
	GtkTreePath *tpath;
	GtkTreeIter iter;

	if (!fd ||
	    !vdtree_find_row(vdt, fd, &iter, NULL)) return;

	store = gtk_tree_view_get_model(GTK_TREE_VIEW(vdt->treeview));
	tpath = gtk_tree_model_get_path(store, &iter);

	tree_edit_by_path(GTK_TREE_VIEW(vdt->treeview), tpath, 0, fd->name,
			  vdtree_rename_row_cb, vdt);
	gtk_tree_path_free(tpath);
}
#endif

static void vdtree_node_free(NodeData *nd)
{
	if (!nd) return;

	file_data_free(nd->fd);
	g_free(nd);
}

static void vdtree_popup_destroy_cb(GtkWidget *widget, gpointer data)
{
	ViewDirTree *vdt = data;

	vdtree_color_set(vdt, vdt->click_fd, FALSE);
	vdt->click_fd = NULL;
	vdt->popup = NULL;

	vdtree_color_set(vdt, vdt->drop_fd, FALSE);
	path_list_free(vdt->drop_list);
	vdt->drop_list = NULL;
	vdt->drop_fd = NULL;
}

/*
 *-----------------------------------------------------------------------------
 * drop menu (from dnd)
 *-----------------------------------------------------------------------------
 */

static void vdtree_drop_menu_copy_cb(GtkWidget *widget, gpointer data)
{
	ViewDirTree *vdt = data;
	const gchar *path;
	GList *list;

	if (!vdt->drop_fd) return;

	path = vdt->drop_fd->path;
	list = vdt->drop_list;

	vdt->drop_list = NULL;

	dbg(0, "FIXME file_util_copy_simple");
	//file_util_copy_simple(list, path);
}

static void vdtree_drop_menu_move_cb(GtkWidget *widget, gpointer data)
{
	PF;
	ViewDirTree *vdt = data;
	const gchar *path;
	GList *list;

	if (!vdt->drop_fd) return;

	path = vdt->drop_fd->path;
	list = vdt->drop_list;

	vdt->drop_list = NULL;

	listmodel__move_files(list, path);
	file_util_move_simple(list, path);
	file_manager__update_all();
}

static GtkWidget *vdtree_drop_menu(ViewDirTree *vdt, gint active)
{
	GtkWidget *menu;

	menu = popup_menu_short_lived();
	g_signal_connect(G_OBJECT(menu), "destroy", G_CALLBACK(vdtree_popup_destroy_cb), vdt);

	menu_item_add_stock_sensitive(menu, "_Copy", GTK_STOCK_COPY, active, G_CALLBACK(vdtree_drop_menu_copy_cb), vdt);
	menu_item_add_sensitive(menu, "_Move", active, G_CALLBACK(vdtree_drop_menu_move_cb), vdt);

	menu_item_add_divider(menu);
	menu_item_add_stock(menu, "Cancel", GTK_STOCK_CANCEL, NULL, vdt);

	return menu;
}

/*
 *-----------------------------------------------------------------------------
 * pop-up menu
 *-----------------------------------------------------------------------------
 */

#ifdef LATER
static void
vdtree_pop_menu_up_cb(GtkWidget *widget, gpointer data)
{
	ViewDirTree *vdt = data;
	gchar *path;

	if (!vdt->path || strcmp(vdt->path, "/") == 0) return;
	path = remove_level_from_path(vdt->path);

	if (vdt->select_func)
		{
		vdt->select_func(vdt, path, vdt->select_data);
		}

	g_free(path);
}
#endif

#ifdef LATER
static void vdtree_pop_menu_slide_cb(GtkWidget *widget, gpointer data)
{
	ViewDirTree *vdt = data;
	gchar *path;

	if (!vdt->layout) return;

	if (!vdt->click_fd) return;
	path = g_strdup(vdt->click_fd->path);

	layout_set_path(vdt->layout, path);
	layout_select_none(vdt->layout);
	layout_image_slideshow_stop(vdt->layout);
	layout_image_slideshow_start(vdt->layout);

	g_free(path);
}

static void vdtree_pop_menu_slide_rec_cb(GtkWidget *widget, gpointer data)
{
	ViewDirTree *vdt = data;
	gchar *path;
	GList *list;

	if (!vdt->layout) return;

	if (!vdt->click_fd) return;
	path = g_strdup(vdt->click_fd->path);

	list = path_list_recursive(path);

	layout_image_slideshow_stop(vdt->layout);
	layout_image_slideshow_start_from_list(vdt->layout, list);

	g_free(path);
}

static void vdtree_pop_menu_dupe(ViewDirTree *vdt, gint recursive)
{
	DupeWindow *dw;
	const gchar *path;
	GList *list = NULL;

	if (!vdt->click_fd) return;
	path = vdt->click_fd->path;

	if (recursive)
		{
		list = g_list_append(list, g_strdup(path));
		}
	else
		{
		path_list(path, &list, NULL);
		list = path_list_filter(list, FALSE);
		}

	dw = dupe_window_new(DUPE_MATCH_NAME);
	dupe_window_add_files(dw, list, recursive);

	path_list_free(list);
}

static void vdtree_pop_menu_dupe_cb(GtkWidget *widget, gpointer data)
{
	ViewDirTree *vdt = data;
	vdtree_pop_menu_dupe(vdt, FALSE);
}

static void vdtree_pop_menu_dupe_rec_cb(GtkWidget *widget, gpointer data)
{
	ViewDirTree *vdt = data;
	vdtree_pop_menu_dupe(vdt, TRUE);
}

static void vdtree_pop_menu_new_cb(GtkWidget *widget, gpointer data)
{
	ViewDirTree *vdt = data;
	const gchar *path;
	gchar *new_path;
	gchar *buf;

	if (!vdt->click_fd) return;
	path = vdt->click_fd->path;

	buf = concat_dir_and_file(path, _("new_folder"));
	new_path = unique_filename(buf, NULL, NULL, FALSE);
	g_free(buf);
	if (!new_path) return;

	if (!mkdir_utf8(new_path, 0755))
		{
		gchar *text;

		text = g_strdup_printf(_("Unable to create folder:\n%s"), new_path);
		file_util_warning_dialog(_("Error creating folder"), text, GTK_STOCK_DIALOG_ERROR, vdt->treeview);
		g_free(text);
		}
	else
		{
		FileData *fd;

		fd = vdtree_populate_path(vdt, new_path, TRUE, TRUE);

		vdtree_rename_by_data(vdt, fd);
		}

	g_free(new_path);
}
#endif

static void vdtree_pop_menu_rename_cb(GtkWidget *widget, gpointer data)
{
#ifdef LATER
	ViewDirTree *vdt = data;

	vdtree_rename_by_data(vdt, vdt->click_fd);
#endif
}

#ifdef LATER
static void vdtree_pop_menu_tree_cb(GtkWidget *widget, gpointer data)
{
	ViewDirTree *vdt = data;

	if (vdt->layout) layout_views_set(vdt->layout, FALSE, vdt->layout->icon_view);
}

static void
vdtree_pop_menu_refresh_cb(GtkWidget *widget, gpointer data)
{
	ViewDirTree *vdt = data;

	if (vdt->layout) layout_refresh(vdt->layout);
}
#endif

static GtkWidget*
vdtree_pop_menu(ViewDirTree *vdt, FileData *fd)
{
	gint active;

	active = (fd != NULL);

	GtkWidget* menu = popup_menu_short_lived();
	g_signal_connect(G_OBJECT(menu), "destroy",
			 G_CALLBACK(vdtree_popup_destroy_cb), vdt);

/*
	menu_item_add_stock_sensitive(menu, "_Up to parent", GTK_STOCK_GO_UP,
		       		      (vdt->path && strcmp(vdt->path, "/") != 0),
				      G_CALLBACK(vdtree_pop_menu_up_cb), vdt);

	menu_item_add_divider(menu);
	menu_item_add_sensitive(menu, _("_Slideshow"), active, G_CALLBACK(vdtree_pop_menu_slide_cb), vdt);
	menu_item_add_sensitive(menu, _("Slideshow recursive"), active, G_CALLBACK(vdtree_pop_menu_slide_rec_cb), vdt);

	menu_item_add_divider(menu);
	menu_item_add_stock_sensitive(menu, _("Find _duplicates..."), GTK_STOCK_FIND, active, G_CALLBACK(vdtree_pop_menu_dupe_cb), vdt);
	menu_item_add_stock_sensitive(menu, _("Find duplicates recursive..."), GTK_STOCK_FIND, active, G_CALLBACK(vdtree_pop_menu_dupe_rec_cb), vdt);

	menu_item_add_divider(menu);

	active = (fd && access_file(fd->path, W_OK | X_OK));
	menu_item_add_sensitive(menu, _("_New folder..."), active, G_CALLBACK(vdtree_pop_menu_new_cb), vdt);
*/

/*
	menu_item_add_sensitive(menu, "_Rename...", active, G_CALLBACK(vdtree_pop_menu_rename_cb), vdt);
*/

/*
	menu_item_add_divider(menu);
	menu_item_add_check(menu, _("View as _tree"), TRUE, G_CALLBACK(vdtree_pop_menu_tree_cb), vdt);
	menu_item_add_stock(menu, _("Re_fresh"), GTK_STOCK_REFRESH, G_CALLBACK(vdtree_pop_menu_refresh_cb), vdt);
*/
	GList* l = menu_items;
	for(;l;l=l->next){
		GtkAction* action = l->data;

		GtkWidget* menu_item = gtk_action_create_menu_item(action);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		gtk_widget_show(menu_item);
	}

	return menu;
}

/*
 *----------------------------------------------------------------------------
 * dnd
 *----------------------------------------------------------------------------
 */

static GtkTargetEntry vdtree_dnd_drop_types[] = {
	{ "text/uri-list", 0, TARGET_URI_LIST }
};
static gint vdtree_dnd_drop_types_count = 1;


static void vdtree_dest_set(ViewDirTree *vdt, gint enable)
{
	if (enable)
		{
		gtk_drag_dest_set(vdt->treeview,
				  GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_DROP,
				  vdtree_dnd_drop_types, vdtree_dnd_drop_types_count,
				  GDK_ACTION_MOVE | GDK_ACTION_COPY);
		}
	else
		{
		gtk_drag_dest_unset(vdt->treeview);
		}
}

#ifdef LATER
static void vdtree_dnd_get(GtkWidget *widget, GdkDragContext *context,
			   GtkSelectionData *selection_data, guint info,
			   guint time, gpointer data)
{
	PF;

	ViewDirTree *vdt = data;
	gchar *path;
	GList *list;
	gchar *uri_text = NULL;
	gint length = 0;

	if (!vdt->click_fd) return;
	path = vdt->click_fd->path;

	switch (info)
		{
		case TARGET_URI_LIST:
		case TARGET_TEXT_PLAIN:
			list = g_list_prepend(NULL, path);
			uri_text = uri_text_from_list(list, &length, (info == TARGET_TEXT_PLAIN));
			g_list_free(list);
			break;
		}

	if (uri_text)
		{
		gtk_selection_data_set(selection_data, selection_data->target,
				       8, uri_text, length);
		g_free(uri_text);
		}
}

static void vdtree_dnd_begin(GtkWidget *widget, GdkDragContext *context, gpointer data)
{
	PF;
	ViewDirTree *vdt = data;

	vdtree_color_set(vdt, vdt->click_fd, TRUE);
	vdtree_dest_set(vdt, FALSE);
}

static void vdtree_dnd_end(GtkWidget *widget, GdkDragContext *context, gpointer data)
{
	PF;
	ViewDirTree *vdt = data;

	vdtree_color_set(vdt, vdt->click_fd, FALSE);
	vdtree_dest_set(vdt, TRUE);
}
#endif

static void vdtree_dnd_drop_receive(GtkWidget *widget,
				    GdkDragContext *context, gint x, gint y,
				    GtkSelectionData *selection_data, guint info,
				    guint time, gpointer data)
{
	PF;
	ViewDirTree *vdt = data;
	GtkTreePath *tpath;
	GtkTreeIter iter;
	FileData *fd = NULL;

	vdt->click_fd = NULL;

	if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(widget), x, y,
					  &tpath, NULL, NULL, NULL))
		{
		GtkTreeModel *store;
		NodeData *nd;

		store = gtk_tree_view_get_model(GTK_TREE_VIEW(widget));
		gtk_tree_model_get_iter(store, &iter, tpath);
		gtk_tree_model_get(store, &iter, DIR_COLUMN_POINTER, &nd, -1);
		gtk_tree_path_free(tpath);

		fd = (nd) ? nd->fd : NULL;
		}

	if (!fd) return;

        if (info == TARGET_URI_LIST)
                {
		GList *list;
		gint active;

		list = uri_list_from_text((char*)selection_data->data, TRUE);
		if (!list) return;

		active = access_file(fd->path, W_OK | X_OK);

		vdtree_color_set(vdt, fd, TRUE);
		vdt->popup = vdtree_drop_menu(vdt, active);
		gtk_menu_popup(GTK_MENU(vdt->popup), NULL, NULL, NULL, NULL, 0, time);

		vdt->drop_fd = fd;
		vdt->drop_list = list;
		}
}

#ifdef LATER
static gint vdtree_dnd_drop_expand_cb(gpointer data)
{
	ViewDirTree *vdt = data;
	GtkTreeIter iter;

	if (vdt->drop_fd &&
	    vdtree_find_row(vdt, vdt->drop_fd, &iter, NULL))
		{
		vdtree_populate_path_by_iter(vdt, &iter, FALSE, vdt->path);
		vdtree_expand_by_data(vdt, vdt->drop_fd, TRUE);
		}

	vdt->drop_expand_id = -1;
	return FALSE;
}

static void vdtree_dnd_drop_expand_cancel(ViewDirTree *vdt)
{
	if (vdt->drop_expand_id != -1) g_source_remove(vdt->drop_expand_id);
	vdt->drop_expand_id = -1;
}

static void vdtree_dnd_drop_expand(ViewDirTree *vdt)
{
	vdtree_dnd_drop_expand_cancel(vdt);
	vdt->drop_expand_id = g_timeout_add(1000, vdtree_dnd_drop_expand_cb, vdt);
}

static void vdtree_drop_update(ViewDirTree *vdt, gint x, gint y)
{
	GtkTreePath *tpath;
	GtkTreeIter iter;
	FileData *fd = NULL;

	if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(vdt->treeview), x, y,
					  &tpath, NULL, NULL, NULL))
		{
		GtkTreeModel *store;
		NodeData *nd;

		store = gtk_tree_view_get_model(GTK_TREE_VIEW(vdt->treeview));
		gtk_tree_model_get_iter(store, &iter, tpath);
		gtk_tree_model_get(store, &iter, DIR_COLUMN_POINTER, &nd, -1);
		gtk_tree_path_free(tpath);

		fd = (nd) ? nd->fd : NULL;
		}

	if (fd != vdt->drop_fd)
		{
		vdtree_color_set(vdt, vdt->drop_fd, FALSE);
		vdtree_color_set(vdt, fd, TRUE);
		if (fd) vdtree_dnd_drop_expand(vdt);
		}

	vdt->drop_fd = fd;
}

static void vdtree_dnd_drop_scroll_cancel(ViewDirTree *vdt)
{
	if (vdt->drop_scroll_id != -1) g_source_remove(vdt->drop_scroll_id);
	vdt->drop_scroll_id = -1;
}

static gint vdtree_auto_scroll_idle_cb(gpointer data)
{
	ViewDirTree *vdt = data;

	if (vdt->drop_fd)
		{
		GdkWindow *window;
		gint x, y;
		gint w, h;

		window = vdt->treeview->window;
		gdk_window_get_pointer(window, &x, &y, NULL);
		gdk_drawable_get_size(window, &w, &h);
		if (x >= 0 && x < w && y >= 0 && y < h)
			{
			vdtree_drop_update(vdt, x, y);
			}
		}

	vdt->drop_scroll_id = -1;
	return FALSE;
}

static gint vdtree_auto_scroll_notify_cb(GtkWidget *widget, gint x, gint y, gpointer data)
{
	ViewDirTree *vdt = data;

	if (!vdt->drop_fd || vdt->drop_list) return FALSE;

	if (vdt->drop_scroll_id == -1) vdt->drop_scroll_id = g_idle_add(vdtree_auto_scroll_idle_cb, vdt);

	return TRUE;
}

static gint vdtree_dnd_drop_motion(GtkWidget *widget, GdkDragContext *context,
				   gint x, gint y, guint time, gpointer data)
{
        ViewDirTree *vdt = data;

	vdt->click_fd = NULL;

	if (gtk_drag_get_source_widget(context) == vdt->treeview)
		{
		gdk_drag_status(context, 0, time);
		return TRUE;
		}
	else
		{
		gdk_drag_status(context, context->suggested_action, time);
		}

	vdtree_drop_update(vdt, x, y);

	if (vdt->drop_fd)
		{
		GtkAdjustment *adj = gtk_tree_view_get_vadjustment(GTK_TREE_VIEW(vdt->treeview));
		widget_auto_scroll_start(vdt->treeview, adj, -1, -1, vdtree_auto_scroll_notify_cb, vdt);
		}

	return FALSE;
}

static void vdtree_dnd_drop_leave(GtkWidget *widget, GdkDragContext *context, guint time, gpointer data)
{
	ViewDirTree *vdt = data;

	if (vdt->drop_fd != vdt->click_fd) vdtree_color_set(vdt, vdt->drop_fd, FALSE);

	vdt->drop_fd = NULL;

	vdtree_dnd_drop_expand_cancel(vdt);
}
#endif

static void vdtree_dnd_init(ViewDirTree *vdt)
{
#ifdef LATER
	gtk_drag_source_set(vdt->treeview, GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
			    dnd_file_drag_types, dnd_file_drag_types_count,
			    GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_ASK);
	g_signal_connect(G_OBJECT(vdt->treeview), "drag_data_get",
			 G_CALLBACK(vdtree_dnd_get), vdt);
	g_signal_connect(G_OBJECT(vdt->treeview), "drag_begin",
			 G_CALLBACK(vdtree_dnd_begin), vdt);
	g_signal_connect(G_OBJECT(vdt->treeview), "drag_end",
			 G_CALLBACK(vdtree_dnd_end), vdt);
#endif

	vdtree_dest_set(vdt, TRUE);
	g_signal_connect(G_OBJECT(vdt->treeview), "drag_data_received", G_CALLBACK(vdtree_dnd_drop_receive), vdt);
	//g_signal_connect(G_OBJECT(vdt->treeview), "drag_motion", G_CALLBACK(vdtree_dnd_drop_motion), vdt);
	//g_signal_connect(G_OBJECT(vdt->treeview), "drag_leave", G_CALLBACK(vdtree_dnd_drop_leave), vdt);
}

/*
 *----------------------------------------------------------------------------
 * parts lists
 *----------------------------------------------------------------------------
 */

static GList *parts_list(const gchar *path)
{
	//path must be absolute

	GList *list = NULL;
	const gchar *strb, *strp;
	gint l;

	strp = path;

	if (*strp != '/') return NULL;

	strp++;
	strb = strp;
	l = 0;

	while (*strp != '\0')
		{
		if (*strp == '/')
			{
			if (l > 0) list = g_list_prepend(list, g_strndup(strb, l));
			strp++;
			strb = strp;
			l = 0;
			}
		else
			{
			strp++;
			l++;
			}
		}
	if (l > 0) list = g_list_prepend(list, g_strndup(strb, l));

	list = g_list_reverse(list);

	//##########################
	//list = g_list_remove_link(list, list);
	//##########################

	list = g_list_prepend(list, g_strdup("/"));

	return list;
}

static void parts_list_free(GList *list)
{
	GList *work = list;
	while (work)
		{
		PathData *pd = work->data;
		g_free(pd->name);
		g_free(pd);
		work = work->next;
		}

	g_list_free(list);
}

static GList *parts_list_add_node_points(ViewDirTree *vdt, GList *list)
{
	//takes a list of path fragment names, and returns a corresponding list of PathData.

	GList *work;
	GtkTreeModel *store;
	GtkTreeIter iter;
	gint valid;

	store = gtk_tree_view_get_model(GTK_TREE_VIEW(vdt->treeview));
	valid = gtk_tree_model_get_iter_first(store, &iter);

	work = list;
	while (work)
		{
		PathData *pd;
		FileData *fd = NULL;

		pd = g_new0(PathData, 1);
		pd->name = work->data;

		//printf("%s(): looking for: '%s'...\n", __func__, pd->name);
		while (valid && !fd) //look through the model until we find our path
			{
			NodeData *nd;

			gtk_tree_model_get(store, &iter, DIR_COLUMN_POINTER, &nd, -1);
			if (strcmp(nd->fd->name, pd->name) == 0)
				{
				fd = nd->fd;
				}
			else
				{
				valid = gtk_tree_model_iter_next(store, &iter);
				}
			}

		//printf("%s(): '%s': fd=%p\n", __func__, pd->name, fd);
		pd->node = fd;
		work->data = pd;

		if (fd)
			{
			GtkTreeIter parent;
			memcpy(&parent, &iter, sizeof(parent));
			valid = gtk_tree_model_iter_children(store, &iter, &parent);
			}

		work = work->next;
		}

	return list;
}

/*
 *----------------------------------------------------------------------------
 * misc
 *----------------------------------------------------------------------------
 */

#if 0
static void vdtree_row_deleted_cb(GtkTreeModel *tree_model, GtkTreePath *tpath, gpointer data)
{
	GtkTreeIter iter;
	NodeData *nd;

	gtk_tree_model_get_iter(tree_model, &iter, tpath);
	gtk_tree_model_get(tree_model, &iter, DIR_COLUMN_POINTER, &nd, -1);

	if (!nd) return;

	file_data_free(nd->fd);
	g_free(nd);
}
#endif

/*
 *----------------------------------------------------------------------------
 * node traversal, management
 *----------------------------------------------------------------------------
 */

static gint vdtree_find_iter_by_data(ViewDirTree *vdt, GtkTreeIter *parent, NodeData *nd, GtkTreeIter *iter)
{
	GtkTreeModel *store;

	store = gtk_tree_view_get_model(GTK_TREE_VIEW(vdt->treeview));
	if (!nd || !gtk_tree_model_iter_children(store, iter, parent)) return -1;
	do	{
		NodeData *cnd;

		gtk_tree_model_get(store, iter, DIR_COLUMN_POINTER, &cnd, -1);
		if (cnd == nd) return TRUE;
		} while (gtk_tree_model_iter_next(store, iter));

	return FALSE;
}

static NodeData *vdtree_find_iter_by_name(ViewDirTree *vdt, GtkTreeIter *parent, const gchar *name, GtkTreeIter *iter)
{
	GtkTreeModel *store;

	store = gtk_tree_view_get_model(GTK_TREE_VIEW(vdt->treeview));
	if (!name || !gtk_tree_model_iter_children(store, iter, parent)) return NULL;
	do	{
		NodeData *nd;

		gtk_tree_model_get(store, iter, DIR_COLUMN_POINTER, &nd, -1);
		if (nd && strcmp(nd->fd->name, name) == 0) return nd;
		} while (gtk_tree_model_iter_next(store, iter));

	return NULL;
}

static void vdtree_add_by_data(ViewDirTree *vdt, FileData *fd, GtkTreeIter *parent)
{
	GtkTreeStore *store;
	GtkTreeIter child;
	GList *list;
	NodeData *nd;
	GdkPixbuf *pixbuf;
	NodeData *end;
	GtkTreeIter empty;

	if (!fd) return;

	list = parts_list(fd->path);
	if (!list) return;

	if (access_file(fd->path, R_OK | X_OK))
		{
		pixbuf = vdt->pf->close;
		}
	else
		{
		pixbuf = vdt->pf->deny;
		}

	nd = g_new0(NodeData, 1);
	nd->fd = fd;
	nd->expanded = FALSE;
	nd->last_update = time(NULL);

	store = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(vdt->treeview)));
	gtk_tree_store_append(store, &child, parent);
	gtk_tree_store_set(store, &child, DIR_COLUMN_POINTER, nd,
					 DIR_COLUMN_ICON, pixbuf,
					 DIR_COLUMN_NAME, nd->fd->name,
					 DIR_COLUMN_COLOR, FALSE, -1);

	/* all nodes are created with an "empty" node, so that the expander is shown
	 * this is removed when the child is populated */
	end = g_new0(NodeData, 1);
	end->fd = g_new0(FileData, 1);
	end->fd->path = g_strdup("");
	end->fd->name = end->fd->path;
	end->expanded = TRUE;

	gtk_tree_store_append(store, &empty, &child);
	gtk_tree_store_set(store, &empty, DIR_COLUMN_POINTER, end,
					  DIR_COLUMN_NAME, "empty", -1);

	if (parent)
		{
		NodeData *pnd;
		GtkTreePath *tpath;

		gtk_tree_model_get(GTK_TREE_MODEL(store), parent, DIR_COLUMN_POINTER, &pnd, -1);
		tpath = gtk_tree_model_get_path(GTK_TREE_MODEL(store), parent);
		if (tree_descend_subdirs &&
		    gtk_tree_view_row_expanded(GTK_TREE_VIEW(vdt->treeview), tpath) &&
		    !nd->expanded)
			{
			vdtree_populate_path_by_iter(vdt, &child, FALSE, vdt->path);
			}
		gtk_tree_path_free(tpath);
		}
}


static gint vdt_filelist_read(ViewDirTree *vdt, const gchar *path, GList **files, GList **dirs)
{
	//this fn fools the treeview into thinking that a directory is a root directory by prepending vdt->root_path if neccesary.

	GList *dlist;
	GList *flist;

	dlist = NULL;
	flist = NULL;

	gchar *real_path = NULL;
	if(strstr(path, vdt->root_path) != path) real_path = g_build_filename(vdt->root_path, path, NULL);

	gint ret = filelist_read(real_path ? real_path : path, files, dirs);

	if(real_path) g_free(real_path);
	return ret;
}

static gint vdtree_populate_path_by_iter(ViewDirTree *vdt, GtkTreeIter *iter, gint force, const gchar *target_path)
{
	GtkTreeModel *store;
	GList *list;
	GList *work;
	GList *old;
	time_t current_time;
	GtkTreeIter child;
	NodeData *nd;

	store = gtk_tree_view_get_model(GTK_TREE_VIEW(vdt->treeview));
	gtk_tree_model_get(store, iter, DIR_COLUMN_POINTER, &nd, -1);

	if (!nd) return FALSE;

	current_time = time(NULL);
	
	if (nd->expanded)
		{
		if (!force && current_time - nd->last_update < 10) return TRUE;
		if (!isdir(nd->fd->path))
			{
			if (vdt->click_fd == nd->fd) vdt->click_fd = NULL;
			if (vdt->drop_fd == nd->fd) vdt->drop_fd = NULL;
			gtk_tree_store_remove(GTK_TREE_STORE(store), iter);
			vdtree_node_free(nd);
			return FALSE;
			}
		if (!force && filetime(nd->fd->path) == nd->fd->date) return TRUE;
		}

	vdtree_busy_push(vdt);

	list = NULL;
	//filelist_read(nd->fd->path, NULL, &list);
	vdt_filelist_read(vdt, nd->fd->path, NULL, &list);

	/* when hidden files are not enabled, and the user enters a hidden path,
	 * allow the tree to display that path by specifically inserting the hidden entries
	 */
	if (!show_dot_files &&
	    target_path &&
	    strncmp(nd->fd->path, target_path, strlen(nd->fd->path)) == 0)
		{
		gint n;

		n = strlen(nd->fd->path);
		if (target_path[n] == '/' && target_path[n+1] == '.')
			{
			gchar *name8;
			gchar *namel;
			struct stat sbuf;

			n++;

			while (target_path[n] != '\0' && target_path[n] != '/') n++;
			name8 = g_strndup(target_path, n);
			namel = path_from_utf8(name8);

			if (stat_utf8(name8, &sbuf))
				{
				list = g_list_prepend(list, file_data_new(namel, &sbuf));
				}

			g_free(namel);
			g_free(name8);
			}
		}

	old = NULL;
	if (gtk_tree_model_iter_children(store, &child, iter))
		{
		do	{
			NodeData *cnd;

			gtk_tree_model_get(store, &child, DIR_COLUMN_POINTER, &cnd, -1);
			old = g_list_prepend(old, cnd);
			} while (gtk_tree_model_iter_next(store, &child));
		}

	work = list;
	while (work)
		{
		FileData *fd;

		fd = work->data;
		work = work->next;

		if (strcmp(fd->name, ".") == 0 || strcmp(fd->name, "..") == 0)
			{
			file_data_free(fd);
			}
		else
			{
			NodeData *cnd;

			cnd = vdtree_find_iter_by_name(vdt, iter, fd->name, &child);
			if (cnd)
				{
				old = g_list_remove(old, cnd);
				if (cnd->expanded && cnd->fd->date != fd->date &&
				    vdtree_populate_path_by_iter(vdt, &child, FALSE, target_path))
					{
					cnd->fd->size = fd->size;
					cnd->fd->date = fd->date;
					}

				file_data_free(fd);
				}
			else
				{
				vdtree_add_by_data(vdt, fd, iter);
				}
			}
		}

	work = old;
	while (work)
		{
		NodeData *cnd = work->data;
		work = work->next;

		if (vdt->click_fd == cnd->fd) vdt->click_fd = NULL;
		if (vdt->drop_fd == cnd->fd) vdt->drop_fd = NULL;

		if (vdtree_find_iter_by_data(vdt, iter, cnd, &child))
			{
			gtk_tree_store_remove(GTK_TREE_STORE(store), &child);
			vdtree_node_free(cnd);
			}
		}

	g_list_free(old);
	g_list_free(list);

	vdtree_busy_pop(vdt);

	nd->expanded = TRUE;
	nd->last_update = current_time;

	return TRUE;
}

static FileData *vdtree_populate_path(ViewDirTree *vdt, const gchar *path, gint expand, gint force)
{
	dbg(2, "path=%s", path);
	GList *list;
	GList *work;
	FileData *fd = NULL;

	if (!path) return NULL;

	vdtree_busy_push(vdt);

	list = parts_list(path); //split the path into its constituent parts
#if 0
	{
		printf("vdtree_populate_path():");
		GList* l = list;
		for(;l;l=l->next)
			printf(" %s", (char*)l->data);
		printf("\n");
	}
#endif
	list = parts_list_add_node_points(vdt, list); //convert the list into list of PathData*.

	work = list;
	while (work)
		{
		PathData *pd = work->data;
		dbg(2, "pd=%s node=%p", pd->name, pd->node);
		if (pd->node == NULL) //item is not already in the model.
			{
			dbg(2, "pd=%s", pd->name);
			PathData *parent_pd;
			GtkTreeIter parent_iter;
			GtkTreeIter iter;
			NodeData *nd;

			if (work == list) //first part
				{
				/* should not happen */
				printf("vdtree warning, root node not found\n");
				parts_list_free(list);
				vdtree_busy_pop(vdt);
				return NULL;
				}

			parent_pd = work->prev->data;

			if (!vdtree_find_row(vdt, parent_pd->node, &parent_iter, NULL) ||
			    !vdtree_populate_path_by_iter(vdt, &parent_iter, force, path) ||
			    (nd = vdtree_find_iter_by_name(vdt, &parent_iter, pd->name, &iter)) == NULL)
				{
				if(debug) printf("vdtree warning, aborted at %s\n", parent_pd->name);
				parts_list_free(list);
				vdtree_busy_pop(vdt);
				return NULL;
				}

			pd->node = nd->fd;

			if (pd->node)
				{
				if (expand)
					{
					vdtree_expand_by_iter(vdt, &parent_iter, TRUE);
					vdtree_expand_by_iter(vdt, &iter, TRUE);
					}
				vdtree_populate_path_by_iter(vdt, &iter, force, path);
				}
			}
		else
			{
			GtkTreeIter iter;

			if (vdtree_find_row(vdt, pd->node, &iter, NULL))
				{
				if (expand) vdtree_expand_by_iter(vdt, &iter, TRUE);
				vdtree_populate_path_by_iter(vdt, &iter, force, path);
				}
			}

		work = work->next;
		}

	work = g_list_last(list);
	if (work)
		{
		PathData *pd = work->data;
		fd = pd->node;
		}
	parts_list_free(list);

	vdtree_busy_pop(vdt);

	return fd;
}

/*
 *----------------------------------------------------------------------------
 * access
 *----------------------------------------------------------------------------
 */

static gint selection_is_ok = FALSE;

static gboolean vdtree_select_cb(GtkTreeSelection *selection, GtkTreeModel *store, GtkTreePath *tpath,
                                 gboolean path_currently_selected, gpointer data)
{
	return selection_is_ok;
}

static void
vdtree_select_row(ViewDirTree *vdt, FileData *fd)
{
	GtkTreeIter iter;
                                                                                                                               
	if (!vdtree_find_row(vdt, fd, &iter, NULL)) return;
	GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(vdt->treeview));

	/* hack, such that selection is only allowed to be changed from here */
	selection_is_ok = TRUE;
	gtk_tree_selection_select_iter(selection, &iter);
	selection_is_ok = FALSE;

	if (!vdtree_populate_path_by_iter(vdt, &iter, FALSE, vdt->path)) return;

	vdtree_expand_by_iter(vdt, &iter, TRUE);

	if (fd && vdt->select_func)
		{
		vdt->select_func(vdt, fd->path, vdt->select_data);
		}
}

gint vdtree_set_path(ViewDirTree *vdt, const gchar *path)
{
	dbg(1, "path=%s", path);
	FileData *fd;
	GtkTreeIter iter;

	if (!path) return FALSE;
	if (vdt->path && strcmp(path, vdt->path) == 0) return TRUE;

	g_free(vdt->path);
	vdt->path = g_strdup(path);

	fd = vdtree_populate_path(vdt, vdt->path, TRUE, TRUE);

	if (!fd) return FALSE;

	if (vdtree_find_row(vdt, fd, &iter, NULL))
		{
		GtkTreeModel *store;
		GtkTreePath *tpath;

		tree_view_row_make_visible(GTK_TREE_VIEW(vdt->treeview), &iter, TRUE);

		store = gtk_tree_view_get_model(GTK_TREE_VIEW(vdt->treeview));
		tpath = gtk_tree_model_get_path(store, &iter);
		gtk_tree_view_set_cursor(GTK_TREE_VIEW(vdt->treeview), tpath, NULL, FALSE);
		gtk_tree_path_free(tpath);

		vdtree_select_row(vdt, fd);
		}

	return TRUE;
}

#if 0
const gchar *vdtree_get_path(ViewDirTree *vdt)
{
	return vdt->path;
}
#endif

void vdtree_refresh(ViewDirTree *vdt)
{
	vdtree_populate_path(vdt, vdt->path, FALSE, TRUE);
}

const gchar *vdtree_row_get_path(ViewDirTree *vdt, gint row)
{
	printf("FIXME: no get row path\n");
	return NULL;
}

/*
 *----------------------------------------------------------------------------
 * callbacks
 *----------------------------------------------------------------------------
 */

static void vdtree_menu_position_cb(GtkMenu *menu, gint *x, gint *y, gboolean *push_in, gpointer data)
{
	ViewDirTree *vdt = data;
	GtkTreeModel *store;
	GtkTreeIter iter;
	GtkTreePath *tpath;
	gint cw, ch;

	if (vdtree_find_row(vdt, vdt->click_fd, &iter, NULL) < 0) return;
	store = gtk_tree_view_get_model(GTK_TREE_VIEW(vdt->treeview));
	tpath = gtk_tree_model_get_path(store, &iter);
	tree_view_get_cell_clamped(GTK_TREE_VIEW(vdt->treeview), tpath, 0, TRUE, x, y, &cw, &ch);
	gtk_tree_path_free(tpath);
	*y += ch;

	popup_menu_position_clamp(menu, x, y, 0);
}

static gint vdtree_press_key_cb(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	ViewDirTree *vdt = data;
	GtkTreePath *tpath;
	GtkTreeIter iter;
	FileData *fd = NULL;

	gtk_tree_view_get_cursor(GTK_TREE_VIEW(vdt->treeview), &tpath, NULL);
	if (tpath)
		{
		GtkTreeModel *store;
		NodeData *nd;

		store = gtk_tree_view_get_model(GTK_TREE_VIEW(widget));
		gtk_tree_model_get_iter(store, &iter, tpath);
		gtk_tree_model_get(store, &iter, DIR_COLUMN_POINTER, &nd, -1);

		gtk_tree_path_free(tpath);

		fd = (nd) ? nd->fd : NULL;
		}

	switch (event->keyval)
		{
		case GDK_Menu:
			vdt->click_fd = fd;
			vdtree_color_set(vdt, vdt->click_fd, TRUE);

			vdt->popup = vdtree_pop_menu(vdt, vdt->click_fd);
			gtk_menu_popup(GTK_MENU(vdt->popup), NULL, NULL, vdtree_menu_position_cb, vdt, 0, GDK_CURRENT_TIME);

			return TRUE;
			break;
		case GDK_plus:
		case GDK_Right:
		case GDK_KP_Add:
			if (fd)
				{
				vdtree_populate_path_by_iter(vdt, &iter, FALSE, vdt->path);
				vdtree_icon_set_by_iter(vdt, &iter, vdt->pf->open);
				}
			break;
		}

	return FALSE;
}

static gint
vdtree_clicked_on_expander(GtkTreeView *treeview, GtkTreePath *tpath, GtkTreeViewColumn *column, gint x, gint y, gint *left_of_expander)
{
	gint depth;
	gint size;
	gint sep;
	gint exp_width;

	if (column != gtk_tree_view_get_expander_column(treeview)) return FALSE;

	gtk_widget_style_get(GTK_WIDGET(treeview), "expander-size", &size, "horizontal-separator", &sep, NULL);
	depth = gtk_tree_path_get_depth(tpath);

	exp_width = sep + size + sep;

	if (x <= depth * exp_width)
		{
		if (left_of_expander) *left_of_expander = !(x >= (depth - 1) * exp_width);
		return TRUE;
		}

	return FALSE;
}

static gint
vdtree_press_cb(GtkWidget *widget, GdkEventButton *bevent, gpointer data)
{
	PF;
	ViewDirTree *vdt = data;
	GtkTreePath *tpath;
	GtkTreeViewColumn *column;
	GtkTreeIter iter;
	NodeData *nd = NULL;

	if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(widget), bevent->x, bevent->y, &tpath, &column, NULL, NULL))
		{
		GtkTreeModel *store;
		gint left_of_expander;

		store = gtk_tree_view_get_model(GTK_TREE_VIEW(widget));
		gtk_tree_model_get_iter(store, &iter, tpath);
		gtk_tree_model_get(store, &iter, DIR_COLUMN_POINTER, &nd, -1);
		gtk_tree_view_set_cursor(GTK_TREE_VIEW(widget), tpath, NULL, FALSE);

		if (vdtree_clicked_on_expander(GTK_TREE_VIEW(widget), tpath, column, bevent->x, bevent->y, &left_of_expander))
			{
			vdt->click_fd = NULL;

			/* clicking this region should automatically reveal an expander, if necessary
			 * treeview bug: the expander will not expand until a button_motion_event highlights it.
			 */
			if (bevent->button == 1 &&
			    !left_of_expander &&
			    !gtk_tree_view_row_expanded(GTK_TREE_VIEW(vdt->treeview), tpath))
				{
				vdtree_populate_path_by_iter(vdt, &iter, FALSE, vdt->path);
				vdtree_icon_set_by_iter(vdt, &iter, vdt->pf->open);
				}

			gtk_tree_path_free(tpath);
			return FALSE;
			}

		gtk_tree_path_free(tpath);
		}

	vdt->click_fd = (nd) ? nd->fd : NULL;
	vdtree_color_set(vdt, vdt->click_fd, TRUE);

	if (bevent->button == 3)
		{
		vdt->popup = vdtree_pop_menu(vdt, vdt->click_fd);
		gtk_menu_popup(GTK_MENU(vdt->popup), NULL, NULL, NULL, NULL, bevent->button, bevent->time);
		}

	return (bevent->button != 1);
}

static gint
vdtree_release_cb(GtkWidget *widget, GdkEventButton *bevent, gpointer data)
{
	ViewDirTree *vdt = data;
	GtkTreePath *tpath;
	GtkTreeIter iter;
	NodeData *nd = NULL;

	if (!vdt->click_fd) return FALSE;
	vdtree_color_set(vdt, vdt->click_fd, FALSE);

	if (bevent->button != 1) return TRUE;

	if ((bevent->x != 0 || bevent->y != 0) &&
	    gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(widget), bevent->x, bevent->y, &tpath, NULL, NULL, NULL))
		{
		GtkTreeModel* store = gtk_tree_view_get_model(GTK_TREE_VIEW(widget));
		gtk_tree_model_get_iter(store, &iter, tpath);
		gtk_tree_model_get(store, &iter, DIR_COLUMN_POINTER, &nd, -1);
		gtk_tree_path_free(tpath);
		}

	if (nd && vdt->click_fd == nd->fd)
		{
		vdtree_select_row(vdt, vdt->click_fd);
		}

	return FALSE;
}

static void vdtree_row_expanded(GtkTreeView *treeview, GtkTreeIter *iter, GtkTreePath *tpath, gpointer data)
{
	ViewDirTree *vdt = data;

	vdtree_icon_set_by_iter(vdt, iter, vdt->pf->open);
}

static void vdtree_row_collapsed(GtkTreeView *treeview, GtkTreeIter *iter, GtkTreePath *tpath, gpointer data)
{
	ViewDirTree *vdt = data;

	vdtree_icon_set_by_iter(vdt, iter, vdt->pf->close);
}

static gint vdtree_sort_cb(GtkTreeModel *store, GtkTreeIter *a, GtkTreeIter *b, gpointer data)
{
	NodeData *nda;
	NodeData *ndb;

	gtk_tree_model_get(store, a, DIR_COLUMN_POINTER, &nda, -1);
	gtk_tree_model_get(store, b, DIR_COLUMN_POINTER, &ndb, -1);

	return CASE_SORT(nda->fd->name, ndb->fd->name);
}

/*
 *----------------------------------------------------------------------------
 * core
 *----------------------------------------------------------------------------
 */

static void vdtree_setup_root(ViewDirTree *vdt)
{
	const gchar *path = "/";

	FileData *fd = g_new0(FileData, 1);
	//fd->path = g_strdup(path);
	fd->path = g_strdup("/");
	fd->name = fd->path;
	fd->size = 0;
	fd->date = filetime(path);
	vdtree_add_by_data(vdt, fd, NULL); //add entry to tree_model for root

	//testing:
	{
		//g_list_remove_link()
		//path = "tim";
	}

	vdtree_expand_by_data(vdt, fd, TRUE);
	vdtree_populate_path(vdt, path, FALSE, FALSE); //add child nodes to tree_model.
}

static void vdtree_activate_cb(GtkTreeView *tview, GtkTreePath *tpath, GtkTreeViewColumn *column, gpointer data)
{
	PF;
	ViewDirTree *vdt = data;
	GtkTreeModel *store;
	GtkTreeIter iter;
	NodeData *nd;

	store = gtk_tree_view_get_model(tview);
	gtk_tree_model_get_iter(store, &iter, tpath);
	gtk_tree_model_get(store, &iter, DIR_COLUMN_POINTER, &nd, -1);

	vdtree_select_row(vdt, nd->fd);
}

static GdkColor *vdtree_color_shifted(GtkWidget *widget)
{
	static GdkColor color;
	static GtkWidget *done = NULL;

	if (done != widget)
		{
		GtkStyle *style;

		style = gtk_widget_get_style(widget);
		memcpy(&color, &style->base[GTK_STATE_NORMAL], sizeof(color));
		shift_color(&color, -1, 0);
		done = widget;
		}

	return &color;
}

static void vdtree_color_cb(GtkTreeViewColumn *tree_column, GtkCellRenderer *cell,
			    GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer data)
{
	ViewDirTree *vdt = data;
	gboolean set;

	gtk_tree_model_get(tree_model, iter, DIR_COLUMN_COLOR, &set, -1);
	g_object_set(G_OBJECT(cell),
		     "cell-background-gdk", vdtree_color_shifted(vdt->treeview),
		     "cell-background-set", set, NULL);
}

static gboolean vdtree_destroy_node_cb(GtkTreeModel *store, GtkTreePath *tpath, GtkTreeIter *iter, gpointer data)
{
	NodeData *nd;

	gtk_tree_model_get(store, iter, DIR_COLUMN_POINTER, &nd, -1);
	vdtree_node_free(nd);

	return FALSE;
}

static void vdtree_destroy_cb(GtkWidget *widget, gpointer data)
{
	ViewDirTree *vdt = data;
	GtkTreeModel *store;

	if (vdt->popup)
		{
		g_signal_handlers_disconnect_matched(G_OBJECT(vdt->popup), G_SIGNAL_MATCH_DATA, 0, 0, 0, NULL, vdt);
		gtk_widget_destroy(vdt->popup);
		}

#ifdef LATER
	vdtree_dnd_drop_expand_cancel(vdt);
	vdtree_dnd_drop_scroll_cancel(vdt);
#endif
	widget_auto_scroll_stop(vdt->treeview);

	store = gtk_tree_view_get_model(GTK_TREE_VIEW(vdt->treeview));
	gtk_tree_model_foreach(store, vdtree_destroy_node_cb, vdt);

	path_list_free(vdt->drop_list);

	folder_icons_free(vdt->pf);

	g_free(vdt->path);
	g_free(vdt);
}

ViewDirTree *vdtree_new(const gchar *path, gint expand)
{
	PF;
	ViewDirTree *vdt;
	GtkTreeStore *store;
	GtkTreeSelection *selection;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;

	vdt = g_new0(ViewDirTree, 1);

	vdt->path = NULL;
	vdt->root_path = "/" ; //(gchar*)g_get_home_dir(); //TODO add menu item to change this dynamically
	vdt->click_fd = NULL;

	vdt->drop_fd = NULL;
	vdt->drop_list = NULL;
	vdt->drop_scroll_id = -1;
	vdt->drop_expand_id = -1;

	vdt->popup = NULL;

	vdt->busy_ref = 0;

	vdt->widget = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(vdt->widget), GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(vdt->widget),
				       GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	g_signal_connect(G_OBJECT(vdt->widget), "destroy",
			 G_CALLBACK(vdtree_destroy_cb), vdt);

	store = gtk_tree_store_new(4, G_TYPE_POINTER, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_INT);
	vdt->treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	g_object_unref(store);

//#ifdef HAVE_GTK_2_10
	//use treeview lines:
	GValue gval = {0,};
	g_value_init(&gval, G_TYPE_CHAR);
	g_value_set_char(&gval, '1');
	g_object_set_property(G_OBJECT(vdt->treeview), "enable-tree-lines", &gval);
//#endif

	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(vdt->treeview), FALSE);
	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(vdt->treeview), FALSE);
	gtk_tree_sortable_set_default_sort_func(GTK_TREE_SORTABLE(store), vdtree_sort_cb, vdt, NULL);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store),
					     GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID, GTK_SORT_ASCENDING);

	g_signal_connect(G_OBJECT(vdt->treeview), "row_activated",
			 G_CALLBACK(vdtree_activate_cb), vdt);
	g_signal_connect(G_OBJECT(vdt->treeview), "row_expanded",
			 G_CALLBACK(vdtree_row_expanded), vdt);
	g_signal_connect(G_OBJECT(vdt->treeview), "row_collapsed",
			 G_CALLBACK(vdtree_row_collapsed), vdt);
#if 0
	g_signal_connect(G_OBJECT(store), "row_deleted",
			 G_CALLBACK(vdtree_row_deleted_cb), vdt);
#endif

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(vdt->treeview));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
	gtk_tree_selection_set_select_function(selection, vdtree_select_cb, vdt, NULL);

	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);

	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_add_attribute(column, renderer, "pixbuf", DIR_COLUMN_ICON);
	gtk_tree_view_column_set_cell_data_func(column, renderer, vdtree_color_cb, vdt, NULL);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_add_attribute(column, renderer, "text", DIR_COLUMN_NAME);
	gtk_tree_view_column_set_cell_data_func(column, renderer, vdtree_color_cb, vdt, NULL);

	gtk_tree_view_append_column(GTK_TREE_VIEW(vdt->treeview), column);

	g_signal_connect(G_OBJECT(vdt->treeview), "key_press_event",
			 G_CALLBACK(vdtree_press_key_cb), vdt);

	gtk_container_add(GTK_CONTAINER(vdt->widget), vdt->treeview);
	gtk_widget_show(vdt->treeview);

	vdt->pf = folder_icons_new();

	vdtree_setup_root(vdt);

	vdtree_dnd_init(vdt);

	g_signal_connect(G_OBJECT(vdt->treeview), "button_press_event", G_CALLBACK(vdtree_press_cb), vdt);
	g_signal_connect(G_OBJECT(vdt->treeview), "button_release_event", G_CALLBACK(vdtree_release_cb), vdt);

	vdtree_set_path(vdt, path);

	return vdt;
}

void
vdtree_set_select_func(ViewDirTree *vdt, void (*func)(ViewDirTree *vdt, const gchar *path, gpointer data), gpointer data)
{
	vdt->select_func = func;
	vdt->select_data = data;
}

#if 0
void vdtree_set_click_func(ViewDirTree *vdt,
			   void (*func)(ViewDirTree *vdt, GdkEventButton *event, FileData *fd, gpointer), gpointer data)
{
	if (!td) return;
	vdt->click_func = func;
	vdt->click_data = data;
}
#endif

void vdtree_set_layout(ViewDirTree *vdt, LayoutWindow *layout)
{
	vdt->layout = layout;
}


ViewDirTree *vdt1 = NULL; //temp

static gboolean
icon_foreach(GtkTreeModel *model, GtkTreePath  *path, GtkTreeIter *iter, gpointer _pixbuf)
{
	GdkPixbuf* pixbuf = _pixbuf;
	vdtree_icon_set_by_iter(vdt1, iter, pixbuf);
	return FALSE; //continue
}


const char*
vdtree_get_selected(ViewDirTree* vdt)
{
	GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(vdt->treeview));
	if(selection){
		GList* rows = gtk_tree_selection_get_selected_rows(selection, NULL);
		if(rows){
			GtkTreePath* path = rows->data;
			GtkTreeModel* store = gtk_tree_view_get_model(GTK_TREE_VIEW(vdt->treeview));
			GtkTreeIter iter;
			gtk_tree_model_get_iter(store, &iter, path);
			NodeData* nd = NULL;
			gtk_tree_model_get(store, &iter, DIR_COLUMN_POINTER, &nd, -1);
			if(nd){
				FileData* fd = nd->fd;
				return fd ? fd->path : NULL;
			}
		}
	}
	return NULL;
}


void
vdtree_on_icon_theme_changed(ViewDirTree *vdt)
{
	//FIXME use the correct icon: open/closed etc

	PF;

	vdt1 = vdt;

	//MIME_type* mime_type = mime_type_lookup(row[MYSQL_MIMETYPE]);
	//MIME_type* mime_type = inode_directory;
	GdkPixbuf* pixbuf = mime_type_get_pixbuf(inode_directory);

	GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(vdt->treeview));

	gtk_tree_model_foreach(model, icon_foreach, pixbuf);
	/*
	GtkTreeIter iter;
	if(gtk_tree_model_get_iter_first(model, &iter)){
		dbg(0, "first iter..");
		vdtree_icon_set_by_iter(vdt, &iter, pixbuf);
		while(gtk_tree_model_iter_next(model, &iter)){
			dbg(0, "next iter..");
			vdtree_icon_set_by_iter(vdt, &iter, pixbuf);
		}
	}
	*/
}


void
vdtree_add_menu_item(GtkAction* action)
{
	g_return_if_fail(action);

	menu_items = g_list_append(menu_items, action);
}


