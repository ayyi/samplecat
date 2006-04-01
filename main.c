	/*
	create database samplelib;
	
	create table samples (
		id int(11) NOT NULL auto_increment,
		filename text NOT NULL,
		filedir text,
		keywords varchar(60) default '',
		pixbuf BLOB,
		PRIMARY KEY  (id)
	) TYPE=MyISAM;"

	BLOB can handle up to 64k.

	*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <my_global.h>   // for strmov
#include <m_string.h>    // for strmov
#include <sndfile.h>
#include <gtk/gtk.h>

#include <gdk-pixbuf/gdk-pixdata.h>
#include <libart_lgpl/libart.h>
#include <libgnomevfs/gnome-vfs.h>

#include "mysql/mysql.h"
#include "dh-link.h"
#include "main.h"
#include "support.h"
#include "audio.h"
#include "overview.h"
#include "cellrenderer_hypertext.h"
#include "tree.h"

#include "rox_global.h"
#include "type.h"
#include "pixmaps.h"
//#include "gnome-vfs-uri.h"

struct _app app;
GList* mime_types; // list of *MIME_type

char database_name[64] = "samplelib";

//strings for console output:
char white [16];
char red   [16];
char green [16];
char yellow[16];
char bold  [16];

//treeview/store layout:
enum
{
  COL_ICON = 0,
  COL_IDX,
  COL_NAME,
  COL_FNAME,
  COL_KEYWORDS,
  COL_OVERVIEW,
  COL_LENGTH,
  COL_SAMPLERATE,
  COL_CHANNELS,
  COL_MIMETYPE,
  NUM_COLS
};

//mysql table layout:
#define MYSQL_ONLINE 8
#define MYSQL_MIMETYPE 10
#define MYSQL_KEYWORDS 3

//dnd:
GtkTargetEntry dnd_file_drag_types[] = {
      { "text/uri-list", 0, TARGET_URI_LIST },
      { "text/plain", 0, TARGET_TEXT_PLAIN }
};
gint dnd_file_drag_types_count = 2;

char       *app_name    = "Samplelib";
const char *app_version = "0.0.1";

static const struct option long_options[] = {
  { "input",            1, NULL, 'i' },
  { "offline",          0, NULL, 'o' },
  { "help",             0, NULL, 'h' },
  { "verbose",          0, NULL, 'v' },
};

static const char* const short_options = "i:ohv";

static const char* const usage =
  "Usage: %s [ options ]\n"
  " -o --offline   dont connect to core. For testing only.\n"
  " -v --verbose   show more information (currently has no effect).\n"
  " -h --help      show this usage information and quit.\n"
  "\n";

GAsyncQueue* msg_queue = NULL;


//dummy treeview visibility filter function
gboolean 
func (GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
	return FALSE;
}


int 
mysql_exec_sql(MYSQL *mysql, const char *create_definition)
{
   return mysql_real_query(mysql,create_definition,strlen(create_definition));
}


void
app_init()
{
	//app.fg_colour.pixel = 0;
	app.dir_tree = NULL;
	app.statusbar = NULL;
	app.statusbar2 = NULL;
	//sprintf(app.search_phrase, "");
	app.search_phrase[0] = 0;
	app.search_dir = NULL;
	app.search_category = NULL;
}


int 
main(int argc, char* *argv)
{
	//make gdb break on g errors:
    g_log_set_always_fatal( G_LOG_FLAG_RECURSION | G_LOG_FLAG_FATAL | G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING );

	//init console escape commands:
	sprintf(white,  "%c[0;39m", 0x1b);
	sprintf(red,    "%c[1;31m", 0x1b);
	sprintf(green,  "%c[1;32m", 0x1b);
	sprintf(yellow, "%c[1;33m", 0x1b);
	sprintf(bold,   "%c[1;39m", 0x1b);
	sprintf(err,    "%serror!%s", red, white);
	sprintf(warn,   "%swarning:%s", yellow, white);

	app_init();

	printf("%s%s. Version %s%s\n", yellow, app_name, app_version, white);

	gtk_init(&argc, &argv);
	type_init();
	pixmaps_init();

	GError *error = NULL;
	g_thread_init(NULL);
	msg_queue = g_async_queue_new();
	if(!g_thread_create(overview_thread, NULL, FALSE, &error)){
		printf("main(): error creating thread: %s\n", error->message);
		g_error_free(error);
	}

	if (!gnome_vfs_init()){ errprintf("could not initialize GnomeVFS.\n"); return 1; }

	/*
	Calls:
	MYSQL *mysql_init(MYSQL *mysql)
	char *mysql_get_server_info(MYSQL *mysql)
	void mysql_close(MYSQL *mysql)
	*/
 
	scan_dir();

	app.store = gtk_list_store_new(NUM_COLS, GDK_TYPE_PIXBUF, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING);
	//gtk_list_store_append(store, &iter);
	//gtk_list_store_set(store, &iter, COL_NAME, "row 1", -1);
 
	//GtkTreeModel *filter = gtk_tree_model_filter_new(GTK_TREE_MODEL(store), NULL);
	//gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(filter), (GtkTreeModelFilterVisibleFunc)func, NULL, NULL);

	//detach the model from the view to speed up row inserts (actually its not yet attached):
	/*
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(view));
	g_object_ref(model);
	gtk_tree_view_set_model(GTK_TREE_VIEW(view), NULL);
	*/ 

	/*
	MYSQL *mysql_real_connect(MYSQL *mysql, const char *host, const char *user, const char *passwd, const char *db,
			unsigned int port, const char *unix_socket, unsigned int client_flag)
	*/

	MYSQL *mysql;
	if(db_connect()){
		mysql = &app.mysql;

		do_search(app.search_phrase, NULL);
	}
	//how many rows are in the list?

 
	GtkWidget *view = app.view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(app.store));
	g_signal_connect(view, "motion-notify-event", (GCallback)treeview_on_motion, NULL);
	//gtk_tree_view_set_fixed_height_mode(GTK_TREE_VIEW(view), TRUE); //supposed to be faster. gtk >= 2.6

	GtkCellRenderer *cell9 = gtk_cell_renderer_pixbuf_new();
	GtkTreeViewColumn *col9 /*= app.col_icon*/ = gtk_tree_view_column_new_with_attributes("Icon", cell9, "pixbuf", COL_ICON, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col9);
	//g_object_set(cell4,   "cell-background", "Orange",     "cell-background-set", TRUE,  NULL);
	g_object_set(G_OBJECT(cell9), "xalign", 0.0, NULL);
	gtk_tree_view_column_set_resizable(col9, TRUE);
	gtk_tree_view_column_set_min_width(col9, 0);

	GtkCellRenderer* cell0 = gtk_cell_renderer_text_new();
	GtkTreeViewColumn *col0 = gtk_tree_view_column_new_with_attributes("Id", cell0, "text", COL_IDX, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col0);
	//g_object_set(cell0, "ypad", 0, NULL);

	GtkCellRenderer* cell1 = app.cell1 = gtk_cell_renderer_text_new();
	GtkTreeViewColumn *col1 = gtk_tree_view_column_new_with_attributes("Sample Name", cell1, "text", COL_NAME, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col1);
	gtk_tree_view_column_set_sort_column_id(col1, COL_NAME);
	gtk_tree_view_column_set_resizable(col1, TRUE);
	gtk_tree_view_column_set_reorderable(col1, TRUE);
	gtk_tree_view_column_set_min_width(col1, 0);
	//gtk_tree_view_column_set_spacing(col1, 10);
	//g_object_set(cell1, "ypad", 0, NULL);
	
	GtkCellRenderer *cell2 = gtk_cell_renderer_text_new();
	GtkTreeViewColumn *col2 = gtk_tree_view_column_new_with_attributes("Path", cell2, "text", COL_FNAME, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col2);
	gtk_tree_view_column_set_sort_column_id(col2, COL_FNAME);
	gtk_tree_view_column_set_resizable(col2, TRUE);
	gtk_tree_view_column_set_reorderable(col2, TRUE);
	gtk_tree_view_column_set_min_width(col2, 0);
	//g_object_set(cell2, "ypad", 0, NULL);

	//GtkCellRenderer *cell3 /*= app.cell_tags*/ = gtk_cell_renderer_text_new();
	GtkCellRenderer *cell3 = app.cell_tags = gtk_cell_renderer_hyper_text_new();
	GtkTreeViewColumn *column3 = app.col_tags = gtk_tree_view_column_new_with_attributes("Keywords", cell3, "text", COL_KEYWORDS, NULL);
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

	GtkCellRenderer* cell5 = gtk_cell_renderer_text_new();
	GtkTreeViewColumn *col5 = gtk_tree_view_column_new_with_attributes("Length", cell5, "text", COL_LENGTH, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col5);
	gtk_tree_view_column_set_resizable(col5, TRUE);
	gtk_tree_view_column_set_reorderable(col5, TRUE);
	gtk_tree_view_column_set_min_width(col5, 0);
	g_object_set(G_OBJECT(cell5), "xalign", 1.0, NULL);
	//g_object_set(cell5, "ypad", 0, NULL);

	GtkCellRenderer* cell6 = gtk_cell_renderer_text_new();
	GtkTreeViewColumn *col6 = gtk_tree_view_column_new_with_attributes("Samplerate", cell6, "text", COL_SAMPLERATE, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col6);
	gtk_tree_view_column_set_resizable(col6, TRUE);
	gtk_tree_view_column_set_reorderable(col6, TRUE);
	gtk_tree_view_column_set_min_width(col6, 0);
	g_object_set(G_OBJECT(cell6), "xalign", 1.0, NULL);
	//g_object_set(cell6, "ypad", 0, NULL);

	GtkCellRenderer* cell7 = gtk_cell_renderer_text_new();
	GtkTreeViewColumn *col7 = gtk_tree_view_column_new_with_attributes("Channels", cell7, "text", COL_CHANNELS, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col7);
	gtk_tree_view_column_set_resizable(col7, TRUE);
	gtk_tree_view_column_set_reorderable(col7, TRUE);
	gtk_tree_view_column_set_min_width(col7, 0);
	g_object_set(G_OBJECT(cell7), "xalign", 1.0, NULL);

	GtkCellRenderer* cell8 = gtk_cell_renderer_text_new();
	GtkTreeViewColumn *col8 = gtk_tree_view_column_new_with_attributes("Mimetype", cell8, "text", COL_MIMETYPE, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col8);
	gtk_tree_view_column_set_resizable(col8, TRUE);
	gtk_tree_view_column_set_reorderable(col8, TRUE);
	gtk_tree_view_column_set_min_width(col8, 0);
	//g_object_set(G_OBJECT(cell8), "xalign", 1.0, NULL);

	GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(app.view));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);
	gtk_tree_view_set_search_column(GTK_TREE_VIEW(app.view), COL_NAME);

	gtk_widget_show(view);

	window_new(); 
	filter_new();
	tagshow_selector_new();
	tag_selector_new();
 
	//gtk_box_pack_start(GTK_BOX(app.vbox), view, EXPAND_TRUE, FILL_TRUE, 0);
	gtk_container_add(GTK_CONTAINER(app.scroll), view);

	app.context_menu = make_context_menu();
	g_signal_connect((gpointer)view, "button-press-event", G_CALLBACK(on_row_clicked), NULL);

	gtk_main();
	exit(0);
}


