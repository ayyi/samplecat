/*

This software is licensed under the GPL. See accompanying file COPYING.

*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <my_global.h>   // for strmov
#include <m_string.h>    // for strmov
#include <sndfile.h>
#include <jack/jack.h>
#include <gtk/gtk.h>
#include <FLAC/all.h>

#include <gdk-pixbuf/gdk-pixdata.h>
#include <libart_lgpl/libart.h>
#include <libgnomevfs/gnome-vfs.h>
#include <jack/ringbuffer.h>

#include "mysql/mysql.h"
#include "dh-link.h"
#include "main.h"
#include "support.h"
#include "audio.h"
#include "overview.h"
#include "cellrenderer_hypertext.h"
#include "tree.h"
#include "db.h"
#include "dnd.h"

#include "rox_global.h"
#include "type.h"
#include "pixmaps.h"
//#include "gnome-vfs-uri.h"

//#define DEBUG_NO_THREADS 1

struct _app app;
struct _palette palette;
GList* mime_types; // list of *MIME_type

unsigned debug = 0;

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
  COL_NOTES, //this is in the store but not the view.
  COL_COLOUR,
  NUM_COLS
};

//mysql table layout (database column numbers):
#define MYSQL_KEYWORDS 3
#define MYSQL_ONLINE 8
#define MYSQL_MIMETYPE 10
#define MYSQL_NOTES 11
#define MYSQL_COLOUR 12

//dnd:
char       *app_name    = "Samplecat";
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

	int i; for(i=0;i<PALETTE_SIZE;i++) app.colour_button[i] = NULL;
	app.colourbox_dirty = TRUE;
}


int 
main(int argc, char* *argv)
{
	//make gdb break on g errors:
    //g_log_set_always_fatal( G_LOG_FLAG_RECURSION | G_LOG_FLAG_FATAL | G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING );

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

	const gchar* home_dir = g_get_home_dir();
	snprintf(app.home_dir, 256, "%s", home_dir); //no dont bother
	snprintf(app.config_filename, 256, "%s/.samplecat", app.home_dir);
	config_load();

	MYSQL *mysql;
	if(db_connect()) mysql = &app.mysql;
	else on_quit(NULL, GINT_TO_POINTER(1));

	g_thread_init(NULL);
	gdk_threads_init();
	app.mutex = g_mutex_new();
	gtk_init(&argc, &argv);

	type_init();
	pixmaps_init();

#ifndef DEBUG_NO_THREADS
	if(debug) printf("main(): creating overview thread...\n");
	GError *error = NULL;
	msg_queue = g_async_queue_new();
	if(!g_thread_create(overview_thread, NULL, FALSE, &error)){
		errprintf("main(): error creating thread: %s\n", error->message);
		g_error_free(error);
	}
#endif

	if (!gnome_vfs_init()){ errprintf("could not initialize GnomeVFS.\n"); return 1; }

	app.store = gtk_list_store_new(NUM_COLS, GDK_TYPE_PIXBUF, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);
 
	//GtkTreeModel *filter = gtk_tree_model_filter_new(GTK_TREE_MODEL(store), NULL);
	//gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(filter), (GtkTreeModelFilterVisibleFunc)func, NULL, NULL);

	do_search(app.search_phrase, app.search_dir);

	window_new(); 
	filter_new();
	tagshow_selector_new();
	tag_selector_new();
 
	//gtk_box_pack_start(GTK_BOX(app.vbox), view, EXPAND_TRUE, FILL_TRUE, 0);
	gtk_container_add(GTK_CONTAINER(app.scroll), app.view);

	app.context_menu = make_context_menu();
	g_signal_connect((gpointer)app.view, "button-press-event", G_CALLBACK(on_row_clicked), NULL);

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
      +--hpaned
      |  +--vpaned (left pane)
      |  |  +--directory tree
      |  |  +--inspector
      |  | 
      |  +--scrollwin (right pane)
      |     +--treeview
      |
      +--statusbar hbox
         +--statusbar	
         +--statusbar2

*/
	printf("window_new().\n");
	
	GtkWidget *window = app.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	g_signal_connect (G_OBJECT(window), "delete_event", G_CALLBACK(on_quit), NULL);
	//g_signal_connect(window, "destroy", gtk_main_quit, NULL);

	GtkWidget *vbox = app.vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);
	gtk_container_add(GTK_CONTAINER(window), vbox);

	GtkWidget *hbox_statusbar = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox_statusbar);
	gtk_box_pack_end(GTK_BOX(vbox), hbox_statusbar, EXPAND_FALSE, FALSE, 0);

	//alignment to give top border to main hpane.
	GtkWidget* align1 = gtk_alignment_new(0.0, 0.0, 1.0, 1.0);
	gtk_alignment_set_padding(GTK_ALIGNMENT(align1), 2, 1, 0, 0); //top, bottom, left, right.
	gtk_widget_show(align1);
	gtk_box_pack_end(GTK_BOX(vbox), align1, EXPAND_TRUE, TRUE, 0);

	GtkWidget *hpaned = gtk_hpaned_new();
	gtk_paned_set_position(GTK_PANED(hpaned), 160);
	//gtk_box_pack_end(GTK_BOX(vbox), hpaned, EXPAND_TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(align1), hpaned);	
	gtk_widget_show(hpaned);

	GtkWidget* tree = left_pane();
	gtk_paned_add1(GTK_PANED(hpaned), tree);

	GtkWidget *scroll = app.scroll = gtk_scrolled_window_new(NULL, NULL);  //adjustments created automatically.
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_widget_show(scroll);
	//gtk_box_pack_end(GTK_BOX(vbox), scroll, EXPAND_TRUE, TRUE, 0);
	gtk_paned_add2(GTK_PANED(hpaned), scroll);

	make_listview();

	GtkWidget *statusbar = app.statusbar = gtk_statusbar_new();
	//printf("statusbar=%p\n", app.statusbar);
	//gtk_statusbar_set_has_resize_grip(GTK_STATUSBAR(statusbar), TRUE);	//why does give a warning??????
	gtk_statusbar_set_has_resize_grip(GTK_STATUSBAR(statusbar), FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(statusbar), 5);
	gtk_box_pack_start(GTK_BOX(hbox_statusbar), statusbar, EXPAND_TRUE, FILL_TRUE, 0);
	gtk_widget_show(statusbar);

	GtkWidget *statusbar2 = app.statusbar2 = gtk_statusbar_new();
	//gtk_statusbar_set_has_resize_grip(GTK_STATUSBAR(statusbar), TRUE);	//why does give a warning??????
	gtk_statusbar_set_has_resize_grip(GTK_STATUSBAR(statusbar2), FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(statusbar2), 5);
	gtk_box_pack_start(GTK_BOX(hbox_statusbar), statusbar2, EXPAND_TRUE, FILL_TRUE, 0);
	gtk_widget_show(statusbar2);

	g_signal_connect(G_OBJECT(window), "realize", G_CALLBACK(window_on_realise), NULL);
	g_signal_connect(G_OBJECT(window), "size-allocate", G_CALLBACK(window_on_allocate), NULL);
	g_signal_connect(G_OBJECT(window), "configure_event", G_CALLBACK(window_on_configure), NULL);

	gtk_widget_show_all(window);

	dnd_setup();

	return TRUE;
}


void
window_on_realise(GtkWidget *win, gpointer user_data)
{
	//printf("window_on_realise()...\n");

}


