/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
 | copyright (C) 2007-2023 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#include "config.h"
#include <gtk/gtk.h>
#include "debug/debug.h"
#ifdef USE_AYYI
#include <ayyi/ayyi.h>
#endif
#include "file_manager/support.h"
#include "file_manager/mimetype.h"
#include "file_manager/pixmaps.h"
#include "gtk/cellrenderer_hypertext.h"

#include "typedefs.h"
#include "types.h"
#include "support.h"
#include "model.h"
#include "application.h"
#include "sample.h"
#include "dnd.h"
#include "worker.h"
#include "library.h"

static LibraryView* instance;

#define START_EDITING 1

#ifdef GTK4_TODO
static gboolean     listview__on_row_clicked          (GtkWidget*, GdkEventButton*, gpointer);
#endif
static void         listview__on_cursor_change        (GtkTreeView*, gpointer);
static void         listview__on_store_changed        (GtkListStore*, LibraryView*);
#ifdef GTK4_TODO
static void         listview__dnd_get                 (GtkWidget*, GdkDragContext*, GtkSelectionData*, guint info, guint time, gpointer);
static gint         listview__drag_received           (GtkWidget*, GdkDragContext*, gint x, gint y, GtkSelectionData*, guint info, guint time, gpointer);
#endif
static void         listview__on_realise              (GtkWidget*, gpointer);
#ifdef GTK4_TODO
static gboolean     listview__on_motion               (GtkWidget*, GdkEventMotion*, gpointer);
#endif
static void         listview__on_keywords_edited      (GtkCellRendererText*, gchar* path_string, gchar* new_text, gpointer);
static void         listview__path_cell_data          (GtkTreeViewColumn*, GtkCellRenderer*, GtkTreeModel*, GtkTreeIter*, gpointer);
static void         listview__tag_cell_data           (GtkTreeViewColumn*, GtkCellRenderer*, GtkTreeModel*, GtkTreeIter*, gpointer);
static void         listview__cell_data_bg            (GtkTreeViewColumn*, GtkCellRenderer*, GtkTreeModel*, GtkTreeIter*, gpointer);
static GtkTreePath* listview__get_first_selected_path ();
static Sample*      listview__get_first_selected_sample();
static void         listview__unblock_motion_handler  ();
#if NEVER
static void         cell_bg_lighter                   (GtkTreeViewColumn*, GtkCellRenderer*, GtkTreeModel*, GtkTreeIter*);
static int          listview__path_get_id             (GtkTreePath*);
static gboolean     treeview_get_tags_cell            (GtkTreeView*, guint x, guint y, GtkCellRenderer**);
#endif
static void         listview__highlight_playing_by_path(GtkTreePath*);
static void         listview__highlight_playing_by_ref(GtkTreeRowReference*);

static void         listview__edit_row                (GSimpleAction*, GVariant*, gpointer);
static void         listview__reset_colours           (GSimpleAction*, GVariant*, gpointer);
static void         listview__update_selected         (GSimpleAction*, GVariant*, gpointer);
static void         listview__delete_selected         ();


/*
 *  The main samplecat pane. A treeview with a list of samples.
 */
GtkWidget*
listview__new ()
{
	LibraryView* lv = instance = g_new0(LibraryView, 1);

	lv->scroll = scrolled_window_new();

	GtkWidget* view = lv->widget = gtk_tree_view_new_with_model(GTK_TREE_MODEL(samplecat.store));
	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(lv->scroll), view);
	g_signal_connect(view, "realize", G_CALLBACK(listview__on_realise), NULL);
#ifdef GTK4_TODO
	g_signal_connect(view, "motion-notify-event", (GCallback)listview__on_motion, NULL);
	g_signal_connect(view, "drag-data-received", G_CALLBACK(listview__drag_received), NULL); // currently the window traps this before we get here.
	g_signal_connect(view, "drag-motion", G_CALLBACK(drag_motion), NULL);
#endif

	// set up as dnd source
#ifdef GTK4_TODO
	gtk_drag_source_set(view, GDK_BUTTON1_MASK | GDK_BUTTON2_MASK, dnd_file_drag_types, dnd_file_drag_types_count, GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_ASK);
	g_signal_connect(G_OBJECT(view), "drag_data_get", G_CALLBACK(listview__dnd_get), NULL);
#endif

	// icon
	GtkCellRenderer* cell9 = gtk_cell_renderer_pixbuf_new();
	GtkTreeViewColumn* col9 = gtk_tree_view_column_new_with_attributes("", cell9, "pixbuf", COL_ICON, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col9);
	g_object_set(G_OBJECT(cell9), "xalign", 0.0, NULL);
	gtk_tree_view_column_set_resizable(col9, TRUE);
	gtk_tree_view_column_set_min_width(col9, 0);
	gtk_tree_view_column_set_cell_data_func(col9, cell9, listview__cell_data_bg, NULL, NULL);

#ifdef SHOW_INDEX
	GtkCellRenderer* cell0 = gtk_cell_renderer_text_new();
	GtkTreeViewColumn* col0 = gtk_tree_view_column_new_with_attributes("Id", cell0, "text", COL_IDX, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col0);
#endif

	GtkCellRenderer* cell1 = lv->cells.name = gtk_cell_renderer_text_new();
	GtkTreeViewColumn* col1 = lv->col_name = gtk_tree_view_column_new_with_attributes("Sample Name", cell1, "text", COL_NAME, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col1);
	gtk_tree_view_column_set_sort_column_id(col1, COL_NAME);
	gtk_tree_view_column_set_reorderable(col1, TRUE);
	gtk_tree_view_column_set_min_width(col1, 0);
	gtk_tree_view_column_set_cell_data_func(col1, cell1, (gpointer)listview__cell_data_bg, NULL, NULL);

	gtk_tree_view_column_set_sizing(col1, GTK_TREE_VIEW_COLUMN_FIXED);
	int width = atoi(app->config.column_widths[0]);
	if(width > 0) gtk_tree_view_column_set_fixed_width(col1, MAX(width, 30)); //FIXME set range in config section instead.
	else gtk_tree_view_column_set_fixed_width(col1, 130);

	GtkCellRenderer* cell2 = gtk_cell_renderer_text_new();
	GtkTreeViewColumn* col2 = lv->col_path = gtk_tree_view_column_new_with_attributes("Path", cell2, "text", COL_FNAME, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col2);
	gtk_tree_view_column_set_sort_column_id(col2, COL_FNAME);
	gtk_tree_view_column_set_reorderable(col2, TRUE);
	gtk_tree_view_column_set_min_width(col2, 0);
	gtk_tree_view_column_set_cell_data_func(col2, cell2, listview__path_cell_data, NULL, NULL);
