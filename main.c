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
#include "main.h"
#include "support.h"
#include "audio.h"

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


int 
main(int argc, char* *argv)
{
	gtk_init(&argc, &argv);
	type_init();
	pixmaps_init();

	//init console escape commands:
	sprintf(white,  "%c[0;39m", 0x1b);
	sprintf(red,    "%c[1;31m", 0x1b);
	sprintf(green,  "%c[1;32m", 0x1b);
	sprintf(yellow, "%c[1;33m", 0x1b);
	sprintf(bold,   "%c[1;39m", 0x1b);
	sprintf(err,    "%serror!%s", red, white);
	sprintf(warn,   "%swarning:%s", yellow, white);

	//app.fg_colour.pixel = 0;

	printf("%s%s. Version %s%s\n", yellow, app_name, app_version, white);

	if (!gnome_vfs_init()){ errprintf("could not initialize GnomeVFS.\n"); return 1; }

	/*
	Calls:
	MYSQL *mysql_init(MYSQL *mysql)
	char *mysql_get_server_info(MYSQL *mysql)
	void mysql_close(MYSQL *mysql)
	*/
 
	scan_dir();

	app.store = gtk_list_store_new(NUM_COLS, GDK_TYPE_PIXBUF, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT, G_TYPE_STRING);
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

		do_search("");
	}
	//how many rows are in the list?

 
	GtkWidget *view = app.view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(app.store));
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

	GtkCellRenderer *cell3 = gtk_cell_renderer_text_new();
	GtkTreeViewColumn *column3 = gtk_tree_view_column_new_with_attributes("Keywords", cell3, "text", COL_KEYWORDS, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), column3);
	gtk_tree_view_column_set_sort_column_id(column3, COL_KEYWORDS);
	gtk_tree_view_column_set_resizable(column3, TRUE);
	gtk_tree_view_column_set_reorderable(column3, TRUE);
	gtk_tree_view_column_set_min_width(column3, 0);
	g_object_set(cell3, "editable", TRUE, NULL);
	g_signal_connect(cell3, "edited", (GCallback) keywords_on_edited, NULL);
	//g_object_set(cell3, "ypad", 0, NULL);

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

	gtk_widget_show(view);

	window_new(); 
	filter_new();
 
	//gtk_box_pack_start(GTK_BOX(app.vbox), view, EXPAND_TRUE, FILL_TRUE, 0);
	gtk_container_add(GTK_CONTAINER(app.scroll), view);

	app.context_menu = make_context_menu();
	g_signal_connect((gpointer)view, "button-press-event", G_CALLBACK(on_row_clicked), NULL);

	gtk_main();
	exit(0);
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
do_search(char *search)
{
	unsigned int i, samplerate, channels;
	char length[64];
	GtkTreeIter iter;

	MYSQL_RES *result;
	MYSQL_ROW row;
	unsigned long *lengths;
	unsigned int num_fields;
	char query[1024];
	char where[512]="";
	GdkPixbuf* pixbuf  = NULL;
	GdkPixbuf* iconbuf = NULL;
	GdkPixdata pixdata;
	gboolean online = FALSE;

	MYSQL *mysql;
	mysql=&app.mysql;

	//strmov(query_def, "SELECT * FROM samples"); //where is this defined?
	strcpy(query, "SELECT * FROM samples ");
	if(strlen(search)){ 
		snprintf(where, 512, "WHERE filename LIKE '%%%s%%' OR filedir LIKE '%%%s%%' OR keywords LIKE '%%%s%%'", search, search, search);
		snprintf(query, 1024, "SELECT * FROM samples  %s", where);
		printf("%s\n", query);
	}

	if(mysql_exec_sql(mysql, query)==0){ //success
		
		//problem with wierd mysql int type:
		//printf( "%ld Records Found\n", (long) mysql_affected_rows(&mysql));
		//printf( "%lu Records Found\n", (unsigned long)mysql_affected_rows(mysql));
	
		//FIXME clear the treeview.
		gtk_list_store_clear(app.store);

		result = mysql_store_result(mysql);
		if(result){// there are rows
			num_fields = mysql_num_fields(result);
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
				//if(row[5]==NULL) length     = 0; else length     = atoi(row[5]);
				if(row[6]==NULL) samplerate = 0; else samplerate = atoi(row[6]);
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
				gtk_list_store_set(app.store, &iter, COL_ICON, iconbuf, COL_IDX, atoi(row[0]), COL_NAME, row[1], COL_FNAME, row[2], COL_KEYWORDS, row[3], 
                                                     COL_OVERVIEW, pixbuf, COL_LENGTH, length, COL_SAMPLERATE, samplerate, COL_CHANNELS, channels, 
													 COL_MIMETYPE, row[MYSQL_MIMETYPE], -1);
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
		printf( "Failed to find any records and caused an error: %s\n", mysql_error(mysql));
	}
}


gboolean
filter_new()
{
	//search box

	if(!app.window) return FALSE; //FIXME make this a macro with printf etc

	GtkWidget* hbox = gtk_hbox_new(FALSE, 0);
    gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(app.vbox), hbox, EXPAND_FALSE, FILL_FALSE, 0);

	GtkWidget* label = gtk_label_new("Search");
	gtk_misc_set_padding(GTK_MISC(label), 5,5);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	
	GtkWidget *entry = app.search = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(entry), 64);
	gtk_entry_set_text(GTK_ENTRY(entry), "textEntry");
	gtk_widget_show(entry);	
	gtk_box_pack_start(GTK_BOX(hbox), entry, EXPAND_TRUE, TRUE, 0);
	g_signal_connect(G_OBJECT(entry), "focus-out-event", G_CALLBACK(new_search), NULL);
	
	return TRUE;
}


