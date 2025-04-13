/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. https://ayyi.github.io/samplecat/    |
 | copyright (C) 2007-2025 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 | LibraryView
 |
 | The main Samplecat pane. A treeview with a list of samples.
 |
 */

#include "config.h"
#define GDK_VERSION_MIN_REQUIRED GDK_VERSION_4_8
#include <gtk/gtk.h>
#include "debug/debug.h"
#ifdef USE_AYYI
#include <ayyi/ayyi.h>
#endif
#include "gdl/gdl-dock-item.h"
#include "file_manager/support.h"
#include "file_manager/mimetype.h"
#include "file_manager/pixmaps.h"
#include "gtk/cellrenderer_hypertext.h"
#include "gtk/menu.h"

#include "types.h"
#include "support.h"
#include "model.h"
#include "application.h"
#include "sample.h"
#include "worker.h"
#include "library.h"

extern GtkWidget* make_panel_context_menu (GtkWidget* widget, int size, MenuDef defn[size]);

typedef struct {
	GdlDockItemClass parent_class;
} LibraryClass;

typedef struct {
   GdlDockItem        item;
   GtkWidget*         treeview;
   GtkWidget*         scroll;
   GtkWidget*         menu;

   struct {
      GtkCellRenderer* name;
      GtkCellRenderer* tags;
   }                   cells;
   GtkTreeViewColumn* col_name;
   GtkTreeViewColumn* col_path;
   GtkTreeViewColumn* col_pixbuf;
   GtkTreeViewColumn* col_tags;

   int                  selected;
   GtkTreeRowReference* mouseover_row_ref;
} Library;

#define TYPE_LIBRARY            (library_get_type ())
#define LIBRARY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_LIBRARY, Library))
#define LIBRARY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_LIBRARY, LibraryClass))
#define IS_LIBRARY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_LIBRARY))
#define IS_LIBRARY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_LIBRARY))
#define LIBRARY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_LIBRARY, LibraryClass))

#define LibraryView Library

static gpointer library_parent_class = NULL;

static LibraryView* instance;
static bool blocked = false;

#define START_EDITING 1

GType               library_get_type                  (void);

static void         library__on_row_clicked           (GtkGestureClick*, int, double, double, gpointer);
static void         library__on_cursor_change         (GtkTreeView*, gpointer);
static void         library__on_store_changed         (GtkListStore*, LibraryView*);
static GdkContentProvider*
                    library__on_drag_prepare          (GtkDragSource*, double x, double y, GtkWidget*);
#ifdef GTK4_TODO
static gboolean     library__on_motion                (GtkWidget*, GdkEventMotion*, gpointer);
#endif
static void         listview__on_keywords_edited      (GtkCellRendererText*, gchar* path_string, gchar* new_text, gpointer);
static void         listview__path_cell_data          (GtkTreeViewColumn*, GtkCellRenderer*, GtkTreeModel*, GtkTreeIter*, gpointer);
static void         listview__tag_cell_data           (GtkTreeViewColumn*, GtkCellRenderer*, GtkTreeModel*, GtkTreeIter*, gpointer);
static void         listview__cell_data_bg            (GtkTreeViewColumn*, GtkCellRenderer*, GtkTreeModel*, GtkTreeIter*, gpointer);
static GtkTreePath* listview__get_first_selected_path ();
static Sample*      library__get_first_selected_sample();
static void         library__unblock_motion_handler   ();
#if NEVER
static void         cell_bg_lighter                   (GtkTreeViewColumn*, GtkCellRenderer*, GtkTreeModel*, GtkTreeIter*);
static int          listview__path_get_id             (GtkTreePath*);
static gboolean     treeview_get_tags_cell            (GtkTreeView*, guint x, guint y, GtkCellRenderer**);
#endif
static void         listview__highlight_playing_by_path(GtkTreePath*);
static void         listview__highlight_playing_by_ref(GtkTreeRowReference*);

