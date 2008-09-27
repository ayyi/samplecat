#include "config.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <gtk/gtk.h>
#include "mysql/mysql.h"
#ifdef USE_AYYI
#include <ayyi/ayyi.h>
#endif
#include "dh-link.h"

#include "typedefs.h"
#include <gqview2/typedefs.h>
#include "support.h"
#include "main.h"
#include "sample.h"
#include "dnd.h"
#include "cellrenderer_hypertext.h"
#include "inspector.h"
#include "listview.h"

extern struct _app app;
int             playback_init(sample* sample);
void            playback_stop();

static gboolean listview__on_row_clicked(GtkWidget*, GdkEventButton*, gpointer user_data);
static void     listview__dnd_get       (GtkWidget*, GdkDragContext*, GtkSelectionData*, guint info, guint time, gpointer data);
static gint     listview__drag_received (GtkWidget*, GdkDragContext*, gint x, gint y, GtkSelectionData*, guint info, guint time, gpointer user_data);


void
listview__new()
{
	//the main pane. A treeview with a list of samples.

	GtkWidget *view = app.view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(app.store));
	//g_signal_connect(view, "motion-notify-event", (GCallback)treeview_on_motion, NULL); //FIXME this is causing segfaults?
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
	gtk_tree_view_column_set_cell_data_func(col9, cell9, path_cell_data_func, NULL, NULL);

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
	gtk_tree_view_column_set_cell_data_func(col1, cell1, (gpointer)path_cell_bg_lighter, NULL, NULL);

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
	gtk_tree_view_column_set_cell_data_func(col2, cell2, path_cell_data_func, NULL, NULL);
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
	g_signal_connect(cell3, "edited", (GCallback)keywords_on_edited, NULL);
	gtk_tree_view_column_add_attribute(column3, cell3, "markup", COL_KEYWORDS);

	//GtkTreeCellDataFunc
	gtk_tree_view_column_set_cell_data_func(column3, cell3, tag_cell_data, NULL, NULL);

	GtkCellRenderer *cell4 = gtk_cell_renderer_pixbuf_new();
	GtkTreeViewColumn *col4 = app.col_pixbuf = gtk_tree_view_column_new_with_attributes("Overview", cell4, "pixbuf", COL_OVERVIEW, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col4);
	//g_object_set(cell4,   "cell-background", "Orange",     "cell-background-set", TRUE,  NULL);
	g_object_set(G_OBJECT(cell4), "xalign", 0.0, NULL);
	gtk_tree_view_column_set_resizable(col4, TRUE);
	gtk_tree_view_column_set_min_width(col4, 0);
	//g_object_set(cell4, "ypad", 0, NULL);
	gtk_tree_view_column_set_cell_data_func(col4, cell4, path_cell_data_func, NULL, NULL);

	GtkCellRenderer* cell5 = gtk_cell_renderer_text_new();
	GtkTreeViewColumn *col5 = gtk_tree_view_column_new_with_attributes("Length", cell5, "text", COL_LENGTH, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col5);
	gtk_tree_view_column_set_resizable(col5, TRUE);
	gtk_tree_view_column_set_reorderable(col5, TRUE);
	gtk_tree_view_column_set_min_width(col5, 0);
	g_object_set(G_OBJECT(cell5), "xalign", 1.0, NULL);
	//g_object_set(cell5, "ypad", 0, NULL);
	gtk_tree_view_column_set_cell_data_func(col5, cell5, path_cell_data_func, NULL, NULL);

	GtkCellRenderer* cell6 = gtk_cell_renderer_text_new();
	GtkTreeViewColumn *col6 = gtk_tree_view_column_new_with_attributes("Srate", cell6, "text", COL_SAMPLERATE, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col6);
	gtk_tree_view_column_set_resizable(col6, TRUE);
	gtk_tree_view_column_set_reorderable(col6, TRUE);
	gtk_tree_view_column_set_min_width(col6, 0);
	g_object_set(G_OBJECT(cell6), "xalign", 1.0, NULL);
	//g_object_set(cell6, "ypad", 0, NULL);
	gtk_tree_view_column_set_cell_data_func(col6, cell6, path_cell_data_func, NULL, NULL);

	GtkCellRenderer* cell7 = gtk_cell_renderer_text_new();
	GtkTreeViewColumn *col7 = gtk_tree_view_column_new_with_attributes("Chs", cell7, "text", COL_CHANNELS, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col7);
	gtk_tree_view_column_set_resizable(col7, TRUE);
	gtk_tree_view_column_set_reorderable(col7, TRUE);
	gtk_tree_view_column_set_min_width(col7, 0);
	g_object_set(G_OBJECT(cell7), "xalign", 1.0, NULL);
	gtk_tree_view_column_set_cell_data_func(col7, cell7, path_cell_data_func, NULL, NULL);

	GtkCellRenderer* cell8 = gtk_cell_renderer_text_new();
	GtkTreeViewColumn *col8 = gtk_tree_view_column_new_with_attributes("Mimetype", cell8, "text", COL_MIMETYPE, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col8);
	gtk_tree_view_column_set_resizable(col8, TRUE);
	gtk_tree_view_column_set_reorderable(col8, TRUE);
	gtk_tree_view_column_set_min_width(col8, 0);
	//g_object_set(G_OBJECT(cell8), "xalign", 1.0, NULL);
	gtk_tree_view_column_set_cell_data_func(col8, cell8, path_cell_data_func, NULL, NULL);

	GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(app.view));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);
	gtk_tree_view_set_search_column(GTK_TREE_VIEW(app.view), COL_NAME);

	g_signal_connect((gpointer)app.view, "button-press-event", G_CALLBACK(listview__on_row_clicked), NULL);

	gtk_widget_show(view);
}


