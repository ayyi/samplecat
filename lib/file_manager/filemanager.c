/*
 +----------------------------------------------------------------------+
 | This file is part of the Ayyi project. https://www.ayyi.org          |
 | copyright (C) 2011-2025 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#include <glib.h>
#include <glib-object.h>
#include <stdlib.h>
#include <string.h>
#include <fnmatch.h>
#include <gtk/gtk.h>
#include "debug/debug.h"
#include "dnd/dnd.h"
#include "file_manager/file_manager.h"
#include "file_manager/mimetype.h"
#include "file_manager/fscache.h"
#include "pixmaps.h"
#include "minibuffer.h"
#include "filemanager.h"

extern GList* all_filer_windows;
extern char theme_name[];

#define _g_free0(var) ((var == NULL) ? NULL : (var = (g_free(var), NULL)))
#define _g_object_unref0(var) ((var == NULL) ? NULL : (var = (g_object_unref (var), NULL)))
#define g_source_remove0(S) {if(S) g_source_remove(S); S = 0;}

static gpointer ayyi_filemanager_parent_class = NULL;

enum  {
	AYYI_LIBFILEMANAGER_DUMMY_PROPERTY
};

static GObject* ayyi_filemanager_constructor (GType, guint n_construct_properties, GObjectConstructParam*);
static void     ayyi_filemanager_finalize    (GObject*);

static void     update_display               (Directory*, DirAction, GPtrArray*, AyyiFilemanager*);
static void     detach                       (AyyiFilemanager*);
static void     tidy_sympath                 (gchar*);
static void     fm_next_thumb                (GObject*, const gchar* path);
static void     start_thumb_scanning         (AyyiFilemanager*);
static void     fm_connect_signals           (AyyiFilemanager*);

#ifdef GTK4_TODO
static GdkCursor* crosshair = NULL; // TODO is never set
#endif


AyyiFilemanager*
ayyi_filemanager_construct (GType object_type)
{
	AyyiFilemanager* self = (AyyiFilemanager*) g_object_new(object_type, NULL);

	self->sort_type = SORT_NAME;
	self->display_style_wanted = SMALL_ICONS;
	self->filter = FILER_SHOW_ALL;
	self->window = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

	fm_connect_signals(self);

#ifdef USE_MINIBUFFER
	create_minibuffer(self);
	gtk_box_append(GTK_BOX(self->window), self->mini.area);
#endif

	return self;
}


AyyiFilemanager*
ayyi_filemanager_new ()
{
	return ayyi_filemanager_construct (AYYI_TYPE_FILEMANAGER);
}


GtkWidget*
ayyi_filemanager_new_window (AyyiFilemanager* self, const gchar* path)
{
	g_return_val_if_fail (self, NULL);
	g_return_val_if_fail (path, NULL);

	fm__update_dir (self, TRUE);

	return NULL;
}


void
ayyi_filemanager_emit_dir_changed (AyyiFilemanager* self)
{
	g_return_if_fail (self);

	g_signal_emit_by_name (self, "dir-changed", self->real_path);
}


static GObject*
ayyi_filemanager_constructor (GType type, guint n_construct_properties, GObjectConstructParam* construct_properties)
{
	GObjectClass* parent_class = G_OBJECT_CLASS (ayyi_filemanager_parent_class);
	return parent_class->constructor (type, n_construct_properties, construct_properties);
}


static void
ayyi_filemanager_class_init (AyyiFilemanagerClass* klass)
{
	ayyi_filemanager_parent_class = g_type_class_peek_parent (klass);

	G_OBJECT_CLASS (klass)->constructor = ayyi_filemanager_constructor;
	G_OBJECT_CLASS (klass)->finalize = ayyi_filemanager_finalize;

	g_signal_new ("dir_changed", AYYI_TYPE_FILEMANAGER, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__STRING, G_TYPE_NONE, 1, G_TYPE_STRING);

	klass->filetypes = g_hash_table_new(g_str_hash, g_direct_equal);
}


static void
ayyi_filemanager_instance_init (AyyiFilemanager* self)
{
}


static void
ayyi_filemanager_finalize (GObject* obj)
{
	G_OBJECT_CLASS (ayyi_filemanager_parent_class)->finalize (obj);
}


GType
ayyi_filemanager_get_type (void)
{
	static volatile gsize ayyi_filemanager_type_id__volatile = 0;
	if (g_once_init_enter ((gsize*)&ayyi_filemanager_type_id__volatile)) {
		static const GTypeInfo g_define_type_info = { sizeof (AyyiFilemanagerClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) ayyi_filemanager_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (AyyiFilemanager), 0, (GInstanceInitFunc) ayyi_filemanager_instance_init, NULL };
		GType ayyi_filemanager_type_id;
		ayyi_filemanager_type_id = g_type_register_static (G_TYPE_OBJECT, "AyyiFilemanager", &g_define_type_info, 0);
		g_once_init_leave (&ayyi_filemanager_type_id__volatile, ayyi_filemanager_type_id);
	}
	return ayyi_filemanager_type_id__volatile;
}


/*
 *   Make filer_window display path. When finished, highlight item 'from', or
 *   the first item if from is NULL. If there is currently no cursor then
 *   simply wink 'from' (if not NULL).
 *   If the cause was a key event and we resize, warp the pointer.
 */