static void         listview__edit_row                (GSimpleAction*, GVariant*, gpointer);
static void         listview__reset_colours           (GSimpleAction*, GVariant*, gpointer);
static void         library__update_selected          (GSimpleAction*, GVariant*, gpointer);
static void         library__delete_selected          ();

static bool         listview_item_set_colour          (GtkTreePath*, unsigned colour_index);


static GObject*
library_constructor (GType type, guint n_construct_properties, GObjectConstructParam* construct_properties)
{
	GObjectClass* parent_class = G_OBJECT_CLASS (library_parent_class);
	return parent_class->constructor (type, n_construct_properties, construct_properties);
}


static void
library_realize (GtkWidget* base)
{
	Library* lv = (Library*) base;

	GTK_WIDGET_CLASS (library_parent_class)->realize ((GtkWidget*) G_TYPE_CHECK_INSTANCE_CAST (lv, GDL_TYPE_DOCK_ITEM, GdlDockItem));

	gtk_tree_view_column_set_resizable(lv->col_name, true);
	gtk_tree_view_column_set_resizable(lv->col_path, true);

	library__on_store_changed(samplecat.store, lv);
}


static void
library_dispose (GObject* base)
{
	Library* lv = (Library*) base;

	g_signal_handlers_disconnect_matched(lv->treeview, G_SIGNAL_MATCH_DATA, 0, 0, 0, NULL, lv);
	g_signal_handlers_disconnect_matched(G_OBJECT(samplecat.store), G_SIGNAL_MATCH_DATA, 0, 0, 0, NULL, lv);

	G_OBJECT_CLASS (library_parent_class)->dispose ((GObject*) G_TYPE_CHECK_INSTANCE_CAST (lv, GDL_TYPE_DOCK_ITEM, GdlDockItem));
}


static void
library_finalize (GObject* obj)
{
	PF;
	G_OBJECT_CLASS (library_parent_class)->finalize (obj);
}


static void
library_class_init (LibraryClass* klass, gpointer klass_data)
{
	library_parent_class = g_type_class_peek_parent (klass);

	((GtkWidgetClass*)klass)->realize = library_realize;
	((GObjectClass*)klass)->dispose = library_dispose;
	G_OBJECT_CLASS (klass)->constructor = library_constructor;
	G_OBJECT_CLASS (klass)->finalize = library_finalize;

	gtk_widget_class_add_binding_action ((GtkWidgetClass*)klass, GDK_KEY_h, GDK_CONTROL_MASK, "library.reset-colours", NULL);
	gtk_widget_class_add_binding_action ((GtkWidgetClass*)klass, GDK_KEY_Delete, 0, "library.delete-rows", NULL);
	gtk_widget_class_add_binding_action ((GtkWidgetClass*)klass, GDK_KEY_u, 0, "library.update-rows", NULL);
	gtk_widget_class_add_binding_action ((GtkWidgetClass*)klass, GDK_KEY_c, 0, "library.reset-colours", NULL);
}

static void
library_instance_init (Library* lv, gpointer klass)
{
	instance = lv;

	lv->scroll = scrolled_window_new();
	gdl_dock_item_set_child(GDL_DOCK_ITEM(lv), lv->scroll);

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	GtkWidget* view = lv->treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(samplecat.store));
#pragma GCC diagnostic warning "-Wdeprecated-declarations"
	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(lv->scroll), view);
#ifdef GTK4_TODO
	g_signal_connect(view, "motion-notify-event", (GCallback)library__on_motion, NULL);
