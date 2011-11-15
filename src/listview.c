#include "config.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <gtk/gtk.h>
#ifdef USE_AYYI
#include <ayyi/ayyi.h>
#endif
#include "file_manager/support.h"

#include "typedefs.h"
#include "mimetype.h"
#include "support.h"
#include "main.h"
#include "sample.h"
#include "dnd.h"
#include "cellrenderer_hypertext.h"
#include "pixmaps.h"
#include "overview.h"
#include "listview.h"
#include "inspector.h"
#include "auditioner.h"

extern int      debug;
extern struct _app app;

static gboolean listview__on_row_clicked         (GtkWidget*, GdkEventButton*, gpointer);
static void     listview__on_cursor_change       (GtkTreeView*, gpointer);
static void     listview__dnd_get                (GtkWidget*, GdkDragContext*, GtkSelectionData*, guint info, guint time, gpointer);
static gint     listview__drag_received          (GtkWidget*, GdkDragContext*, gint x, gint y, GtkSelectionData*, guint info, guint time, gpointer user_data);
static gboolean listview__on_motion              (GtkWidget*, GdkEventMotion*, gpointer);
static void     listview__on_keywords_edited     (GtkCellRendererText*, gchar *path_string, gchar *new_text, gpointer);
static void     path_cell_data                   (GtkTreeViewColumn*, GtkCellRenderer*, GtkTreeModel*, GtkTreeIter*, gpointer);
static void     tag_cell_data                    (GtkTreeViewColumn*, GtkCellRenderer*, GtkTreeModel*, GtkTreeIter*, gpointer);
//static void     cell_bg_lighter                  (GtkTreeViewColumn*, GtkCellRenderer*, GtkTreeModel*, GtkTreeIter*);
static void     cell_data_bg                     (GtkTreeViewColumn*, GtkCellRenderer*, GtkTreeModel*, GtkTreeIter*, gpointer);
static gboolean listview__get_first_selected_iter(GtkTreeIter*);
static GtkTreePath* listview__get_first_selected_path();
#if NEVER
static int      listview__path_get_id            (GtkTreePath*);
static gboolean treeview_get_tags_cell  (GtkTreeView*, guint x, guint y, GtkCellRenderer**);
#endif


void
listview__new()
{
	//the main pane. A treeview with a list of samples.

	GtkWidget *view = app.view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(app.store));
	g_signal_connect(view, "motion-notify-event", (GCallback)listview__on_motion, NULL);
	g_signal_connect(view, "drag-data-received", G_CALLBACK(listview__drag_received), NULL); //currently the window traps this before we get here.
	g_signal_connect(view, "drag-motion", G_CALLBACK(drag_motion), NULL);
	//gtk_tree_view_set_fixed_height_mode(GTK_TREE_VIEW(view), TRUE); //supposed to be faster. gtk >= 2.6

	//set up as dnd source:
	gtk_drag_source_set(app.view, GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
	                    dnd_file_drag_types, dnd_file_drag_types_count,
	                    GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_ASK);
	g_signal_connect(G_OBJECT(app.view), "drag_data_get", G_CALLBACK(listview__dnd_get), NULL);

	//icon:
	GtkCellRenderer *cell9 = gtk_cell_renderer_pixbuf_new();
	GtkTreeViewColumn *col9 /*= app.col_icon*/ = gtk_tree_view_column_new_with_attributes("", cell9, "pixbuf", COL_ICON, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col9);
	//g_object_set(cell4,   "cell-background", "Orange",     "cell-background-set", TRUE,  NULL);
	g_object_set(G_OBJECT(cell9), "xalign", 0.0, NULL);
	gtk_tree_view_column_set_resizable(col9, TRUE);
	gtk_tree_view_column_set_min_width(col9, 0);
	gtk_tree_view_column_set_cell_data_func(col9, cell9, cell_data_bg, NULL, NULL);

#ifdef SHOW_INDEX
	GtkCellRenderer* cell0 = gtk_cell_renderer_text_new();
	GtkTreeViewColumn *col0 = gtk_tree_view_column_new_with_attributes("Id", cell0, "text", COL_IDX, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col0);