void
fm__change_to (AyyiFilemanager* fm, const char* path, const char* from)
{
	dbg(2, "%s", path);
	g_return_if_fail(fm);

	fm__cancel_thumbnails(fm);

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

	if (fm->directory) detach(fm);
	g_free(fm->real_path);
	g_free(fm->sym_path);
	fm->real_path = real_path;
	fm->sym_path = sym_path;
	tidy_sympath(fm->sym_path);

	fm->directory = new_dir;

	g_free(fm->auto_select);
	fm->auto_select = from_dup;

	//had_cursor = had_cursor || view_cursor_visible(fm->view);

	//filer_set_title(filer_window);
	view_cursor_to_iter(fm->view, NULL);

	attach(fm);

	//check_settings(filer_window);

	fm__menu_on_view_change(fm);
}


void
fm__change_to_parent (AyyiFilemanager* fm)
{
	const char* current = fm->sym_path;

	if (current[0] == '/' && current[1] == '\0') { // Already at root
		return;
	}

	char* dir = g_path_get_dirname(current);
	char* basename = g_path_get_basename(current);
	fm__change_to(fm, dir, basename);
	g_free(basename);
	g_free(dir);
}


#ifdef GTK4_TODO
static void
set_selection_state (AyyiFilemanager* fm, gboolean normal)
{
#ifdef GTK4_TODO
	GtkStateType old_state = fm->selection_state;

	fm->selection_state = normal ? GTK_STATE_SELECTED : GTK_STATE_INSENSITIVE;

	if (old_state != fm->selection_state && view_count_selected(fm->view))
		gtk_widget_queue_draw(GTK_WIDGET(fm->view));
#endif
}
#endif


/* Selection has been changed -- try to grab the primary selection
 * if we don't have it. Also called when clicking on an insensitive selection
 * to regain primary.
 * Also updates toolbar info.
 */
void
fm__selection_changed (AyyiFilemanager* fm, gint time)
{
	g_return_if_fail(fm);

	//toolbar_update_info(filer_window);

	//if (window_with_primary == filer_window) return;		// Already got primary

	if (!view_count_selected(fm->view)) return;             // Nothing selected

#ifdef GTK4_TODO
	if (fm->temp_item_selected == FALSE &&
		gtk_selection_owner_set(GTK_WIDGET(fm->window), GDK_SELECTION_PRIMARY, time))
	{
		//window_with_primary = filer_window;
		set_selection_state(fm, TRUE);
	}
	else
		set_selection_state(fm, FALSE);
#endif
}