#endif

	// set up as dnd source
	{
		void on_drag_begin (GtkDragSource* source, GdkDrag* drag, GtkWidget* self)
		{
			g_autoptr(GtkTreePath) path = listview__get_first_selected_path();
			g_autoptr(GdkPaintable) icon = gtk_tree_view_create_row_drag_icon (GTK_TREE_VIEW(self), path);
			gtk_drag_source_set_icon (source, icon, 0, 0);
		}

		GtkDragSource* drag_source = gtk_drag_source_new ();
		g_signal_connect (drag_source, "prepare", G_CALLBACK (library__on_drag_prepare), view);
		g_signal_connect (drag_source, "drag-begin", G_CALLBACK (on_drag_begin), view);

		gtk_widget_add_controller (view, GTK_EVENT_CONTROLLER (drag_source));
	}

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

	GtkGesture* click = gtk_gesture_click_new ();
	g_signal_connect (click, "pressed", G_CALLBACK (library__on_row_clicked), view);
	gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (click), 0);
	gtk_widget_add_controller (view, GTK_EVENT_CONTROLLER (click));

	g_signal_connect((gpointer)view, "cursor-changed", G_CALLBACK(library__on_cursor_change), lv);
	g_signal_connect(G_OBJECT(samplecat.store), "content-changed", G_CALLBACK(library__on_store_changed), lv);

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
			{ "delete-rows", library__delete_selected, NULL, NULL, NULL },
			{ "reset-colours", listview__reset_colours, NULL, NULL, NULL },
			{ "update-rows", library__update_selected, NULL, NULL, NULL },
		};
		GSimpleActionGroup* action_group = g_simple_action_group_new ();
		g_action_map_add_action_entries (G_ACTION_MAP (action_group), entries, G_N_ELEMENTS (entries), NULL);
		gtk_widget_insert_action_group (GTK_WIDGET(gtk_application_get_active_window(GTK_APPLICATION(app))), "library", G_ACTION_GROUP (action_group));
	}
	{
		gboolean on_drop (GtkDropTarget* target, const GValue* value, double x, double y, gpointer data)
		{
			LibraryView* lv = data;

			if (G_VALUE_HOLDS (value, GDK_TYPE_FILE_LIST)) {
				pwarn("GDK_TYPE_FILE_LIST NEVER GET HERE");

			} else if (G_VALUE_HOLDS (value, G_TYPE_FILE)) {
				GFile* file = g_value_get_object (value);
				g_autofree char* path = g_file_get_path(file);
				ScanResults result = {0,};
				if (is_dir(path))
					samplecat_application_add_dir(path, &result);
				else
					samplecat_application_add_file(path, &result);

				statusbar_print(1, "import complete. %i files added", result.n_added);

			} else if (G_VALUE_HOLDS (value, AYYI_TYPE_COLOUR)) {
				int colour = value->data[0].v_int;

				g_autoptr(GtkTreePath) path;
				int bin_x, bin_y;
				gtk_tree_view_convert_widget_to_bin_window_coords (GTK_TREE_VIEW(lv->treeview), x, y, &bin_x, &bin_y);
				if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(lv->treeview), bin_x, bin_y, &path, NULL, NULL, NULL)) {

					GtkTreeIter iter;
					gtk_tree_model_get_iter(GTK_TREE_MODEL(samplecat.store), &iter, path);
					g_autofree gchar* path_str = gtk_tree_model_get_string_from_iter(GTK_TREE_MODEL(samplecat.store), &iter);

					listview_item_set_colour(path, colour);
				}
			} else
				return FALSE;

			return TRUE;
		}

		GtkDropTarget* target = gtk_drop_target_new (G_TYPE_INVALID, GDK_ACTION_COPY);
		gtk_drop_target_set_gtypes (target, (GType [2]) { G_TYPE_FILE, AYYI_TYPE_COLOUR, }, 2);
		g_signal_connect (target, "drop", G_CALLBACK (on_drop), lv);
		gtk_widget_add_controller (view, GTK_EVENT_CONTROLLER (target));
	}
}


static GType
library_get_type_once (void)
{
	static const GTypeInfo g_define_type_info = { sizeof (LibraryClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) library_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (Library), 0, (GInstanceInitFunc) library_instance_init, NULL };
	GType library_type_id;
	library_type_id = g_type_register_static (GDL_TYPE_DOCK_ITEM, "Library", &g_define_type_info, 0);
	return library_type_id;
}