gboolean
window_new()
{
/*
window
+--vbox
   +--search box
   |  +--label
   |  +--text entry
   |
   +--scrollwin
   |  +--treeview
   |
   +--statusbar hbox
      +--statusbar	

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

	GtkWidget *scroll = app.scroll = gtk_scrolled_window_new(NULL, NULL);  //adjustments created automatically.
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_widget_show(scroll);
	gtk_box_pack_end(GTK_BOX(vbox), scroll, EXPAND_TRUE, TRUE, 0);

	GtkWidget *statusbar = app.statusbar = gtk_statusbar_new();
	//gtk_statusbar_set_has_resize_grip(GTK_STATUSBAR(statusbar), TRUE);	//why does give a warning??????
	gtk_statusbar_set_has_resize_grip(GTK_STATUSBAR(statusbar), FALSE);
	gtk_box_pack_start(GTK_BOX(hbox_statusbar), statusbar, EXPAND_TRUE, FILL_TRUE, 0);
	gtk_widget_show(statusbar);

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


gboolean
db_connect()
{
	MYSQL *mysql;

	if(mysql_init(&(app.mysql))==NULL){
		printf("Failed to initate MySQL connection.\n");
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
 

gboolean
db_insert(char *qry)
{
	MYSQL *mysql = &app.mysql;

	if(mysql_exec_sql(mysql, qry)==0)/*success*/{
		printf("db_insert(): ok!\n");
	}else{
		printf("db_insert(): not ok...\n");
		return FALSE;
	}
	return TRUE;
}