/* Returns a list containing the full (sym) pathname of every selected item.
 * You must g_free() each item in the list.
 */
GList*
fm__selected_items (AyyiFilemanager* fm)
{
	GList* retval = NULL;
	guchar* dir = (guchar*)fm->sym_path;
	ViewIter iter;
	DirItem *item;

	view_get_iter(fm->view, &iter, VIEW_ITER_SELECTED);
	while ((item = iter.next(&iter)))
	{
		retval = g_list_prepend(retval, g_strdup((char*)make_path((char*)dir, item->leafname)));
	}

	return g_list_reverse(retval);
}


/* Move the cursor to the next selected item in direction 'dir'
 * (+1 or -1).
 */
void
fm__next_selected (AyyiFilemanager* fm, int dir)
{
	ViewIter iter, cursor;
	ViewIface* view = fm->view;

	g_return_if_fail(dir == 1 || dir == -1);

	view_get_cursor(view, &cursor);
	gboolean have_cursor = cursor.peek(&cursor) != NULL;

	view_get_iter(view, &iter,
			VIEW_ITER_SELECTED |
			(have_cursor ? VIEW_ITER_FROM_CURSOR : 0) | 
			(dir < 0 ? VIEW_ITER_BACKWARDS : 0));

	if (have_cursor && view_get_selected(view, &cursor))
		iter.next(&iter);	/* Skip the cursor itself */

	if (iter.next(&iter))
		view_cursor_to_iter(view, &iter);
#ifdef GTK4_TODO
	else
		gdk_beep();
#endif

	return;
}

/* Note that filer_window may not exist after this call.
 * Returns TRUE if the directory still exists.
 */
gboolean
fm__update_dir (AyyiFilemanager* fm, gboolean warning)
{
	/*gboolean still_exists = may_rescan(filer_window, warning);

	if (still_exists)*/ dir_update(fm->directory, fm->sym_path);

	//return still_exists;
	return TRUE;
}


void
fm__cancel_thumbnails (AyyiFilemanager* fm)
{
	//gtk_widget_hide(filer_window->thumb_bar);

	destroy_glist(&fm->thumb_queue);
	fm->max_thumbs = 0;
}


/*
 *  Open the item (or add it to the shell command minibuffer)
 */
void
fm__open_item (AyyiFilemanager* fm, ViewIter *iter, OpenFlags flags)
{
#if 0
	gboolean shift = (flags & OPEN_SHIFT) != 0;
	gboolean close_mini = flags & OPEN_FROM_MINI;
	gboolean close_window = (flags & OPEN_CLOSE_WINDOW) != 0;
#endif

	g_return_if_fail(fm && iter);

	DirItem* item = iter->peek(iter);

	g_return_if_fail(item);

	if (fm->mini.type == MINI_SHELL)
	{
#ifdef GTK4_TODO
		minibuffer_add(fm, item->leafname);
#endif
		return;
	}

	if (item->base_type == TYPE_UNKNOWN)
		dir_update_item(fm->directory, item->leafname);

#if 0
	const guchar* full_path = make_path(fm->sym_path, item->leafname);
	if (shift && (item->flags & ITEM_FLAG_SYMLINK))
		wink = FALSE;

	gboolean wink = TRUE;
	Directory* old_dir = fm->directory;
	if (run_diritem(full_path, item, flags & OPEN_SAME_WINDOW ? filer_window : NULL, filer_window, shift))
	{
		if (old_dir != fm->directory)
			return;

		if (close_window)
			gtk_widget_destroy(fm->window);
		else
		{
#if 0
			if (wink)
				view_wink_item(filer_window->view, iter);
#endif
			if (close_mini)
				minibuffer_hide(filer_window);
		}
	}
#endif
}