GType
library_get_type (void)
{
	static volatile gsize library_type_id__once = 0;
	if (g_once_init_enter ((gsize*)&library_type_id__once)) {
		GType library_type_id;
		library_type_id = library_get_type_once ();
		g_once_init_leave (&library_type_id__once, library_type_id);
	}
	return library_type_id__once;
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


static void
library__on_row_clicked (GtkGestureClick* gesture, int n_press, double x, double y, gpointer widget)
{
	GtkTreeView* treeview = GTK_TREE_VIEW(widget);

	switch (gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(gesture))) {
	  case 1:
		;GtkTreePath* path;
		if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(instance->treeview), (gint)x, (gint)y, &path, NULL, NULL, NULL)) {

			// auditioning
			GdkRectangle rect;
			gtk_tree_view_get_cell_area(treeview, path, instance->col_pixbuf, &rect);
			if (((gint)x > rect.x) && ((gint)x < (rect.x + rect.width))) {

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

				if (((gint)x > rect.x) && ((gint)x < (rect.x + rect.width))) {
					// tags column
					GtkTreeIter iter;
					GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(instance->treeview));
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
		break;

	  // popup menu
	  case 3:
		dbg (2, "right button press");

		// Unless there is a multi-selection, select current row.
		// This overrides the default gtktreeview behaviour which removes a multiselection on right click
		GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(instance->treeview));
		if (gtk_tree_selection_count_selected_rows(selection) <= 1) {
			int bx, by;
			gtk_tree_view_convert_widget_to_bin_window_coords((GtkTreeView*)instance->treeview, (int)x, (int)y, &bx, &by);

			GtkTreePath* path;
			if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(instance->treeview), (gint)bx, (gint)by, &path, NULL, NULL, NULL)) {
				gtk_tree_selection_unselect_all(selection);
				gtk_tree_selection_select_path(selection, path);
				gtk_tree_path_free(path);

				library__on_cursor_change(treeview, NULL);
			}
		}

		if (!instance->menu) {
			MenuDef menu_def[] = {
				{"Delete",         "library.delete-rows",   "edit-delete-symbolic"},
				{"Update",         "library.update-rows",   "view-refresh-symbolic"},
				{"Reset Colours",  "library.reset-colours", ""},
				{"Edit tags", "library.edit-row", "text-editor-symbolic"},
			};

			instance->menu = make_panel_context_menu((GtkWidget*)instance, G_N_ELEMENTS(menu_def), menu_def);
		}

		gtk_popover_set_position(GTK_POPOVER(instance->menu), GTK_POS_BOTTOM);
		gtk_popover_set_pointing_to(GTK_POPOVER(instance->menu), &(GdkRectangle){ x, y, 1, 1 });
		gtk_popover_set_offset(GTK_POPOVER(instance->menu), 80, 20);
		gtk_popover_popup(GTK_POPOVER(instance->menu));

		gtk_gesture_set_state (GTK_GESTURE (gesture), GTK_EVENT_SEQUENCE_CLAIMED);

		break;

	  default:
		break;
	}
}


static void
library__on_cursor_change (GtkTreeView* widget, gpointer user_data)
{
	PF2;

	if (blocked) return;

	GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(instance->treeview));
	if (gtk_tree_selection_count_selected_rows(selection) > 1) {
		return;
	}

	Sample* s;
	if ((s = library__get_first_selected_sample())) {
		if (s->id != instance->selected) {
			instance->selected = s->id;
			samplecat_model_set_selection (samplecat.model, s);
		}
		sample_unref(s);
	}
}


static void
library__on_store_changed (GtkListStore* store, LibraryView* view)
{
	PF;

	library__unblock_motion_handler();

	GtkTreeSelection* selection = gtk_tree_view_get_selection((GtkTreeView*)view->treeview);
	if (!gtk_tree_selection_count_selected_rows(selection)) {
		GtkTreePath* path;
		if (gtk_tree_view_get_visible_range((GtkTreeView*)view->treeview, &path, NULL)) {
			gtk_tree_view_set_cursor((GtkTreeView*)view->treeview, path, NULL, 0);
			gtk_tree_path_free(path);
		}
	}
}