#endif

	GtkCellRenderer* cell1 = app.cell1 = gtk_cell_renderer_text_new();
	GtkTreeViewColumn *col1 = app.col_name = gtk_tree_view_column_new_with_attributes("Sample Name", cell1, "text", COL_NAME, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col1);
	gtk_tree_view_column_set_sort_column_id(col1, COL_NAME);
	gtk_tree_view_column_set_reorderable(col1, TRUE);
	gtk_tree_view_column_set_min_width(col1, 0);
	//gtk_tree_view_column_set_spacing(col1, 10);
	//g_object_set(cell1, "ypad", 0, NULL);
	gtk_tree_view_column_set_cell_data_func(col1, cell1, (gpointer)cell_data_bg, NULL, NULL);
	//gtk_tree_view_column_set_cell_data_func(col1, cell1, (gpointer)cell_bg_lighter, NULL, NULL);

	gtk_tree_view_column_set_sizing(col1, GTK_TREE_VIEW_COLUMN_FIXED);
	int width = atoi(app.config.column_widths[0]);
	if(width > 0) gtk_tree_view_column_set_fixed_width(col1, MAX(width, 30)); //FIXME set range in config section instead.
	else gtk_tree_view_column_set_fixed_width(col1, 130);

	GtkCellRenderer *cell2 = gtk_cell_renderer_text_new();
	GtkTreeViewColumn *col2 = app.col_path = gtk_tree_view_column_new_with_attributes("Path", cell2, "text", COL_FNAME, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col2);
	gtk_tree_view_column_set_sort_column_id(col2, COL_FNAME);
	//gtk_tree_view_column_set_resizable(col2, FALSE);
	gtk_tree_view_column_set_reorderable(col2, TRUE);
	gtk_tree_view_column_set_min_width(col2, 0);
	//g_object_set(cell2, "ypad", 0, NULL);
	gtk_tree_view_column_set_cell_data_func(col2, cell2, path_cell_data, NULL, NULL);
#ifdef USE_AYYI
	//icon that shows when file is in current active song.
	GtkCellRenderer *ayyi_renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(col2, ayyi_renderer, FALSE);
	gtk_tree_view_column_add_attribute(col2, ayyi_renderer, "pixbuf", COL_AYYI_ICON);
	//gtk_tree_view_column_set_cell_data_func(col2, ayyi_renderer, vdtree_color_cb, vdt, NULL);