/*
 *  Puts the filer window into target mode. When an item is chosen,
 *  fn(filer_window, iter, data) is called. 'reason' will be displayed
 *  on the toolbar while target mode is active.
 *
 *  Use fn == NULL to cancel target mode.
 */
void
fm__target_mode (AyyiFilemanager* fm, TargetFunc fn, gpointer data, const char *reason)
{
#ifdef GTK4_TODO
	TargetFunc old_fn = fm->target_cb;

	if (fn != old_fn)
		gdk_window_set_cursor(GTK_WIDGET(fm->view)->window, fn ? crosshair : NULL);
#endif

	fm->target_cb = fn;
	fm->target_data = data;

	if (fm->toolbar_text == NULL)
		return;

	if (fn)
		gtk_label_set_text(GTK_LABEL(fm->toolbar_text), reason);
	else if (FALSE) {
#if 0
		if (old_fn)
			toolbar_update_info(fm);
#endif
	}
	else
		gtk_label_set_text(GTK_LABEL(fm->toolbar_text), "");
}


static inline gboolean
is_hidden (const char* dir, DirItem* item)
{
	/* If the leaf name starts with '.' then the item is hidden */
	if (item->leafname[0] == '.')
		return TRUE;

	return FALSE;
}


gboolean
fm__match_filter (AyyiFilemanager* fm, DirItem* item)
{
	g_return_val_if_fail(item, FALSE);

	if (is_hidden(fm->real_path, item) && (!fm->temp_show_hidden && !fm->show_hidden)) {
		return FALSE;
	}

	switch (fm->filter) {
		case FILER_SHOW_GLOB:
			return fnmatch(fm->filter_string, item->leafname, 0)==0 || (item->base_type==TYPE_DIRECTORY && !fm->filter_directories);

		case FILER_SHOW_ALL:
		default:
			break;
	}
	return TRUE;
}


/*
 *   Set the filter type. Returns TRUE if the type has changed
 *   (must call filer_detach_rescan).
 */
gboolean
fm__set_filter (AyyiFilemanager* fm, FilterType type, const gchar* filter_string)
{
	// Is this new filter the same as the old one?
	if (fm->filter == type)
	{
		switch (fm->filter) {
			case FILER_SHOW_ALL:
				return FALSE;
			case FILER_SHOW_GLOB:
				if (strcmp(fm->filter_string, filter_string) == 0)
					return FALSE;
				break;
		}
	}

	// Clean up old filter
	if (fm->filter_string)
	{
		g_free(fm->filter_string);
		fm->filter_string = NULL;
	}
	// Also clean up compiled regexp when implemented

	fm->filter = type;

	switch (type)
	{
		case FILER_SHOW_ALL:
			// No extra work
			break;

		case FILER_SHOW_GLOB:
			fm->filter_string = g_strdup(filter_string);
			break;

		default:
			fm->filter = FILER_SHOW_ALL;
			g_warning("Impossible: filter type %d", type);
			break;
	}

	return TRUE;
}


/* Set this image to be loaded some time in the future */
void
fm__create_thumb (AyyiFilemanager* fm, const gchar *path)
{
	dbg(0, "%s", path);
	if (g_list_find_custom(fm->thumb_queue, path, (GCompareFunc) strcmp))	return;

	if (!fm->thumb_queue) fm->max_thumbs=0;
	fm->max_thumbs++;

	fm->thumb_queue = g_list_append(fm->thumb_queue, g_strdup(path));

	if (fm->scanning) return; /* Will start when scan ends */

	start_thumb_scanning(fm);
}


/* If thumbnail display is on, look through all the items in this directory
 * and start creating or updating the thumbnails as needed.
 */