gboolean
window_new()
{
/*
window
+--paned
   +--dir tree
   |
   +--vbox
      +--search box
      |  +--label
      |  +--text entry
      |
      +--edit metadata hbox
      |
      |--hpaned
      +--scrollwin
      |  +--treeview
      |
      +--statusbar hbox
         +--statusbar	
         +--statusbar2

*/
	printf("window_new().\n");
	
	GtkWidget *window = app.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	g_signal_connect(window, "destroy", gtk_main_quit, NULL);

	GtkWidget *vbox = app.vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);
	gtk_container_add(GTK_CONTAINER(window), vbox);

	GtkWidget *hbox_statusbar = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox_statusbar);
	gtk_box_pack_end(GTK_BOX(vbox), hbox_statusbar, EXPAND_FALSE, FALSE, 0);

	GtkWidget *hpaned = gtk_hpaned_new();
	gtk_box_pack_end(GTK_BOX(vbox), hpaned, EXPAND_TRUE, TRUE, 0);
	gtk_widget_show(hpaned);

	GtkWidget* tree = left_pane();
	gtk_paned_add1(GTK_PANED(hpaned), tree);

	GtkWidget *scroll = app.scroll = gtk_scrolled_window_new(NULL, NULL);  //adjustments created automatically.
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_widget_show(scroll);
	//gtk_box_pack_end(GTK_BOX(vbox), scroll, EXPAND_TRUE, TRUE, 0);
	gtk_paned_add2(GTK_PANED(hpaned), scroll);

	GtkWidget *statusbar = app.statusbar = gtk_statusbar_new();
	printf("statusbar=%p\n", app.statusbar);
	//gtk_statusbar_set_has_resize_grip(GTK_STATUSBAR(statusbar), TRUE);	//why does give a warning??????
	gtk_statusbar_set_has_resize_grip(GTK_STATUSBAR(statusbar), FALSE);
	gtk_box_pack_start(GTK_BOX(hbox_statusbar), statusbar, EXPAND_TRUE, FILL_TRUE, 0);
	gtk_widget_show(statusbar);

	GtkWidget *statusbar2 = app.statusbar2 = gtk_statusbar_new();
	//gtk_statusbar_set_has_resize_grip(GTK_STATUSBAR(statusbar), TRUE);	//why does give a warning??????
	gtk_statusbar_set_has_resize_grip(GTK_STATUSBAR(statusbar2), FALSE);
	gtk_box_pack_start(GTK_BOX(hbox_statusbar), statusbar2, EXPAND_TRUE, FILL_TRUE, 0);
	gtk_widget_show(statusbar2);

	g_signal_connect(G_OBJECT(window), "size-allocate", G_CALLBACK(window_on_realise), NULL);

	gtk_widget_show_all(window);

	dnd_setup();

	return TRUE;
}


void
window_on_realise(GtkWidget *win, gpointer user_data)
{
	#define SCROLLBAR_WIDTH_HACK 32
	static gboolean done = FALSE;
	if(!app.view->requisition.width) return;

	if(!done){
		//printf("window_new(): wid=%i\n", app.view->requisition.width);
		gtk_widget_set_size_request(win, app.view->requisition.width + SCROLLBAR_WIDTH_HACK, app.view->requisition.height);
		done = TRUE;
	}else{
		//now reduce the request to allow the user to manually make the window smaller.
		gtk_widget_set_size_request(win, 100, 100);
	}

	colour_get_style_bg(&app.bg_colour, GTK_STATE_NORMAL);
	colour_get_style_fg(&app.fg_colour, GTK_STATE_NORMAL);


	//set column colours:
	//printf("window_on_realise(): fg_color: %x %x %x\n", app.fg_colour.red, app.fg_colour.green, app.fg_colour.blue);
	g_object_set(app.cell1, "cell-background-gdk", &app.fg_colour, "cell-background-set", TRUE, NULL);
	g_object_set(app.cell1, "foreground-gdk", &app.bg_colour, "foreground-set", TRUE, NULL);
}