#ifdef USE_AYYI
	// icon that shows when file is in current active song
	GtkCellRenderer* ayyi_renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(col2, ayyi_renderer, FALSE);
	gtk_tree_view_column_add_attribute(col2, ayyi_renderer, "pixbuf", COL_AYYI_ICON);
#endif

	gtk_tree_view_column_set_sizing(col2, GTK_TREE_VIEW_COLUMN_FIXED);
	width = atoi(app->config.column_widths[1]);
	if(width > 0) gtk_tree_view_column_set_fixed_width(col2, MAX(width, 30));
	else gtk_tree_view_column_set_fixed_width(col2, 130);

	GtkCellRenderer* cell3 = lv->cells.tags = gtk_cell_renderer_hypertext_new();
	GtkTreeViewColumn* column3 = lv->col_tags = gtk_tree_view_column_new_with_attributes("Tags", cell3, "text", COL_KEYWORDS, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), column3);
	gtk_tree_view_column_set_sort_column_id(column3, COL_KEYWORDS);
	gtk_tree_view_column_set_resizable(column3, TRUE);
	gtk_tree_view_column_set_reorderable(column3, TRUE);
	gtk_tree_view_column_set_min_width(column3, 0);
	g_object_set(cell3, "editable", TRUE, NULL);
	g_signal_connect(cell3, "edited", (GCallback)listview__on_keywords_edited, NULL);
	gtk_tree_view_column_add_attribute(column3, cell3, "markup", COL_KEYWORDS);
	gtk_tree_view_column_set_cell_data_func(column3, cell3, listview__tag_cell_data, NULL, NULL);

	GtkCellRenderer* cell4 = gtk_cell_renderer_pixbuf_new();
	GtkTreeViewColumn* col4 = lv->col_pixbuf = gtk_tree_view_column_new_with_attributes("Overview", cell4, "pixbuf", COL_OVERVIEW, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col4);
	g_object_set(G_OBJECT(cell4), "xalign", 0.0, NULL);
	gtk_tree_view_column_set_resizable(col4, TRUE);
	gtk_tree_view_column_set_min_width(col4, 0);
	gtk_tree_view_column_set_cell_data_func(col4, cell4, listview__cell_data_bg, NULL, NULL);

	GtkCellRenderer* cell5 = gtk_cell_renderer_text_new();
	GtkTreeViewColumn* col5 = gtk_tree_view_column_new_with_attributes("Length", cell5, "text", COL_LENGTH, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col5);
	gtk_tree_view_column_set_sort_column_id(col5, COL_LEN);
	gtk_tree_view_column_set_resizable(col5, TRUE);
	gtk_tree_view_column_set_reorderable(col5, TRUE);
	gtk_tree_view_column_set_min_width(col5, 0);
	g_object_set(G_OBJECT(cell5), "xalign", 1.0, NULL);
	gtk_tree_view_column_set_cell_data_func(col5, cell5, listview__cell_data_bg, NULL, NULL);

	GtkCellRenderer* cell6 = gtk_cell_renderer_text_new();
	GtkTreeViewColumn* col6 = gtk_tree_view_column_new_with_attributes("Srate", cell6, "text", COL_SAMPLERATE, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col6);
	gtk_tree_view_column_set_resizable(col6, TRUE);
	gtk_tree_view_column_set_reorderable(col6, TRUE);
	gtk_tree_view_column_set_min_width(col6, 0);
	g_object_set(G_OBJECT(cell6), "xalign", 1.0, NULL);
	gtk_tree_view_column_set_cell_data_func(col6, cell6, listview__cell_data_bg, NULL, NULL);

	GtkCellRenderer* cell7 = gtk_cell_renderer_text_new();
	GtkTreeViewColumn* col7 = gtk_tree_view_column_new_with_attributes("Chs", cell7, "text", COL_CHANNELS, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col7);
	gtk_tree_view_column_set_resizable(col7, TRUE);
	gtk_tree_view_column_set_reorderable(col7, TRUE);
	gtk_tree_view_column_set_min_width(col7, 0);
	g_object_set(G_OBJECT(cell7), "xalign", 1.0, NULL);
	gtk_tree_view_column_set_cell_data_func(col7, cell7, listview__cell_data_bg, NULL, NULL);

	GtkCellRenderer* cell8 = gtk_cell_renderer_text_new();
	GtkTreeViewColumn* col8 = gtk_tree_view_column_new_with_attributes("Mimetype", cell8, "text", COL_MIMETYPE, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col8);
	gtk_tree_view_column_set_resizable(col8, TRUE);
	gtk_tree_view_column_set_reorderable(col8, TRUE);
	gtk_tree_view_column_set_min_width(col8, 0);
	gtk_tree_view_column_set_cell_data_func(col8, cell8, listview__cell_data_bg, NULL, NULL);

	GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);
	gtk_tree_view_set_search_column(GTK_TREE_VIEW(view), COL_NAME);

#ifdef GTK4_TODO
	g_signal_connect((gpointer)view, "button-press-event", G_CALLBACK(listview__on_row_clicked), NULL);
#endif
	g_signal_connect((gpointer)view, "cursor-changed", G_CALLBACK(listview__on_cursor_change), NULL);
	g_signal_connect(G_OBJECT(samplecat.store), "content-changed", G_CALLBACK(listview__on_store_changed), lv);