#endif

	gtk_tree_view_column_set_sizing(col2, GTK_TREE_VIEW_COLUMN_FIXED);
	width = atoi(app.config.column_widths[1]);
	if(width > 0) gtk_tree_view_column_set_fixed_width(col2, MAX(width, 30));
	else gtk_tree_view_column_set_fixed_width(col2, 130);

	//GtkCellRenderer *cell3 /*= app.cell_tags*/ = gtk_cell_renderer_text_new();
	GtkCellRenderer *cell3 = app.cell_tags = gtk_cell_renderer_hyper_text_new();
	GtkTreeViewColumn *column3 = app.col_tags = gtk_tree_view_column_new_with_attributes("Tags", cell3, "text", COL_KEYWORDS, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), column3);
	gtk_tree_view_column_set_sort_column_id(column3, COL_KEYWORDS);
	gtk_tree_view_column_set_resizable(column3, TRUE);
	gtk_tree_view_column_set_reorderable(column3, TRUE);
	gtk_tree_view_column_set_min_width(column3, 0);
	g_object_set(cell3, "editable", TRUE, NULL);
	g_signal_connect(cell3, "edited", (GCallback)listview__on_keywords_edited, NULL);
	gtk_tree_view_column_add_attribute(column3, cell3, "markup", COL_KEYWORDS);
	gtk_tree_view_column_set_cell_data_func(column3, cell3, tag_cell_data, NULL, NULL);

	GtkCellRenderer *cell4 = gtk_cell_renderer_pixbuf_new();
	GtkTreeViewColumn *col4 = app.col_pixbuf = gtk_tree_view_column_new_with_attributes("Overview", cell4, "pixbuf", COL_OVERVIEW, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col4);
	//g_object_set(cell4,   "cell-background", "Orange",     "cell-background-set", TRUE,  NULL);
	g_object_set(G_OBJECT(cell4), "xalign", 0.0, NULL);
	gtk_tree_view_column_set_resizable(col4, TRUE);
	gtk_tree_view_column_set_min_width(col4, 0);
	//g_object_set(cell4, "ypad", 0, NULL);
	gtk_tree_view_column_set_cell_data_func(col4, cell4, cell_data_bg, NULL, NULL);

	GtkCellRenderer* cell5 = gtk_cell_renderer_text_new();
	GtkTreeViewColumn *col5 = gtk_tree_view_column_new_with_attributes("Length", cell5, "text", COL_LENGTH, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col5);
	gtk_tree_view_column_set_resizable(col5, TRUE);
	gtk_tree_view_column_set_reorderable(col5, TRUE);
	gtk_tree_view_column_set_min_width(col5, 0);
	g_object_set(G_OBJECT(cell5), "xalign", 1.0, NULL);
	//g_object_set(cell5, "ypad", 0, NULL);
	gtk_tree_view_column_set_cell_data_func(col5, cell5, cell_data_bg, NULL, NULL);

	GtkCellRenderer* cell6 = gtk_cell_renderer_text_new();
	GtkTreeViewColumn *col6 = gtk_tree_view_column_new_with_attributes("Srate", cell6, "text", COL_SAMPLERATE, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col6);
	gtk_tree_view_column_set_resizable(col6, TRUE);
	gtk_tree_view_column_set_reorderable(col6, TRUE);
	gtk_tree_view_column_set_min_width(col6, 0);
	g_object_set(G_OBJECT(cell6), "xalign", 1.0, NULL);
	//g_object_set(cell6, "ypad", 0, NULL);
	gtk_tree_view_column_set_cell_data_func(col6, cell6, cell_data_bg, NULL, NULL);

	GtkCellRenderer* cell7 = gtk_cell_renderer_text_new();
	GtkTreeViewColumn *col7 = gtk_tree_view_column_new_with_attributes("Chs", cell7, "text", COL_CHANNELS, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col7);
	gtk_tree_view_column_set_resizable(col7, TRUE);
	gtk_tree_view_column_set_reorderable(col7, TRUE);
	gtk_tree_view_column_set_min_width(col7, 0);
	g_object_set(G_OBJECT(cell7), "xalign", 1.0, NULL);
	gtk_tree_view_column_set_cell_data_func(col7, cell7, cell_data_bg, NULL, NULL);

	GtkCellRenderer* cell8 = gtk_cell_renderer_text_new();
	GtkTreeViewColumn *col8 = gtk_tree_view_column_new_with_attributes("Mimetype", cell8, "text", COL_MIMETYPE, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col8);
	gtk_tree_view_column_set_resizable(col8, TRUE);
	gtk_tree_view_column_set_reorderable(col8, TRUE);
	gtk_tree_view_column_set_min_width(col8, 0);
	//g_object_set(G_OBJECT(cell8), "xalign", 1.0, NULL);
	gtk_tree_view_column_set_cell_data_func(col8, cell8, cell_data_bg, NULL, NULL);

	GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(app.view));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);
	gtk_tree_view_set_search_column(GTK_TREE_VIEW(app.view), COL_NAME);

	g_signal_connect((gpointer)app.view, "button-press-event", G_CALLBACK(listview__on_row_clicked), NULL);
	g_signal_connect((gpointer)app.view, "cursor-changed", G_CALLBACK(listview__on_cursor_change), NULL);
}


#if NEVER
static int
listview__path_get_id(GtkTreePath* path)
{
	int id;
	GtkTreeIter iter;
	gchar *filename;
	GtkTreeModel* store = GTK_TREE_MODEL(app.store);
	gtk_tree_model_get_iter(store, &iter, path);
	gtk_tree_model_get(store, &iter, COL_IDX, &id, COL_NAME, &filename, -1);

	dbg(1, "filename=%s", filename);
	if(!id) pwarn("failed to get id! id must be non-zero.");
	return id;
}

void
listview__show_db_missing()
{
	PF;
	// deprecated - use message window instead.
	static gboolean done = FALSE;
	if(done) return;

	GtkTreeIter iter;
	gtk_list_store_append(app.store, &iter);
	gtk_list_store_set(app.store, &iter, COL_NAME, "no database available", -1); 

	done = TRUE;
}
#endif