GtkWidget*
left_pane()
{
	//examples of file navigation: nautilus? (rox has no tree)

	app.dir_tree = g_node_new(NULL);

	db_get_dirs();

	GtkWidget* tree = dh_book_tree_new(app.dir_tree);
	gtk_widget_show(tree);

	g_signal_connect(tree, "link_selected", G_CALLBACK(tree_on_link_selected), NULL);

	return tree;
}


gboolean
tree_on_link_selected(GObject *ignored, DhLink *link, gpointer data)
{
	if((unsigned int)link<1024){ errprintf("tree_on_link_selected(): bad link arg.\n"); return FALSE; }
	printf("tree_on_link_selected()...uri=%s\n", link->uri);
	app.search_dir = link->uri;
	do_search("", link->uri);
	return FALSE; //?
}

gint
get_mouseover_row()
{
	//get the row number the mouse is currently over from the stored row_reference.
	gint row_num = -1;
	GtkTreePath* path;
	GtkTreeIter iter;
	if((app.mouseover_row_ref && (path = gtk_tree_row_reference_get_path(app.mouseover_row_ref)))){
		gtk_tree_model_get_iter(GTK_TREE_MODEL(app.store), &iter, path);
		gchar* path_str = gtk_tree_model_get_string_from_iter(GTK_TREE_MODEL(app.store), &iter);
		row_num = atoi(path_str);

		g_free(path_str);
		gtk_tree_path_free(path);
	}
	return row_num;
}

void
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
	//printf("tag_cell_data()...\n");

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
	//printf("tag_cell_data(): %s mouse_y=%i cell_y=%i-%i.\n", path_str, app.mouse_y, cellrect.y, cellrect.y + cellrect.height);
	//if(//(app.mouse_x > cellrect.x) && (app.mouse_x < (cellrect.x + cellrect.width)) &&
	//			(app.mouse_y >= cellrect.y) && (app.mouse_y <= (cellrect.y + cellrect.height))){
	if(cell_row_num == mouse_row_num){
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
			//printf("split: [%s] %p %p %p %s\n", str, split[0], split[1], split[2], split[0]);
			int char_index = 0;
			int word_index = 0;
			int mouse_word = 0;
			gchar formatted[256] = "";
			char word[256] = "";
			while(split[word_index]){
				char_index += strlen(split[word_index]);

				pango_layout_line_index_to_x(layout_line, char_index, trailing, &char_pos);
				if(char_pos/PANGO_SCALE > mouse_cell_x){
					mouse_word = word_index;
					printf("tag_cell_data(): word=%i\n", word_index);

					snprintf(word, 256, "<u>%s</u> ", split[word_index]);
					g_strlcat(formatted, word, 256);
					//g_free(split[word_index]);
					//split[word_index] = word;

					while(split[++word_index]){
						//snprintf(formatted, 256, "%s ", split[word_index]);
						snprintf(word, 256, "%s ", split[word_index]);
						g_strlcat(formatted, word, 256);
					}

					break;
				}

				snprintf(word, 256, "%s ", split[word_index]);
				g_strlcat(formatted, word, 256);

				word_index++;
			}
			printf("tag_cell_data(): joined: %s\n", formatted);

			g_object_unref(layout);

			//-------------------------

			//set new markup:

			//g_object_set();
			gchar *text = NULL;
			GError *error = NULL;
			PangoAttrList *attrs = NULL;

			if (formatted && !pango_parse_markup(formatted, -1, 0, &attrs, &text, NULL, &error)){
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
			hypercell->markup_set = TRUE;
		}
	}
	}
	//else g_object_set(cell, "markup", "outside", NULL);
	//else hypercell->markup_set = FALSE;

	g_free(path_str);


/*
			gchar *text = NULL;
			GError *error = NULL;
			PangoAttrList *attrs = NULL;
			
			printf("tag_cell_data(): text=%s\n", celltext->text);
			if (!pango_parse_markup(celltext->text, -1, 0, &attrs, &text, NULL, &error)){
				g_warning("Failed to set cell text from markup due to error parsing markup: %s", error->message);
				g_error_free(error);
				return;
			}
			//if (celltext->text) g_free (celltext->text);
			//if (celltext->extra_attrs) pango_attr_list_unref (celltext->extra_attrs);
			celltext->text = text;
			celltext->extra_attrs = attrs;
	hypercell->markup_set = TRUE;
	*/
}

void
on_quit()
{
	MYSQL *mysql;
	mysql=&app.mysql;
  	mysql_close(mysql);
}


void
scan_dir()
{
	/*
	i guess we'll have to have a file selector window. Can we suffer the gtk selector box for now?

	GtkWidget*  gtk_file_selection_new("Select files to add");
	
	*/
	printf("scan_dir()....\n");

	gchar path[256];
	char filepath[256];
	G_CONST_RETURN gchar *file;
	GError **error = NULL;
	sprintf(path, "/home/tim/");
	GDir *dir;
	if((dir = g_dir_open(path, 0, error))){
		while((file = g_dir_read_name(dir))){
			if(file[0]=='.') continue;
			snprintf(filepath, 128, "%s/%s", path, file);
			if(!g_file_test(filepath, G_FILE_TEST_IS_DIR)){
				//printf("scan_dir(): checking files: %s\n", file);
				//dssi_plugin_add(filepath, (char*)file);
			}
		}
		g_dir_close(dir);
	}
	
}