void
fm__create_thumbs (AyyiFilemanager* fm)
{
	if (!fm->show_thumbs) return;

	ViewIter iter;
	view_get_iter(fm->view, &iter, 0);

	DirItem *item;
	while ((item = iter.next(&iter)))
	{
		gboolean found;

		if (item->base_type != TYPE_FILE) continue;

		/*if (strcmp(item->mime_type->media_type, "image") != 0) continue;*/

		const guchar* path = make_path(fm->real_path, item->leafname);

		MaskedPixmap* pixmap = g_fscache_lookup_full(pixmap_cache, (char*)path, FSCACHE_LOOKUP_ONLY_NEW, &found);
		if (pixmap){ g_object_unref(pixmap); dbg(0, "unreffing..."); pixmap = NULL; }

		/* If we didn't get an image, it could be because:
		 *
		 * - We're loading the image now. found is TRUE,
		 *   and we'll update the item later.
		 * - We tried to load the image and failed. found
		 *   is TRUE.
		 * - We haven't tried loading the image. found is
		 *   FALSE, and we start creating the thumb here.
		 */
		if (!found) fm__create_thumb(fm, (char*)path);
	}
}


/*static */void
attach (AyyiFilemanager* fm)
{
	//gdk_window_set_cursor(filer_window->window->window, busy_cursor);

	view_clear(fm->view);
	fm->scanning = TRUE;
	dir_attach(fm->directory, (DirCallback) update_display, fm);

	//filer_set_title(filer_window);
	//bookmarks_add_history(filer_window->sym_path);

	if (fm->directory->error)
	{
#if 0
		if (spring_in_progress)
			g_printerr(_("Error scanning '%s':\n%s\n"),
				filer_window->sym_path,
				fm->directory->error);
		else
#endif
			//delayed_error(_("Error scanning '%s':\n%s"), filer_window->sym_path, filer_window->directory->error);
			dbg(0, "Error scanning '%s':\n%s", fm->sym_path, fm->directory->error);
	}
}


static void
detach (AyyiFilemanager* fm)
{
	g_return_if_fail(fm->directory);

	dir_detach(fm->directory, (DirCallback)update_display, fm);
	g_clear_pointer(&fm->directory, g_object_unref);
}


/*
 *   Reconnect to the same directory (used when the Show Hidden option is
 *   toggled). This has the side-effect of updating the window title.
 */
void
filer_detach_rescan (AyyiFilemanager* fm)
{
	Directory* dir = fm->directory;

	g_object_ref(dir);
	detach(fm);
	fm->directory = dir;
	attach(fm);
}


/*
 *   Remove trailing /s from path (modified in place)
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
 *   Look through all items we want to display, and queue a recheck on any
 *   that require it.
 */
static void
queue_interesting (AyyiFilemanager* fm)
{
	DirItem* item;
	ViewIter iter;

	view_get_iter(fm->view, &iter, 0);
	while ((item = iter.next(&iter))) {
		if (item->flags & ITEM_FLAG_NEED_RESCAN_QUEUE) dir_queue_recheck(fm->directory, item);
	}
}


/* Generate the next thumb for this window. The window object is
 * unref'd when there is nothing more to do.
 * If the window no longer has a filer window, nothing is done.
 */
static gboolean
fm_next_thumb_real (GObject* window)
{
	AyyiFilemanager* fm = g_object_get_data(window, "file_window");

	if (!fm) {
		g_object_unref(window);
		return FALSE;
	}

	if (!fm->thumb_queue) {
		fm__cancel_thumbnails(fm);
		g_object_unref(window);
		return FALSE;
	}

	gchar* path = (gchar *) fm->thumb_queue->data;

	pixmap_background_thumb(path, (GFunc)fm_next_thumb, window);

	fm->thumb_queue = g_list_remove(fm->thumb_queue, path);
	g_free(path);

#if 0
	int total = fm->max_thumbs;
	int done = total - g_list_length(fm->thumb_queue);

	gtk_progress_bar_set_fraction( GTK_PROGRESS_BAR(filer_window->thumb_progress), done / (float) total);
#endif

	return FALSE;
}


/* path is the thumb just loaded, if any.
 * window is unref'd (eventually).
 */