static gboolean
listview__on_row_clicked(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
	GtkTreeView *treeview = GTK_TREE_VIEW(app.view);

	if(event->button == 1){
		GtkTreePath *path;
		if(gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(app.view), (gint)event->x, (gint)event->y, &path, NULL, NULL, NULL)){

			//auditioning:
			GdkRectangle rect;
			gtk_tree_view_get_cell_area(treeview, path, app.col_pixbuf, &rect);
			if(((gint)event->x > rect.x) && ((gint)event->x < (rect.x + rect.width))){

				//overview column:
				dbg(2, "overview. column rect: %i %i %i %i", rect.x, rect.y, rect.width, rect.height);

				Sample* sample = sample_get_from_model(path);

				if(sample->id != app.playing_id){
					if(app.playing_id){
						//a sample was previously played, and it wasnt this one
						app.auditioner->play(sample);
					}else{
						app.auditioner->toggle(sample);
					}
					app.playing_id = sample->id;
				}
				else {
					app.auditioner->toggle(sample);
				}
#if 1 /* highlight played items - until new search or user resets it via menu. */
				highlight_playing_by_path(path);
#endif
				sample_unref(sample);
			}else{
				gtk_tree_view_get_cell_area(treeview, path, app.col_tags, &rect);
				if(((gint)event->x > rect.x) && ((gint)event->x < (rect.x + rect.width))){
					//tags column:
					GtkTreeIter iter;
					GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(app.view));
					gtk_tree_model_get_iter(model, &iter, path);
					gchar *tags;
					int id;
					gtk_tree_model_get(model, &iter, /*COL_FNAME, &fpath, COL_NAME, &fname, */COL_KEYWORDS, &tags, COL_IDX, &id, -1);

					if(tags && strlen(tags)){
						gtk_entry_set_text(GTK_ENTRY(app.search), tags);
						strncpy(app.search_phrase, tags, 255);
						do_search(tags, NULL);
					}
				}
			}
			gtk_tree_path_free(path);
		}
		return NOT_HANDLED;
	}

	//popup menu:
	if(event->button == 3){
		dbg (2, "right button press!");

		//if one or no rows selected, select one:
		GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(app.view));
        if(gtk_tree_selection_count_selected_rows(selection) <= 1){
			GtkTreePath* path;
			if(gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(app.view), (gint)event->x, (gint)event->y, &path, NULL, NULL, NULL)){
				gtk_tree_selection_unselect_all(selection);
				gtk_tree_selection_select_path(selection, path);
				gtk_tree_path_free(path);
			}
		}

		GtkWidget *context_menu = app.context_menu;
		if(context_menu && (GPOINTER_TO_INT(context_menu) > 1024)){
			//open pop-up menu:
			gtk_menu_popup(GTK_MENU(context_menu),
                           NULL, NULL,  //parents
                           NULL, NULL,  //fn and data used to position the menu.
                           event->button, event->time);
		}
		return true;
	} else return false;
}


static void
listview__on_cursor_change(GtkTreeView* widget, gpointer user_data)
{
	dbg(2, "...");
	Sample* s;
	if((s = listview__get_first_selected_result())){
		observer__item_selected(s);
		sample_unref(s);
	}
}


gboolean
listview__item_set_colour(GtkTreePath* path, unsigned colour_index)
{
	g_return_val_if_fail(path, false);
	GtkTreeIter iter;
	gtk_tree_model_get_iter(GTK_TREE_MODEL(app.store), &iter, path);

	if(!listmodel__update_by_ref(&iter, COL_COLOUR, (void*)&colour_index)) {
		statusbar_print(1, "error! colour not updated");
		return false;
	}
	statusbar_print(1, "colour updated");
	return true;
}

static gboolean
listview__get_first_selected_iter(GtkTreeIter* iter)
{
	GtkTreeSelection* selection = gtk_tree_view_get_selection((GtkTreeView*)app.view);
	GList* list                 = gtk_tree_selection_get_selected_rows(selection, NULL);
	if(list){
		GtkTreePath* path = list->data;

		GtkTreeModel* model = gtk_tree_view_get_model((GtkTreeView*)app.view);
		gtk_tree_model_get_iter(model, iter, path);

		g_list_foreach (list, (GFunc)gtk_tree_path_free, NULL);
		g_list_free (list);

		return true;
	}
	return false;
}

void
listview__reset_colours()
{
	gboolean reset_colours (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data) {
		Sample *s = sample_get_by_tree_iter(iter);
		gtk_list_store_set(app.store, iter, COL_COLOUR, s->colour_index, -1);
		sample_unref(s);
		return FALSE;
	}

	GtkTreeModel* model = GTK_TREE_MODEL(app.store);
	gtk_tree_model_foreach(model, &reset_colours, NULL);
}

static GtkTreePath*
listview__get_first_selected_path()
{
	GtkTreeSelection* selection = gtk_tree_view_get_selection((GtkTreeView*)app.view);
	GList* list                 = gtk_tree_selection_get_selected_rows(selection, NULL);
	if(list){
		GtkTreePath* path = gtk_tree_path_copy(list->data);

		g_list_foreach (list, (GFunc)gtk_tree_path_free, NULL);
		g_list_free (list);

		return path;
	}
	return NULL;
}

/**
 * @return: Sample* -- must be sample_unref() after use.
 */