void
do_search(char *search, char *dir)
{
	//fill the display with the results for the given search phrase.

	unsigned int i, channels;
	float samplerate; char samplerate_s[32];
	char length[64];
	GtkTreeIter iter;

	MYSQL_RES *result;
	MYSQL_ROW row;
	unsigned long *lengths;
	unsigned int num_fields;
	char query[1024];
	char where[512]="";
	char category[256]="";
	//char phrase[256]="";
	GdkPixbuf* pixbuf  = NULL;
	GdkPixbuf* iconbuf = NULL;
	GdkPixdata pixdata;
	gboolean online = FALSE;

	MYSQL *mysql;
	mysql=&app.mysql;

	//lets assume that the search_phrase filter should *always* be in effect.

	//FIXME should we use SET datatype for keywords?
	if (app.search_category) snprintf(category, 256, "AND keywords LIKE '%%%s%%' ", app.search_category);

	//strmov(query_def, "SELECT * FROM samples"); //where is this defined?
	//strcpy(query, "SELECT * FROM samples WHERE 1 ");
	if(strlen(search)){ 
		snprintf(where, 512, "AND (filename LIKE '%%%s%%' OR filedir LIKE '%%%s%%' OR keywords LIKE '%%%s%%') ", search, search, search); //FIXME duplicate category LIKE's
		//snprintf(query, 1024, "SELECT * FROM samples WHERE 1 %s", where);
	}
	if(dir && strlen(dir)){
		snprintf(where, 512, "AND filedir='%s' %s ", dir, category);
	}
	snprintf(query, 1024, "SELECT * FROM samples WHERE 1 %s", where);

	printf("%s\n", query);
	if(mysql_exec_sql(mysql, query)==0){ //success
		
		//problem with wierd mysql int type:
		//printf( "%ld Records Found\n", (long) mysql_affected_rows(&mysql));
		//printf( "%lu Records Found\n", (unsigned long)mysql_affected_rows(mysql));
	
		//FIXME clear the treeview.
		gtk_list_store_clear(app.store);

		result = mysql_store_result(mysql);
		if(result){// there are rows
			num_fields = mysql_num_fields(result);
			char keywords[256];

			while((row = mysql_fetch_row(result))){
				lengths = mysql_fetch_lengths(result); //free? 
				for(i = 0; i < num_fields; i++){ 
					//printf("[%s] ", row[i] ? row[i] : "NULL"); 
				}
				//printf("\n"); 

				//deserialise the pixbuf field:
				pixbuf = NULL;
				if(row[4]){
					//printf("do_search(): deserializing...\n"); 
					if(gdk_pixdata_deserialize(&pixdata, lengths[4], (guint8*)row[4], NULL)){
						//printf("do_search(): extracting pixbuf...\n"); 
						pixbuf = gdk_pixbuf_from_pixdata(&pixdata, TRUE, NULL);
					}
				}
				format_time(length, row[5]);
				if(row[MYSQL_KEYWORDS]) snprintf(keywords, 256, "%s", row[MYSQL_KEYWORDS]); else keywords[0] = 0;
				//if(row[5]==NULL) length     = 0; else length     = atoi(row[5]);
				if(row[6]==NULL) samplerate = 0; else samplerate = atoi(row[6]); samplerate_format(samplerate_s, samplerate);
				if(row[7]==NULL) channels   = 0; else channels   = atoi(row[7]);
				if(row[MYSQL_ONLINE]==NULL) online = 0; else online = atoi(row[MYSQL_ONLINE]);

				//icon (dont show if the sound file is not available):
				if(online){
	  				MIME_type* mime_type = mime_type_lookup(row[MYSQL_MIMETYPE]);
					/*MaskedPixmap* pixmap =*/ type_to_icon(mime_type);
					if ( mime_type->image == NULL ) printf("do_search(): no icon.\n");
					iconbuf = mime_type->image->sm_pixbuf;
				} else iconbuf = NULL;

				gtk_list_store_append(app.store, &iter); 
				gtk_list_store_set(app.store, &iter, COL_ICON, iconbuf, COL_IDX, atoi(row[0]), COL_NAME, row[1], COL_FNAME, row[2], COL_KEYWORDS, keywords, 
                                                     COL_OVERVIEW, pixbuf, COL_LENGTH, length, COL_SAMPLERATE, samplerate_s, COL_CHANNELS, channels, 
													 COL_MIMETYPE, row[MYSQL_MIMETYPE], -1);
			}
			mysql_free_result(result);
			statusbar_print(1, "search done");
		}
		else{  // mysql_store_result() returned nothing
		  if(mysql_field_count(mysql) > 0){
				// mysql_store_result() should have returned data
				printf( "Error getting records: %s\n", mysql_error(mysql));
		  }
		  else printf( "do_search(): Failed to find any records (fieldcount<1): %s\n", mysql_error(mysql));
		}
	}
	else{
		printf( "do_search(): Failed to find any records: %s\n", mysql_error(mysql));
	}
}


gboolean
filter_new()
{
	//search box

	if(!app.window) return FALSE; //FIXME make this a macro with printf etc

	GtkWidget* hbox = app.toolbar = gtk_hbox_new(FALSE, 0);
    gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(app.vbox), hbox, EXPAND_FALSE, FILL_FALSE, 0);

	GtkWidget* label = gtk_label_new("Search");
	gtk_misc_set_padding(GTK_MISC(label), 5,5);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	
	GtkWidget *entry = app.search = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(entry), 64);
	gtk_entry_set_text(GTK_ENTRY(entry), "");
	gtk_widget_show(entry);	
	gtk_box_pack_start(GTK_BOX(hbox), entry, EXPAND_TRUE, TRUE, 0);
	g_signal_connect(G_OBJECT(entry), "focus-out-event", G_CALLBACK(new_search), NULL);


	//second row (metadata edit):
	GtkWidget* hbox_edit = app.toolbar2 = gtk_hbox_new(FALSE, 0);
    gtk_widget_show(hbox_edit);
	gtk_box_pack_start(GTK_BOX(app.vbox), hbox_edit, EXPAND_FALSE, FILL_FALSE, 0);

	label = gtk_label_new("Edit");
	gtk_misc_set_padding(GTK_MISC(label), 5,5);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(hbox_edit), label, FALSE, FALSE, 0);
	
	
	return TRUE;
}


gboolean
tag_selector_new()
{
	//the tag _edit_ selector

	//GtkWidget* combo = gtk_combo_box_new_text();
	GtkWidget* combo = app.category = gtk_combo_box_new_text();
	GtkComboBox* combo_ = GTK_COMBO_BOX(combo);
	gtk_combo_box_append_text(combo_, "no categories");
	gtk_combo_box_append_text(combo_, "drums");
	gtk_combo_box_append_text(combo_, "perc");
	gtk_combo_box_append_text(combo_, "keys");
	gtk_combo_box_append_text(combo_, "strings");
	gtk_combo_box_append_text(combo_, "fx");
	gtk_combo_box_append_text(combo_, "impulses");
	gtk_combo_box_set_active(combo_, 0);
	gtk_widget_show(combo);	
	gtk_box_pack_start(GTK_BOX(app.toolbar2), combo, EXPAND_FALSE, FALSE, 0);
	g_signal_connect(combo, "changed", G_CALLBACK(on_category_changed), NULL);
	//gtk_combo_box_get_active_text(combo);

	GtkWidget* combo2 = gtk_combo_box_entry_new_text();
	combo_ = GTK_COMBO_BOX(combo2);
	gtk_combo_box_append_text(combo_, "no categories");
	gtk_widget_show(combo2);	
	gtk_box_pack_start(GTK_BOX(app.toolbar2), combo2, EXPAND_FALSE, FALSE, 0);

	//"set" button:
	GtkWidget* set = gtk_button_new_with_label("Set");
	gtk_widget_show(set);	
	gtk_box_pack_start(GTK_BOX(app.toolbar2), set, EXPAND_FALSE, FALSE, 0);
	g_signal_connect(set, "clicked", G_CALLBACK(on_set_clicked), NULL);

	return TRUE;
}


gboolean
tagshow_selector_new()
{
	//the view-filter tag-select.

	//GtkWidget* combo = gtk_combo_box_new_text();
	GtkWidget* combo = app.view_category = gtk_combo_box_new_text();
	GtkComboBox* combo_ = GTK_COMBO_BOX(combo);
	gtk_combo_box_append_text(combo_, "all categories");
	gtk_combo_box_append_text(combo_, "drums");
	gtk_combo_box_append_text(combo_, "keys");
	gtk_combo_box_append_text(combo_, "strings");
	gtk_combo_box_append_text(combo_, "fx");
	gtk_combo_box_set_active(combo_, 0);
	gtk_widget_show(combo);	
	gtk_box_pack_start(GTK_BOX(app.toolbar), combo, EXPAND_FALSE, FALSE, 0);
	g_signal_connect(combo, "changed", G_CALLBACK(on_view_category_changed), NULL);
	//gtk_combo_box_get_active_text(combo);

	return TRUE;
}


void
on_category_changed(GtkComboBox *widget, gpointer user_data)
{
	printf("on_category_changed()...\n");
}