void
list__update()
{
	do_search(NULL, NULL);
}


int
listview__path_get_id(GtkTreePath* path)
{
	int id;
	GtkTreeIter iter;
	gchar *filename;
	GtkTreeModel* store = GTK_TREE_MODEL(app.store);
	gtk_tree_model_get_iter(store, &iter, path);
	gtk_tree_model_get(store, &iter, COL_IDX, &id,  COL_NAME, &filename, -1);

	dbg(0, "filename=%s", filename);
	return id;
}


void
listview__show_db_missing()
{
	static gboolean done = FALSE;
	if(done) return;

	GtkTreeIter iter;
	gtk_list_store_append(app.store, &iter);
	gtk_list_store_set(app.store, &iter, COL_NAME, "no database available", -1); 

	done = TRUE;
}


static gboolean
listview__on_row_clicked(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
	GtkTreeView *treeview = GTK_TREE_VIEW(app.view);

	//auditioning:
	if(event->button == 1){
		GtkTreePath *path;
		if(gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(app.view), (gint)event->x, (gint)event->y, &path, NULL, NULL, NULL)){
			inspector_update_from_listview(path);

			GdkRectangle rect;
			gtk_tree_view_get_cell_area(treeview, path, app.col_pixbuf, &rect);
			if(((gint)event->x > rect.x) && ((gint)event->x < (rect.x + rect.width))){

				//overview column:
				dbg(2, "column rect: %i %i %i %i", rect.x, rect.y, rect.width, rect.height);

				/*
				GtkTreeIter iter;
				GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(app.view));
				gtk_tree_model_get_iter(model, &iter, path);
				gchar *fpath, *fname, *mimetype;
				int id;
				gtk_tree_model_get(model, &iter, COL_FNAME, &fpath, COL_NAME, &fname, COL_IDX, &id, COL_MIMETYPE, &mimetype, -1);

				char file[256]; snprintf(file, 256, "%s/%s", fpath, fname);
				*/

				sample* sample = sample_new_from_model(path);

				if(sample->id != app.playing_id){
					if(!playback_init(sample)) sample_free(sample);
				}
				else playback_stop();
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

					if(strlen(tags)){
						gtk_entry_set_text(GTK_ENTRY(app.search), tags);
						do_search(tags, NULL);
					}
				}
			}
		}
		return FALSE; //propogate the signal
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
		return TRUE;
	} else return FALSE;
}