void
window_on_allocate(GtkWidget *win, gpointer user_data)
{
	//printf("window_on_allocate()...\n");
	GdkColor colour;
	#define SCROLLBAR_WIDTH_HACK 32
	static gboolean done = FALSE;
	if(!app.view->requisition.width) return;

	if(!done){
		//printf("window_on_allocate(): requisition: wid=%i height=%i\n", app.view->requisition.width, app.view->requisition.height);
		//moved!
		//gtk_widget_set_size_request(win, app.view->requisition.width + SCROLLBAR_WIDTH_HACK, MIN(app.view->requisition.height, 900));
		done = TRUE;
	}else{
		//now reduce the request to allow the user to manually make the window smaller.
		gtk_widget_set_size_request(win, 100, 100);
	}

	colour_get_style_bg(&app.bg_colour, GTK_STATE_NORMAL);
	colour_get_style_fg(&app.fg_colour, GTK_STATE_NORMAL);
	colour_get_style_base(&app.base_colour, GTK_STATE_NORMAL);
	colour_get_style_text(&app.text_colour, GTK_STATE_NORMAL);

	if(app.colourbox_dirty){
		//put the style colours into the palette:
		if(colour_darker (&colour, &app.fg_colour)) colour_box_add(&colour);
		colour_box_add(&app.fg_colour);
		if(colour_lighter(&colour, &app.fg_colour)) colour_box_add(&colour);
		colour_box_add(&app.bg_colour);
		colour_box_add(&app.base_colour);
		colour_box_add(&app.text_colour);

		if(colour_darker (&colour, &app.base_colour)) colour_box_add(&colour);
		if(colour_lighter(&colour, &app.base_colour)) colour_box_add(&colour);
		if(colour_lighter(&colour, &colour)         ) colour_box_add(&colour);

		colour_get_style_base(&colour, GTK_STATE_SELECTED);
		if(colour_lighter(&colour, &colour)) colour_box_add(&colour);
		if(colour_lighter(&colour, &colour)) colour_box_add(&colour);
		if(colour_lighter(&colour, &colour)) colour_box_add(&colour);

		//add greys:
		gdk_color_parse("#555555", &colour);
		colour_box_add(&colour);
		if(colour_lighter(&colour, &colour)) colour_box_add(&colour);
		if(colour_lighter(&colour, &colour)) colour_box_add(&colour);
		if(colour_lighter(&colour, &colour)) colour_box_add(&colour);

		//make modifier colours:
		colour_get_style_bg(&app.bg_colour_mod1, GTK_STATE_NORMAL);
		app.bg_colour_mod1.red   = min(app.bg_colour_mod1.red   + 0x1000, 0xffff);
		app.bg_colour_mod1.green = min(app.bg_colour_mod1.green + 0x1000, 0xffff);
		app.bg_colour_mod1.blue  = min(app.bg_colour_mod1.blue  + 0x1000, 0xffff);

		//set column colours:
		//printf("window_on_allocate(): fg_color: %x %x %x\n", app.fg_colour.red, app.fg_colour.green, app.fg_colour.blue);

		g_object_set(app.cell1, "cell-background-gdk", &app.bg_colour_mod1, "cell-background-set", TRUE, NULL);
		g_object_set(app.cell1, "foreground-gdk", &app.fg_colour, "foreground-set", TRUE, NULL);

		colour_box_update();
		app.colourbox_dirty = FALSE;
	}
}


gboolean
window_on_configure(GtkWidget *widget, GdkEventConfigure *event, gpointer user_data)
{
	static gboolean window_size_set = FALSE;
	if(!window_size_set){
		//take the window size from the config file, or failing that, from the treeview requisition.
		int width = atoi(app.config.window_width);
		if(!width) width = app.view->requisition.width + SCROLLBAR_WIDTH_HACK;
		int height = atoi(app.config.window_height);
		if(!height) height = MIN(app.view->requisition.height, 900);
		if(width && height){
			printf("window_on_configure()...width=%s height=%s\n", app.config.window_width, app.config.window_height);
			gtk_window_resize(GTK_WINDOW(app.window), width, height);
			window_size_set = TRUE;
		}
	}
	return FALSE;
}


gboolean
colour_box_exists(GdkColor* colour)
{
	//returns true if a similar colour already exists in the colour_box.

	GdkColor existing_colour;
	char string[8];
	int i;
	for(i=0;i<PALETTE_SIZE;i++){
		snprintf(string, 8, "#%s", app.config.colour[i]);
		if(!gdk_color_parse(string, &existing_colour)) warnprintf("colour_box_exists(): parsing of colour string failed (%s).\n", string);
		if(is_similar(colour, &existing_colour)) return TRUE;
	}

	return FALSE;
}


gboolean
colour_box_add(GdkColor* colour)
{
	static unsigned slot = 0;

	if(slot >= PALETTE_SIZE){ warnprintf("colour_box_add() colour_box full.\n"); return FALSE; }

	//only add a colour if a similar colour isnt already there.
	if(colour_box_exists(colour)) return FALSE;

	hexstring_from_gdkcolor(app.config.colour[slot++], colour);
	return TRUE;
}


void
make_listview()
{
	//the main pane. A treeview with a list of samples.

	GtkWidget *view = app.view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(app.store));
	g_signal_connect(view, "motion-notify-event", (GCallback)treeview_on_motion, NULL);
	g_signal_connect(view, "drag-data-received", G_CALLBACK(treeview_drag_received), NULL); //currently the window traps this before we get here.
	g_signal_connect(view, "drag-motion", G_CALLBACK(drag_motion), NULL);
	//gtk_tree_view_set_fixed_height_mode(GTK_TREE_VIEW(view), TRUE); //supposed to be faster. gtk >= 2.6

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
	GtkTreeViewColumn *col1 = gtk_tree_view_column_new_with_attributes("Sample Name", cell1, "text", COL_NAME, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col1);
	gtk_tree_view_column_set_sort_column_id(col1, COL_NAME);
	gtk_tree_view_column_set_resizable(col1, TRUE);
	gtk_tree_view_column_set_reorderable(col1, TRUE);
	gtk_tree_view_column_set_min_width(col1, 0);
	//gtk_tree_view_column_set_spacing(col1, 10);
	//g_object_set(cell1, "ypad", 0, NULL);
	gtk_tree_view_column_set_cell_data_func(col1, cell1, (gpointer)path_cell_bg_lighter, NULL, NULL);
	
	GtkCellRenderer *cell2 = gtk_cell_renderer_text_new();
	GtkTreeViewColumn *col2 = gtk_tree_view_column_new_with_attributes("Path", cell2, "text", COL_FNAME, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col2);
	gtk_tree_view_column_set_sort_column_id(col2, COL_FNAME);
	gtk_tree_view_column_set_resizable(col2, TRUE);
	gtk_tree_view_column_set_reorderable(col2, TRUE);
	gtk_tree_view_column_set_min_width(col2, 0);
	//g_object_set(cell2, "ypad", 0, NULL);
	gtk_tree_view_column_set_cell_data_func(col2, cell2, path_cell_data_func, NULL, NULL);

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

	gtk_widget_show(view);
}


GtkWidget*
left_pane()
{
	//other examples of file navigation: nautilus? (rox has no tree)

	app.vpaned = gtk_vpaned_new();
	//gtk_box_pack_end(GTK_BOX(vbox), hpaned, EXPAND_TRUE, TRUE, 0);
	gtk_widget_show(app.vpaned);

	GtkWidget* tree = dir_tree_new();
	gtk_paned_add1(GTK_PANED(app.vpaned), tree);
	gtk_widget_show(tree);
	g_signal_connect(tree, "link_selected", G_CALLBACK(dir_tree_on_link_selected), NULL);

#ifdef INSPECTOR
#endif
	GtkWidget* inspector = inspector_pane();
	gtk_paned_add2(GTK_PANED(app.vpaned), inspector);

	//return tree;
	return app.vpaned;
}