void
on_view_category_changed(GtkComboBox *widget, gpointer user_data)
{
	//update the sample list with the new view-category.

	printf("on_view_category_changed()...\n");

	if (app.search_category) g_free(app.search_category);
	app.search_category = gtk_combo_box_get_active_text(GTK_COMBO_BOX(app.view_category));

	do_search(app.search_phrase, app.search_dir);
}


void
on_set_clicked(GtkComboBox *widget, gpointer user_data)
{
	//add selected category to selected samples.

	printf("on_set_clicked()...\n");

	//selected category?
	gchar* category = gtk_combo_box_get_active_text(GTK_COMBO_BOX(app.category));

	GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(app.view));
	GList* selectionlist = gtk_tree_selection_get_selected_rows(selection, NULL);
	if(!selectionlist){ printf("on_set_clicked(): no files selected.\n"); return; }
	//printf("delete_row(): %i rows selected.\n", g_list_length(selectionlist));

	int i;
    GtkTreeIter iter;
	for(i=0;i<g_list_length(selectionlist);i++){
		GtkTreePath *treepath_selection = g_list_nth_data(selectionlist, i);

		if(gtk_tree_model_get_iter(GTK_TREE_MODEL(app.store), &iter, treepath_selection)){
			gchar *fname; gchar *tags;
			int id;
			gtk_tree_model_get(GTK_TREE_MODEL(app.store), &iter, COL_NAME, &fname, COL_KEYWORDS, &tags, COL_IDX, &id, -1);

			char tags_new[1024];
			snprintf(tags_new, 1024, "%s %s", tags, category);
			g_strstrip(tags_new);//trim

			char sql[1024];
			snprintf(sql, 1024, "UPDATE samples SET keywords='%s' WHERE id=%i", tags_new, id);
			printf("on_set_clicked(): row: %s sql=%s\n", fname, sql);
			if(mysql_query(&app.mysql, sql)){
				errprintf("on_set_clicked(): update failed! sql=%s\n", sql);
				return;
			}
			//update the store:
    		gtk_list_store_set(app.store, &iter, COL_KEYWORDS, tags_new, -1);


		} else errprintf("on_set_clicked() bad iter! i=%i (<%i)\n", i, g_list_length(selectionlist));
	}
	g_list_foreach(selectionlist, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(selectionlist);

	g_free(category);
}


gboolean
db_connect()
{
	MYSQL *mysql;

	if(mysql_init(&(app.mysql))==NULL){
		printf("Failed to initiate MySQL connection.\n");
		exit(1);
	}
	printf("MySQL Client Version is %s\n", mysql_get_client_info());

	mysql = &app.mysql;

	if(!mysql_real_connect(mysql, "localhost", "root", "hlongporto", "test", 0, NULL, 0)){
		printf("cannot connect. %s\n", mysql_error(mysql));
		exit(1);
	}
	printf("MySQL Server Version is %s\n", mysql_get_server_info(mysql));
	//printf("Logged on to database.\n");

	if(!mysql_select_db(mysql, database_name /*const char *db*/)==0)/*success*/{
		//printf( "Database Selected.\n");
	//else
		printf( "Failed to connect to Database: Error: %s\n", mysql_error(mysql));
		return FALSE;
	}

	return TRUE;
}
 

int
db_insert(char *qry)
{
	MYSQL *mysql = &app.mysql;
	int id = 0;

	if(mysql_exec_sql(mysql, qry)==0){
		printf("db_insert(): ok!\n");
		id = mysql_insert_id(mysql);
	}else{
		errprintf("db_insert(): not ok...\n");
		return 0;
	}
	return id;
}


void
db_get_dirs()
{
	if (!app.dir_tree) { errprintf("db_get_dirs(): dir_tree not initialised.\n"); return; }

	char qry[1024];
	MYSQL *mysql;
	mysql=&app.mysql;
	MYSQL_RES *result;
	MYSQL_ROW row;

	snprintf(qry, 1024, "SELECT DISTINCT filedir FROM samples ORDER BY filedir");

	if(mysql_exec_sql(mysql, qry)==0){
		result = mysql_store_result(mysql);
		if(result){// there are rows
			DhLink* link = NULL;
			GNode* leaf = NULL;
			char uri[256];
			gint position;

			//snprintf(uri, 256, "%s", "all directories");
			link = dh_link_new(DH_LINK_TYPE_PAGE, "all directories", "");
			leaf = g_node_new(link);
			g_node_insert(app.dir_tree, -1, leaf);

			while((row = mysql_fetch_row(result))){

				printf("db_get_dirs(): dir=%s\n", row[0]);
				snprintf(uri, 256, "%s", row[0]);
				link = dh_link_new(DH_LINK_TYPE_PAGE, row[0], uri);
				leaf = g_node_new(link);

				position = -1;//end
				g_node_insert(app.dir_tree, position, leaf);
			}
			mysql_free_result(result);
		}
		else{  // mysql_store_result() returned nothing
		  if(mysql_field_count(mysql) > 0){
				// mysql_store_result() should have returned data
				printf( "Error getting records: %s\n", mysql_error(mysql));
		  }
		}
	}
	else{
		printf( "db_get_dirs(): failed to find any records: %s\n", mysql_error(mysql));
	}
}


gboolean
new_search(GtkWidget *widget, gpointer userdata)
{
	//the search box focus-out signal ocurred.
	//printf("new search...\n");

	const gchar* text = gtk_entry_get_text(GTK_ENTRY(app.search));
	
	do_search((gchar*)text, NULL);
	return FALSE;
}


void
dnd_setup(){
  gtk_drag_dest_set(app.window, GTK_DEST_DEFAULT_ALL,
                        dnd_file_drag_types,       //const GtkTargetEntry *targets,
                        dnd_file_drag_types_count,    //gint n_targets,
                        (GdkDragAction)(GDK_ACTION_MOVE | GDK_ACTION_COPY));
  //g_signal_connect (G_OBJECT(ch->e), "drag-drop", G_CALLBACK(mixer_drag_drop), NULL);
  //g_signal_connect(G_OBJECT(view), "drag-motion", G_CALLBACK(pool_drag_motion), NULL);
  g_signal_connect(G_OBJECT(app.window), "drag-data-received", G_CALLBACK(drag_received), NULL);
}


gint
drag_received(GtkWidget *widget, GdkDragContext *drag_context, gint x, gint y,
              GtkSelectionData *data, guint info, guint time, gpointer user_data)
{
  if(data == NULL || data->length < 0){ errprintf("drag_received(): no data!\n"); return -1; }

  //printf("drag_received()! %s\n", data->data);

  if(info==GPOINTER_TO_INT(GDK_SELECTION_TYPE_STRING)) printf(" type=string.\n");

  if(info==GPOINTER_TO_INT(TARGET_URI_LIST)){
    printf("drag_received(): type=uri_list. len=%i\n", data->length);
    GList *list = gnome_vfs_uri_list_parse(data->data);
    int i;
    for(i=0;i<g_list_length(list); i++){
      GnomeVFSURI* u = g_list_nth_data(list, i);
      printf("drag_received(): %i: %s\n", i, u->text);

      //printf("drag_received(): method=%s\n", u->method_string);

      if(!strcmp(u->method_string, "file")){
        //we could probably better use g_filename_from_uri() here
        //http://10.0.0.151/man/glib-2.0/glib-Character-Set-Conversion.html#g-filename-from-uri
        //-or perhaps get_local_path() from rox/src/support.c

        //printf("drag_received(): importing file...\n");
        add_file(u->text);
      }
      else warnprintf("drag_received(): drag drop: unknown format: '%s'. Ignoring.\n", u);
    }
    printf("drag_received(): freeing uri list...\n");
    gnome_vfs_uri_list_free(list);
  }

  return FALSE;
}