#if 0
static gboolean
listview__get_first_selected_iter (GtkTreeIter* iter)
{
	GtkTreeSelection* selection = gtk_tree_view_get_selection((GtkTreeView*)instance->treeview);
	GList* list                 = gtk_tree_selection_get_selected_rows(selection, NULL);
	if(list){
		GtkTreePath* path = list->data;

		GtkTreeModel* model = gtk_tree_view_get_model((GtkTreeView*)instance->treeview);
		gtk_tree_model_get_iter(model, iter, path);

		g_list_foreach (list, (GFunc)gtk_tree_path_free, NULL);
		g_list_free (list);

		return true;
	}
	return false;
}
#endif


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
library__update_selected (GSimpleAction*, GVariant*, gpointer)
{
	PF;

	g_autoptr(GList) selectionlist = gtk_tree_selection_get_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(instance->treeview)), NULL);
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
	GtkTreeSelection* selection = gtk_tree_view_get_selection((GtkTreeView*)instance->treeview);
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
library__get_first_selected_sample ()
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
static GdkContentProvider*
library__on_drag_prepare (GtkDragSource *source, double x, double y, GtkWidget* self)
{
	GSList* paths = ({
		g_autoptr(GList) selected_rows = gtk_tree_selection_get_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(instance->treeview)), (GtkTreeModel**)&samplecat.store);

		GSList* l = NULL;
		for (GList* row=selected_rows; row; row=row->next) {
			g_autoptr(GtkTreePath) treepath = row->data;
			Sample* s = samplecat_list_store_get_sample_by_path(treepath);
			l = g_slist_append (l, g_file_new_for_path (s->full_path));
			sample_unref(s);
		}
		l;
	});

	GValue value = G_VALUE_INIT;
	g_value_init (&value, GDK_TYPE_FILE_LIST);
	g_value_set_boxed (&value, paths);

	g_slist_free_full(paths, g_object_unref);

	return gdk_content_provider_new_for_value (&value);
}