static void
fm_next_thumb (GObject* window, const gchar *path)
{
	if (path) dir_force_update_path(path);

	g_idle_add((GSourceFunc)fm_next_thumb_real, window);
}


static void
start_thumb_scanning (AyyiFilemanager* fm)
{
	dbg(0, "FIXME add scanning flag");
	//if (GTK_WIDGET_VISIBLE(filer_window->thumb_bar)) return; /* Already scanning */

	g_object_ref(G_OBJECT(fm->window));
	fm_next_thumb(G_OBJECT(fm->window), NULL);
}


/* Called on a timeout while scanning or when scanning ends
 * (whichever happens first).
 */
static gboolean
open_filer_window (AyyiFilemanager* fm)
{
	PF;

/*
	Settings* dir_settings = (Settings *) g_hash_table_lookup(settings_table, filer_window->sym_path);

	force_resize = !(o_filer_auto_resize.int_value == RESIZE_NEVER &&
			 dir_settings && dir_settings->flags & SET_POSITION);
*/
	view_style_changed(fm->view, 0);

	g_source_remove0(fm->open_timeout);

/*
	gboolean force_resize;

	if (!GTK_WIDGET_VISIBLE(filer_window->window))
	{
		display_set_actual_size(filer_window, force_resize);
		gtk_widget_show(filer_window->window);
	}
*/
	return G_SOURCE_REMOVE;
}


static gboolean
if_deleted (gpointer item, gpointer removed)
{
	int	i = ((GPtrArray *) removed)->len;
	DirItem** r = (DirItem **) ((GPtrArray *) removed)->pdata;
	char* leafname = ((DirItem *) item)->leafname;

	while (i--) {
		if (strcmp(leafname, r[i]->leafname) == 0)
			return TRUE;
	}

	return FALSE;
}


static void
set_scanning_display (AyyiFilemanager* fm, gboolean scanning)
{
	if (scanning == fm->scanning) return;
	fm->scanning = scanning;

/*
	if (scanning) filer_set_title(filer_window);
	else          g_timeout_add(300, (GSourceFunc) clear_scanning_display, filer_window);
*/
}


static void
update_display (Directory* dir, DirAction action, GPtrArray* items, AyyiFilemanager* fm)
{
	ViewIface* view = (ViewIface*)fm->view;
	g_return_if_fail(view);

	switch (action)
	{
		case DIR_ADD:
			dbg(2, "DIR_ADD...");
			view_add_items(view, items);
			break;
		case DIR_REMOVE:
			view_delete_if(view, if_deleted, items);
			//toolbar_update_info(filer_window);
			break;
		case DIR_START_SCAN:
			dbg(2, "DIR_START_SCAN");
			set_scanning_display(fm, TRUE);
			file_manager__on_dir_changed();
			//toolbar_update_info(filer_window);
			break;
		case DIR_END_SCAN:
			dbg(2, "DIR_END_SCAN");
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
			break;
		case DIR_UPDATE:
			dbg(2, "DIR_UPDATE");
			view_update_items(view, items);
			break;
		case DIR_ERROR_CHANGED:
			//filer_set_title(filer_window);
			break;
		case DIR_QUEUE_INTERESTING:
			dbg(2, "DIR_QUEUE_INTERESTING");
			queue_interesting(fm);
			break;
	}
}