#if 0
	void on_unrealize (GtkWidget* widget, gpointer user_data)
	{
		PF0;
	}
	g_signal_connect((gpointer)view, "unrealize", G_CALLBACK(on_unrealize), NULL);
#endif

	void listview_on_play (GObject* _player, gpointer _)
	{
		if (play->sample->row_ref) {
			listview__highlight_playing_by_ref(play->sample->row_ref);
		}
	}

	g_signal_connect(play, "play", (GCallback)listview_on_play, NULL);

	void on_sort_order_changed (GtkTreeSortable* sortable, gpointer user_data)
	{
		gint sort_column_id;
		GtkSortType order;
		if (gtk_tree_sortable_get_sort_column_id(sortable, &sort_column_id, &order)) {
			if (sort_column_id == COL_LEN) {
				int n_rows = ((SamplecatListStore*)samplecat.store)->row_count;
				if (n_rows >= LIST_STORE_MAX_ROWS) {
					dbg(0, "TODO need to requery database ordered by length...");
				}
			}
		}
	}
	g_signal_connect(GTK_TREE_SORTABLE(samplecat.store), "sort-column-changed", (GCallback)on_sort_order_changed, NULL);

	void list_view_on_search_start ()
	{
		listview__block_motion_handler();
	}
	g_signal_connect(G_OBJECT(app), "search-starting", G_CALLBACK(list_view_on_search_start), NULL);

	{
		GActionEntry entries[] = {
			{ "edit-row", listview__edit_row, NULL, NULL, NULL },
			{ "delete-rows", listview__delete_selected, NULL, NULL, NULL },
			{ "reset-colours", listview__reset_colours, NULL, NULL, NULL },
			{ "update-rows", listview__update_selected, NULL, NULL, NULL },
		};
		GSimpleActionGroup* action_group = g_simple_action_group_new ();
		g_action_map_add_action_entries (G_ACTION_MAP (action_group), entries, G_N_ELEMENTS (entries), NULL);
		gtk_widget_insert_action_group (GTK_WIDGET(gtk_application_get_active_window(GTK_APPLICATION(app))), "library", G_ACTION_GROUP (action_group));
	}
	{
		GtkEventController* mnemonic_controller = gtk_shortcut_controller_new ();
		gtk_event_controller_set_propagation_phase (mnemonic_controller, GTK_PHASE_CAPTURE);
		gtk_shortcut_controller_set_scope (GTK_SHORTCUT_CONTROLLER (mnemonic_controller), GTK_SHORTCUT_SCOPE_MANAGED);
		GtkShortcutAction* action = gtk_named_action_new ("library.action-1");
		GtkShortcut* shortcut = gtk_shortcut_new_with_arguments (gtk_keyval_trigger_new ('h', GDK_CONTROL_MASK), action, NULL);
		gtk_shortcut_controller_add_shortcut (GTK_SHORTCUT_CONTROLLER (mnemonic_controller), shortcut);
		gtk_widget_add_controller (lv->scroll, mnemonic_controller);
	}

	return lv->scroll;
}


static void
listview__on_realise (GtkWidget* widget, gpointer user_data)
{
	gtk_tree_view_column_set_resizable(instance->col_name, true);
	gtk_tree_view_column_set_resizable(instance->col_path, true);

	listview__on_store_changed(samplecat.store, instance);
}


/*
 *  Free the returned list with g_list_free
 */
GList*
listview__get_selection ()
{
	if (!instance) return NULL;

	GtkTreeSelection* selection = gtk_tree_view_get_selection((GtkTreeView*)instance);
	return gtk_tree_selection_get_selected_rows(selection, NULL);
}


#if NEVER
static int
listview__path_get_id (GtkTreePath* path)
{
	int id;
	GtkTreeIter iter;
	gchar* filename;
	GtkTreeModel* store = GTK_TREE_MODEL(samplecat.store);
	gtk_tree_model_get_iter(store, &iter, path);
	gtk_tree_model_get(store, &iter, COL_IDX, &id, COL_NAME, &filename, -1);

	dbg(1, "filename=%s", filename);
	if (!id) pwarn("failed to get id! id must be non-zero.");
	return id;
}


/*
 *  deprecated - use message window instead.
 */
void
listview__show_db_missing ()
{
	static bool done = FALSE;
	if (done) return;

	GtkTreeIter iter;
	gtk_list_store_append(samplecat.store, &iter);
	gtk_list_store_set(samplecat.store, &iter, COL_NAME, "no database available", -1);

	done = TRUE;
}
#endif


#ifdef GTK4_TODO
static gboolean
listview__on_row_clicked (GtkWidget* widget, GdkEventButton* event, gpointer user_data)
{
	GtkTreeView* treeview = GTK_TREE_VIEW(widget);

	switch (event->button) {
	  case 1:
		;GtkTreePath* path;
		if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(instance->widget), (gint)event->x, (gint)event->y, &path, NULL, NULL, NULL)) {

			// auditioning
			GdkRectangle rect;
			gtk_tree_view_get_cell_area(treeview, path, instance->col_pixbuf, &rect);
			if (((gint)event->x > rect.x) && ((gint)event->x < (rect.x + rect.width))) {

				// overview column
				dbg(2, "overview. column rect: %i %i %i %i", rect.x, rect.y, rect.width, rect.height);
				if (play->auditioner) {
					Sample* sample = samplecat_list_store_get_sample_by_path(path);
					if (play->sample) {
						(sample->id == play->sample->id)
							? player_stop(sample)
							: application_play(sample);
					} else {
						application_play(sample);
					}
					sample_unref(sample);
				}
			} else {
				gtk_tree_view_get_cell_area(treeview, path, instance->col_tags, &rect);

				if (((gint)event->x > rect.x) && ((gint)event->x < (rect.x + rect.width))) {
					// tags column
					GtkTreeIter iter;
					GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(instance->widget));
					gtk_tree_model_get_iter(model, &iter, path);
					gchar* tags;
					int id;
					gtk_tree_model_get(model, &iter, COL_KEYWORDS, &tags, COL_IDX, &id, -1);

					if (tags && strlen(tags)) {
						observable_string_set(samplecat.model->filters2.search, g_strdup(tags));
					}
				}
			}
			gtk_tree_path_free(path);
		}
		return NOT_HANDLED;

	  // popup menu
	  case 3:
		dbg (2, "right button press!");

		//if one or no rows selected, select one:
		GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(instance->widget));
		if (gtk_tree_selection_count_selected_rows(selection) <= 1) {
			GtkTreePath* path;
			if(gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(instance->widget), (gint)event->x, (gint)event->y, &path, NULL, NULL, NULL)){
				gtk_tree_selection_unselect_all(selection);
				gtk_tree_selection_select_path(selection, path);
				gtk_tree_path_free(path);

				listview__on_cursor_change(treeview, NULL);
			}
		}

		if (app->context_menu) {
			gtk_menu_popup(GTK_MENU(app->context_menu), NULL, NULL, NULL, NULL, event->button, event->time);
		}
		return HANDLED;

	  default:
		break;
	}
	return NOT_HANDLED;
}
#endif