GtkWidget*
inspector_pane()
{
	//close up on a single sample. Bottom left of main window.

	app.inspector = malloc(sizeof(*app.inspector));
	app.inspector->row_ref = NULL;

	int margin_left = 5;

	GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);

	//left align the label:
	GtkWidget* align1 = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
	gtk_widget_show(align1);
	gtk_box_pack_start(GTK_BOX(vbox), align1, EXPAND_FALSE, FILL_FALSE, 0);

	// sample name:
	GtkWidget* label1 = app.inspector->name = gtk_label_new("");
	gtk_misc_set_padding(GTK_MISC(label1), margin_left, 5);
	gtk_widget_show(label1);
	gtk_container_add(GTK_CONTAINER(align1), label1);	

	PangoFontDescription* pangofont = pango_font_description_from_string("San-Serif 18");
	gtk_widget_modify_font(label1, pangofont);
	pango_font_description_free(pangofont);

	//-----------

	app.inspector->image = gtk_image_new_from_pixbuf(NULL);
	gtk_misc_set_alignment(GTK_MISC(app.inspector->image), 0.0, 0.5);
	gtk_misc_set_padding(GTK_MISC(app.inspector->image), margin_left, 2);
	gtk_box_pack_start(GTK_BOX(vbox), app.inspector->image, EXPAND_FALSE, FILL_FALSE, 0);
	gtk_widget_show(app.inspector->image);

	//-----------

	GtkWidget* align2 = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
	gtk_widget_show(align2);
	gtk_box_pack_start(GTK_BOX(vbox), align2, EXPAND_FALSE, FILL_FALSE, 0);

	GtkWidget* label2 = app.inspector->filename = gtk_label_new("Filename");
	gtk_misc_set_padding(GTK_MISC(label2), margin_left, 2);
	gtk_widget_show(label2);
	gtk_container_add(GTK_CONTAINER(align2), label2);	

	//-----------

	GtkWidget* align3 = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
	gtk_widget_show(align3);
	gtk_box_pack_start(GTK_BOX(vbox), align3, EXPAND_FALSE, FILL_FALSE, 0);

	//the tags label has an event box to catch mouse clicks.
	GtkWidget *ev_tags = app.inspector->tags_ev = gtk_event_box_new ();
	gtk_widget_show(ev_tags);
	gtk_container_add(GTK_CONTAINER(align3), ev_tags);	
	g_signal_connect((gpointer)app.inspector->tags_ev, "button-release-event", G_CALLBACK(inspector_on_tags_clicked), NULL);

	GtkWidget* label3 = app.inspector->tags = gtk_label_new("Tags");
	gtk_misc_set_padding(GTK_MISC(label3), margin_left, 2);
	gtk_widget_show(label3);
	//gtk_container_add(GTK_CONTAINER(align3), label3);	
	gtk_container_add(GTK_CONTAINER(ev_tags), label3);	
	

	//-----------

	GtkWidget* align4 = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
	gtk_widget_show(align4);
	gtk_box_pack_start(GTK_BOX(vbox), align4, EXPAND_FALSE, FILL_FALSE, 0);

	GtkWidget* label4 = app.inspector->length = gtk_label_new("Length");
	gtk_misc_set_padding(GTK_MISC(label4), margin_left, 2);
	gtk_widget_show(label4);
	gtk_container_add(GTK_CONTAINER(align4), label4);	

	//-----------
	
	GtkWidget* align5 = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
	gtk_widget_show(align5);
	gtk_box_pack_start(GTK_BOX(vbox), align5, EXPAND_FALSE, FILL_FALSE, 0);

	GtkWidget* label5 = app.inspector->samplerate = gtk_label_new("Samplerate");
	gtk_misc_set_padding(GTK_MISC(label5), margin_left, 2);
	gtk_widget_show(label5);
	gtk_container_add(GTK_CONTAINER(align5), label5);	

	//-----------

	GtkWidget* align6 = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
	gtk_widget_show(align6);
	gtk_box_pack_start(GTK_BOX(vbox), align6, EXPAND_FALSE, FILL_FALSE, 0);

	GtkWidget* label6 = app.inspector->channels = gtk_label_new("Channels");
	gtk_misc_set_padding(GTK_MISC(label6), margin_left, 2);
	gtk_widget_show(label6);
	gtk_container_add(GTK_CONTAINER(align6), label6);	

	//-----------

	GtkWidget* align7 = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
	gtk_widget_show(align7);
	gtk_box_pack_start(GTK_BOX(vbox), align7, EXPAND_FALSE, FILL_FALSE, 0);

	GtkWidget* label7 = app.inspector->mimetype = gtk_label_new("Mimetype");
	gtk_misc_set_padding(GTK_MISC(label7), margin_left, 2);
	gtk_widget_show(label7);
	gtk_container_add(GTK_CONTAINER(align7), label7);	
	
	//-----------

	//notes:

	GtkTextBuffer *txt_buf1;
	GtkWidget *text1 = gtk_text_view_new();
	//gtk_widget_show(text1);
	txt_buf1 = app.inspector->notes = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text1));
	gtk_text_buffer_set_text(txt_buf1, "this sample works really well on mid-tempo tracks", -1);
	gtk_text_view_set_accepts_tab(GTK_TEXT_VIEW(text1), FALSE);
	g_signal_connect(G_OBJECT(text1), "focus-out-event", G_CALLBACK(on_notes_focus_out), NULL);
	g_signal_connect(G_OBJECT(text1), "insert-at-cursor", G_CALLBACK(on_notes_insert), NULL);
	gtk_box_pack_start(GTK_BOX(vbox), text1, FALSE, TRUE, 2);

	GValue gval = {0,};
	g_value_init(&gval, G_TYPE_CHAR);
	g_value_set_char(&gval, margin_left);
	g_object_set_property(G_OBJECT(text1), "border-width", &gval);

	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text1), GTK_WRAP_WORD_CHAR);
	gtk_text_view_set_pixels_above_lines(GTK_TEXT_VIEW(text1), 5);
	gtk_text_view_set_pixels_below_lines(GTK_TEXT_VIEW(text1), 5);

	//invisible edit widget:
	GtkWidget *edit = app.inspector->edit = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(edit), 64);
	gtk_entry_set_text(GTK_ENTRY(edit), "");
	gtk_widget_show(edit);
	/*	
	g_signal_connect(G_OBJECT(entry), "focus-out-event", G_CALLBACK(new_search), NULL);
	//this is supposed to enable REUTRN to enter the text - does it work?
	gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
	//g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(on_entry_activate), NULL);
	g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(new_search), NULL);
	*/
    gtk_widget_ref(edit);//stops gtk deleting the widget.

	//this also sets the margin:
	//gtk_text_view_set_border_window_size(GTK_TEXT_VIEW(text1), GTK_TEXT_WINDOW_LEFT, 20);

	//printf("inspector_pane(): size=%i inspector=%p textbuf=%p\n", sizeof(*app.inspector), app.inspector, app.inspector->notes);

	return vbox;
}


void
inspector_update(GtkTreePath *path)
{
	if(!app.inspector) return;
	ASSERT_POINTER(path, "inspector_update", "path");
	//if((unsigned)path<1024){ errprintf("inspector_update(): bad path arg.\n"); return; }
	printf("inspector_update()...\n");

	if (app.inspector->row_ref){ gtk_tree_row_reference_free(app.inspector->row_ref); app.inspector->row_ref = NULL; }

	sample* sample = sample_new_from_model(path);

	GtkTreeIter iter;
	gtk_tree_model_get_iter(GTK_TREE_MODEL(app.store), &iter, path);
	gchar *tags;
	gchar *fpath;
	gchar *fname;
	gchar *length;
	gchar *samplerate;
	int channels;
	gchar *mimetype;
	gchar *notes;
	GdkPixbuf* pixbuf = NULL;
	int id;
	gtk_tree_model_get(GTK_TREE_MODEL(app.store), &iter, COL_NAME, &fname, COL_FNAME, &fpath, COL_KEYWORDS, &tags, COL_LENGTH, &length, COL_SAMPLERATE, &samplerate, COL_CHANNELS, &channels, COL_MIMETYPE, &mimetype, COL_NOTES, &notes, COL_OVERVIEW, &pixbuf, COL_IDX, &id, -1);

	char ch_str[64]; snprintf(ch_str, 64, "%u channels", channels);
	char fs_str[64]; snprintf(fs_str, 64, "%s kHz",      samplerate);

	gtk_label_set_text(GTK_LABEL(app.inspector->name),       fname);
	gtk_label_set_text(GTK_LABEL(app.inspector->filename),   sample->filename);
	gtk_label_set_text(GTK_LABEL(app.inspector->tags),       tags);
	gtk_label_set_text(GTK_LABEL(app.inspector->length),     length);
	gtk_label_set_text(GTK_LABEL(app.inspector->channels),   ch_str);
	gtk_label_set_text(GTK_LABEL(app.inspector->samplerate), fs_str);
	gtk_label_set_text(GTK_LABEL(app.inspector->mimetype),   mimetype);
	gtk_text_buffer_set_text(app.inspector->notes, notes ? notes : "", -1);
	gtk_image_set_from_pixbuf(GTK_IMAGE(app.inspector->image), pixbuf);

	//store a reference to the row id in the inspector widget:
	//g_object_set_data(G_OBJECT(app.inspector->name), "id", GUINT_TO_POINTER(id));
	app.inspector->row_id = id;
	app.inspector->row_ref = gtk_tree_row_reference_new(GTK_TREE_MODEL(app.store), path);
	if(!app.inspector->row_ref) errprintf("inspector_update(): setting row_ref failed!\n");

	free(sample);
	//printf("inspector_update(): done.\n");
}


gboolean
inspector_on_tags_clicked(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
	/*
	a single click on the Tags label puts us into edit mode.
	*/
	//printf("inspector_on_tags_clicked()...\n");

	if(event->button == 3) return FALSE;

	tag_edit_start(0);

	return TRUE;
}


GtkWidget*
colour_box_new(GtkWidget* parent)
{
	GtkWidget* e;
	//GtkWidget* h = gtk_hbox_new(FALSE, 0);
	int i;
	for(i=PALETTE_SIZE-1;i>=0;i--){
		e = app.colour_button[i] = gtk_event_box_new();

		//dnd:
		gtk_drag_source_set(e, GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
                          dnd_file_drag_types,          //const GtkTargetEntry *targets,
                          dnd_file_drag_types_count,    //gint n_targets,
                          GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_LINK);
		//g_signal_connect(G_OBJECT(e), "drag-begin", G_CALLBACK(colour_drag_begin), NULL);
		//g_signal_connect(G_OBJECT(e), "drag-end",   G_CALLBACK(colour_drag_end),   NULL);
		//g_signal_connect(G_OBJECT(e), "drag-data-received", G_CALLBACK(colour_drag_datareceived), NULL);
		g_signal_connect(G_OBJECT(e), "drag-data-get", G_CALLBACK(colour_drag_dataget), GUINT_TO_POINTER(i));

		gtk_widget_show(e);

		gtk_box_pack_end(GTK_BOX(parent), e, TRUE, TRUE, 0);
	}
	//gtk_widget_show_all(h);
	return e;
}