/* Someone wants us to send them the selection */
#ifdef GTK4_TODO
static void
selection_get (GtkWidget* widget, GtkSelectionData* selection_data, guint info, guint time, gpointer data)
{
	AyyiFilemanager* fm = (AyyiFilemanager*)data;
	ViewIter iter;

	GString* reply = g_string_new(NULL);
	GString* header = g_string_new(NULL);

	switch (info) {
#if 0
		case TARGET_STRING:
			g_string_printf(header, " %s", make_path(filer_window->sym_path, ""));
			break;
#endif
		case TARGET_URI_LIST:
			g_string_printf(header, " file://%s%s", our_host_name_for_dnd(), make_path(fm->sym_path, ""));
			break;
	}

	view_get_iter(fm->view, &iter, VIEW_ITER_SELECTED);

	DirItem* item;
	while ((item = iter.next(&iter))) {
		g_string_append(reply, header->str);
		g_string_append(reply, item->leafname);
	}

	if (reply->len > 0)
		gtk_selection_data_set_text(selection_data, reply->str + 1, reply->len - 1);
	else
	{
		g_warning("Attempt to paste empty selection!");
		gtk_selection_data_set_text(selection_data, "", 0);
	}

	g_string_free(reply, TRUE);
	g_string_free(header, TRUE);
}
#endif


static void
filer_window_destroyed (GtkWidget* widget, AyyiFilemanager* fm)
{
	all_filer_windows = g_list_remove(all_filer_windows, fm);

	g_object_set_data(G_OBJECT(widget), "file_window", NULL);

#if 0
	if (window_with_primary == filer_window)
		window_with_primary = NULL;

	if (window_with_focus == filer_window)
	{
		menu_popdown();
		window_with_focus = NULL;
	}
#endif

	if (fm->directory) detach(fm);

	g_source_remove0(fm->open_timeout);

#if 0
	if (fm->auto_scroll != -1)
	{
		g_source_remove(fm->auto_scroll);
		fm->auto_scroll = -1;
	}
#endif

	if (fm->thumb_queue)
		destroy_glist(&fm->thumb_queue);

#if 0
	filer_set_id(filer_window, NULL);
#endif

	_g_free0(fm->filter_string);
	_g_free0(fm->regexp);
	_g_free0(fm->auto_select);
	_g_free0(fm->real_path);
	_g_free0(fm->sym_path);
	g_object_unref(fm);

#if 0
	one_less_window();
#endif
}

static void
fm_connect_signals (AyyiFilemanager* fm)
{
#if 0
	GtkTargetEntry 	target_table[] =
	{
		{"text/uri-list", 0, TARGET_URI_LIST},
		{"UTF8_STRING", 0, TARGET_STRING},
		{"STRING", 0, TARGET_STRING},
		{"COMPOUND_TEXT", 0, TARGET_STRING},/* XXX: Treats as STRING */
	};

	/* Events on the top-level window */
	gtk_widget_add_events(fm->window, GDK_ENTER_NOTIFY);
	g_signal_connect(fm->window, "enter-notify-event", G_CALLBACK(pointer_in), fm->file_window);
	g_signal_connect(fm->window, "leave-notify-event", G_CALLBACK(pointer_out), filer_window);
#endif

	g_signal_connect(fm->window, "destroy", G_CALLBACK(filer_window_destroyed), fm);

#if 0
	g_signal_connect(fm->window, "selection_clear_event", G_CALLBACK(filer_lost_primary), filer_window);
#endif

#ifdef GTK4_TODO
	g_signal_connect(fm->window, "selection_get", G_CALLBACK(selection_get), fm);
#endif
#if 0
	gtk_selection_add_targets(GTK_WIDGET(fm->window), GDK_SELECTION_PRIMARY, target_table, sizeof(target_table) / sizeof(*target_table));

	g_signal_connect(fm->window, "popup-menu", G_CALLBACK(popup_menu), fm->file_window);
	g_signal_connect(fm->window, "key_press_event", G_CALLBACK(filer_key_press_event), fm->file_window);

	gtk_window_add_accel_group(GTK_WINDOW(fm->window), filer_keys);
#endif
}


/* TRUE iff filer_window points to an existing FilerWindow
 * structure.
 */
gboolean
fm__exists(AyyiFilemanager* fm)
{
	GList* next;

	for (next = all_filer_windows; next; next = next->next) {
		AyyiFilemanager* fw = (AyyiFilemanager*) next->data;

		if (fw == fm)
			return TRUE;
	}

	return FALSE;
}