Sample*
listview__get_first_selected_result()
{
	GtkTreePath* path = listview__get_first_selected_path();
	if(path){
		Sample* result = sample_get_from_model(path);
		gtk_tree_path_free(path);
		return result;
	}
	return NULL;
}


gchar*
listview__get_first_selected_filename()
{
	GtkTreeIter iter;
	if(listview__get_first_selected_iter(&iter)){
		gchar* fname;
		gtk_tree_model_get(gtk_tree_view_get_model((GtkTreeView*)app.view), &iter, COL_NAME, &fname, -1);
		return fname;
	}
	return NULL;
}


gchar*
listview__get_first_selected_filepath()
{
	//returned value must be freed

	GtkTreeIter iter;
	if(listview__get_first_selected_iter(&iter)){
		gchar* fname, *path;
		gtk_tree_model_get(gtk_tree_view_get_model((GtkTreeView*)app.view), &iter, COL_NAME, &fname, COL_FNAME, &path, -1);
		gchar* filepath = g_build_filename(path, fname, NULL);
		return filepath;
	}
	return NULL;
}


static void
listview__dnd_get(GtkWidget *widget, GdkDragContext *context, GtkSelectionData *selection_data, guint info, guint time, gpointer data)
{
	//outgoing drop. provide the dropee with info on which samples were dropped.

	GtkTreeModel* model = GTK_TREE_MODEL(app.store);
	GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(app.view));
	GList* selected_rows = gtk_tree_selection_get_selected_rows(selection, &(model));

	GList* list = NULL;
	GList* row  = selected_rows;

	for(; row; row=row->next){
		GtkTreePath* treepath_selection = row->data;
		Sample *s = sample_get_from_model(treepath_selection);
		//append is slow, but g_list_prepend() is wrong order :(
		list = g_list_append(list, g_strdup(s->full_path));
		sample_unref(s);
		gtk_tree_path_free(treepath_selection);
	}

	gchar *uri_text = NULL;
	gint length = 0;
	switch (info) {
		case TARGET_URI_LIST:
		case TARGET_TEXT_PLAIN:
			uri_text = uri_text_from_list(list, &length, (info == TARGET_TEXT_PLAIN));
			break;
	}

	if (uri_text) {
		gtk_selection_data_set(selection_data, selection_data->target, BITS_PER_CHAR_8, (guchar*)uri_text, length);
		g_free(uri_text);
	}

	GList* l = list;
	for(;l;l=l->next) g_free(l->data);
	g_list_free(list);
	g_list_free(selected_rows);
}


static gint
listview__drag_received(GtkWidget *widget, GdkDragContext *drag_context, gint x, gint y, GtkSelectionData *data, guint info, guint time, gpointer user_data)
{
	PF;
	return FALSE;
}


void
treeview_block_motion_handler()
{
	if(app.view){
		gulong id1 = g_signal_handler_find(app.view,
							   G_SIGNAL_MATCH_FUNC, // | G_SIGNAL_MATCH_DATA,
							   0,//arrange->hzoom_handler,   //guint signal_id      ?handler_id?
							   0,        //GQuark detail
							   0,        //GClosure *closure
							   listview__on_motion, //callback
							   NULL);    //data
		if(id1) g_signal_handler_block(app.view, id1);
		else warnprintf("%s(): failed to find handler.\n", __func__);

		gtk_tree_row_reference_free(app.mouseover_row_ref);
		app.mouseover_row_ref = NULL;
	}
}


void
treeview_unblock_motion_handler()
{
	PF;
	if(app.view){
		gulong id1 = g_signal_handler_find(app.view,
							   G_SIGNAL_MATCH_FUNC, // | G_SIGNAL_MATCH_DATA,
							   0,        //guint signal_id
							   0,        //GQuark detail
							   0,        //GClosure *closure
							   listview__on_motion, //callback
							   NULL);    //data
		if(id1) g_signal_handler_unblock(app.view, id1);
		else warnprintf("%s(): failed to find handler.\n", __func__);
	}
}