void
colour_box_update()
{
	//show the current palette colours in the colour_box
	printf("colour_box_update()...\n");
	int i;
	GdkColor colour;
	char colour_string[16];
	for(i=PALETTE_SIZE-1;i>=0;i--){
		snprintf(colour_string, 16, "#%s", app.config.colour[i]);
		if(!gdk_color_parse(colour_string, &colour)) warnprintf("colour_box_update(): parsing of colour string failed.\n");
		//printf("colour_box_update(): colour: %x %x %x\n", colour.red, colour.green, colour.blue);

		if(app.colour_button[i]){
			if(colour.red != app.colour_button[i]->style->bg[GTK_STATE_NORMAL].red) 
				gtk_widget_modify_bg(app.colour_button[i], GTK_STATE_NORMAL, &colour);
		}
	}
}


GtkWidget*
dir_tree_new()
{
	//data:
	app.dir_tree = g_node_new(NULL); //this is unneccesary ?
	db_get_dirs();

	//view:
	app.dir_treeview = dh_book_tree_new(app.dir_tree);

	return app.dir_treeview;
}


gboolean
dir_tree_on_link_selected(GObject *ignored, DhLink *link, gpointer data)
{
	/*
	what does it mean if link->uri is empty?
	*/
	ASSERT_POINTER_FALSE("dir_tree_on_link_selected", link, "link");

	printf("tree_on_link_selected()...uri=%s\n", link->uri);
	//FIXME segfault if we use this if()
	//if(strlen(link->uri)){
		set_search_dir(link->uri);

		const gchar* text = app.search ? gtk_entry_get_text(GTK_ENTRY(app.search)) : "";
		do_search((gchar*)text, link->uri);
	//}
	return FALSE;
}


gboolean
dir_tree_update(gpointer data)
{
	/*
	refresh the directory tree. Called from an idle.

	note: destroying the whole node tree is wasteful - can we just make modifications to it?

	*/
	printf("dir_tree_update()... \n");

	GNode *node;
	node = app.dir_tree;

	db_get_dirs();

	dh_book_tree_reload((DhBookTree*)app.dir_treeview);

	return FALSE; //source should be removed.
}


void
set_search_dir(char* dir)
{
	//this doesnt actually do the search. When calling, follow with do_search() if neccesary.

	//if string is empty, we show all directories?

	if(!dir){ errprintf("set_search_dir()\n"); return; }

	printf("set_search_dir(): dir=%s\n", dir);
	app.search_dir = dir;
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
	//else printf("get_mouseover_row() failed. rowref=%p\n", app.mouseover_row_ref);
	return row_num;
}


void
path_cell_data_func(GtkTreeViewColumn *tree_column, GtkCellRenderer *cell, GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer data)
{
	unsigned colour_index = 0;
	gtk_tree_model_get(GTK_TREE_MODEL(app.store), iter, COL_COLOUR, &colour_index, -1);
	if(colour_index > PALETTE_SIZE){ warnprintf("path_cell_data_func(): bad colour data. Index out of range (%u).\n", colour_index); return; }

	char colour[16];
	snprintf(colour, 16, "#%s", app.config.colour[colour_index]);
	//printf("path_cell_data_func(): colour=%i %s\n", colour_index, colour);

	if(strlen(colour) != 7 ){ errprintf("path_cell_data_func(): bad colour string (%s) index=%u.\n", colour, colour_index); return; }

	if(colour_index) g_object_set(cell, "cell-background-set", TRUE, "cell-background", colour, NULL);
	else             g_object_set(cell, "cell-background-set", FALSE, NULL);
}


void
path_cell_bg_lighter(GtkTreeViewColumn *tree_column, GtkCellRenderer *cell, GtkTreeModel *tree_model, GtkTreeIter *iter)
{
	unsigned colour_index = 0;
	gtk_tree_model_get(GTK_TREE_MODEL(app.store), iter, COL_COLOUR, &colour_index, -1);
	if(colour_index > PALETTE_SIZE){ warnprintf("path_cell_data_func(): bad colour data. Index out of range (%u).\n", colour_index); return; }

	if(colour_index == 0){
		colour_index = 4; //FIXME temp
	}

	GdkColor colour, colour2;
	char hexstring[8];
	snprintf(hexstring, 8, "#%s", app.config.colour[colour_index]);
	if(!gdk_color_parse(hexstring, &colour)) warnprintf("path_cell_bg_lighter(): parsing of colour string failed.\n");
	colour_lighter(&colour2, &colour);

	//printf("path_cell_bg_lighter(): index=%i #%s %x %x %x\n", colour_index, hexstring, colour.red, colour.green, colour.blue);

	//if(strlen(colour) != 7 ){ errprintf("path_cell_data_func(): bad colour string (%s) index=%u.\n", colour, colour_index); return; }

	g_object_set(cell, "cell-background-set", TRUE, "cell-background-gdk", &colour2, NULL);
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

	//set background colour:
	path_cell_data_func(tree_column, cell, tree_model, iter, data);

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
file_selector()
{
	//GtkWidget*  gtk_file_selection_new("Select files to add");
}


void
scan_dir(const char* path)
{
	/*
	scan the directory and try and add any files we find.
	
	*/
	printf("scan_dir()....\n");

	//gchar path[256];
	char filepath[256];
	G_CONST_RETURN gchar *file;
	GError *error = NULL;
	//sprintf(path, app.home_dir);
	GDir *dir;
	if((dir = g_dir_open(path, 0, &error))){
		while((file = g_dir_read_name(dir))){
			if(file[0]=='.') continue;
			snprintf(filepath, 128, "%s/%s", path, file);

			if(!g_file_test(filepath, G_FILE_TEST_IS_DIR)){
				//printf("scan_dir(): checking files: %s\n", file);
				add_file(filepath);
			}
			// IS_DIR
			else if(app.add_recursive){
				scan_dir(filepath);
			}
		}
		g_dir_close(dir);
	}else{
		errprintf("scan_dir(): cannot open directory. %s\n", error->message);
		g_error_free(error);
		error = NULL;
	}
}


void
do_search(char *search, char *dir)
{
	//fill the display with the results for the given search phrase.

	unsigned i, channels, colour;
	float samplerate; char samplerate_s[32];
	char length[64];
	GtkTreeIter iter;

	MYSQL_RES *result;
	MYSQL_ROW row;
	unsigned long *lengths;
	unsigned int num_fields;
	char query[1024];
	char where[512]="";
	char where_dir[512]="";
	char category[256]="";
	char sb_text[256];
	//char phrase[256]="";
	GdkPixbuf* pixbuf  = NULL;
	GdkPixbuf* iconbuf = NULL;
	GdkPixdata pixdata;
	gboolean online = FALSE;

	MYSQL *mysql;
	mysql=&app.mysql;

	//lets assume that the search_phrase filter should *always* be in effect.

	if (app.search_category) snprintf(category, 256, "AND keywords LIKE '%%%s%%' ", app.search_category);

	if(dir && strlen(dir)){
		snprintf(where_dir, 512, "AND filedir='%s' %s ", dir, category);
	}

	//strmov(dst, src) moves all the  characters  of  src to dst, and returns a pointer to the new closing NUL in dst. 
	//strmov(query_def, "SELECT * FROM samples"); //this is defined in mysql m_string.h
	//strcpy(query, "SELECT * FROM samples WHERE 1 ");

	char* a = NULL;

	if(strlen(search)){ 
		snprintf(where, 512, "AND (filename LIKE '%%%s%%' OR filedir LIKE '%%%s%%' OR keywords LIKE '%%%s%%') ", search, search, search); //FIXME duplicate category LIKE's
		//snprintf(query, 1024, "SELECT * FROM samples WHERE 1 %s", where);
	}

	//append the dir-where part:
	a = where + strlen(where);
	strmov(a, where_dir);

	snprintf(query, 1024, "SELECT * FROM samples WHERE 1 %s", where);

	printf("%s\n", query);
	if(mysql_exec_sql(mysql, query)==0){ //success
		
		//problem with wierd mysql int type:
		//printf( "%ld Records Found\n", (long) mysql_affected_rows(&mysql));
		//printf( "%lu Records Found\n", (unsigned long)mysql_affected_rows(mysql));

	
		treeview_block_motion_handler(); //dunno exactly why but this prevents a segfault.

		clear_store();

		//detach the model from the view to speed up row inserts:
		/*
		model = gtk_tree_view_get_model(GTK_TREE_VIEW(view));
		g_object_ref(model);
		gtk_tree_view_set_model(GTK_TREE_VIEW(view), NULL);
		*/ 

		result = mysql_store_result(mysql);
		if(result){// there are rows
			num_fields = mysql_num_fields(result);
			char keywords[256];

			int row_count = 0;
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
				if(row[MYSQL_COLOUR]==NULL) colour = 0; else colour = atoi(row[MYSQL_COLOUR]);

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
													 COL_MIMETYPE, row[MYSQL_MIMETYPE], COL_NOTES, row[MYSQL_NOTES], COL_COLOUR, colour, -1);
				if(pixbuf) g_object_unref(pixbuf);
				row_count++;
			}
			mysql_free_result(result);
			snprintf(sb_text, 256, "%i samples found.", row_count);
			statusbar_print(1, sb_text);
		}
		else{  // mysql_store_result() returned nothing
		  if(mysql_field_count(mysql) > 0){
				// mysql_store_result() should have returned data
				printf( "Error getting records: %s\n", mysql_error(mysql));
		  }
		  else printf( "do_search(): Failed to find any records (fieldcount<1): %s\n", mysql_error(mysql));
		}

		//treeview_unblock_motion_handler();  //causes a segfault even before it is run ??!!
	}
	else{
		printf( "do_search(): Failed to find any records: %s\n", mysql_error(mysql));
	}
	if(debug) printf( "do_search(): done.\n");
}


