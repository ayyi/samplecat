	/*
	create database samplelib;
	
	create table samples (
		id int(11) NOT NULL auto_increment,
		filename text NOT NULL,
		filedir text,
		keywords varchar(60) default '',
		PRIMARY KEY  (id)
	) TYPE=MyISAM;"
	*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <sndfile.h>
#include <gtk/gtk.h>

#include <libart_lgpl/libart.h>
#include <libgnomevfs/gnome-vfs.h>

#include "mysql/mysql.h"
#include "main.h"
#include "support.h"
//#include "gnome-vfs-uri.h"

struct _app app;

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
  COL_NAME = 0,
  COL_FNAME,
  COL_KEYWORDS,
  COL_OVERVIEW,
  NUM_COLS
};

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

	//init console escape commands:
	sprintf(white,  "%c[0;39m", 0x1b);
	sprintf(red,    "%c[1;31m", 0x1b);
	sprintf(green,  "%c[1;32m", 0x1b);
	sprintf(yellow, "%c[1;33m", 0x1b);
	sprintf(bold,   "%c[1;39m", 0x1b);
	sprintf(err,    "%serror!%s", red, white);
	sprintf(warn,   "%swarning:%s", yellow, white);

	printf("%s%s. Version %s%s\n", yellow, app_name, app_version, white);

	if (!gnome_vfs_init()){ errprintf("could not initialize GnomeVFS.\n"); return 1; }

	/*
	Calls:
	MYSQL *mysql_init(MYSQL *mysql)
	char *mysql_get_server_info(MYSQL *mysql)
	void mysql_close(MYSQL *mysql)
	*/
 
	scan_dir();

	MYSQL *mysql;

	app.store = gtk_list_store_new(NUM_COLS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, GDK_TYPE_PIXBUF);
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

	if(db_connect()){
		mysql = &app.mysql;

		do_search("");
	}
	//how many rows are in the list?

 
	GtkWidget *view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(app.store));

	GtkTreeViewColumn *column;
	GtkCellRenderer* cell1 = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Sample Name", cell1, "text", COL_NAME, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);
	gtk_tree_view_column_set_sort_column_id(column, COL_NAME);
	
	GtkCellRenderer *cell2 = gtk_cell_renderer_text_new();
	GtkTreeViewColumn *column2 = gtk_tree_view_column_new_with_attributes("File", cell2, "text", COL_FNAME, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), column2);
	gtk_tree_view_column_set_sort_column_id(column2, COL_FNAME);

	GtkCellRenderer *cell3 = gtk_cell_renderer_text_new();
	GtkTreeViewColumn *column3 = gtk_tree_view_column_new_with_attributes("Keywords", cell3, "text", COL_KEYWORDS, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), column3);
	gtk_tree_view_column_set_sort_column_id(column3, COL_KEYWORDS);

	GtkCellRenderer *cell4 = gtk_cell_renderer_pixbuf_new();
	GtkTreeViewColumn *col4 = gtk_tree_view_column_new_with_attributes("Overview", cell4, "pixbuf", COL_OVERVIEW, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col4);
	//g_object_set(cell4,   "cell-background", "Orange",     "cell-background-set", TRUE,  NULL);
	g_object_set(G_OBJECT(cell4), "xalign", 0.0, NULL);

	window_new(); 
	filter_new();
 
	gtk_box_pack_start(GTK_BOX(app.vbox), view, EXPAND_TRUE, FILL_TRUE, 0);
	gtk_widget_show(view);

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
				printf("scan_dir(): checking files: %s\n", file);
				//dssi_plugin_add(filepath, (char*)file);
			}
		}
		g_dir_close(dir);
	}
	
}


