/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2017-2020 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
* +----------------------------------------------------------------------+
* | * model     - file_manager/directory pure model                      |
* | * viewmodel - this file - does not render anything, but provides a   |
* |               data specific to a particular view                     |
* |               (using GtkTreeModel interface).                        |
* | * view      - views/files - pure view - rendering and user input only|
* +----------------------------------------------------------------------+
*
*/
#include <glib.h>
#include <glib-object.h>
#include <errno.h>
#include <fnmatch.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include "debug/debug.h"
#include "file_manager/typedefs.h"
#include "file_manager/diritem.h"
#include "file_manager/fscache.h"
#include "file_manager/file_manager.h"
#include "directory.h"

#define _g_free0(var) (var = (g_free (var), NULL))
#define _g_object_unref0(var) ((var == NULL) ? NULL : (var = (g_object_unref (var), NULL)))

#define TYPE_DIRECTORY            (vm_directory_get_type ())
#define DIRECTORY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_DIRECTORY, VMDirectory))
#define DIRECTORY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  TYPE_DIRECTORY, VMDirectoryClass))
#define IS_DIRECTORY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_DIRECTORY))
#define IS_DIRECTORY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  TYPE_DIRECTORY))
#define DIRECTORY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  TYPE_DIRECTORY, VMDirectoryClass))

struct _VMDirectoryClass {
    GObjectClass parent_class;
};

static GtkTreeModelIface* directory_gtk_tree_model_parent_iface = NULL;
static void directory_gtk_tree_model_interface_init (GtkTreeModelIface*);

GType vm_directory_get_type (void) G_GNUC_CONST;
enum  {
	DIRECTORY_DUMMY_PROPERTY
};

struct _VMDirectoryPrivate {
	/*
    GList*      users;           // Functions to call on update
    char*       error;           // NULL => no error
    struct stat stat_info;       // Internal use

    gboolean	notify_active;   // Notify timeout is running
    gint		idle_callback;   // Idle callback ID

	gboolean    needs_update;
    GList*      recheck_list;    // Items to check on callback

    gboolean    have_scanned;    // TRUE after first complete scan
    gboolean    scanning;        // TRUE if we sent DIR_START_SCAN

    GHashTable* known_items;     // What our users know about
    GPtrArray*  new_items;       // New items to add in
    GPtrArray*  up_items;        // Items to redraw
    GPtrArray*  gone_items;      // Items removed
	*/
    gboolean	               scanning;        // State of the 'scanning' indicator
    gchar*                     sym_path;        // Path the user sees
    gchar*                     real_path;       // realpath(sym_path)
    Directory*                 directory;
    char*                      auto_select;     // If it we find while scanning
    gboolean                   show_hidden;
    gboolean 	               temp_show_hidden;// TRUE if hidden files are shown because the minibuffer leafname starts with a dot.

    FilterType                 filter;
    gchar*                     filter_string;   // Glob or regexp pattern
    gboolean                   filter_directories;
};

G_DEFINE_TYPE_WITH_CODE (
	VMDirectory,
	vm_directory,
	G_TYPE_OBJECT,
	G_ADD_PRIVATE (VMDirectory)
	G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_MODEL, directory_gtk_tree_model_interface_init)
);

static GType             directory_real_get_column_type (GtkTreeModel*, gint index);
static GtkTreeModelFlags directory_real_get_flags       (GtkTreeModel*);
static gboolean          directory_real_get_iter        (GtkTreeModel*, GtkTreeIter*, GtkTreePath*);
static gint              directory_real_get_n_columns   (GtkTreeModel*);
static GtkTreePath*      directory_real_get_path        (GtkTreeModel*, GtkTreeIter*);
static void              directory_real_get_value       (GtkTreeModel*, GtkTreeIter*, gint column, GValue*);
static gboolean          directory_real_iter_children   (GtkTreeModel*, GtkTreeIter*, GtkTreeIter* parent);
static gboolean          directory_real_iter_has_child  (GtkTreeModel*, GtkTreeIter*);
static gint              directory_real_iter_n_children (GtkTreeModel*, GtkTreeIter*);
static gboolean          directory_real_iter_next       (GtkTreeModel*, GtkTreeIter*);
static gboolean          directory_real_iter_nth_child  (GtkTreeModel*, GtkTreeIter*, GtkTreeIter* parent, gint n);
static gboolean          directory_real_iter_parent     (GtkTreeModel*, GtkTreeIter*, GtkTreeIter* child);