static void
listview__on_cursor_change (GtkTreeView* widget, gpointer user_data)
{
	PF2;

	Sample* s;
	if ((s = listview__get_first_selected_sample())) {
		if (s->id != instance->selected) {
			instance->selected = s->id;
			samplecat_model_set_selection (samplecat.model, s);
		}
		sample_unref(s);
	}
}


static void
listview__on_store_changed (GtkListStore* store, LibraryView* view)
{
	PF;

	listview__unblock_motion_handler();

	gboolean select_first (gpointer data)
	{
		LibraryView* view = data;

		GtkTreeSelection* selection = gtk_tree_view_get_selection((GtkTreeView*)view->widget);
		if (!gtk_tree_selection_count_selected_rows(selection)) {
			GtkTreePath* path;
			if (gtk_tree_view_get_visible_range((GtkTreeView*)view->widget, &path, NULL)) {
				gtk_tree_view_set_cursor((GtkTreeView*)view->widget, path, NULL, 0);
				gtk_tree_path_free(path);
			}
		}
		return G_SOURCE_REMOVE;
	}
	g_idle_add(select_first, view);
}


#if 0
static gboolean
listview__get_first_selected_iter(GtkTreeIter* iter)
{
	GtkTreeSelection* selection = gtk_tree_view_get_selection((GtkTreeView*)instance->widget);
	GList* list                 = gtk_tree_selection_get_selected_rows(selection, NULL);
	if(list){
		GtkTreePath* path = list->data;

		GtkTreeModel* model = gtk_tree_view_get_model((GtkTreeView*)instance->widget);
		gtk_tree_model_get_iter(model, iter, path);

		g_list_foreach (list, (GFunc)gtk_tree_path_free, NULL);
		g_list_free (list);

		return true;
	}
	return false;
}
#endif


																	// TODO why doesn't this apply to the selection?
static void
listview__reset_colours (GSimpleAction*, GVariant*, gpointer)
{
	gboolean reset_colours (GtkTreeModel* model, GtkTreePath* path, GtkTreeIter* iter, gpointer data)
	{
		Sample *s = samplecat_list_store_get_sample_by_iter(iter);
		gtk_list_store_set(samplecat.store, iter, COL_COLOUR, s->colour_index, -1);
		sample_unref(s);
		return FALSE;
	}

	GtkTreeModel* model = GTK_TREE_MODEL(samplecat.store);
	gtk_tree_model_foreach(model, &reset_colours, NULL);
}


/*
 *  Sync the selected catalogue row with the filesystem.
 */
static void
listview__update_selected (GSimpleAction*, GVariant*, gpointer)
{
	PF;

	g_autoptr(GList) selectionlist = gtk_tree_selection_get_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(instance->widget)), NULL);
	if (selectionlist) {
		dbg(2, "%i rows selected.", g_list_length(selectionlist));

		for (int i=0;i<g_list_length(selectionlist);i++) {
			g_autoptr(GtkTreePath) treepath = g_list_nth_data(selectionlist, i);
			Sample* sample = samplecat_list_store_get_sample_by_path(treepath);
#if 0
			if (do_progress(0, 0)) break; // TODO: set progress title to "updating"
#endif
			samplecat_model_refresh_sample (samplecat.model, sample, true);
			statusbar_print(1, "online status updated (%s)", sample->online ? "online" : "not online");
			sample_unref(sample);
		}
#if 0
		hide_progress();
#endif
	}
}


static GtkTreePath*
listview__get_first_selected_path ()
{
	GtkTreeSelection* selection = gtk_tree_view_get_selection((GtkTreeView*)instance->widget);
	g_autoptr(GList) list = gtk_tree_selection_get_selected_rows(selection, NULL);
	if (list) {
		GtkTreePath* path = gtk_tree_path_copy(list->data);

		g_list_foreach (list, (GFunc)gtk_tree_path_free, NULL);

		return path;
	}
	return NULL;
}


/**
 * @return: Sample* -- must sample_unref() after use.
 */
static Sample*
listview__get_first_selected_sample ()
{
	GtkTreePath* path = listview__get_first_selected_path();
	if (path) {
		Sample* result = samplecat_list_store_get_sample_by_path(path);
		gtk_tree_path_free(path);
		return result;
	}
	return NULL;
}


/*
 *  Outgoing drop. provide the dropee with info on which samples were dropped.
 */