void
clear_store()
{
	printf("clear_store()...\n");

	//gtk_list_store_clear(app.store);

	GtkTreeIter iter;
	GdkPixbuf* pixbuf = NULL;

	while(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(app.store), &iter)){

		gtk_tree_model_get(GTK_TREE_MODEL(app.store), &iter, COL_OVERVIEW, &pixbuf, -1);

		gtk_list_store_remove(app.store, &iter);

		if(pixbuf) g_object_unref(pixbuf);
	}
	//printf("clear_store(): store cleared.\n");
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
	printf("treeview_unblock_motion_handler()...\n");
	if(app.view){
		gulong id1= g_signal_handler_find(app.view,
							   G_SIGNAL_MATCH_FUNC, // | G_SIGNAL_MATCH_DATA,
							   0,        //guint signal_id
							   0,        //GQuark detail
							   0,        //GClosure *closure
							   treeview_on_motion, //callback
							   NULL);    //data
		if(id1) g_signal_handler_unblock(app.view, id1);
		else warnprintf("treeview_unblock_motion_handler(): failed to find handler.\n");
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

	GtkWidget* label1 = gtk_label_new("Search");
	gtk_misc_set_padding(GTK_MISC(label1), 5,5);
	gtk_widget_show(label1);
	gtk_box_pack_start(GTK_BOX(hbox), label1, FALSE, FALSE, 0);
	
	GtkWidget *entry = app.search = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(entry), 64);
	gtk_entry_set_text(GTK_ENTRY(entry), "");
	gtk_widget_show(entry);	
	gtk_box_pack_start(GTK_BOX(hbox), entry, EXPAND_TRUE, TRUE, 0);
	g_signal_connect(G_OBJECT(entry), "focus-out-event", G_CALLBACK(new_search), NULL);
	//this is supposed to enable REUTRN to enter the text - does it work?
	gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
	//g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(on_entry_activate), NULL);
	g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(new_search), NULL);

	//----------------------------------------------------------------------

	//second row (metadata edit):
	GtkWidget* hbox_edit = app.toolbar2 = gtk_hbox_new(FALSE, 0);
    gtk_widget_show(hbox_edit);
	gtk_box_pack_start(GTK_BOX(app.vbox), hbox_edit, EXPAND_FALSE, FILL_FALSE, 0);

	//left align the label:
	GtkWidget* align1 = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
	gtk_widget_show(align1);
	gtk_box_pack_start(GTK_BOX(hbox_edit), align1, EXPAND_FALSE, FILL_FALSE, 0);

	GtkWidget* label2 = gtk_label_new("Edit");
	gtk_misc_set_padding(GTK_MISC(label2), 5,5);
	gtk_widget_show(label2);
	gtk_container_add(GTK_CONTAINER(align1), label2);	

	//make the two lhs labels the same width:
	GtkSizeGroup* size_group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	gtk_size_group_add_widget(size_group, label1);
	gtk_size_group_add_widget(size_group, align1);
	
	/*GtkWidget* colour_box = */colour_box_new(hbox_edit);
	//gtk_box_pack_start(GTK_BOX(hbox_edit), colour_box, EXPAND_FALSE, FILL_FALSE, 0);

	return TRUE;
}