static void              directory_finalize             (GObject*);


static GType
directory_real_get_column_type (GtkTreeModel* base, gint index)
{
	GType result = 0UL;
	result = (GType) NULL;
	return result;
}


static GtkTreeModelFlags
directory_real_get_flags (GtkTreeModel* base)
{
	return GTK_TREE_MODEL_LIST_ONLY;
}


static gboolean
directory_real_get_iter (GtkTreeModel* base, GtkTreeIter* iter, GtkTreePath* path)
{
	GtkTreeIter _vala_iter = {0};
	g_return_val_if_fail (path != NULL, FALSE);
	gboolean result = TRUE;
	if (iter) {
		*iter = _vala_iter;
	}
	return result;
}


static gint
directory_real_get_n_columns (GtkTreeModel* base)
{
	gint result = 0;
	return result;
}


static GtkTreePath*
directory_real_get_path (GtkTreeModel* base, GtkTreeIter* iter)
{
	GtkTreePath* result = NULL;
	g_return_val_if_fail (iter != NULL, NULL);
	result = NULL;
	return result;
}


static void
directory_real_get_value (GtkTreeModel* base, GtkTreeIter* iter, gint column, GValue* value)
{
	GValue _vala_value = {0};
	g_return_if_fail (iter != NULL);
	if (value) {
		*value = _vala_value;
	} else {
		G_IS_VALUE (&_vala_value) ? (g_value_unset (&_vala_value), NULL) : NULL;
	}
}


static gboolean
directory_real_iter_children (GtkTreeModel* base, GtkTreeIter* iter, GtkTreeIter* parent)
{
	GtkTreeIter _vala_iter = {0};
	gboolean result = TRUE;
	if (iter) {
		*iter = _vala_iter;
	}
	return result;
}


static gboolean
directory_real_iter_has_child (GtkTreeModel* base, GtkTreeIter* iter)
{
	gboolean result = FALSE;
	g_return_val_if_fail (iter != NULL, FALSE);
	result = TRUE;
	return result;
}


static gint
directory_real_iter_n_children (GtkTreeModel* base, GtkTreeIter* iter)
{
	gint result = 0;
	result = 0;
	return result;
}


static gboolean
directory_real_iter_next (GtkTreeModel* base, GtkTreeIter* iter)
{
	gboolean result = FALSE;
	g_return_val_if_fail (iter != NULL, FALSE);
	result = TRUE;
	return result;
}


static gboolean
directory_real_iter_nth_child (GtkTreeModel* base, GtkTreeIter* iter, GtkTreeIter* parent, gint n)
{
	GtkTreeIter _vala_iter = {0};
	gboolean result = FALSE;
	result = TRUE;
	if (iter) {
		*iter = _vala_iter;
	}
	return result;
}


static gboolean
directory_real_iter_parent (GtkTreeModel* base, GtkTreeIter* iter, GtkTreeIter* child)
{
	GtkTreeIter _vala_iter = {0};
	gboolean result = FALSE;
	g_return_val_if_fail (child != NULL, FALSE);
	result = TRUE;
	if (iter) {
		*iter = _vala_iter;
	}
	return result;
}


VMDirectory*
vm_directory_construct (GType object_type)
{
	VMDirectory* self = (VMDirectory*) g_object_new (object_type, NULL);
	return self;
}


VMDirectory*
vm_directory_new (void)
{
	return vm_directory_construct (TYPE_DIRECTORY);
}


static void
vm_directory_class_init (VMDirectoryClass* klass)
{
	vm_directory_parent_class = g_type_class_peek_parent (klass);
	G_OBJECT_CLASS (klass)->finalize = directory_finalize;
}