#ifdef GTK4_TODO
static void
listview__dnd_get (GtkWidget* widget, GdkDragContext* context, GtkSelectionData* selection_data, guint info, guint time, gpointer data)
{
	GtkTreeModel* model = GTK_TREE_MODEL(samplecat.store);
	GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(instance->widget));
	GList* selected_rows = gtk_tree_selection_get_selected_rows(selection, &(model));

	GList* list = NULL;
	GList* row  = selected_rows;

	for (; row; row=row->next) {
		GtkTreePath* treepath_selection = row->data;
		Sample* s = samplecat_list_store_get_sample_by_path(treepath_selection);
		// append is slow, but g_list_prepend() is wrong order :(
		list = g_list_append(list, g_strdup(s->full_path));
		sample_unref(s);
		gtk_tree_path_free(treepath_selection);
	}

	gchar* uri_text = NULL;
	gint length = 0;
	switch (info) {
		case TARGET_URI_LIST:
		case TARGET_TEXT_PLAIN:
			uri_text = uri_text_from_list(list, &length, (info == TARGET_TEXT_PLAIN));
			break;
	}

	if (uri_text) {
		gtk_selection_data_set(selection_data, selection_data->target, 8, (guchar*)uri_text, length);
		g_free(uri_text);
	}

	GList* l = list;
	for (;l;l=l->next) g_free(l->data);
	g_list_free(list);
	g_list_free(selected_rows);
}


static gint
listview__drag_received (GtkWidget* widget, GdkDragContext* drag_context, gint x, gint y, GtkSelectionData* data, guint info, guint time, gpointer user_data)
{
	PF;
	return FALSE;
}
#endif


static bool blocked = false;


void
listview__block_motion_handler ()
{
	LibraryView* view = instance;

	if (!blocked && view && view->widget) {
#ifdef GTK4_TODO
		gulong id1 = g_signal_handler_find(view->widget, G_SIGNAL_MATCH_FUNC, 0, 0, 0, listview__on_motion, NULL);
		if (id1) g_signal_handler_block(view->widget, id1);
#ifdef DEBUG
		else pwarn("failed to find handler.");
#endif
#endif

		gtk_tree_row_reference_free(view->mouseover_row_ref);
		view->mouseover_row_ref = NULL;

		blocked = true;
	}
}


static void
listview__unblock_motion_handler ()
{
	LibraryView* view = instance;

	g_return_if_fail(view && view->widget);

	PF;

	if (blocked) {
#ifdef GTK4_TODO
		gulong id1 = g_signal_handler_find(view->widget, G_SIGNAL_MATCH_FUNC, 0, 0, 0, listview__on_motion, NULL);
		if (id1) g_signal_handler_unblock(view->widget, id1);
#ifdef DEBUG
		else pwarn("failed to find handler.");
#endif
#endif
		blocked = false;
	}
}


#ifdef GTK4_TODO
static gboolean
listview__on_motion (GtkWidget* widget, GdkEventMotion* event, gpointer user_data)
{
	// set mouse-over styling for links
	// TODO should be done by the renderer not by modifying the store. currently causing segfaults.
#if 0
	LibraryView* view = app->libraryview;

	//static gint prev_row_num = 0;
	static GtkTreeRowReference* prev_row_ref = NULL;
	static gchar prev_text[256] = "";
	app->mouse_x = event->x;
	app->mouse_y = event->y;
	//gdouble x = event->x; //distance from left edge of treeview.
	//gdouble y = event->y; //distance from bottom of column heads.
	//GdkRectangle rect;
	//gtk_tree_view_get_cell_area(widget, GtkTreePath *path, GtkTreeViewColumn *column, &rect);

	//GtkCellRenderer *cell = app->cell_tags;
	//GList* gtk_cell_view_get_cell_renderers(GtkCellView *cell_view);
	//GList* gtk_tree_view_column_get_cell_renderers(GtkTreeViewColumn *tree_column);

	/*
	GtkCellRenderer *cell_renderer = NULL;
	if(treeview_get_tags_cell(GTK_TREE_VIEW(app->libraryview->widget), (gint)event->x, (gint)event->y, &cell_renderer)){
		printf("%s() tags cell found!\n", __func__);
		g_object_set(cell_renderer, "markup", "<b>important</b>", "text", NULL, NULL);
	}
	*/

	//dbg(1, "x=%f y=%f. l=%i", x, y, rect.x, rect.width);


	//which row are we on?
	GtkTreePath* path;
	GtkTreeIter iter, prev_iter;
	//GtkTreeRowReference* row_ref = NULL;
	if(gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(app->libraryview->widget), (gint)event->x, (gint)event->y, &path, NULL, NULL, NULL)){

		gtk_tree_model_get_iter(GTK_TREE_MODEL(samplecat.store), &iter, path);
		gchar* path_str = gtk_tree_model_get_string_from_iter(GTK_TREE_MODEL(samplecat.store), &iter);

		if(prev_row_ref){
			GtkTreePath* prev_path;
			if((prev_path = gtk_tree_row_reference_get_path(prev_row_ref))){
			
				gtk_tree_model_get_iter(GTK_TREE_MODEL(samplecat.store), &prev_iter, prev_path);
				gchar* prev_path_str = gtk_tree_model_get_string_from_iter(GTK_TREE_MODEL(samplecat.store), &prev_iter);

				//if(row_ref != prev_row_ref)
				//if(prev_path && (path != prev_path))
				if(prev_path && (atoi(path_str) != atoi(prev_path_str)))
				{
					dbg(1, "new row! path=%p (%s) prev_path=%p (%s)", path, path_str, prev_path, prev_path_str);

					//restore text to previous row:
					gtk_list_store_set(samplecat.store, &prev_iter, COL_KEYWORDS, prev_text, -1);

					//store original text:
					gchar* txt;
					gtk_tree_model_get(GTK_TREE_MODEL(samplecat.store), &iter, COL_KEYWORDS, &txt, -1);
					dbg(1, "text=%s", prev_text);
					snprintf(prev_text, 256, "%s", txt);
					g_free(txt);

					/*
					//split by word, etc:
					if(strlen(prev_text)){
						g_strstrip(prev_text);
						const gchar *str = prev_text;//"<b>pre</b> light";
						//split the string:
						gchar** split = g_strsplit(str, " ", 100);
						//printf("split: [%s] %p %p %p %s\n", str, split[0], split[1], split[2], split[0]);
						int word_index = 0;
						gchar* joined = NULL;
						if(split[word_index]){
							gchar word[64];
							snprintf(word, 64, "<u>%s</u>", split[word_index]);
							//g_free(split[word_index]);
							split[word_index] = word;
							joined = g_strjoinv(" ", split);
							dbg(0, "joined: %s", joined);
						}
						//g_strfreev(split); //segfault - doesnt like reassigning split[word_index] ?

						//g_object_set();
						gchar *text = NULL;
						GError *error = NULL;
						PangoAttrList *attrs = NULL;

						if (joined && !pango_parse_markup(joined, -1, 0, &attrs, &text, NULL, &error)){
							g_warning("Failed to set cell text from markup due to error parsing markup: %s", error->message);
							g_error_free(error);
							return false;
						}

						//if (celltext->text) g_free (celltext->text);
						//if (celltext->extra_attrs) pango_attr_list_unref (celltext->extra_attrs);

						dbg(1, "setting text: %s", text);
						//celltext->text = text;
						//celltext->extra_attrs = attrs;
						//hypercell->markup_set = true;
						gtk_list_store_set(samplecat.store, &iter, COL_KEYWORDS, joined, -1);

						if (joined) g_free(joined);
					}
						*/

					g_free(prev_row_ref);
					prev_row_ref = gtk_tree_row_reference_new(GTK_TREE_MODEL(samplecat.store), path);
					//prev_row_ref = row_ref;
				}
			}else{
				//table has probably changed. previous row is not available.
				g_free(prev_row_ref);
				prev_row_ref = NULL;
			}

			gtk_tree_path_free(prev_path);

		}else{
			prev_row_ref = gtk_tree_row_reference_new(GTK_TREE_MODEL(samplecat.store), path);
		}

		gtk_tree_path_free(path);
	}

	view->mouseover_row_ref = prev_row_ref;