gboolean
tag_selector_new()
{
	//the tag _edit_ selector

	/*
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
	*/

	GtkWidget* combo2 = app.category = gtk_combo_box_entry_new_text();
	GtkComboBox* combo_ = GTK_COMBO_BOX(combo2);
	gtk_combo_box_append_text(combo_, "no categories");
	gtk_combo_box_append_text(combo_, "drums");
	gtk_combo_box_append_text(combo_, "perc");
	gtk_combo_box_append_text(combo_, "keys");
	gtk_combo_box_append_text(combo_, "strings");
	gtk_combo_box_append_text(combo_, "fx");
	gtk_combo_box_append_text(combo_, "impulses");
	gtk_widget_show(combo2);	
	gtk_box_pack_start(GTK_BOX(app.toolbar2), combo2, EXPAND_FALSE, FALSE, 0);

	//"set" button:
	GtkWidget* set = gtk_button_new_with_label("Set Category");
	gtk_widget_show(set);	
	gtk_box_pack_start(GTK_BOX(app.toolbar2), set, EXPAND_FALSE, FALSE, 0);
	g_signal_connect(set, "clicked", G_CALLBACK(on_category_set_clicked), NULL);

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
on_category_set_clicked(GtkComboBox *widget, gpointer user_data)
{
	//add selected category to selected samples.

	printf("on_category_set_clicked()...\n");

	//selected category?
	gchar* category = gtk_combo_box_get_active_text(GTK_COMBO_BOX(app.category));

	GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(app.view));
	GList* selectionlist = gtk_tree_selection_get_selected_rows(selection, NULL);
	if(!selectionlist){ printf("on_category_set_clicked(): no files selected.\n"); return; }
	//printf("delete_row(): %i rows selected.\n", g_list_length(selectionlist));

	int i;
    GtkTreeIter iter;
	for(i=0;i<g_list_length(selectionlist);i++){
		GtkTreePath *treepath_selection = g_list_nth_data(selectionlist, i);

		if(gtk_tree_model_get_iter(GTK_TREE_MODEL(app.store), &iter, treepath_selection)){
			gchar *fname; gchar *tags;
			int id;
			gtk_tree_model_get(GTK_TREE_MODEL(app.store), &iter, COL_NAME, &fname, COL_KEYWORDS, &tags, COL_IDX, &id, -1);

			if(!strcmp(category, "no categories")) row_clear_tags(&iter, id);
			else{

				if(!keyword_is_dupe(category, tags)){
					char tags_new[1024];
					snprintf(tags_new, 1024, "%s %s", tags, category);
					g_strstrip(tags_new);//trim

					row_set_tags(&iter, id, tags_new);
					/*
					char sql[1024];
					snprintf(sql, 1024, "UPDATE samples SET keywords='%s' WHERE id=%i", tags_new, id);
					printf("on_category_set_clicked(): row: %s sql=%s\n", fname, sql);
					if(mysql_query(&app.mysql, sql)){
						errprintf("on_category_set_clicked(): update failed! sql=%s\n", sql);
						return;
					}
					//update the store:
					gtk_list_store_set(app.store, &iter, COL_KEYWORDS, tags_new, -1);
					*/
				}else{
					printf("on_category_set_clicked(): keyword is a dupe - not applying.\n");
					statusbar_print(1, "ignoring duplicate keyword.");
				}
			}

		} else errprintf("on_category_set_clicked() bad iter! i=%i (<%i)\n", i, g_list_length(selectionlist));
	}
	g_list_foreach(selectionlist, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(selectionlist);

	g_free(category);
}


gboolean
row_set_tags(GtkTreeIter* iter, int id, const char* tags_new)
{
	char sql[1024];
	snprintf(sql, 1024, "UPDATE samples SET keywords='%s' WHERE id=%i", tags_new, id);
	printf("on_category_set_clicked(): sql=%s\n", sql);
	if(mysql_query(&app.mysql, sql)){
		errprintf("on_category_set_clicked(): update failed! sql=%s\n", sql);
		return FALSE;
	}
	//update the store:
	gtk_list_store_set(app.store, iter, COL_KEYWORDS, tags_new, -1);
	return TRUE;
}


gboolean
row_set_tags_from_id(int id, GtkTreeRowReference* row_ref, const char* tags_new)
{
	if(!id){ errprintf("row_set_tags_from_id(): bad arg: id (%i)\n", id); return FALSE; }
	ASSERT_POINTER_FALSE("row_set_tags_from_id", row_ref, "row_ref");

	char sql[1024];
	snprintf(sql, 1024, "UPDATE samples SET keywords='%s' WHERE id=%i", tags_new, id);
	printf("on_category_set_clicked(): sql=%s\n", sql);
	if(mysql_query(&app.mysql, sql)){
		errprintf("on_category_set_clicked(): update failed! sql=%s\n", sql);
		return FALSE;
	}

	//update the store:

	GtkTreePath *path;
	if((path = gtk_tree_row_reference_get_path(row_ref))){
		GtkTreeIter iter;
		gtk_tree_model_get_iter(GTK_TREE_MODEL(app.store), &iter, path);
		gtk_tree_path_free(path);

		gtk_list_store_set(app.store, &iter, COL_KEYWORDS, tags_new, -1);
	}
	else { errprintf("row_set_tags_from_id(): cannot get row path: id=%i.\n", id); return FALSE; }

	return TRUE;
}


gboolean
row_clear_tags(GtkTreeIter* iter, int id)
{
	if(!id){ errprintf("row_clear_tags(): bad arg: id\n"); return FALSE; }

	char sql[1024];
	snprintf(sql, 1024, "UPDATE samples SET keywords='' WHERE id=%i", id);
	printf("row_clear_tags(): sql=%s\n", sql);
	if(mysql_query(&app.mysql, sql)){
		errprintf("on_category_set_clicked(): update failed! sql=%s\n", sql);
		return FALSE;
	}
	//update the store:
	gtk_list_store_set(app.store, iter, COL_KEYWORDS, "", -1);
	return TRUE;
}


void
db_get_dirs()
{
	/*
	builds a node list of directories listed in the database.
	*/
	if (!app.dir_tree) { errprintf("db_get_dirs(): dir_tree not initialised.\n"); return; }

	char qry[1024];
	MYSQL *mysql;
	mysql=&app.mysql;
	MYSQL_RES *result;
	MYSQL_ROW row;

	g_node_destroy(app.dir_tree);    //remove any previous nodes.
	app.dir_tree = g_node_new(NULL); //make an empty root node.

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

				//printf("db_get_dirs(): dir=%s\n", row[0]);
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
	
	do_search((gchar*)text, app.search_dir);
	return FALSE;
}


void
on_notes_insert(GtkTextView *textview, gchar *arg1, gpointer user_data)
{
	printf("on_notes_insert()...\n");
}


gboolean
on_notes_focus_out(GtkWidget *widget, gpointer userdata)
{
	printf("on_notes_focus_out()...\n");

	GtkTextBuffer* textbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
	if(!textbuf){ errprintf("on_notes_focus_out(): bad arg: widget.\n"); return FALSE; }

	GtkTextIter start_iter, end_iter;
	gtk_text_buffer_get_start_iter(textbuf,  &start_iter);
	gtk_text_buffer_get_end_iter  (textbuf,  &end_iter);
	gchar* notes = gtk_text_buffer_get_text(textbuf, &start_iter, &end_iter, TRUE);
	printf("on_notes_focus_out(): start=%i end=%i\n", gtk_text_iter_get_offset(&start_iter), gtk_text_iter_get_offset(&end_iter));

	unsigned id = app.inspector->row_id;
	char sql[1024];
	snprintf(sql, 1024, "UPDATE samples SET notes='%s' WHERE id=%u", notes, id);
	//printf("on_notes_focus_out(): %s\n", sql);
	if(mysql_query(&app.mysql, sql)){
		errprintf("on_notes_focus_out(): update failed! sql=%s\n", sql);
		return FALSE;
	}else{
		GtkTreePath *path;
		//if((path = gtk_tree_model_get_path(GTK_TREE_MODEL(app.store), &iter))){
		if((path = gtk_tree_row_reference_get_path(app.inspector->row_ref))){
  			GtkTreeIter iter;
			gtk_tree_model_get_iter(GTK_TREE_MODEL(app.store), &iter, path);
	    	gtk_list_store_set(app.store, &iter, COL_NOTES, notes, -1);
			gtk_tree_path_free(path);
		}
	}

	g_free(notes);
	return FALSE;
}


gboolean
item_set_colour(GtkTreePath* path, unsigned colour)
{
	ASSERT_POINTER_FALSE(path, "item_set_colour", "path")

	int id = path_get_id(path);

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


sample*
sample_new()
{
  sample* sample;
  sample = malloc(sizeof(*sample));
  sample->id = 0;
  sample->filetype = 0;
  sample->pixbuf = NULL;
  return sample;
}


sample*
sample_new_from_model(GtkTreePath *path)
{
  GtkTreeIter iter;
  GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(app.view));
  gtk_tree_model_get_iter(model, &iter, path);
  gchar *fpath, *fname, *mimetype;
  int id;
  gtk_tree_model_get(model, &iter, COL_FNAME, &fpath, COL_NAME, &fname, COL_IDX, &id, COL_MIMETYPE, &mimetype, -1);

  sample* sample = malloc(sizeof(*sample));
  sample->id = id;
  snprintf(sample->filename, 256, "%s/%s", fpath, fname);
  if(!strcmp(mimetype, "audio/x-flac")) sample->filetype = TYPE_FLAC; else sample->filetype = TYPE_SNDFILE; 
  sample->pixbuf = NULL;
  return sample;
}


void
sample_free(sample* sample)
{
	gtk_tree_row_reference_free(sample->row_ref);
	g_free(sample);
}


gboolean
add_file(char *uri)
{
  /*
  uri must be "unescaped" before calling this fn.
  */
  if(debug) printf("add_file()... %s\n", uri);
  gboolean ok = TRUE;

  sample* sample = sample_new(); //free'd after db and store are updated.
  snprintf(sample->filename, 256, "%s", uri);

  #define SQL_LEN 66000
  char sql[1024], sql2[SQL_LEN];
  char length_ms[64];
  char samplerate_s[32];
  //strcpy(sql, "INSERT samples SET ");
  gchar* filedir = g_path_get_dirname(uri);
  gchar* filename = g_path_get_basename(uri);
  snprintf(sql, 1024, "INSERT samples SET filename='%s', filedir='%s'", filename, filedir);

  MIME_type* mime_type = type_from_path(filename);
  char mime_string[64];
  snprintf(mime_string, 64, "%s/%s", mime_type->media_type, mime_type->subtype);

  char extn[64];
  file_extension(filename, extn);
  if(!strcmp(extn, "rfl")){
    printf("cannot add file: file type \"%s\" not supported.\n", extn);
    statusbar_print(1, "cannot add file: rfl files not supported.");
    ok = FALSE;
    goto out;
  }

  if(!strcmp(mime_string, "audio/x-flac")) sample->filetype = TYPE_FLAC;
  printf("add_file() mimetype: %s type=%i\n", mime_string, sample->filetype);

  if(!strcmp(mime_type->media_type, "text")){
    printf("ignoring text file...\n");
    ok = FALSE;
    goto out;
  }

  if(!strcmp(mime_string, "audio/mpeg")){
    printf("cannot add file: file type \"%s\" not supported.\n", mime_string);
    statusbar_print(1, "cannot add file: mpeg files not supported.");
    ok = FALSE;
    goto out;
  }
  if(!strcmp(mime_string, "application/x-par2")){
    printf("cannot add file: file type \"%s\" not supported.\n", mime_string);
    statusbar_print(1, "cannot add file: mpeg files not supported.");
    ok = FALSE;
    goto out;
  }

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
		//printf("add_file(): rowref=%p\n", sample->row_ref);
		gtk_tree_path_free(treepath);
	}else errprintf("add_file(): failed to make treepath from inserted iter.\n");

	//printf("add_file(): iter=%p=%s\n", &iter, gtk_tree_model_get_string_from_iter(GTK_TREE_MODEL(app.store), &iter));
    if(debug) printf("setting store... filename=%s mime=%s\n", filename, mime_string);
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

    if(debug) printf("sending message: sample=%p filename=%s (%p)\n", sample, sample->filename, sample->filename);
	g_async_queue_push(msg_queue, sample); //notify the overview thread.

	on_directory_list_changed();

  }else{
    //strcpy(sql2, sql);
    ok = FALSE;
  }

out:
  g_free(filedir);
  g_free(filename);
  return ok;
}


gboolean
add_dir(char *uri)
{
  printf("add_dir(): dir=%s\n", uri);

  //GDir* dir = g_dir_open(const gchar *path, guint flags, GError **error);

  return FALSE;
}


gboolean
get_file_info(sample* sample)
{
  if(sample->filetype==TYPE_FLAC) return get_file_info_flac(sample);
  else                            return get_file_info_sndfile(sample);
}