gboolean
new_search(GtkWidget *widget, gpointer userdata)
{
	//the search box focus-out signal ocurred.
	//printf("new search...\n");

	const gchar* text = gtk_entry_get_text(GTK_ENTRY(app.search));
	
	do_search((gchar*)text);
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


void
add_file(char *uri)
{
  printf("add_file()... %s\n", uri);
  char* uri_unescaped = gnome_vfs_unescape_string(uri, NULL);

  sample sample;
  //sample.filename = uri;
  snprintf(sample.filename, 256, "%s", uri_unescaped);

  #define SQL_LEN 66000
  char sql[1024], sql2[SQL_LEN];
  char length_ms[64];
  guint8 blob[SQL_LEN];
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

  GdkPixbuf* pixbuf = make_overview(&sample);
  if(pixbuf){

	//serialise the pixbuf:
	GdkPixdata pixdata;
	gdk_pixdata_from_pixbuf(&pixdata, pixbuf, 0);
	guint length;
	guint8* ser = gdk_pixdata_serialize(&pixdata, &length);
	mysql_real_escape_string(&app.mysql, blob, ser, length);
	printf("add_file() serial length: %i, strlen: %i\n", length, strlen(ser));

	snprintf(sql2, SQL_LEN, "%s, pixbuf='%s', length=%i, sample_rate=%i, channels=%i, mimetype='%s/%s' ", sql, blob, sample.length, sample.sample_rate, sample.channels, mime_type->media_type, mime_type->subtype);
	format_time_int(length_ms, sample.length);

    GtkTreeIter iter;
    gtk_list_store_append(app.store, &iter);
    gtk_list_store_set(app.store, &iter,
                      COL_NAME, filename,
                      COL_FNAME, filedir,
                      COL_OVERVIEW, pixbuf,
	                  COL_LENGTH, length_ms,
                      COL_SAMPLERATE, sample.sample_rate,
	                  COL_CHANNELS, sample.channels,
	                  COL_MIMETYPE, mime_string,
                      -1);

    //at this pt, refcount should be two, we make it 1 so that pixbuf is destroyed with the row:
    g_object_unref(pixbuf);
	free(ser);

    printf("add_file(): sql=%s\n", sql2);
    db_insert(sql2);

  }// else strcpy(sql2, sql);

  g_free(filedir);
  g_free(filename);
  g_free(uri_unescaped);
}


GdkPixbuf*
make_overview(sample* sample)
{
  /*
  we need to load a file onto a pixbuf.

  specimen: 
		-loads complete file into ram. (true)
		-makes a widget not a bitmap.
	probably not much we can use....

  */

  //can we get sf to give us a mono file? no

  #define OVERVIEW_WIDTH 150
  #define OVERVIEW_HEIGHT 20

  char *filename = sample->filename;

  SF_INFO        sfinfo;   //the libsndfile struct pointer
  SNDFILE        *sffile;
  sfinfo.format  = 0;
  int            readcount;

  if(!(sffile = sf_open(filename, SFM_READ, &sfinfo))){
    printf("make_overview(): not able to open input file %s.\n", filename);
    puts(sf_strerror(NULL));    // print the error message from libsndfile:
	return NULL;
  }

  char chanwidstr[64];
  if     (sfinfo.channels==1) snprintf(chanwidstr, 64, "mono");
  else if(sfinfo.channels==2) snprintf(chanwidstr, 64, "stereo");
  else                        snprintf(chanwidstr, 64, "channels=%i", sfinfo.channels);
  printf("%iHz %s frames=%i\n", sfinfo.samplerate, chanwidstr, (int)sfinfo.frames);
  sample->channels    = sfinfo.channels;
  sample->sample_rate = sfinfo.samplerate;
  sample->length      = (sfinfo.frames * 1000) / sfinfo.samplerate;

  GdkPixbuf* pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB,
                                        FALSE,  //gboolean has_alpha
                                        8,      //int bits_per_sample
                                        OVERVIEW_WIDTH, OVERVIEW_HEIGHT);  //int width, int height)
  pixbuf_clear(pixbuf, &app.fg_colour);
  /*
  GdkColor colour;
  colour.red   = 0xffff;
  colour.green = 0x0000;
  colour.blue  = 0x0000;
  */
  //how many samples should we load at a time? Lets make it a multiple of the image width.
  //-this will use up a lot of ram for a large file, 600M file will use 4MB.
  int frames_per_buf = sfinfo.frames / OVERVIEW_WIDTH;
  int buffer_len = frames_per_buf * sfinfo.channels; //may be a small remainder?
  printf("make_overview(): buffer_len=%i\n", buffer_len);
  short *data = malloc(sizeof(*data) * buffer_len);

  int x=0, frame, ch;
  int srcidx_start=0;             //index into the source buffer for each sample pt.
  int srcidx_stop =0;
  short min;                //negative peak value for each pixel.
  short max;                //positive peak value for each pixel.
  short sample_val;

  while ((readcount = sf_read_short(sffile, data, buffer_len))){
    //if(readcount < buffer_len) printf("EOF %i<%i\n", readcount, buffer_len);

    if(sf_error(sffile)) printf("%s read error\n", err);

    srcidx_start = 0;
    srcidx_stop  = frames_per_buf;

    min = 0; max = 0;
    for(frame=srcidx_start;frame<srcidx_stop;frame++){ //iterate over all the source samples for this pixel.
      for(ch=0;ch<sfinfo.channels;ch++){
        
        if(frame * sfinfo.channels + ch > buffer_len){ printf("index error!\n"); break; }    
        sample_val = data[frame * sfinfo.channels + ch];
        max = MAX(max, sample_val);
        min = MIN(min, sample_val);
      }
    }

    //scale the values to the part height:
    min = (min * OVERVIEW_HEIGHT) / (256*128*2);
    max = (max * OVERVIEW_HEIGHT) / (256*128*2);

    struct _ArtDRect pts = {x, OVERVIEW_HEIGHT/2 + min, x, OVERVIEW_HEIGHT/2 + max};
    pixbuf_draw_line(pixbuf, &pts, 1.0, &app.bg_colour);

    //printf(" %i max=%i\n", x,OVERVIEW_HEIGHT/2);
    x++;
  }  

  if(sf_close(sffile)) printf("error! bad file close.\n");
  //printf("end of loop...\n");
  free(data);
  return pixbuf;
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

		gtk_tree_model_get_iter(model, &iter, treepath_selection);
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
			if ( mime_type->image == NULL ) printf("do_search(): no icon.\n");
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