void
listview__block_motion_handler ()
{
	LibraryView* view = instance;

	if (!blocked && view && view->treeview) {
#ifdef GTK4_TODO
		gulong id1 = g_signal_handler_find(view->treeview, G_SIGNAL_MATCH_FUNC, 0, 0, 0, library__on_motion, NULL);
		if (id1) g_signal_handler_block(view->treeview, id1);
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
library__unblock_motion_handler ()
{
	LibraryView* view = instance;

	g_return_if_fail(view && view->treeview);

	PF;

	if (blocked) {
#ifdef GTK4_TODO
		gulong id1 = g_signal_handler_find(view->treeview, G_SIGNAL_MATCH_FUNC, 0, 0, 0, library__on_motion, NULL);
		if (id1) g_signal_handler_unblock(view->treeview, id1);
#ifdef DEBUG
		else pwarn("failed to find handler.");
#endif
#endif
		blocked = false;
	}
}


#ifdef GTK4_TODO
static gboolean
library__on_motion (GtkWidget* widget, GdkEventMotion* event, gpointer user_data)
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
	if(treeview_get_tags_cell(GTK_TREE_VIEW(app->libraryview->treeview), (gint)event->x, (gint)event->y, &cell_renderer)){
		printf("%s() tags cell found!\n", __func__);
		g_object_set(cell_renderer, "markup", "<b>important</b>", "text", NULL, NULL);
	}
	*/

	//dbg(1, "x=%f y=%f. l=%i", x, y, rect.x, rect.width);


	//which row are we on?
	GtkTreePath* path;
	GtkTreeIter iter, prev_iter;
	//GtkTreeRowReference* row_ref = NULL;
	if(gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(app->libraryview->treeview), (gint)event->x, (gint)event->y, &path, NULL, NULL, NULL)){

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
	GtkTreeView* treeview = GTK_TREE_VIEW(view->treeview);

	GtkTreeSelection* selection = gtk_tree_view_get_selection(treeview);
	if (!selection) { perr("cannot get selection.\n");/* return;*/ }
	GtkTreeModel* model = GTK_TREE_MODEL(samplecat.store);
	GList* selectionlist = gtk_tree_selection_get_selected_rows(selection, &(model));
	if (!selectionlist) { perr("no files selected?\n"); return; }

	GtkTreePath* treepath;
	if ((treepath = g_list_nth_data(selectionlist, 0))){
		GtkTreeIter iter;
		if (gtk_tree_model_get_iter(GTK_TREE_MODEL(samplecat.store), &iter, treepath)) {
			gchar* path_str = gtk_tree_model_get_string_from_iter(GTK_TREE_MODEL(samplecat.store), &iter);
			dbg(2, "path=%s", path_str);

			GtkTreeViewColumn* focus_column = view->col_tags;
			GtkCellRenderer*   focus_cell   = view->cells.tags;
			//g_signal_handlers_block_by_func(treeview, cursor_changed, self);
			gtk_widget_grab_focus(view->treeview);
			gtk_tree_view_set_cursor_on_cell(
				GTK_TREE_VIEW(view->treeview),
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
library__delete_selected ()
{
	GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(instance->treeview));

	GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(instance->treeview));
	g_autoptr(GList) selectionlist = gtk_tree_selection_get_selected_rows(selection, &(model));
	if (!selectionlist) { perr("no files selected\n"); return; }
	dbg(1, "%i rows selected.", g_list_length(selectionlist));

	statusbar_print(1, "deleting %i files...", g_list_length(selectionlist));

	GList* selected_row_refs = NULL;

	// get row-refs for each selected row before the list is modified
	for (GList* l = selectionlist;l;l=l->next) {
		GtkTreePath* treepath_selection = l->data;

		GtkTreeRowReference* row_ref = gtk_tree_row_reference_new(GTK_TREE_MODEL(samplecat.store), treepath_selection);
		selected_row_refs = g_list_prepend(selected_row_refs, row_ref);
	}

	int n = 0;
	GtkTreePath* path;
	GtkTreeIter iter;
	for (GList* l = selected_row_refs;l;l=l->next) {
		g_autoptr(GtkTreeRowReference) row_ref = l->data;
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
	g_list_free(selected_row_refs);

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

	if (!gtk_widget_get_realized (view->treeview)) return;

#ifdef GTK4_TODO
	GtkCellRendererHyperText* hypercell = (GtkCellRendererHyperText*)cell;
	GtkTreePath* path = gtk_tree_model_get_path(GTK_TREE_MODEL(samplecat.store), iter);

	gint mouse_row_num = listview__get_mouseover_row();

	gchar* path_str = gtk_tree_model_get_string_from_iter(GTK_TREE_MODEL(samplecat.store), iter);
	gint cell_row_num = atoi(path_str);

	// get the coords for this cell
	GdkRectangle cellrect;
	gtk_tree_view_get_cell_area(GTK_TREE_VIEW(view->treeview), path, tree_column, &cellrect);
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

			PangoContext* context = gtk_widget_get_pango_context(view->treeview); //free?
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
	if (!(path = gtk_tree_row_reference_get_path(ref))) return;

	GtkTreeRowReference* prev = ((SamplecatListStore*)samplecat.store)->playing;
	if (prev) {
		GtkTreePath* path;
		if ((path = gtk_tree_row_reference_get_path(prev))) {
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


static bool
listview_item_set_colour (GtkTreePath* path, unsigned colour_index)
{
	g_return_val_if_fail(path, false);
	GtkTreeIter iter;
	gtk_tree_model_get_iter(GTK_TREE_MODEL(samplecat.store), &iter, path);

	bool ok;
	Sample* s = samplecat_list_store_get_sample_by_iter(&iter);
	if ((ok = samplecat_model_update_sample (samplecat.model, s, COL_COLOUR, (void*)&colour_index))) {
		statusbar_print(1, "colour updated");
	} else {
		statusbar_print(1, "error! colour not updated");
	}
	sample_unref(s);

	return ok;
}