sample*
sample_new()
{
  sample* sample;
  sample = malloc(sizeof(*sample));
  sample->id = 0;
  sample->pixbuf = NULL;
  return sample;
}


void
sample_free(sample* sample)
{
	gtk_tree_row_reference_free(sample->row_ref);
	g_free(sample);
}


void
add_file(char *uri)
{
  printf("add_file()... %s\n", uri);
  char* uri_unescaped = gnome_vfs_unescape_string(uri, NULL);

  sample* sample = sample_new(); //free'd after db and store are updated.
  snprintf(sample->filename, 256, "%s", uri_unescaped);

  #define SQL_LEN 66000
  char sql[1024], sql2[SQL_LEN];
  char length_ms[64];
  char samplerate_s[32];
  //strcpy(sql, "INSERT samples SET ");
  gchar* filedir = g_path_get_dirname(uri_unescaped);
  gchar* filename = g_path_get_basename(uri_unescaped);
  snprintf(sql, 1024, "INSERT samples SET filename='%s', filedir='%s'", filename, filedir);

  MIME_type* mime_type = type_from_path(filename);
  char mime_string[64];
  snprintf(mime_string, 64, "%s/%s", mime_type->media_type, mime_type->subtype);
  printf("add_file() mimetype: %s\n", mime_string);

  /* better way to do the string appending (or use glib?):
  tmppos = strmov(tmp, "INSERT INTO test_blob (a_blob) VALUES ('");
  tmppos += mysql_real_escape_string(conn, tmppos, fbuffer, fsize);
  tmppos = strmov(tmppos, "')");
  *tmppos++ = (char)0;
  mysql_query(conn, tmp);
  */

  //GdkPixbuf* pixbuf = make_overview(&sample);
  //if(pixbuf){
  if(get_file_info(sample)){

	//snprintf(sql2, SQL_LEN, "%s, pixbuf='%s', length=%i, sample_rate=%i, channels=%i, mimetype='%s/%s' ", sql, blob, sample.length, sample.sample_rate, sample.channels, mime_type->media_type, mime_type->subtype);
	snprintf(sql2, SQL_LEN, "%s, length=%i, sample_rate=%i, channels=%i, mimetype='%s/%s' ", sql, sample->length, sample->sample_rate, sample->channels, mime_type->media_type, mime_type->subtype);
	format_time_int(length_ms, sample->length);
	samplerate_format(samplerate_s, sample->sample_rate);
    printf("add_file(): sql=%s\n", sql2);

    sample->id = db_insert(sql2);

    GtkTreeIter iter;
    gtk_list_store_append(app.store, &iter);

	//store a row reference:
	GtkTreePath* treepath;
	if((treepath = gtk_tree_model_get_path(GTK_TREE_MODEL(app.store), &iter))){
		sample->row_ref = gtk_tree_row_reference_new(GTK_TREE_MODEL(app.store), treepath);
		printf("add_file(): rowref=%p\n", sample->row_ref);
		gtk_tree_path_free(treepath);
	}else errprintf("add_file(): failed to make treepath from inserted iter.\n");

	//printf("add_file(): iter=%p=%s\n", &iter, gtk_tree_model_get_string_from_iter(GTK_TREE_MODEL(app.store), &iter));
    printf("setting store... filename=%s mime=%s\n", filename, mime_string);
    gtk_list_store_set(app.store, &iter,
	                  COL_IDX, sample->id,
                      COL_NAME, filename,
                      COL_FNAME, filedir,
                      //COL_OVERVIEW, pixbuf,
                      //COL_OVERVIEW, NULL,
	                  COL_LENGTH, length_ms,
                      COL_SAMPLERATE, samplerate_s,
	                  COL_CHANNELS, sample->channels,
	                  COL_MIMETYPE, mime_string,
                      -1);

    printf("sending message: sample=%p filename=%s (%p)\n", sample, sample->filename, sample->filename);
	g_async_queue_push(msg_queue, sample); //notify the overview thread.

  }// else strcpy(sql2, sql);

  g_free(filedir);
  g_free(filename);
  g_free(uri_unescaped);
}



gboolean
get_file_info(sample* sample)
{
	char *filename = sample->filename;

	SF_INFO        sfinfo;   //the libsndfile struct pointer
	SNDFILE        *sffile;
	sfinfo.format  = 0;

	if(!(sffile = sf_open(filename, SFM_READ, &sfinfo))){
		printf("get_file_info(): not able to open input file %s.\n", filename);
		puts(sf_strerror(NULL));    // print the error message from libsndfile:
		return FALSE;
	}

	char chanwidstr[64];
	if     (sfinfo.channels==1) snprintf(chanwidstr, 64, "mono");
	else if(sfinfo.channels==2) snprintf(chanwidstr, 64, "stereo");
	else                        snprintf(chanwidstr, 64, "channels=%i", sfinfo.channels);
	printf("%iHz %s frames=%i\n", sfinfo.samplerate, chanwidstr, (int)sfinfo.frames);
	sample->channels    = sfinfo.channels;
	sample->sample_rate = sfinfo.samplerate;
	sample->length      = (sfinfo.frames * 1000) / sfinfo.samplerate;

	if(sample->channels<1 || sample->channels>100){ printf("get_file_info(): bad channel count: %i\n", sample->channels); return FALSE; }
	if(sf_close(sffile)) printf("error! bad file close.\n");

	//printf("get_file_info(): failed to get info.\n"); return FALSE;
	return TRUE;
}


gboolean
on_overview_done(gpointer data)
{
	sample* sample = data;
	printf("on_overview_done()...\n");
	if(!sample){ errprintf("on_overview_done(): bad arg: sample.\n"); return FALSE; }
	if(!sample->pixbuf){ errprintf("on_overview_done(): overview creation failed (no pixbuf).\n"); return FALSE; }

	db_update_pixbuf(sample);
	//printf("on_overview_done(): row_ref=%p\n", sample->row_ref);
	//printf("on_overview_done(): pixbuf=%p\n", sample->pixbuf);

	GtkTreePath* treepath = gtk_tree_row_reference_get_path(sample->row_ref);
	GtkTreeIter iter;
	if(gtk_tree_model_get_iter(GTK_TREE_MODEL(app.store), &iter, treepath)){
		//printf("on_overview_done(): setting pixbuf in store... pixbuf=%p\n", sample->pixbuf);
		if(GDK_IS_PIXBUF(sample->pixbuf)){
			gtk_list_store_set(app.store, &iter, COL_OVERVIEW, sample->pixbuf, -1);
		}else errprintf("on_overview_done(): pixbuf is not a pixbuf.\n");

	}else errprintf("on_overview_done(): failed to get row iter. row_ref=%p\n", sample->row_ref);
	gtk_tree_path_free(treepath);

	sample_free(sample);
	return FALSE; //source should be removed.
}