#endif
	return false;
}
#endif


static void
listview__edit_row (GSimpleAction* action, GVariant* parameter, gpointer user_data)
{
	// currently this only works for the Tags cell.

	PF;

	LibraryView* view = instance;
	GtkTreeView* treeview = GTK_TREE_VIEW(view->widget);

	GtkTreeSelection* selection = gtk_tree_view_get_selection(treeview);
	if(!selection){ perr("cannot get selection.\n");/* return;*/ }
	GtkTreeModel* model = GTK_TREE_MODEL(samplecat.store);
	GList* selectionlist = gtk_tree_selection_get_selected_rows(selection, &(model));
	if(!selectionlist){ perr("no files selected?\n"); return; }

	GtkTreePath* treepath;
	if((treepath = g_list_nth_data(selectionlist, 0))){
		GtkTreeIter iter;
		if(gtk_tree_model_get_iter(GTK_TREE_MODEL(samplecat.store), &iter, treepath)){
			gchar* path_str = gtk_tree_model_get_string_from_iter(GTK_TREE_MODEL(samplecat.store), &iter);
			dbg(2, "path=%s", path_str);

			GtkTreeViewColumn* focus_column = view->col_tags;
			GtkCellRenderer*   focus_cell   = view->cells.tags;
			//g_signal_handlers_block_by_func(view->widget, cursor_changed, self);
			gtk_widget_grab_focus(view->widget);
			gtk_tree_view_set_cursor_on_cell(
				GTK_TREE_VIEW(view->widget),
				treepath,
				focus_column, // GtkTreeViewColumn *focus_column - this needs to be set for start_editing to work.
				focus_cell,   // the cell to be edited.
				START_EDITING
			);
			//g_signal_handlers_unblock_by_func(treeview, cursor_changed, self);

			g_free(path_str);
		} else perr("cannot get iter.\n");
		gtk_tree_path_free(treepath);
	}
	g_list_free(selectionlist);
}


/*
 *  The keywords column has been edited. Update the database to reflect the new text.
 */
static void
listview__on_keywords_edited (GtkCellRendererText* cell, gchar* path_string, gchar* new_text, gpointer user_data)
{
	PF;
	GtkTreeIter iter;
	int idx;
	gchar* filename;
	Sample* sample;
	GtkTreeModel* store = GTK_TREE_MODEL(samplecat.store);
	GtkTreePath* path = gtk_tree_path_new_from_string(path_string);
	gtk_tree_model_get_iter(store, &iter, path);
	gtk_tree_model_get(store, &iter, COL_SAMPLEPTR, &sample, COL_IDX, &idx, COL_NAME, &filename, -1);
	dbg(1, "filename=%s idx=%i", filename, idx);
	gtk_tree_path_free(path);

	sample_ref(sample);
	if (samplecat_model_update_sample (samplecat.model, sample, COL_KEYWORDS, (void*)new_text)) {
		statusbar_print(1, "keywords updated");
	} else {
		statusbar_print(1, "failed to update keywords");
	}
	sample_unref(sample);
}


static void
listview__delete_selected ()
{
	GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(instance->widget));

	GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(instance->widget));
	g_autoptr(GList) selectionlist = gtk_tree_selection_get_selected_rows(selection, &(model));
	if (!selectionlist) { perr("no files selected?\n"); return; }
	dbg(1, "%i rows selected.", g_list_length(selectionlist));

	statusbar_print(1, "deleting %i files...", g_list_length(selectionlist));

	GList* selected_row_refs = NULL;

	// get row refs for each selected row before the list is modified
	GList* l = selectionlist;
	for (;l;l=l->next) {
		GtkTreePath* treepath_selection = l->data;

		GtkTreeRowReference* row_ref = gtk_tree_row_reference_new(GTK_TREE_MODEL(samplecat.store), treepath_selection);
		selected_row_refs = g_list_prepend(selected_row_refs, row_ref);
	}

	int n = 0;
	GtkTreePath* path;
	GtkTreeIter iter;
	l = selected_row_refs;
	for (;l;l=l->next) {
		GtkTreeRowReference* row_ref = l->data;
		if ((path = gtk_tree_row_reference_get_path(row_ref))) {

			if (gtk_tree_model_get_iter(model, &iter, path)) {
				gchar* fname;
				int id;
				gtk_tree_model_get(model, &iter, COL_NAME, &fname, COL_IDX, &id, -1);

				if (!samplecat_model_remove(samplecat.model, id)) return;

				gtk_list_store_remove(samplecat.store, &iter);
				n++;

			} else perr("bad iter!\n");
			gtk_tree_path_free(path);
		} else perr("cannot get path from row_ref!\n");
	}
	g_list_free(selected_row_refs); //FIXME free the row_refs?

	statusbar_print(1, "%i files deleted", n);
}