static gboolean
listview__on_motion(GtkWidget *widget, GdkEventMotion *event, gpointer user_data)
{
	//static gint prev_row_num = 0;
	static GtkTreeRowReference* prev_row_ref = NULL;
	static gchar prev_text[256] = "";
	app.mouse_x = event->x;
	app.mouse_y = event->y;
	//gdouble x = event->x; //distance from left edge of treeview.
	//gdouble y = event->y; //distance from bottom of column heads.
	//GdkRectangle rect;
	//gtk_tree_view_get_cell_area(widget, GtkTreePath *path, GtkTreeViewColumn *column, &rect);

	//GtkCellRenderer *cell = app.cell_tags;
	//GList* gtk_cell_view_get_cell_renderers(GtkCellView *cell_view);
	//GList* gtk_tree_view_column_get_cell_renderers(GtkTreeViewColumn *tree_column);

	/*
	GtkCellRenderer *cell_renderer = NULL;
	if(treeview_get_tags_cell(GTK_TREE_VIEW(app.view), (gint)event->x, (gint)event->y, &cell_renderer)){
		printf("%s() tags cell found!\n", __func__);
		g_object_set(cell_renderer, "markup", "<b>important</b>", "text", NULL, NULL);
	}
	*/

	//dbg(1, "x=%f y=%f. l=%i", x, y, rect.x, rect.width);


	//which row are we on?
	GtkTreePath* path;
	GtkTreeIter iter, prev_iter;
	//GtkTreeRowReference* row_ref = NULL;
	if(gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(app.view), (gint)event->x, (gint)event->y, &path, NULL, NULL, NULL)){

		gtk_tree_model_get_iter(GTK_TREE_MODEL(app.store), &iter, path);
		gchar* path_str = gtk_tree_model_get_string_from_iter(GTK_TREE_MODEL(app.store), &iter);

		if(prev_row_ref){
			GtkTreePath* prev_path;
			if((prev_path = gtk_tree_row_reference_get_path(prev_row_ref))){
			
				gtk_tree_model_get_iter(GTK_TREE_MODEL(app.store), &prev_iter, prev_path);
				gchar* prev_path_str = gtk_tree_model_get_string_from_iter(GTK_TREE_MODEL(app.store), &prev_iter);

				//if(row_ref != prev_row_ref)
				//if(prev_path && (path != prev_path))
				if(prev_path && (atoi(path_str) != atoi(prev_path_str)))
				{
					dbg(0, "new row! path=%p (%s) prev_path=%p (%s)", path, path_str, prev_path, prev_path_str);

					//restore text to previous row:
					gtk_list_store_set(app.store, &prev_iter, COL_KEYWORDS, prev_text, -1);

					//store original text:
					gchar* txt;
					gtk_tree_model_get(GTK_TREE_MODEL(app.store), &iter, COL_KEYWORDS, &txt, -1);
					dbg(0, "text=%s", prev_text);
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
						gtk_list_store_set(app.store, &iter, COL_KEYWORDS, joined, -1);

						if (joined) g_free(joined);
					}
						*/

					g_free(prev_row_ref);
					prev_row_ref = gtk_tree_row_reference_new(GTK_TREE_MODEL(app.store), path);
					//prev_row_ref = row_ref;
				}
			}else{
				//table has probably changed. previous row is not available.
				g_free(prev_row_ref);
				prev_row_ref = NULL;
			}

			gtk_tree_path_free(prev_path);

		}else{
			prev_row_ref = gtk_tree_row_reference_new(GTK_TREE_MODEL(app.store), path);
		}

		gtk_tree_path_free(path);
	}

	dbg(2, "setting rowref=%p", prev_row_ref);
	app.mouseover_row_ref = prev_row_ref;
	return false;
}

/**the keywords column has been edited. Update the database to reflect the new text.  */
static void
listview__on_keywords_edited(GtkCellRendererText *cell, gchar *path_string, gchar *new_text, gpointer user_data)
{
	PF;
	GtkTreeIter iter;
	int idx;
	gchar *filename;
	GtkTreeModel* store = GTK_TREE_MODEL(app.store);
	GtkTreePath* path = gtk_tree_path_new_from_string(path_string);
	gtk_tree_model_get_iter(store, &iter, path);
	gtk_tree_model_get(store, &iter, COL_IDX, &idx, COL_NAME, &filename, -1);
	dbg(1, "filename=%s idx=%i", filename, idx);
	gtk_tree_path_free(path);

	//convert to lowercase:
	//gchar* lower = g_ascii_strdown(new_text, -1);
	//g_free(lower);

	if(listmodel__update_by_ref(&iter, COL_KEYWORDS, (void*)new_text)) {
		statusbar_print(1, "keywords updated");
	}
	else statusbar_print(1, "database error! keywords not updated");
}


static void
path_cell_data(GtkTreeViewColumn *tree_column, GtkCellRenderer *cell, GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer data)
{
	cell_data_bg(tree_column, cell, tree_model, iter, data);

	GtkCellRendererText *celltext = (GtkCellRendererText*)cell;
	if(celltext){
		gchar* text = g_strdup(dir_format(celltext->text));
		g_free(celltext->text);
		celltext->text = text;
	}
}