void
db_update_pixbuf(sample *sample){
	printf("db_update_pixbuf()...\n");

	GdkPixbuf* pixbuf = sample->pixbuf;
	//printf("db_update_pixbuf(): pixbuf=%p\n", pixbuf);
	if(pixbuf){
		//serialise the pixbuf:
		guint8 blob[SQL_LEN];
		GdkPixdata pixdata;
		gdk_pixdata_from_pixbuf(&pixdata, pixbuf, 0);
		guint length;
		guint8* ser = gdk_pixdata_serialize(&pixdata, &length);
		mysql_real_escape_string(&app.mysql, blob, ser, length);
		//printf("db_update_pixbuf() serial length: %i, strlen: %i\n", length, strlen(ser));

		char sql[SQL_LEN];
		snprintf(sql, SQL_LEN, "UPDATE samples SET pixbuf='%s' WHERE id=%i", blob, sample->id);
		if(mysql_query(&app.mysql, sql)){
			printf("db_update_pixbuf(): update failed! sql=%s\n", sql);
			return;
		}


		free(ser);

	    //at this pt, refcount should be two, we make it 1 so that pixbuf is destroyed with the row:
    	//g_object_unref(pixbuf); //FIXME
	}else errprintf("db_update_pixbuf(): no pixbuf.\n");
}


void
keywords_on_edited(GtkCellRendererText *cell, gchar *path_string, gchar *new_text, gpointer user_data)
{
	printf("cell_edited_callback()...\n");
	//GtkTreeModel* model = GTK_TREE_MODEL();
	GtkTreeIter iter;
	int idx;
	gchar *filename;
	//GtkListStore *store = app.store;
	GtkTreeModel* store = GTK_TREE_MODEL(app.store);
	GtkTreePath* path = gtk_tree_path_new_from_string(path_string);
	gtk_tree_model_get_iter(store, &iter, path);
	gtk_tree_model_get(store, &iter, COL_IDX, &idx,  COL_NAME, &filename, -1);
	printf("cell_edited_callback(): filename=%s\n", filename);

	char sql[1024];
	char* tmppos;
	tmppos = strmov(sql, "UPDATE samples SET keywords='");
	//tmppos += mysql_real_escape_string(conn, tmppos, fbuffer, fsize);
	tmppos = strmov(tmppos, new_text);
	tmppos = strmov(tmppos, "' WHERE id=");
	char idx_str[64];
	snprintf(idx_str, 64, "%i", idx);
	tmppos = strmov(tmppos, idx_str);
	*tmppos++ = (char)0;
	printf("keywords_on_edited(): sql=%s\n", sql);
	if(mysql_query(&app.mysql, sql)){
		printf("keywords_on_edited(): update failed! sql=%s\n", sql);
		return;
	}

	//update the store:
	gtk_list_store_set(app.store, &iter, COL_KEYWORDS, new_text, -1);
}


void
delete_row(GtkWidget *widget, gpointer user_data)
{
	//widget is likely to be a popupmenu.

	printf("delete_row()...\n");
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(app.view));

	GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(app.view));
	GList* selectionlist = gtk_tree_selection_get_selected_rows(selection, &(model));
	if(!selectionlist){ errprintf("delete_row(): no files selected?\n"); return; }
	printf("delete_row(): %i rows selected.\n", g_list_length(selectionlist));

	int i;
	for(i=0;i<g_list_length(selectionlist);i++){
		GtkTreePath *treepath_selection = g_list_nth_data(selectionlist, i);

		if(gtk_tree_model_get_iter(model, &iter, treepath_selection)){
			gchar *fname;
			int id;
			gtk_tree_model_get(model, &iter, COL_NAME, &fname, COL_IDX, &id, -1);

			char sql[1024];
			snprintf(sql, 1024, "DELETE FROM samples WHERE id=%i", id);
			printf("delete_row(): row: %s sql=%s\n", fname, sql);
			if(mysql_query(&app.mysql, sql)){
				errprintf("delete_row(): delete failed! sql=%s\n", sql);
				return;
			}
			//update the store:
			//gtk_tree_store_remove(model, &iter);
			gtk_list_store_remove(app.store, &iter);
		} else errprintf("delete_row() bad iter! i=%i (<%i)\n", i, g_list_length(selectionlist));
	}
	g_list_free(selectionlist);
}


void
update_row(GtkWidget *widget, gpointer user_data)
{
	printf("update_row()...\n");
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(app.view));

	GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(app.view));
	GList* selectionlist = gtk_tree_selection_get_selected_rows(selection, &(model));
	if(!selectionlist){ errprintf("update_row(): no files selected?\n"); return; }
	printf("update_row(): %i rows selected.\n", g_list_length(selectionlist));

	//GDate* date = g_date_new();
	//g_get_current_time(date);

	int i, id; gchar *fname; gchar *fdir; gchar *mimetype; gchar path[256];
	GdkPixbuf* iconbuf;
	gboolean online;
	for(i=0;i<g_list_length(selectionlist);i++){
		GtkTreePath *treepath_selection = g_list_nth_data(selectionlist, i);

		gtk_tree_model_get_iter(model, &iter, treepath_selection);
		gtk_tree_model_get(model, &iter, COL_NAME, &fname, COL_FNAME, &fdir, COL_MIMETYPE, &mimetype, COL_IDX, &id, -1);

		snprintf(path, 256, "%s/%s", fdir, fname);
		if(file_exists(path)){
			online=1;

	  		MIME_type* mime_type = mime_type_lookup(mimetype);
			type_to_icon(mime_type);
			if ( mime_type->image == NULL ) printf("update_row(): no icon.\n");
			iconbuf = mime_type->image->sm_pixbuf;

		}else{
			online=0;
			iconbuf = NULL;
		}
		gtk_list_store_set(app.store, &iter, COL_ICON, iconbuf, -1);

		char sql[1024];
		snprintf(sql, 1024, "UPDATE samples SET online=%i, last_checked=NOW() WHERE id=%i", online, id);
		printf("update_row(): row: %s path=%s sql=%s\n", fname, path, sql);
		if(mysql_query(&app.mysql, sql)){
			errprintf("update_row(): update failed! sql=%s\n", sql);
		}
	}
	g_list_free(selectionlist);
	//g_date_free(date);
}


void
edit_row(GtkWidget *widget, gpointer user_data)
{
	//what exactly is this supposed to be "editing"? any cell mouse happens to be over? currently it looks like only the tags cell.	
	printf("edit_row()...\n");

	GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(app.view));
	if(!selection){ errprintf("edit_row(): cannot get selection.\n");/* return;*/ }
	GtkTreeModel *model = GTK_TREE_MODEL(app.store);
	GList* selectionlist = gtk_tree_selection_get_selected_rows(selection, &(model));
	if(!selectionlist){ errprintf("edit_row(): no files selected?\n"); return; }

	GtkTreePath *treepath;
	if((treepath = g_list_nth_data(selectionlist, 0))){
		GtkTreeIter iter;
		if(gtk_tree_model_get_iter(GTK_TREE_MODEL(app.store), &iter, treepath)){
			gchar* path_str = gtk_tree_model_get_string_from_iter(GTK_TREE_MODEL(app.store), &iter);
			printf("edit_row(): path=%s\n", path_str);

			//segfault!
			//FIXME is this the correct fn?
			GtkCellEditable* editable = gtk_cell_renderer_start_editing(app.cell_tags, 
												 NULL, //GdkEvent *event,
												 app.view,
												 path_str,  //a string representation of GtkTreePath
												 NULL, //GdkRectangle *background_area,
												 NULL, //GdkRectangle *cell_area,
												 GTK_CELL_RENDERER_SELECTED);
		} else errprintf("edit_row(): cannot get iter.\n");
		//free path_str ??
		gtk_tree_path_free(treepath);
	}
	g_list_free(selectionlist);
}