static void
directory_gtk_tree_model_interface_init (GtkTreeModelIface* iface)
{
	directory_gtk_tree_model_parent_iface = g_type_interface_peek_parent (iface);

	iface->get_column_type = (GType    (*)(GtkTreeModel*, gint)) directory_real_get_column_type;
	iface->get_flags       = (GtkTreeModelFlags (*)(GtkTreeModel*)) directory_real_get_flags;
	iface->get_iter =        (gboolean (*)(GtkTreeModel*, GtkTreeIter*, GtkTreePath*)) directory_real_get_iter;
	iface->get_n_columns =   (gint     (*)(GtkTreeModel*)) directory_real_get_n_columns;
	iface->get_path =        (GtkTreePath* (*)(GtkTreeModel*, GtkTreeIter*)) directory_real_get_path;
	iface->get_value =       (void     (*)(GtkTreeModel*, GtkTreeIter*, gint, GValue*)) directory_real_get_value;
	iface->iter_children =   (gboolean (*)(GtkTreeModel*, GtkTreeIter*, GtkTreeIter*)) directory_real_iter_children;
	iface->iter_has_child =  (gboolean (*)(GtkTreeModel*, GtkTreeIter*)) directory_real_iter_has_child;
	iface->iter_n_children = (gint     (*)(GtkTreeModel*, GtkTreeIter*)) directory_real_iter_n_children;
	iface->iter_next       = (gboolean (*)(GtkTreeModel*, GtkTreeIter*)) directory_real_iter_next;
	iface->iter_nth_child  = (gboolean (*)(GtkTreeModel*, GtkTreeIter*, GtkTreeIter*, gint)) directory_real_iter_nth_child;
	iface->iter_parent     = (gboolean (*)(GtkTreeModel*, GtkTreeIter*, GtkTreeIter*)) directory_real_iter_parent;
}


static void
vm_directory_init (VMDirectory* self)
{
	self->priv = vm_directory_get_instance_private(self);
}


static void
directory_finalize (GObject* obj)
{
	//VMDirectory* self = G_TYPE_CHECK_INSTANCE_CAST (obj, TYPE_DIRECTORY, VMDirectory);
	G_OBJECT_CLASS (vm_directory_parent_class)->finalize (obj);
}


/*
 *   Look through all items we want to display, and queue a recheck on any
 *   that require it.
 */
static void
queue_interesting (VMDirectory* vmd)
{
	VMDirectoryPrivate* v = vmd->priv;
	DirItem* item;
	ViewIter iter;

	VIEW_IFACE_GET_CLASS(vmd->view)->get_iter(vmd->view, &iter, 0);
	while ((item = iter.next(&iter))) {
		if (item->flags & ITEM_FLAG_NEED_RESCAN_QUEUE) dir_queue_recheck(v->directory, item);
	}
}


void
_update_display (Directory* dir, DirAction action, GPtrArray* items, VMDirectory* vmd)
{
	ViewIface* view = (ViewIface*)vmd->view;
	g_return_if_fail(view);

	switch (action)
	{
		case DIR_ADD:
			dbg(1, "DIR_ADD...");
			VIEW_IFACE_GET_CLASS(view)->add_items(view, items);
			break;
		case DIR_REMOVE:
//			view_delete_if(view, if_deleted, items);
			//toolbar_update_info(filer_window);
			break;
		case DIR_START_SCAN:
			dbg(1, "DIR_START_SCAN");
//			set_scanning_display(fm, TRUE);
//			file_manager__on_dir_changed();
			//toolbar_update_info(filer_window);
			break;
		case DIR_END_SCAN:
			dbg(1, "DIR_END_SCAN");
/*
			//if (filer_window->window->window) gdk_window_set_cursor(filer_window->window->window, NULL);
			set_scanning_display(fm, FALSE);
			//toolbar_update_info(filer_window);
			open_filer_window(fm);

			view_style_changed(view, 0); //just testing

			if (fm->had_cursor && !view_cursor_visible(view))
			{
				ViewIter start;
				view_get_iter(view, &start, 0);
				if (start.next(&start)) view_cursor_to_iter(view, &start);
				view_show_cursor(view);
				fm->had_cursor = FALSE;
			}
			//if (fm->auto_select) display_set_autoselect(filer_window, fm->auto_select);
			//null_g_free(&fm->auto_select);

			fm__create_thumbs(fm);

			if (fm->thumb_queue) start_thumb_scanning(fm);
*/
			break;
		case DIR_UPDATE:
			dbg(1, "DIR_UPDATE");
			VIEW_IFACE_GET_CLASS(view)->update_items(view, items);
			break;
		case DIR_ERROR_CHANGED:
			//filer_set_title(filer_window);
			break;
		case DIR_QUEUE_INTERESTING:
			dbg(1, "DIR_QUEUE_INTERESTING");
			queue_interesting(vmd);
			break;
	}
}