static void
tag_cell_data(GtkTreeViewColumn *tree_column, GtkCellRenderer *cell, GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer data)
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

	//set background colour:
	cell_data_bg(tree_column, cell, tree_model, iter, data);

	//----------------------

	GtkCellRendererText *celltext = (GtkCellRendererText *)cell;
	GtkCellRendererHyperText *hypercell = (GtkCellRendererHyperText *)cell;
	GtkTreePath* path = gtk_tree_model_get_path(GTK_TREE_MODEL(app.store), iter);
	GdkRectangle cellrect;

	gint mouse_row_num = get_mouseover_row();

	gchar* path_str = gtk_tree_model_get_string_from_iter(GTK_TREE_MODEL(app.store), iter);
	gint cell_row_num = atoi(path_str);

	//----------------------

	//get the coords for this cell:
	gtk_tree_view_get_cell_area(GTK_TREE_VIEW(app.view), path, tree_column, &cellrect);
	gtk_tree_path_free(path);
	//dbg(0, "%s mouse_y=%i cell_y=%i-%i.\n", path_str, app.mouse_y, cellrect.y, cellrect.y + cellrect.height);
	//if(//(app.mouse_x > cellrect.x) && (app.mouse_x < (cellrect.x + cellrect.width)) &&
	//			(app.mouse_y >= cellrect.y) && (app.mouse_y <= (cellrect.y + cellrect.height)))
	if(cell_row_num == mouse_row_num)
	{
	if((app.mouse_x > cellrect.x) && (app.mouse_x < (cellrect.x + cellrect.width))){
		//printf("tag_cell_data(): mouseover! row_num=%i\n", mouse_row_num);
		//printf("tag_cell_data():  inside: x=%i y=%i\n", cellrect.x, cellrect.y);

		if(strlen(celltext->text)){
			g_strstrip(celltext->text);//trim

			gint mouse_cell_x = app.mouse_x - cellrect.x;

			//make a layout to find word sizes:

			PangoContext* context = gtk_widget_get_pango_context(app.view); //free?
			PangoLayout* layout = pango_layout_new(context);
			pango_layout_set_text(layout, celltext->text, strlen(celltext->text));

			int line_num = 0;
			PangoLayoutLine* layout_line = pango_layout_get_line(layout, line_num);
			int char_pos;
			gboolean trailing = 0;
			//printf("tag_cell_data(): line len: %i\n", layout_line->length);
			int i;
			for(i=0;i<layout_line->length;i++){
				//pango_layout_line_index_to_x(layout_line, i, trailing, &char_pos);
				//printf("tag_cell_data(): x=%i\n", (int)(char_pos/PANGO_SCALE));
			}

			//-------------------------

			//split the string into words:

			const gchar *str = celltext->text;
			gchar** split = g_strsplit(str, " ", 100);
			int char_index = 0;
			int word_index = 0;
			gchar formatted[256] = "";
			char word[256] = "";
			while(split[word_index]){
				char_index += strlen(split[word_index]);

				pango_layout_line_index_to_x(layout_line, char_index, trailing, &char_pos);
				if(char_pos/PANGO_SCALE > mouse_cell_x){
					dbg(0, "word=%i\n", word_index);

					snprintf(word, 256, "<u>%s</u> ", split[word_index]);
					g_strlcat(formatted, word, 256);

					while(split[++word_index]){
						snprintf(word, 256, "%s ", split[word_index]);
						g_strlcat(formatted, word, 256);
					}

					break;
				}

				snprintf(word, 256, "%s ", split[word_index]);
				g_strlcat(formatted, word, 256);

				word_index++;
			}
			dbg(0, "joined: %s\n", formatted);

			g_object_unref(layout);

			//-------------------------

			//set new markup:

			//g_object_set();
			gchar *text = NULL;
			GError *error = NULL;
			PangoAttrList *attrs = NULL;

			if (/*formatted &&*/ !pango_parse_markup(formatted, -1, 0, &attrs, &text, NULL, &error)){
				g_warning("Failed to set cell text from markup due to error parsing markup: %s", error->message);
				g_error_free(error);
				return;
			}
			//if (joined) g_free(joined);
			if (celltext->text) g_free(celltext->text);
			if (celltext->extra_attrs) pango_attr_list_unref(celltext->extra_attrs);

			//setting text here doesnt seem to work (text is set but not displayed), but setting markup does.
			//printf("tag_cell_data(): setting text: %s\n", text);
			celltext->text = text;
			celltext->extra_attrs = attrs;
			hypercell->markup_set = true;
		}
	}
	}
	//else g_object_set(cell, "markup", "outside", NULL);
	//else hypercell->markup_set = false;

	g_free(path_str);


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
cell_bg_lighter(GtkTreeViewColumn *tree_column, GtkCellRenderer *cell, GtkTreeModel *tree_model, GtkTreeIter *iter)
{
	unsigned colour_index = 0;
	gtk_tree_model_get(GTK_TREE_MODEL(app.store), iter, COL_COLOUR, &colour_index, -1);
	if(colour_index > PALETTE_SIZE){ gwarn("bad colour data. Index out of range (%u).\n", colour_index); return; }

	if(colour_index == 0){
		colour_index = 4; //FIXME temp
	}

	if(strlen(app.config.colour[colour_index])){
		GdkColor colour, colour2;
		char hexstring[8];
		snprintf(hexstring, 8, "#%s", app.config.colour[colour_index]);
		if(!gdk_color_parse(hexstring, &colour)) gwarn("parsing of colour string failed.\n");
		colour_lighter(&colour2, &colour);

		g_object_set(cell, "cell-background-set", true, "cell-background-gdk", &colour2, NULL);
	}
}
#endif