gboolean
get_file_info_sndfile(sample* sample)
{
	char *filename = sample->filename;

	SF_INFO        sfinfo;   //the libsndfile struct pointer
	SNDFILE        *sffile;
	sfinfo.format  = 0;

	if(!(sffile = sf_open(filename, SFM_READ, &sfinfo))){
		printf("get_file_info_sndfile(): not able to open input file %s.\n", filename);
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

	if(sample->channels<1 || sample->channels>100){ printf("get_file_info_sndfile(): bad channel count: %i\n", sample->channels); return FALSE; }
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

	GtkTreePath* treepath;
	if((treepath = gtk_tree_row_reference_get_path(sample->row_ref))){ //it's possible the row may no longer be visible.
		GtkTreeIter iter;
		if(gtk_tree_model_get_iter(GTK_TREE_MODEL(app.store), &iter, treepath)){
			//printf("on_overview_done(): setting pixbuf in store... pixbuf=%p\n", sample->pixbuf);
			if(GDK_IS_PIXBUF(sample->pixbuf)){
				gtk_list_store_set(app.store, &iter, COL_OVERVIEW, sample->pixbuf, -1);
			}else errprintf("on_overview_done(): pixbuf is not a pixbuf.\n");

		}else errprintf("on_overview_done(): failed to get row iter. row_ref=%p\n", sample->row_ref);
		gtk_tree_path_free(treepath);
	}

	sample_free(sample);
	return FALSE; //source should be removed.
}


void
db_update_pixbuf(sample *sample)
{
	if(debug) printf("db_update_pixbuf()...\n");

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
	//the keywords column has been edited. Update the database to reflect the new text.

	printf("cell_edited_callback()...\n");
	GtkTreeIter iter;
	int idx;
	gchar *filename;
	GtkTreeModel* store = GTK_TREE_MODEL(app.store);
	GtkTreePath* path = gtk_tree_path_new_from_string(path_string);
	gtk_tree_model_get_iter(store, &iter, path);
	gtk_tree_model_get(store, &iter, COL_IDX, &idx,  COL_NAME, &filename, -1);
	printf("cell_edited_callback(): filename=%s\n", filename);

	//convert to lowercase:
	//gchar* lower = g_ascii_strdown(new_text, -1);
	//g_free(lower);

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

	GList* selected_row_refs = NULL;
	GtkTreeRowReference* row_ref = NULL;

	//get row refs for each selected row:
	int i;
	for(i=0;i<g_list_length(selectionlist);i++){
		GtkTreePath *treepath_selection = g_list_nth_data(selectionlist, i);

		row_ref = gtk_tree_row_reference_new(GTK_TREE_MODEL(app.store), treepath_selection);
		selected_row_refs = g_list_prepend(selected_row_refs, row_ref);
	}
	g_list_free(selectionlist);


	GtkTreePath* path;
	for(i=0;i<g_list_length(selected_row_refs);i++){
		row_ref = g_list_nth_data(selected_row_refs, i);
		if((path = gtk_tree_row_reference_get_path(row_ref))){

			if(gtk_tree_model_get_iter(model, &iter, path)){
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
		} else errprintf("delete_row(): cannot get path from row_ref!\n");
	}
	g_list_free(selected_row_refs); //FIXME free the row_refs?

	on_directory_list_changed();
}


void
update_row(GtkWidget *widget, gpointer user_data)
{
	//sync the catalogue row with the filesystem.

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

	//-------

	//separator
	menu_item = gtk_separator_menu_item_new();
	gtk_menu_shell_append (GTK_MENU_SHELL(menu), menu_item);
	gtk_widget_show (menu_item);

	//-------

	//tick image:
	/*
	//GtkIconFactory* icon_factory = gtk_icon_factory_new();
	GtkIconSize image_size = GTK_ICON_SIZE_MENU;
	gchar *stock_id = GTK_STOCK_APPLY;
	GtkWidget* image = gtk_image_new_from_stock(stock_id, image_size);

	//#define GTK_STOCK_APPLY            "gtk-apply"

	//menu_item = gtk_menu_item_new_with_label ("Recursive");
    menu_item = gtk_image_menu_item_new_with_label("Recursive");

    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item), image);

	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(toggle_recursive_add), NULL);
	gtk_widget_show(menu_item);
	*/


	menu_item = gtk_check_menu_item_new_with_mnemonic("Add Recursively");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	gtk_widget_show(menu_item);
	gtk_check_menu_item_set_active(menu_item, app.add_recursive);
	g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(toggle_recursive_add), NULL);

	//-------

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
			inspector_update(path);

			GdkRectangle rect;
			gtk_tree_view_get_cell_area(treeview, path, app.col_pixbuf, &rect);
			if(((gint)event->x > rect.x) && ((gint)event->x < (rect.x + rect.width))){

				//overview column:
				printf("on_row_clicked() column rect: %i %i %i %i\n", rect.x, rect.y, rect.width, rect.height);

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

				if(sample->id != app.playing_id) playback_init(sample);
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

	printf("treeview_on_motion(): setting rowref=%p\n", prev_row_ref);
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


void
on_entry_activate(GtkEntry *entry, gpointer user_data)
{
	printf("on_entry_activate(): entry activated!\n");
}


gboolean
keyword_is_dupe(char* new, char* existing)
{
	//return true if the word 'new' is contained in the 'existing' string.

	//FIXME make case insensitive.
	//gint g_ascii_strncasecmp(const gchar *s1, const gchar *s2, gsize n);
	//gchar*      g_utf8_casefold(const gchar *str, gssize len);

	gboolean found = FALSE;

	//split the existing keyword string into separate words.
	gchar** split = g_strsplit(existing, " ", 0);
	int word_index = 0;
	while(split[word_index]){
		if(!strcmp(split[word_index], new)){
			printf("keyword_is_dupe(): '%s' already in string '%s'\n", new, existing);
			found = TRUE;
			break;
		}
		word_index++;
	}

	g_strfreev(split);
	return found;
}


int
colour_drag_dataget(GtkWidget *widget, GdkDragContext *drag_context,
                    GtkSelectionData *data,
                    guint info,
                    guint time,
                    gpointer user_data)
{
  char text[16];
  gboolean data_sent = FALSE;
  printf("colour_drag_dataget()!\n");

  int box_num = GPOINTER_TO_UINT(user_data); //box_num corresponds to the colour index.

  //convert to a pseudo uri string:
  sprintf(text, "colour:%i", box_num + 1);//1 based to avoid atoi problems.
  
  gtk_selection_data_set(data,	
						GDK_SELECTION_TYPE_STRING,
						8,	/* 8 bits per character. */
						text, strlen(text)
						);
  data_sent = TRUE;

  return FALSE;
}

/*
int
colour_drag_datareceived(GtkWidget *widget, GdkDragContext *drag_context,
                    gint x, gint y, GtkSelectionData *data,
                    guint info,
                    guint time,
                    gpointer user_data)
{
  printf("colour_drag_colour_drag_datareceived()!\n");

  return FALSE;
}
*/

gint
treeview_drag_received(GtkWidget *widget, GdkDragContext *drag_context, gint x, gint y, GtkSelectionData *data, guint info, guint time, gpointer user_data)
{
  printf("treeview_drag_datareceived()!\n");
  return FALSE;
}


int
path_get_id(GtkTreePath* path)
{
	printf("path_get_id()...\n");

	int id;
	GtkTreeIter iter;
	gchar *filename;
	GtkTreeModel* store = GTK_TREE_MODEL(app.store);
	gtk_tree_model_get_iter(store, &iter, path);
	gtk_tree_model_get(store, &iter, COL_IDX, &id,  COL_NAME, &filename, -1);

	printf("cell_edited_callback(): filename=%s\n", filename);
	return id;
}


void
tag_edit_start(int tnum)
{
  /*
  initiate the inplace renaming of a tag label in the inspector following a double-click.

  -rename widget has had an extra ref added at creation time, so we dont have to do it here.

  -current strategy is too swap the non-editable gtklabel with the editable gtkentry widget.
  -fn is fragile in that it makes too many assumptions about the widget layout (widget parent).

  FIXME global keyboard shortcuts are still active during editing?
  FIXME add ESC key to stop editing.
  */

  printf("tag_edit_start()...\n");

  static gulong handler1 =0; // the edit box "focus_out" handler.
  static gulong handler2 =0; // the edit box RETURN key trap.
  
  GtkWidget* parent = app.inspector->tags_ev;
  GtkWidget* edit   = app.inspector->edit;
  GtkWidget *label = app.inspector->tags;

  if(!GTK_IS_WIDGET(app.inspector->edit)){ errprintf("tag_edit_start(): edit widget missing.\n"); return;}
  if(!GTK_IS_WIDGET(label))              { errprintf("tags_edit_start(): label widget is missing.\n"); return;}

  //show the rename widget:
  gtk_entry_set_text(GTK_ENTRY(app.inspector->edit), gtk_label_get_text(GTK_LABEL(label)));
  gtk_widget_ref(label);//stops gtk deleting the widget.
  gtk_container_remove(GTK_CONTAINER(parent), label);
  gtk_container_add   (GTK_CONTAINER(parent), app.inspector->edit);

  //our focus-out could be improved by properly focussing other widgets when clicked on (widgets that dont catch events?).

  if(handler1) g_signal_handler_disconnect((gpointer)edit, handler1);
  
  handler1 = g_signal_connect((gpointer)app.inspector->edit, "focus-out-event", G_CALLBACK(tag_edit_stop), GINT_TO_POINTER(0));

  /*
  //this signal is no good as it quits when you move the mouse:
  //g_signal_connect ((gpointer)arrange->wrename, "leave-notify-event", G_CALLBACK(track_label_edit_stop), 
  //								GINT_TO_POINTER(tnum));
  
  if(handler2) g_signal_handler_disconnect((gpointer)arrange->wrename, handler2);
  handler2 =   g_signal_connect(G_OBJECT(arrange->wrename), "key-press-event", 
  								G_CALLBACK(track_entry_key_press), GINT_TO_POINTER(tnum));//traps the RET key.
  */

  //grab_focus allows us to start typing imediately. It also selects the whole string.
  gtk_widget_grab_focus(GTK_WIDGET(app.inspector->edit));
}


void
tag_edit_stop(GtkWidget *widget, GdkEventCrossing *event, gpointer user_data)
{
  /*
  tidy up widgets and notify core of label change.
  called following a focus-out-event for the gtkEntry renaming widget.
  -track number should correspond to the one being edited...???

  warning! if called incorrectly, this fn can cause mysterious segfaults!!
  Is it something to do with multiple focus-out events? This function causes the
  rename widget to lose focus, it can run a 2nd time before completing the first: segfault!
  Currently it works ok if focus is removed before calling.
  */

  //static gboolean busy = FALSE;
  //if(busy){"track_label_edit_stop(): busy!\n"; return;} //only run once at a time!
  //busy = TRUE;

  GtkWidget* edit = app.inspector->edit;
  GtkWidget* parent = app.inspector->tags_ev;

  //g_signal_handler_disconnect((gpointer)edit, handler_id);
  
  //printf("track_label_edit_stop(): wrename_parent=%p.\n", arrange->wrename->parent);
  //printf("track_label_edit_stop(): label_parent  =%p.\n", trkA[tnum]->wlabel);
  
  //printf("w:%p is_widget: %i\n", trkA[tnum]->wlabel, GTK_IS_WIDGET(trkA[tnum]->wlabel));

  //make sure the rename widget is being shown:
  /*
  if(edit->parent != parent){printf(
    "tags_edit_stop(): info: entry widget not in correct container! trk probably changed. parent=%p\n",
	   							arrange->wrename->parent); }
  */
	   
  //change the text in the label:
  gtk_label_set_text(GTK_LABEL(app.inspector->tags), gtk_entry_get_text(GTK_ENTRY(edit)));
  //printf("track_label_edit_stop(): text set.\n");

  //update the data:
  //update the string for the channel that the current track is using:
  //int ch = track_get_ch_idx(tnum);
  row_set_tags_from_id(app.inspector->row_id, app.inspector->row_ref, gtk_entry_get_text(GTK_ENTRY(edit)));

  //swap back to the normal label:
  gtk_container_remove(GTK_CONTAINER(parent), edit);
  //gtk_container_remove(GTK_CONTAINER(arrange->wrename->parent), arrange->wrename);
  //printf("track_label_edit_stop(): wrename unparented.\n");
  
  gtk_container_add(GTK_CONTAINER(parent), app.inspector->tags);
  gtk_widget_unref(app.inspector->tags); //remove 'artificial' ref added in edit_start.
  
  printf("tags_edit_stop(): finished.\n");
  //busy = FALSE;
}


gboolean
on_directory_list_changed()
{
  /*
  the database has been modified, the directory list may have changed.
  Update all directory views. Atm there is only one.
  */

  if(debug) printf("on_directory_list_changed()...\n");

  g_idle_add(dir_tree_update, NULL);
  return FALSE;
}


gboolean
toggle_recursive_add(GtkWidget *widget, gpointer user_data)
{
  printf("toggle_recursive_add()...\n");
  //if(app.config.add_recursive) app.config.add_recursive = false; else app.config.add_recursive = true;
  if(app.add_recursive) app.add_recursive = false; else app.add_recursive = true;
  return FALSE;
}


gboolean
config_load()
{
	snprintf(app.config.database_name, 64, "samplelib");
	strcpy(app.config.show_dir, "");

	//currently these are overridden anyway
	snprintf(app.config.colour[0], 7, "%s", "000000");
	snprintf(app.config.colour[1], 7, "%s", "000000");
	snprintf(app.config.colour[2], 7, "%s", "000000");
	snprintf(app.config.colour[3], 7, "%s", "000000");
	snprintf(app.config.colour[4], 7, "%s", "000000");
	snprintf(app.config.colour[5], 7, "%s", "000000");
	snprintf(app.config.colour[6], 7, "%s", "000000");
	snprintf(app.config.colour[7], 7, "%s", "000000");
	snprintf(app.config.colour[8], 7, "%s", "000000");
	snprintf(app.config.colour[9], 7, "%s", "000000");
	snprintf(app.config.colour[10],7, "%s", "000000");
	snprintf(app.config.colour[11],7, "%s", "000000");

	GError *error = NULL;
	app.key_file = g_key_file_new();
	if(g_key_file_load_from_file(app.key_file, app.config_filename, G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, &error)){
		printf("ini file loaded.\n");
		gchar* groupname = g_key_file_get_start_group(app.key_file);
		printf("config_load(): group=%s.\n", groupname);
		if(!strcmp(groupname, "Samplecat")){
			/*
			gsize length;
			gchar** keys = g_key_file_get_keys(app.key_file, "Samplecat", &length, &error);
			*/

			//FIXME 64 is not long enough for directories
			char keys[6][64] = {"database_host",          "database_user",          "database_pass",          "show_dir",          "window_height",          "window_width"};
			char *loc[6]     = {app.config.database_host, app.config.database_user, app.config.database_pass, app.config.show_dir, app.config.window_height, app.config.window_width};

			int k;
			gchar* keyname;
			for(k=0;k<6;k++){
				if((keyname = g_key_file_get_string(app.key_file, groupname, keys[k], &error))){
					snprintf(loc[k], 64, "%s", keyname);
					g_free(keyname);
				}else strcpy(loc[k], "");
			}
			set_search_dir(app.config.show_dir);
		}
		else{ warnprintf("config_load(): cannot find Samplecat key group.\n"); return FALSE; }
		g_free(groupname);
	}else{
		printf("unable to load config file: %s\n", error->message);
		g_error_free(error);
		error = NULL;
		config_new();
		return FALSE;
	}

	return TRUE;
}


gboolean
config_save()
{
	//update the search directory:
	g_key_file_set_value(app.key_file, "Samplecat", "show_dir", app.search_dir);

	//save window dimensions:
	gint width, height;
	char value[256];
	gtk_window_get_size(GTK_WINDOW(app.window), &width, &height);
	snprintf(value, 256, "%i", width);
	g_key_file_set_value(app.key_file, "Samplecat", "window_width", value);
	snprintf(value, 256, "%i", height);
	g_key_file_set_value(app.key_file, "Samplecat", "window_height", value);

	GError *error = NULL;
	gsize length;
	gchar* string = g_key_file_to_data(app.key_file, &length, &error);
	if(error){
		printf("on_quit(): error saving config file: %s\n", error->message);
		g_error_free(error);
	}
	//printf("string=%s\n", string);

	FILE* fp;
	if(!(fp = fopen(app.config_filename, "w"))){
		errprintf("cannot open config file for writing (%s).\n", app.config_filename);
		return FALSE;
	}
	if(fprintf(fp, "%s", string) < 0){
		errprintf("error writing data to config file (%s).\n", app.config_filename);
	}
	fclose(fp);
	g_free(string);
	return TRUE;
}


void
config_new()
{
	printf("config_new()...\n");

	//g_key_file_has_group(GKeyFile *key_file, const gchar *group_name);

	GError *error = NULL;
	char data[256 * 256];
	sprintf(data, "#this is the default config file.\n#pls enter the database details.\n"
		"[Samplecat]\n"
		"database_host=localhost\n"
		"database_user=username\n"
		"database_pass=pass\n"
		"database_name=samplelib\n"
		);

	if(!g_key_file_load_from_data(app.key_file, data, strlen(data), G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, &error)){
		errprintf("config_new() error creating new key_file from data. %s\n", error->message);
		g_error_free(error);
		error = NULL;
		return;
	}

	printf("a default config file has been created. Please enter your database details.\n");
}


void
on_quit(GtkMenuItem *menuitem, gpointer user_data)
{
    if(debug) printf("on_quit()...\n");

	jack_close();

	config_save();

	clear_store(); //does this make a difference to valgrind? no.

	MYSQL *mysql;
	mysql=&app.mysql;
  	mysql_close(mysql);

	gtk_main_quit();
    if(debug) printf("on_quit(): done.\n");
    exit(GPOINTER_TO_INT(user_data));
}