static void
listview__path_cell_data (GtkTreeViewColumn* tree_column, GtkCellRenderer* cell, GtkTreeModel* tree_model, GtkTreeIter* iter, gpointer data)
{
	g_return_if_fail (cell);

	listview__cell_data_bg(tree_column, cell, tree_model, iter, data);

	char* text;
	g_object_get(cell, "text", &text, NULL);

	g_object_set(cell, "text", dir_format(text), NULL);

	g_free(text);
}


static void
listview__tag_cell_data (GtkTreeViewColumn* tree_column, GtkCellRenderer* cell, GtkTreeModel* tree_model, GtkTreeIter* iter, gpointer data)
{
	/*
	handle translation of model data to display format for "tag" data.

	note: some stuff here is duplicated in the custom cellrenderer - not sure exactly what happens where!

	mouseovers:
		-usng this fn to do mousovers is slightly difficult.
		-fn is called when mouse enters or leaves a cell. 
			However, because of padding (appears to be 1 pixel), it is not always inside the cell area when this callback occurs!!
			!!!!cell_area.background_area <----try this.
	*/

	LibraryView* view = instance;

	//set background colour:
	listview__cell_data_bg(tree_column, cell, tree_model, iter, data);

	//----------------------

	if (!gtk_widget_get_realized (view->widget)) return;

#ifdef GTK4_TODO
	GtkCellRendererHyperText* hypercell = (GtkCellRendererHyperText*)cell;
	GtkTreePath* path = gtk_tree_model_get_path(GTK_TREE_MODEL(samplecat.store), iter);

	gint mouse_row_num = listview__get_mouseover_row();

	gchar* path_str = gtk_tree_model_get_string_from_iter(GTK_TREE_MODEL(samplecat.store), iter);
	gint cell_row_num = atoi(path_str);

	// get the coords for this cell
	GdkRectangle cellrect;
	gtk_tree_view_get_cell_area(GTK_TREE_VIEW(view->widget), path, tree_column, &cellrect);
	gtk_tree_path_free(path);
	//dbg(0, "%s mouse_y=%i cell_y=%i-%i.\n", path_str, app->mouse_y, cellrect.y, cellrect.y + cellrect.height);
	//if(//(app->mouse_x > cellrect.x) && (app->mouse_x < (cellrect.x + cellrect.width)) &&
	//			(app->mouse_y >= cellrect.y) && (app->mouse_y <= (cellrect.y + cellrect.height)))
	if ((cell_row_num == mouse_row_num) && (app->mouse_x > cellrect.x) && (app->mouse_x < (cellrect.x + cellrect.width))) {
		char* text;
		PangoAttrList* attributes;
		g_object_get(cell, "text", &text, "attributes", &attributes, NULL);

		if (strlen(text)) {
			char* trimmed = g_strstrip(text); // trim

			gint mouse_cell_x = app->mouse_x - cellrect.x;

			//make a layout to find word sizes:

			PangoContext* context = gtk_widget_get_pango_context(view->widget); //free?
			PangoLayout* layout = pango_layout_new(context);
			pango_layout_set_text(layout, trimmed, strlen(trimmed));

			int line_num = 0;
			PangoLayoutLine* layout_line = pango_layout_get_line(layout, line_num);
			int char_pos;
			gboolean trailing = 0;
			/*
			int i; for(i=0;i<layout_line->length;i++){
				//pango_layout_line_index_to_x(layout_line, i, trailing, &char_pos);
			}
			*/

			// split the string into words
			gchar formatted[256] = "";
			{
				const gchar* str = trimmed;
				gchar** split = g_strsplit(str, " ", 100);
				int char_index = 0;
				int word_index = 0;
				char word[256] = "";
				while (split[word_index]) {
					char_index += strlen(split[word_index]);

					pango_layout_line_index_to_x(layout_line, char_index, trailing, &char_pos);
					if (char_pos/PANGO_SCALE > mouse_cell_x) {
						dbg(1, "word=%i\n", word_index);

						snprintf(word, 256, "<u>%s</u> ", split[word_index]);
						g_strlcat(formatted, word, 256);

						while (split[++word_index]) {
							snprintf(word, 256, "%s ", split[word_index]);
							g_strlcat(formatted, word, 256);
						}

						break;
					}

					snprintf(word, 256, "%s ", split[word_index]);
					g_strlcat(formatted, word, 256);

					word_index++;
				}
			}

			g_object_unref(layout);

			// Set new markup

			gchar *text2 = NULL;
			GError *error = NULL;
			PangoAttrList *attrs = NULL;

			if (!pango_parse_markup(formatted, -1, 0, &attrs, &text2, NULL, &error)) {
				g_warning("Failed to set cell text from markup due to error parsing markup: %s", error->message);
				g_error_free(error);
				return;
			}

			//
			// setting text here doesnt seem to work (text is set but not displayed), but setting markup does.
			//
			g_object_set(cell, "text", text2, "attributes", attrs, NULL);
			hypercell->markup_set = true;

			g_free(text);
		}
	}
	//else g_object_set(cell, "markup", "outside", NULL);
	//else hypercell->markup_set = false;

	g_free(path_str);
#endif


/*
			gchar *text = NULL;
			GError *error = NULL;
			PangoAttrList *attrs = NULL;
			
			dbg(0, "text=%s\n", celltext->text);
			if (!pango_parse_markup(celltext->text, -1, 0, &attrs, &text, NULL, &error)){
				g_warning("Failed to set cell text from markup due to error parsing markup: %s", error->message);
				g_error_free(error);
				return;
			}
			//if (celltext->text) g_free (celltext->text);
			//if (celltext->extra_attrs) pango_attr_list_unref (celltext->extra_attrs);
			celltext->text = text;
			celltext->extra_attrs = attrs;
	hypercell->markup_set = true;
	*/
}