gboolean
listview__item_set_colour(GtkTreePath* path, unsigned colour)
{
	ASSERT_POINTER_FALSE(path, "path")

	int id = listview__path_get_id(path);

	char sql[1024];
	snprintf(sql, 1024, "UPDATE samples SET colour=%u WHERE id=%i", colour, id);
	printf("item_set_colour(): sql=%s\n", sql);
	if(mysql_query(&app.mysql, sql)){
		errprintf("item_set_colour(): update failed! sql=%s\n", sql);
		return FALSE;
	}

	GtkTreeIter iter;
	gtk_tree_model_get_iter(GTK_TREE_MODEL(app.store), &iter, path);
	gtk_list_store_set(GTK_LIST_STORE(app.store), &iter, COL_COLOUR, colour, -1);

	return TRUE;
}


static void
listview__dnd_get(GtkWidget *widget, GdkDragContext *context, GtkSelectionData *selection_data, guint info, guint time, gpointer data)
{
	//outgoing drop. provide the dropee with info on which samples were dropped.

	GtkTreeModel* model = GTK_TREE_MODEL(app.store);
	GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(app.view));
	GList* selected_rows = gtk_tree_selection_get_selected_rows(selection, &(model));

	GtkTreeIter iter;
	gchar *filename;
	gchar *pathname;
	GList* row = selected_rows;
	for(; row; row=row->next){
		GtkTreePath* treepath_selection = row->data;
		gtk_tree_model_get_iter(model, &iter, treepath_selection);

		int id;
		gtk_tree_model_get(model, &iter, COL_IDX, &id,  COL_NAME, &filename, COL_FNAME, &pathname, -1);

		dbg(0, "filename=%s", filename);
	}

	//free the selection list data:
	GList* l = selected_rows;
	for(;l;l=l->next) gtk_tree_path_free(l->data);
	g_list_free(selected_rows);

	GList *list;
	gchar *uri_text = NULL;
	gint length = 0;

	gchar* path = g_strconcat(pathname, "/", filename, NULL);

	switch (info) {
		case TARGET_URI_LIST:
		case TARGET_TEXT_PLAIN:
			list = g_list_prepend(NULL, path);
			uri_text = uri_text_from_list(list, &length, (info == TARGET_TEXT_PLAIN));
			g_list_free(list);
			break;
	}

	if (uri_text) {
		gtk_selection_data_set(selection_data, selection_data->target, BITS_PER_CHAR_8, (guchar*)uri_text, length);
		g_free(uri_text);
	}

	g_free(path);
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
		gulong id1= g_signal_handler_find(app.view,
							   G_SIGNAL_MATCH_FUNC, // | G_SIGNAL_MATCH_DATA,
							   0,//arrange->hzoom_handler,   //guint signal_id      ?handler_id?
							   0,        //GQuark detail
							   0,        //GClosure *closure
							   treeview_on_motion, //callback
							   NULL);    //data
		if(id1) g_signal_handler_block(app.view, id1);
		else warnprintf("treeview_block_motion_handler(): failed to find handler.\n");

		gtk_tree_row_reference_free(app.mouseover_row_ref);
		app.mouseover_row_ref = NULL;
	}
}


void
treeview_unblock_motion_handler()
{
	PF;
	if(app.view){
		gulong id1= g_signal_handler_find(app.view,
							   G_SIGNAL_MATCH_FUNC, // | G_SIGNAL_MATCH_DATA,
							   0,        //guint signal_id
							   0,        //GQuark detail
							   0,        //GClosure *closure
							   treeview_on_motion, //callback
							   NULL);    //data
		if(id1) g_signal_handler_unblock(app.view, id1);
		else warnprintf("%s(): failed to find handler.\n", __func__);
	}
}