static void
_attach (VMDirectory* vmd)
{
	VMDirectoryPrivate* v = vmd->priv;

	//gdk_window_set_cursor(filer_window->window->window, busy_cursor);
	view_clear(vmd->view);
	v->scanning = TRUE;
	dir_attach(v->directory, (DirCallback)_update_display, vmd);
	//filer_set_title(filer_window);
	//bookmarks_add_history(filer_window->sym_path);

	if (v->directory->error)
	{
#if 0
		if (spring_in_progress)
			g_printerr(_("Error scanning '%s':\n%s\n"),
				filer_window->sym_path,
				fm->directory->error);
		else
#endif
			//delayed_error(_("Error scanning '%s':\n%s"), filer_window->sym_path, filer_window->directory->error);
			dbg(0, "Error scanning '%s':\n%s", v->sym_path, v->directory->error);
	}
}


static void
detach (VMDirectory* vmd)
{
	VMDirectoryPrivate* v = vmd->priv;
	g_return_if_fail(v->directory);

	dir_detach(v->directory, (DirCallback)_update_display, vmd);
	_g_object_unref0(v->directory);
}


/*
 *  Remove trailing /s from path (modified in place)
 */
static void
tidy_sympath (gchar* path)
{
	g_return_if_fail(path != NULL);

	int l = strlen(path);
	while (l > 1 && path[l - 1] == '/') {
		l--;
		path[l] = '\0';
	}
}


/*
 *   Make filer_window display path. When finished, highlight item 'from', or
 *   the first item if from is NULL. If there is currently no cursor then
 *   simply wink 'from' (if not NULL).
 *   If the cause was a key event and we resize, warp the pointer.
 */
void
//fm__change_to(AyyiLibfilemanager* fm, const char* path, const char* from)
vm_directory_set_path (VMDirectory* vmd, const char* path)
{
											const char* from = NULL;
	//dir_rescan();

	VMDirectoryPrivate* v = vmd->priv;

	dbg(2, "%s", path);
	g_return_if_fail(vmd);

	//fm__cancel_thumbnails(fm);

	char* sym_path = g_strdup(path);
	char* real_path = pathdup(path);
	Directory* new_dir = g_fscache_lookup(dir_cache, real_path);

	if (!new_dir) {
		dbg(0, "directory '%s' is not accessible", sym_path);
		g_free(real_path);
		g_free(sym_path);
		return;
	}

	char* from_dup = from && *from ? g_strdup(from) : NULL;

	if(v->directory) detach(vmd);
	g_free(v->real_path);
	g_free(v->sym_path);
	v->real_path = real_path;
	v->sym_path = sym_path;
	tidy_sympath(v->sym_path);

	v->directory = new_dir;

	g_free(v->auto_select);
	v->auto_select = from_dup;

	//had_cursor = had_cursor || view_cursor_visible(fm->view);

	//filer_set_title(filer_window);
	//if (filer_window->window->window) gdk_window_set_role(filer_window->window->window, filer_window->sym_path);
	view_cursor_to_iter(vmd->view, NULL);

	_attach(vmd);
}


static inline gboolean
is_hidden(const char* dir, DirItem* item)
{
	// If the leaf name starts with '.' then the item is hidden
	if(item->leafname[0] == '.')
		return TRUE;

	return FALSE;
}


gboolean
vm_directory_match_filter(VMDirectory* vmd, DirItem* item)
{
	g_return_val_if_fail(item, FALSE);
	VMDirectoryPrivate* v = vmd->priv;

	if(is_hidden(v->real_path, item) && (!v->temp_show_hidden && !v->show_hidden)){
		return FALSE;
	}

	switch(v->filter) {
		case FILER_SHOW_GLOB:
			return fnmatch(v->filter_string, item->leafname, 0)==0 || (item->base_type==TYPE_DIRECTORY && !v->filter_directories);

		case FILER_SHOW_ALL:
		default:
			break;
	}
	return TRUE;
}