#if NEVER
static void
cell_bg_lighter (GtkTreeViewColumn *tree_column, GtkCellRenderer *cell, GtkTreeModel *tree_model, GtkTreeIter *iter)
{
	unsigned colour_index = 0;
	gtk_tree_model_get(GTK_TREE_MODEL(samplecat.store), iter, COL_COLOUR, &colour_index, -1);
	g_return_if_fail (colour_index <= PALETTE_SIZE);

	if (colour_index == 0) {
		colour_index = 4; //FIXME temp
	}

	if (strlen(app->config.colour[colour_index])) {
		GdkColor colour, colour2;
		char hexstring[8];
		snprintf(hexstring, 8, "#%s", app->config.colour[colour_index]);
		if (!gdk_color_parse(hexstring, &colour)) pwarn("parsing of colour string failed.\n");
		colour_lighter(&colour2, &colour);

		g_object_set(cell, "cell-background-set", true, "cell-background-gdk", &colour2, NULL);
	}
}
#endif

static void
listview__cell_data_bg (GtkTreeViewColumn *tree_column, GtkCellRenderer *cell, GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer data)
{
	unsigned colour_index = 0;
	char colour[16] = "#606060";
	gtk_tree_model_get(GTK_TREE_MODEL(samplecat.store), iter, COL_COLOUR, &colour_index, -1);
	if (colour_index < PALETTE_SIZE) {
		if (strlen(app->config.colour[colour_index])) {
			snprintf(colour, 16, "#%s", app->config.colour[colour_index]);
			if (strlen(colour) != 7) {
				perr("bad colour string (%s) index=%u.\n", colour, colour_index);
				return;
			}
		}
		else colour_index = 0;
	}

	if (colour_index)
		g_object_set(cell, "cell-background-set", true, "cell-background", colour, NULL);
	else
		g_object_set(cell, "cell-background-set", false, NULL);
}

#if NEVER
static gboolean
treeview_get_tags_cell (GtkTreeView *view, guint x, guint y, GtkCellRenderer **cell)
{
	GtkTreeViewColumn *colm = NULL;
	guint colm_x = 0/*, colm_y = 0*/;

	GList* columns = gtk_tree_view_get_columns(view);

	for (GList* node = columns;  node != NULL && colm == NULL;  node = node->next) {
		GtkTreeViewColumn *checkcolm = (GtkTreeViewColumn*) node->data;

		if (x >= colm_x  &&  x < (colm_x + checkcolm->width))
			colm = checkcolm;
		else
			colm_x += checkcolm->width;
	}

	g_list_free(columns);

	if(colm == NULL) return false; // not found
	if(colm != app->col_tags) return false;

	// (2) find the cell renderer within the column 

	GList* cells = gtk_tree_view_column_get_cell_renderers(colm);
	GdkRectangle cell_rect;

	for (node = cells;  node != NULL;  node = node->next) {
		GtkCellRenderer *checkcell = (GtkCellRenderer*) node->data;
		guint            width = 0, height = 0;

		// Will this work for all packing modes? doesn't that return a random width depending on the last content rendered?
		gtk_cell_renderer_get_size(checkcell, GTK_WIDGET(view), &cell_rect, NULL, NULL, (int*)&width, (int*)&height);
		printf("y=%i height=%i\n", cell_rect.y, cell_rect.height);

		//if(x >= colm_x && x < (colm_x + width))
		//if(y >= colm_y && y < (colm_y + height))
		if (y >= cell_rect.y && y < (cell_rect.y + cell_rect.height)) {
			*cell = checkcell;
			g_list_free(cells);
			return true;
		}

		//colm_y += height;
	}

	g_list_free(cells);
	dbg(0, "not found in column. cell_height=%i\n", cell_rect.height);
	return false; // not found
}
#endif


/*
 *  Get the row number the mouse is currently over from the stored row_reference.
 */
gint
listview__get_mouseover_row ()
{
	LibraryView* view = instance;

	gint row_num = -1;
	GtkTreePath* path;
	GtkTreeIter iter;
	if ((view->mouseover_row_ref && (path = gtk_tree_row_reference_get_path(view->mouseover_row_ref)))) {
		gtk_tree_model_get_iter(GTK_TREE_MODEL(samplecat.store), &iter, path);
		gchar* path_str = gtk_tree_model_get_string_from_iter(GTK_TREE_MODEL(samplecat.store), &iter);
		row_num = atoi(path_str);

		g_free(path_str);
		gtk_tree_path_free(path);
	}
	return row_num;
}


static void 
listview__highlight_playing_by_path (GtkTreePath* path)
{
	GtkTreeIter iter;
	gtk_tree_model_get_iter(GTK_TREE_MODEL(samplecat.store), &iter, path);
	gtk_list_store_set(GTK_LIST_STORE(samplecat.store), &iter, COL_COLOUR, /*colour*/ PALETTE_SIZE, -1);
}


static void 
listview__highlight_playing_by_ref (GtkTreeRowReference* ref)
{
	GtkTreePath* path;
	if (!ref || !gtk_tree_row_reference_valid(ref)) return;
	if(!(path = gtk_tree_row_reference_get_path(ref))) return;

	GtkTreeRowReference* prev = ((SamplecatListStore*)samplecat.store)->playing;
	if(prev){
		GtkTreePath* path;
		if((path = gtk_tree_row_reference_get_path(prev))){
			GtkTreeIter iter;
			gtk_tree_model_get_iter(GTK_TREE_MODEL(samplecat.store), &iter, path);
			gtk_list_store_set(GTK_LIST_STORE(samplecat.store), &iter, COL_COLOUR, /*colour*/ 0, -1);
		}
		gtk_tree_row_reference_free(((SamplecatListStore*)samplecat.store)->playing);
		((SamplecatListStore*)samplecat.store)->playing = NULL;
	}
	((SamplecatListStore*)samplecat.store)->playing = gtk_tree_row_reference_copy(ref);

	listview__highlight_playing_by_path(path);

	gtk_tree_path_free(path);
}