void
do_search(char *search)
{
	unsigned int i;
	GtkTreeIter iter;

	MYSQL_RES *result;
	MYSQL_ROW row;
	unsigned int num_fields;
	char query[1024];
	char where[512]="";

	MYSQL *mysql;
	mysql=&app.mysql;

	//strmov(query_def, "SELECT * FROM samples"); //where is this defined?
	strcpy(query, "SELECT * FROM samples ");
	if(strlen(search)){ 
		snprintf(where, 512, "WHERE filename LIKE '%%%s%%' OR filedir LIKE '%%%s%%' OR keywords LIKE '%%%s%%'", search, search, search);
		snprintf(query, 1024, "SELECT * FROM samples  %s", where);
		printf("%s\n", query);
	}

	if(mysql_exec_sql(mysql, query)==0)/*success*/{
		
		//problem with wierd mysql int type:
		//printf( "%ld Records Found\n", (long) mysql_affected_rows(&mysql));
		//printf( "%lu Records Found\n", (unsigned long)mysql_affected_rows(mysql));
	
		//FIXME clear the treeview.
		gtk_list_store_clear(app.store);

		
		result = mysql_store_result(mysql);
		if(result){// there are rows
			num_fields = mysql_num_fields(result);
			while((row = mysql_fetch_row(result))){ 
				for(i = 0; i < num_fields; i++){ 
					//printf("[%.*s] ", (int) lengths[i], row[i] ? row[i] : "NULL"); 
					printf("[%s] ", row[i] ? row[i] : "NULL"); 
				}
				gtk_list_store_append(app.store, &iter); 
				gtk_list_store_set(app.store, &iter, COL_NAME, row[1], COL_FNAME, row[2], COL_KEYWORDS, row[3], -1);
				printf("\n"); 
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
   +--treeview
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

	GtkWidget *statusbar = app.statusbar = gtk_statusbar_new();
	//gtk_statusbar_set_has_resize_grip(GTK_STATUSBAR(statusbar), TRUE);	//why does give a warning??????
	gtk_statusbar_set_has_resize_grip(GTK_STATUSBAR(statusbar), FALSE);
	gtk_box_pack_start(GTK_BOX(hbox_statusbar), statusbar, EXPAND_TRUE, FILL_TRUE, 0);
	gtk_widget_show(statusbar);

	gtk_widget_show_all(window);

	dnd_setup();

	return TRUE;
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

  sample sample;
  sample.filename = uri;

  char sql[1024];
  //strcpy(sql, "INSERT samples SET ");
  gchar* filedir = g_path_get_dirname(uri);
  gchar* filename = g_path_get_basename(uri);
  snprintf(sql, 1024, "INSERT samples SET filename='%s', filedir='%s'", filename, filedir);

  GdkPixbuf* pixbuf = make_overview(&sample);
  if(pixbuf){
    GtkTreeIter iter;
    gtk_list_store_append(app.store, &iter);
    gtk_list_store_set(app.store, &iter,
                      COL_NAME, filename,
                      COL_FNAME, filedir,
                      COL_OVERVIEW, pixbuf,
                      -1);

    //at this pt, refcount should be two, we make it 1 so that pixbuf is destroyed with the row:
    g_object_unref(pixbuf);
  }

  printf("add_file(): sql=%s\n", sql);
  g_free(filedir);
  g_free(filename);
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
  }

  char chanwidstr[64];
  if     (sfinfo.channels==1) snprintf(chanwidstr, 64, "mono");
  else if(sfinfo.channels==2) snprintf(chanwidstr, 64, "stereo");
  else                        snprintf(chanwidstr, 64, "channels=%i", sfinfo.channels);
  printf("%iHz %s frames=%i\n", sfinfo.samplerate, chanwidstr, (int)sfinfo.frames);

  GdkPixbuf* pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB,
                                        FALSE,  //gboolean has_alpha
                                        8,      //int bits_per_sample
                                        OVERVIEW_WIDTH, OVERVIEW_HEIGHT);  //int width, int height)
  gdk_pixbuf_fill(pixbuf, 0x000000ff);

  GdkColor colour;
  colour.red   = 0xffff;
  colour.green = 0x0000;
  colour.blue  = 0x0000;

  //FIXME how many samples shoule we load at a time?
  //lets make it a multiple of the image width.
  //this will use up a lot of ram for a large file, 600M file will use 4MB.
  int frames_per_buf = sfinfo.frames / OVERVIEW_WIDTH;
  int buffer_len = frames_per_buf * sfinfo.channels; //may be a small remainder?
  printf("make_overview(): buffer_len=%i\n", buffer_len);
  short *data = malloc(sizeof(*data) * buffer_len);

  int x=0, frame, ch;
  int srcidx_start=0;             //index into the source buffer for each sample pt.
  int srcidx_stop =0;
  short min;                //negative peak value for each pixel.
  short max;                //positive peak value for each pixel.
  short sample;

  while ((readcount = sf_read_short(sffile, data, buffer_len))){
    //if(readcount < buffer_len) printf("EOF %i<%i\n", readcount, buffer_len);

    if(sf_error(sffile)) printf("%s read error\n", err);

    srcidx_start = 0;
    srcidx_stop  = frames_per_buf;

    min = 0; max = 0;
    for(frame=srcidx_start;frame<srcidx_stop;frame++){ //iterate over all the source samples for this pixel.
      for(ch=0;ch<sfinfo.channels;ch++){
        
        if(frame * sfinfo.channels + ch > buffer_len){ printf("index error!\n"); break; }    
        sample = data[frame * sfinfo.channels + ch];
        max = MAX(max, sample);
        //if(sample < min) min = sample;
        min = MIN(min, sample);
      }
    }

    //scale the values to the part height:
    min = (min * OVERVIEW_HEIGHT) / (256*128*2);
    max = (max * OVERVIEW_HEIGHT) / (256*128*2);

    struct _ArtDRect pts = {x, OVERVIEW_HEIGHT/2 + min, x, OVERVIEW_HEIGHT/2 + max};
    pixbuf_draw_line(pixbuf, &pts, 1.0, &colour);

    //printf(" %i max=%i\n", x,OVERVIEW_HEIGHT/2);
    x++;
  }  

  if(sf_close(sffile)) printf("error! bad file close.\n");
  //printf("end of loop...\n");
  free(data);
  return pixbuf;
}