static void
cell_data_bg(GtkTreeViewColumn *tree_column, GtkCellRenderer *cell, GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer data)
{
	unsigned colour_index = 0;
	char colour[16] = "#606060";
	gtk_tree_model_get(GTK_TREE_MODEL(app.store), iter, COL_COLOUR, &colour_index, -1);
	if(colour_index < PALETTE_SIZE) { 
		if(strlen(app.config.colour[colour_index])){
			snprintf(colour, 16, "#%s", app.config.colour[colour_index]);
			if(strlen(colour) != 7 ){ perr("bad colour string (%s) index=%u.\n", colour, colour_index); return; }
		}
		else colour_index = 0;
	}

	if(colour_index) g_object_set(cell, "cell-background-set", true, "cell-background", colour, NULL);
	else             g_object_set(cell, "cell-background-set", false, NULL);
}

#if NEVER
static gboolean
treeview_get_tags_cell(GtkTreeView *view, guint x, guint y, GtkCellRenderer **cell)
{
	GtkTreeViewColumn *colm = NULL;
	guint              colm_x = 0/*, colm_y = 0*/;

	GList* columns = gtk_tree_view_get_columns(view);

	GList* node;
	for (node = columns;  node != NULL && colm == NULL;  node = node->next){
		GtkTreeViewColumn *checkcolm = (GtkTreeViewColumn*) node->data;

		if (x >= colm_x  &&  x < (colm_x + checkcolm->width))
			colm = checkcolm;
		else
			colm_x += checkcolm->width;
	}

	g_list_free(columns);

	if(colm == NULL) return false; // not found
	if(colm != app.col_tags) return false;

	// (2) find the cell renderer within the column 

	GList* cells = gtk_tree_view_column_get_cell_renderers(colm);
	GdkRectangle cell_rect;

	for (node = cells;  node != NULL;  node = node->next){
		GtkCellRenderer *checkcell = (GtkCellRenderer*) node->data;
		guint            width = 0, height = 0;

		// Will this work for all packing modes? doesn't that return a random width depending on the last content rendered?
		gtk_cell_renderer_get_size(checkcell, GTK_WIDGET(view), &cell_rect, NULL, NULL, (int*)&width, (int*)&height);
		printf("y=%i height=%i\n", cell_rect.y, cell_rect.height);

		//if(x >= colm_x && x < (colm_x + width))
		//if(y >= colm_y && y < (colm_y + height))
		if(y >= cell_rect.y && y < (cell_rect.y + cell_rect.height))
		{
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

void 
highlight_playing_by_path (GtkTreePath *path) {
	GtkTreeIter iter;
	gtk_tree_model_get_iter(GTK_TREE_MODEL(app.store), &iter, path);
	gtk_list_store_set(GTK_LIST_STORE(app.store), &iter, COL_COLOUR, /*colour*/ PALETTE_SIZE, -1);
}

void 
highlight_playing_by_ref (GtkTreeRowReference *ref) {
	GtkTreePath *path;
	if (!ref || !gtk_tree_row_reference_valid(ref)) return;
	if(!(path = gtk_tree_row_reference_get_path(ref))) return;
	highlight_playing_by_path(path);
	gtk_tree_path_free(path);
}