GtkWidget*
make_context_menu()
{
	GtkWidget *menu = gtk_menu_new();
	GtkWidget *menu_item;

	//item 1:
	menu_item = gtk_menu_item_new_with_label ("Delete");
	gtk_menu_shell_append (GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect (G_OBJECT(menu_item), "activate", G_CALLBACK(delete_row), NULL);
	gtk_widget_show (menu_item);

	menu_item = gtk_menu_item_new_with_label ("Update");
	gtk_menu_shell_append (GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect (G_OBJECT(menu_item), "activate", G_CALLBACK(update_row), NULL);
	gtk_widget_show (menu_item);

	menu_item = gtk_menu_item_new_with_label ("Edit");
	gtk_menu_shell_append (GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect (G_OBJECT(menu_item), "activate", G_CALLBACK(edit_row), NULL);
	gtk_widget_show (menu_item);


  /* example a submenu:

  menu_item = gtk_menu_item_new_with_label("Metering");
  //gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
  //g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(strip_hide_sends), mixer_win);
  gtk_widget_show(menu_item);
  gtk_container_add(GTK_CONTAINER(menu), menu_item);

  //sub-menu:
  GtkWidget *sub_menu = gtk_menu_new();
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), sub_menu);
  //printf("mixer_contextmenu_init(): submenu=%p parent=%p\n", sub_menu, menu_item);

  GtkWidget *item = gtk_menu_item_new_with_label("Jamin");
  gtk_menu_shell_append(GTK_MENU_SHELL(sub_menu), item);
  g_object_set_data(G_OBJECT(item), "smwin", smwin);
  g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(mixer_set_meter_type), (gpointer)METER_JAMIN);
  gtk_widget_show(item);
  */

	return menu;
}

gboolean
on_row_clicked(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
	GtkTreeView *treeview = GTK_TREE_VIEW(app.view);

	//auditioning:
	if(event->button == 1){
		//printf("left button press!\n");
		GtkTreePath *path;
		if(gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(app.view), (gint)event->x, (gint)event->y, &path, NULL, NULL, NULL)){
			GdkRectangle rect;
			gtk_tree_view_get_cell_area(treeview, path, app.col_pixbuf, &rect);
			if(((gint)event->x > rect.x) && ((gint)event->x < (rect.x + rect.width))){

				//overview column:
				printf("on_row_clicked() column rect: %i %i %i %i\n", rect.x, rect.y, rect.width, rect.height);

				GtkTreeIter iter;
				GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(app.view));
				gtk_tree_model_get_iter(model, &iter, path);
				gchar *fpath, *fname;
				int id;
				gtk_tree_model_get(model, &iter, COL_FNAME, &fpath, COL_NAME, &fname, COL_IDX, &id, -1);

				char file[256]; snprintf(file, 256, "%s/%s", fpath, fname);
				if(id != app.playing_id) playback_init(file, id);
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

					gtk_entry_set_text(GTK_ENTRY(app.search), tags);
					do_search(tags, NULL);
				}
			}
		}
		return FALSE; //propogate the signal
	}

	//popup menu:
	if(event->button == 3){
		printf("right button press!\n");

		//if one or no rows selected, select one:
		GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(app.view));
        if(gtk_tree_selection_count_selected_rows(selection)  <= 1){
			GtkTreePath *path;
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
treeview_on_motion(GtkWidget *widget, GdkEventMotion *event, gpointer user_data)
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
		printf("treeview_on_motion() tags cell found!\n");
		g_object_set(cell_renderer, "markup", "<b>important</b>", "text", NULL, NULL);
	}
	*/

	//printf("treeview_on_motion(): x=%f y=%f. l=%i\n", x, y, rect.x, rect.width);


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

				//if(row_ref != prev_row_ref){
				//if(prev_path && (path != prev_path)){
				if(prev_path && (atoi(path_str) != atoi(prev_path_str))){
					printf("treeview_on_motion() new row! path=%p (%s) prev_path=%p (%s)\n", path, path_str, prev_path, prev_path_str);

					//restore text to previous row:
					gtk_list_store_set(app.store, &prev_iter, COL_KEYWORDS, prev_text, -1);

					//store original text:
					gchar* txt;
					gtk_tree_model_get(GTK_TREE_MODEL(app.store), &iter, COL_KEYWORDS, &txt, -1);
					printf("treeview_on_motion(): text=%s\n", prev_text);
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
							printf("treeview_on_motion(): joined: %s\n", joined);
						}
						//g_strfreev(split); //segfault - doesnt like reassigning split[word_index] ?

						//g_object_set();
						gchar *text = NULL;
						GError *error = NULL;
						PangoAttrList *attrs = NULL;

						if (joined && !pango_parse_markup(joined, -1, 0, &attrs, &text, NULL, &error)){
							g_warning("Failed to set cell text from markup due to error parsing markup: %s", error->message);
							g_error_free(error);
							return FALSE;
						}

						//if (celltext->text) g_free (celltext->text);
						//if (celltext->extra_attrs) pango_attr_list_unref (celltext->extra_attrs);

						printf("treeview_on_motion(): setting text: %s\n", text);
						//celltext->text = text;
						//celltext->extra_attrs = attrs;
						//hypercell->markup_set = TRUE;
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

	app.mouseover_row_ref = prev_row_ref;
	return FALSE;
}


gboolean
treeview_get_cell(GtkTreeView *view, guint x, guint y, GtkCellRenderer **cell)
{
	//taken from the treeview tutorial:

	//this is useless - we might as well just store the cell at create time.

	GtkTreeViewColumn *colm = NULL;
	guint              colm_x = 0;

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

	if(colm == NULL) return FALSE; // not found

	// (2) find the cell renderer within the column 

	GList* cells = gtk_tree_view_column_get_cell_renderers(colm);

	for (node = cells;  node != NULL;  node = node->next){
		GtkCellRenderer *checkcell = (GtkCellRenderer*) node->data;
		guint            width = 0;

		// Will this work for all packing modes? doesn't that return a random width depending on the last content rendered?
		gtk_cell_renderer_get_size(checkcell, GTK_WIDGET(view), NULL, NULL, NULL, &width, NULL);

		if(x >= colm_x && x < (colm_x + width)){
			*cell = checkcell;
			g_list_free(cells);
			return TRUE;
		}

		colm_x += width;
	}

	g_list_free(cells);
	return FALSE; // not found
}


gboolean
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

	if(colm == NULL) return FALSE; // not found
	if(colm != app.col_tags) return FALSE;

	// (2) find the cell renderer within the column 

	GList* cells = gtk_tree_view_column_get_cell_renderers(colm);
	GdkRectangle cell_rect;

	for (node = cells;  node != NULL;  node = node->next){
		GtkCellRenderer *checkcell = (GtkCellRenderer*) node->data;
		guint            width = 0, height = 0;

		// Will this work for all packing modes? doesn't that return a random width depending on the last content rendered?
		gtk_cell_renderer_get_size(checkcell, GTK_WIDGET(view), &cell_rect, NULL, NULL, &width, &height);
		printf("y=%i height=%i\n", cell_rect.y, cell_rect.height);

		//if(x >= colm_x && x < (colm_x + width)){
		//if(y >= colm_y && y < (colm_y + height)){
		if(y >= cell_rect.y && y < (cell_rect.y + cell_rect.height)){
			*cell = checkcell;
			g_list_free(cells);
			return TRUE;
		}

		//colm_y += height;
	}

	g_list_free(cells);
	printf("not found in column. cell_height=%i\n", cell_rect.height);
	return FALSE; // not found
}


