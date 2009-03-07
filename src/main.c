/*

Copyright (C) Tim Orford 2007-2008

This software is licensed under the GPL. See accompanying file COPYING.

*/
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#define __USE_GNU
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <sndfile.h>
#include <gtk/gtk.h>

#include <gdk-pixbuf/gdk-pixdata.h>
#ifdef OLD
  #include <libart_lgpl/libart.h>
#endif
#ifdef USE_AYYI
  #include "ayyi.h"
  #include "ayyi_model.h"
#endif
#ifdef USE_TRACKER
  #include "tracker.h"
  #include <src/tracker_.h>
#endif

#include "typedefs.h"
#include "mysql/mysql.h"
#include "dh-link.h"
#include "support.h"
#include "gqview_view_dir_tree.h"
#include "main.h"
#include "sample.h"
#include "audio.h"
#include "overview.h"
#include "cellrenderer_hypertext.h"
#include "tree.h"
#include "listview.h"
#include "window.h"
#include "inspector.h"
#include "db.h"
#include "dnd.h"
#ifdef USE_GQVIEW_1_DIRTREE
  #include "filelist.h"
  #include "view_dir_list.h"
  #include "view_dir_tree.h"
#endif

#include "rox/rox_global.h"
#include "mimetype.h"
#include "pixmaps.h"
#include "rox/dir.h"
#include "file_manager.h"

//#define DEBUG_NO_THREADS 1

//#if defined(HAVE_STPCPY) && !defined(HAVE_mit_thread)
  #define strmov(A,B) stpcpy((A),(B))
//#endif

void       dir_init();

struct _app app;
struct _palette palette;
GList* mime_types; // list of MIME_type*
extern GList* themes; 

unsigned debug = 0;

//strings for console output:
char white [16];
char red   [16];
char green [16];
char yellow[16];
char bold  [16];

//mysql table layout (database column numbers):
enum {
  MYSQL_NAME = 1,
  MYSQL_DIR = 2,
  MYSQL_KEYWORDS = 3,
  MYSQL_PIXBUF = 4
};
#define MYSQL_ONLINE 8
#define MYSQL_MIMETYPE 10
#define MYSQL_NOTES 11
#define MYSQL_COLOUR 12

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


void
app_init()
{
	app.loaded           = FALSE;
	//app.fg_colour.pixel = 0;
	app.dir_tree         = NULL;
	app.statusbar        = NULL;
	app.statusbar2       = NULL;
	app.search_phrase[0] = '\0';
	app.search_dir       = NULL;
	app.search_category  = NULL;
	app.window           = NULL;

	int i; for(i=0;i<PALETTE_SIZE;i++) app.colour_button[i] = NULL;
	app.colourbox_dirty = TRUE;

	snprintf(app.config_filename, 256, "%s/.samplecat", g_get_home_dir());
}


int 
main(int argc, char** argv)
{
#if 0
	//make gdb break on g errors:
	g_log_set_always_fatal( G_LOG_FLAG_RECURSION | G_LOG_FLAG_FATAL | G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING );
#endif

	//init console escape commands:
	sprintf(white,  "%c[0;39m", 0x1b);
	sprintf(red,    "%c[1;31m", 0x1b);
	sprintf(green,  "%c[1;32m", 0x1b);
	sprintf(yellow, "%c[1;33m", 0x1b);
	sprintf(bold,   "%c[1;39m", 0x1b);
	sprintf(err,    "%serror!%s", red, white);
	sprintf(warn,   "%swarning:%s", yellow, white);

	app_init();

	printf("%s"PACKAGE_NAME". Version "PACKAGE_VERSION"%s\n", yellow, white);

	config_load();

	g_thread_init(NULL);
	gdk_threads_init();
	app.mutex = g_mutex_new();
	gtk_init(&argc, &argv);

	if(!db_connect()){
	#ifdef QUIT_WITHOUT_DB
		on_quit(NULL, GINT_TO_POINTER(1));
	#endif
	}

	type_init();
	pixmaps_init();
	dir_init();

	file_manager__init();

#ifndef DEBUG_NO_THREADS
	dbg(3, "creating overview thread...");
	GError *error = NULL;
	msg_queue = g_async_queue_new();
	if(!g_thread_create(overview_thread, NULL, FALSE, &error)){
		errprintf("%s(): error creating thread: %s\n", error->message, __func__);
		g_error_free(error);
	}
#endif

	app.store = gtk_list_store_new(NUM_COLS, GDK_TYPE_PIXBUF,
	                               #ifdef USE_AYYI
	                               GDK_TYPE_PIXBUF,
	                               #endif
	                               G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);

	window_new(); 
 
	do_search(app.search_phrase, app.search_dir);

	app.context_menu = make_context_menu();

#ifdef USE_AYYI
	ayyi_client_init();
	ayyi_connect();
#endif

#ifdef USE_TRACKER
	//note: trackerd doesnt have to be running - it will be auto-started.
	//TODO dont do this until the gui has loaded.
	TrackerClient* tc = tracker_connect(TRUE);
	if(tc){
		tracker_search(tc);
		tracker_disconnect (tc);
	}
	else warnprintf("cant connect to tracker daemon.");
#endif

	app.loaded = TRUE;
	dbg(0, "loaded");

	gtk_main();
	exit(0);
}


gboolean
dir_tree_on_link_selected(GObject *ignored, DhLink *link, gpointer data)
{
	/*
	what does it mean if link->uri is empty?
	*/
	ASSERT_POINTER_FALSE(link, "link");

	dbg(0, "uri=%s", link->uri);
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
	dbg(0, "FIXME !! dir list window not updated.\n");

	GNode *node;
	node = app.dir_tree;

	db_get_dirs();

#ifdef NOT_BROKEN
	dh_book_tree_reload((DhBookTree*)app.dir_treeview);
#endif

	return FALSE; //source should be removed.
}


void
set_search_dir(char* dir)
{
	//this doesnt actually do the search. When calling, follow with do_search() if neccesary.

	//if string is empty, we show all directories?

	if(!dir){ perr("dir!\n"); return; }

	dbg (1, "dir=%s\n", dir);
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

	if(strlen(colour) != 7 ){ perr("bad colour string (%s) index=%u.\n", colour, colour_index); return; }

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
					dbg(0, "word=%i\n", word_index);

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
			dbg(0, "joined: %s\n", formatted);

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
scan_dir(const char* path, int* added_count)
{
	/*
	scan the directory and try and add any files we find.
	
	*/
	PF;

	char filepath[256];
	G_CONST_RETURN gchar *file;
	GError *error = NULL;
	GDir *dir;
	if((dir = g_dir_open(path, 0, &error))){
		while((file = g_dir_read_name(dir))){
			if(file[0]=='.') continue;
			snprintf(filepath, 128, "%s/%s", path, file);

			if(!g_file_test(filepath, G_FILE_TEST_IS_DIR)){
				add_file(filepath);
				statusbar_printf(1, "%i files added", ++*added_count);
			}
			// IS_DIR
			else if(app.add_recursive){
				scan_dir(filepath, added_count);
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

	if(!db_is_connected()){ listview__show_db_missing(); return; }

	if(!search) search = app.search_phrase;
	if(!dir)    dir    = app.search_dir;

	unsigned channels, colour;
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
	char sample_name[256];
	GdkPixbuf* pixbuf  = NULL;
	GdkPixbuf* iconbuf = NULL;
#ifdef USE_AYYI
	GdkPixbuf* ayyi_icon = NULL;
#endif
	GdkPixdata pixdata;
	gboolean online = FALSE;

	MYSQL *mysql = &app.mysql;

	//lets assume that the search_phrase filter should *always* be in effect.

	if(app.search_category) snprintf(category, 256, "AND keywords LIKE '%%%s%%' ", app.search_category);

	if(dir && strlen(dir)){
		snprintf(where_dir, 512, "AND filedir='%s' %s ", dir, category);
	}

	//strmov(dst, src) moves all the  characters  of  src to dst, and returns a pointer to the new closing NUL in dst. 
	//strmov(query_def, "SELECT * FROM samples"); //this is defined in mysql m_string.h
	//strcpy(query, "SELECT * FROM samples WHERE 1 ");

	if(strlen(search)){ 
		snprintf(where, 512, "AND (filename LIKE '%%%s%%' OR filedir LIKE '%%%s%%' OR keywords LIKE '%%%s%%') ", search, search, search); //FIXME duplicate category LIKE's
	}

	//append the dir-where part:
	char* a = where + strlen(where);
	strmov(a, where_dir);

	snprintf(query, 1024, "SELECT * FROM samples WHERE 1 %s", where);

	printf("%s\n", query);
	if(mysql_exec_sql(mysql, query)==0){ //success

		treeview_block_motion_handler(); //dont know exactly why but this prevents a segfault.

		clear_store();

		//detach the model from the view to speed up row inserts:
		/*
		model = gtk_tree_view_get_model(GTK_TREE_VIEW(view));
		g_object_ref(model);
		gtk_tree_view_set_model(GTK_TREE_VIEW(view), NULL);
		*/ 

		#define MAX_DISPLAY_ROWS 1000
		result = mysql_store_result(mysql);
		if(result){// there are rows
			num_fields = mysql_num_fields(result);
			char keywords[256];

			int row_count = 0;
			while((row = mysql_fetch_row(result)) && row_count < MAX_DISPLAY_ROWS){
				lengths = mysql_fetch_lengths(result); //free? 
				/*
				for(i = 0; i < num_fields; i++){ 
					printf("[%s] ", row[i] ? row[i] : "NULL"); 
				}
				printf("\n"); 
				*/

				//deserialise the pixbuf field:
				pixbuf = NULL;
				if(row[MYSQL_PIXBUF]){
					//printf("%s(): deserializing...\n", __func__); 
					if(gdk_pixdata_deserialize(&pixdata, lengths[4], (guint8*)row[MYSQL_PIXBUF], NULL)){
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

				strncpy(sample_name, row[MYSQL_NAME], 255);
				//TODO markup should be set in cellrenderer, not model!
#if 0
				if(GTK_WIDGET_REALIZED(app.view)){
					//check colours dont clash:
					long c_num = strtol(app.config.colour[colour], NULL, 16);
					dbg(2, "rowcolour=%s", app.config.colour[colour]);
					GdkColor row_colour; color_rgba_to_gdk(&row_colour, c_num << 8);
					if(is_similar(&row_colour, &app.fg_colour, 0x60)){
						snprintf(sample_name, 255, "%s%s%s", "<span foreground=\"blue\">", row[MYSQL_NAME], "</span>");
					}
				}
#endif

#ifdef USE_AYYI
				//is the file loaded in the current Ayyi song?
				if(ayyi.got_song){
					gchar* fullpath = g_build_filename(row[MYSQL_DIR], sample_name, NULL);
					if(pool__file_exists(fullpath)) dbg(0, "exists"); else dbg(0, "doesnt exist");
					g_free(fullpath);
				}
#endif
				//icon (only shown if the sound file is currently available):
				if(online){
					MIME_type* mime_type = mime_type_lookup(row[MYSQL_MIMETYPE]);
					type_to_icon(mime_type);
					if ( ! mime_type->image ) dbg(0, "no icon.");
					iconbuf = mime_type->image->sm_pixbuf;
				} else iconbuf = NULL;

				//strip the homedir from the dir string:
				char* path = strstr(row[MYSQL_DIR], g_get_home_dir());
				path = path ? path + strlen(g_get_home_dir()) + 1 : row[MYSQL_DIR];

				gtk_list_store_append(app.store, &iter);
				gtk_list_store_set(app.store, &iter, COL_ICON, iconbuf,
#ifdef USE_AYYI
				                     COL_AYYI_ICON, ayyi_icon,
#endif
				                     COL_IDX, atoi(row[0]), COL_NAME, sample_name, COL_FNAME, path, COL_KEYWORDS, keywords, 
				                     COL_OVERVIEW, pixbuf, COL_LENGTH, length, COL_SAMPLERATE, samplerate_s, COL_CHANNELS, channels, 
				                     COL_MIMETYPE, row[MYSQL_MIMETYPE], COL_NOTES, row[MYSQL_NOTES], COL_COLOUR, colour, -1);
				if(pixbuf) g_object_unref(pixbuf);
				row_count++;
			}
			mysql_free_result(result);

			if(0 && row_count < MAX_DISPLAY_ROWS){
				snprintf(sb_text, 256, "%i samples found.", row_count);
			}else{
				uint32_t tot_rows = (uint32_t)mysql_affected_rows(mysql);
				snprintf(sb_text, 256, "showing %i of %i samples", row_count, tot_rows);
			}
			statusbar_print(1, sb_text);
		}
		else{  // mysql_store_result() returned nothing
			if(mysql_field_count(mysql) > 0){
				// mysql_store_result() should have returned data
				printf( "Error getting records: %s\n", mysql_error(mysql));
			}
			else dbg(0, "failed to find any records (fieldcount<1): %s", mysql_error(mysql));
		}

		//treeview_unblock_motion_handler();  //causes a segfault even before it is run ??!!
	}
	else{
		dbg(0, "Failed to find any records: %s", mysql_error(mysql));
	}
}


void
clear_store()
{
	PF;

	//gtk_list_store_clear(app.store);

	GtkTreeIter iter;
	GdkPixbuf* pixbuf = NULL;

	while(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(app.store), &iter)){

		gtk_tree_model_get(GTK_TREE_MODEL(app.store), &iter, COL_OVERVIEW, &pixbuf, -1);

		gtk_list_store_remove(app.store, &iter);

		if(pixbuf) g_object_unref(pixbuf);
	}
}


void
on_category_changed(GtkComboBox *widget, gpointer user_data)
{
	PF;
}


void
on_view_category_changed(GtkComboBox *widget, gpointer user_data)
{
	//update the sample list with the new view-category.
	PF;

	if (app.search_category) g_free(app.search_category);
	app.search_category = gtk_combo_box_get_active_text(GTK_COMBO_BOX(app.view_category));

	do_search(app.search_phrase, app.search_dir);
}


void
on_category_set_clicked(GtkComboBox *widget, gpointer user_data)
{
	//add selected category to selected samples.

	PF;

	//selected category?
	gchar* category = gtk_combo_box_get_active_text(GTK_COMBO_BOX(app.category));

	GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(app.view));
	GList* selectionlist = gtk_tree_selection_get_selected_rows(selection, NULL);
	if(!selectionlist){ printf("on_category_set_clicked(): no files selected.\n"); return; }

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
	ASSERT_POINTER_FALSE(row_ref, "row_ref");

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

	if(!db_is_connected()) return;

	char qry[1024];
	MYSQL *mysql=&app.mysql;
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


gboolean
add_file(char* path)
{
	/*
	uri must be "unescaped" before calling this fn. Method string must be removed.
	*/
	dbg(0, "%s", path);
	if(!db_is_connected()) return FALSE;
	gboolean ok = TRUE;

	sample* sample = sample_new(); //free'd after db and store are updated.
	snprintf(sample->filename, 256, "%s", path);

	#define SQL_LEN 66000
	char sql[1024], sql2[SQL_LEN];
	char length_ms[64];
	char samplerate_s[32];
	gchar* filedir = g_path_get_dirname(path);
	gchar* filename = g_path_get_basename(path);
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

	sample_set_type_from_mime_string(sample, mime_string);

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

	if(get_file_info(sample)){

		//snprintf(sql2, SQL_LEN, "%s, pixbuf='%s', length=%i, sample_rate=%i, channels=%i, mimetype='%s/%s' ", sql, blob, sample.length, sample.sample_rate, sample.channels, mime_type->media_type, mime_type->subtype);
		snprintf(sql2, SQL_LEN, "%s, length=%i, sample_rate=%i, channels=%i, mimetype='%s/%s' ", sql, sample->length, sample->sample_rate, sample->channels, mime_type->media_type, mime_type->subtype);
		format_time_int(length_ms, sample->length);
		samplerate_format(samplerate_s, sample->sample_rate);
		dbg(0, "sql=%s", sql2);

		sample->id = db_insert(sql2);

		GtkTreeIter iter;
		gtk_list_store_append(app.store, &iter);

		//store a row reference:
		GtkTreePath* treepath;
		if((treepath = gtk_tree_model_get_path(GTK_TREE_MODEL(app.store), &iter))){
			sample->row_ref = gtk_tree_row_reference_new(GTK_TREE_MODEL(app.store), treepath);
			gtk_tree_path_free(treepath);
		}else errprintf("add_file(): failed to make treepath from inserted iter.\n");

		dbg(2, "setting store... filename=%s mime=%s", filename, mime_string);
		gtk_list_store_set(app.store, &iter,
						  COL_IDX, sample->id,
						  COL_NAME, filename,
						  COL_FNAME, filedir,
						  COL_LENGTH, length_ms,
						  COL_SAMPLERATE, samplerate_s,
						  COL_CHANNELS, sample->channels,
						  COL_MIMETYPE, mime_string,
						  -1);

		dbg(2, "sending message: sample=%p filename=%s (%p)", sample, sample->filename, sample->filename);
		g_async_queue_push(msg_queue, sample); //notify the overview thread.

		on_directory_list_changed();

	}else{
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
	dbg(0, "dir=%s", uri);

	//GDir* dir = g_dir_open(const gchar *path, guint flags, GError **error);

	return FALSE;
}


gboolean
get_file_info(sample* sample)
{
#ifdef HAVE_FLAC_1_1_1
	if(sample->filetype==TYPE_FLAC) return get_file_info_flac(sample);
#else
	if(0){}
#endif
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
		dbg(0, "not able to open input file %s.", filename);
		puts(sf_strerror(NULL));    // print the error message from libsndfile:
		return FALSE;
	}

	char chanwidstr[64];
	if     (sfinfo.channels==1) snprintf(chanwidstr, 64, "mono");
	else if(sfinfo.channels==2) snprintf(chanwidstr, 64, "stereo");
	else                        snprintf(chanwidstr, 64, "channels=%i", sfinfo.channels);
	if(debug) printf("%iHz %s frames=%i\n", sfinfo.samplerate, chanwidstr, (int)sfinfo.frames);
	sample->channels    = sfinfo.channels;
	sample->sample_rate = sfinfo.samplerate;
	sample->length      = (sfinfo.frames * 1000) / sfinfo.samplerate;

	if(sample->channels<1 || sample->channels>100){ dbg(0, "bad channel count: %i", sample->channels); return FALSE; }
	if(sf_close(sffile)) printf("error! bad file close.\n");

	return TRUE;
}


gboolean
on_overview_done(gpointer data)
{
	sample* sample = data;
	PF;
	if(!sample){ errprintf("on_overview_done(): bad arg: sample.\n"); return FALSE; }
	if(!sample->pixbuf){ errprintf("on_overview_done(): overview creation failed (no pixbuf).\n"); return FALSE; }

	db_update_pixbuf(sample);

	GtkTreePath* treepath;
	if((treepath = gtk_tree_row_reference_get_path(sample->row_ref))){ //it's possible the row may no longer be visible.
		GtkTreeIter iter;
		if(gtk_tree_model_get_iter(GTK_TREE_MODEL(app.store), &iter, treepath)){
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
	GdkPixbuf* pixbuf = sample->pixbuf;
	if(pixbuf){
		//serialise the pixbuf:
		guint8 blob[SQL_LEN];
		GdkPixdata pixdata;
		gdk_pixdata_from_pixbuf(&pixdata, pixbuf, 0);
		guint length;
		guint8* ser = gdk_pixdata_serialize(&pixdata, &length);
		mysql_real_escape_string(&app.mysql, (char*)blob, (char*)ser, length);
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

	PF;
	GtkTreeIter iter;
	int idx;
	gchar *filename;
	GtkTreeModel* store = GTK_TREE_MODEL(app.store);
	GtkTreePath* path = gtk_tree_path_new_from_string(path_string);
	gtk_tree_model_get_iter(store, &iter, path);
	gtk_tree_model_get(store, &iter, COL_IDX, &idx,  COL_NAME, &filename, -1);
	dbg(0, "filename=%s", filename);

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
	dbg(0, "sql=%s", sql);
	if(mysql_query(&app.mysql, sql)){
		dbg(0, "update failed! sql=%s", sql);
		return;
	}

	//update the store:
	gtk_list_store_set(app.store, &iter, COL_KEYWORDS, new_text, -1);
}


void
delete_row(GtkWidget *widget, gpointer user_data)
{
	//widget is likely to be a popupmenu.

	GtkTreeIter iter;
	GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(app.view));

	GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(app.view));
	GList* selectionlist = gtk_tree_selection_get_selected_rows(selection, &(model));
	if(!selectionlist){ perr("no files selected?\n"); return; }
	dbg(0, "%i rows selected.", g_list_length(selectionlist));

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

				if(!db_delete_row(id)) return;

				//update the store:
				gtk_list_store_remove(app.store, &iter);
			} else perr("bad iter! i=%i (<%i)\n", i, g_list_length(selectionlist));
		} else perr("cannot get path from row_ref!\n");
	}
	g_list_free(selected_row_refs); //FIXME free the row_refs?

	statusbar_printf(1, "%i rows deleted", i);
	on_directory_list_changed();
}


void
update_row(GtkWidget *widget, gpointer user_data)
{
	//sync the catalogue row with the filesystem.

	PF;
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(app.view));

	GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(app.view));
	GList* selectionlist = gtk_tree_selection_get_selected_rows(selection, &(model));
	if(!selectionlist){ errprintf("update_row(): no files selected?\n"); return; }
	dbg(0, "%i rows selected.", g_list_length(selectionlist));

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
			if ( mime_type->image == NULL ) dbg(0, "no icon.");
			iconbuf = mime_type->image->sm_pixbuf;

		}else{
			online=0;
			iconbuf = NULL;
		}
		gtk_list_store_set(app.store, &iter, COL_ICON, iconbuf, -1);

		char sql[1024];
		snprintf(sql, 1024, "UPDATE samples SET online=%i, last_checked=NOW() WHERE id=%i", online, id);
		dbg(0, "row: %s path=%s sql=%s", fname, path, sql);
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
	//currently this only works for the The tags cell.	
	PF;
	GtkTreeView* treeview = GTK_TREE_VIEW(app.view);

	GtkTreeSelection* selection = gtk_tree_view_get_selection(treeview);
	if(!selection){ errprintf("edit_row(): cannot get selection.\n");/* return;*/ }
	GtkTreeModel* model = GTK_TREE_MODEL(app.store);
	GList* selectionlist = gtk_tree_selection_get_selected_rows(selection, &(model));
	if(!selectionlist){ errprintf("edit_row(): no files selected?\n"); return; }

	GtkTreePath *treepath;
	if((treepath = g_list_nth_data(selectionlist, 0))){
		GtkTreeIter iter;
		if(gtk_tree_model_get_iter(GTK_TREE_MODEL(app.store), &iter, treepath)){
			gchar* path_str = gtk_tree_model_get_string_from_iter(GTK_TREE_MODEL(app.store), &iter);
			dbg(0, "path=%s", path_str);

			GtkTreeViewColumn* focus_column = app.col_tags;
			GtkCellRenderer*   focus_cell   = app.cell_tags;
	    	//g_signal_handlers_block_by_func(app.view, cursor_changed, self);
    		gtk_widget_grab_focus(app.view);
			gtk_tree_view_set_cursor_on_cell(GTK_TREE_VIEW(app.view), treepath,
			                                 focus_column, //GtkTreeViewColumn *focus_column - this needs to be set for start_editing to work.
			                                 focus_cell,   //the cell to be edited.
			                                 START_EDITING);
    		//g_signal_handlers_unblock_by_func(treeview, cursor_changed, self);


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

	GtkWidget* menu_item = gtk_menu_item_new_with_label ("Delete");
	gtk_menu_shell_append (GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect (G_OBJECT(menu_item), "activate", G_CALLBACK(delete_row), NULL);
	gtk_widget_show (menu_item);

	menu_item = gtk_menu_item_new_with_label ("Update");
	gtk_menu_shell_append (GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect (G_OBJECT(menu_item), "activate", G_CALLBACK(update_row), NULL);
	gtk_widget_show (menu_item);

	menu_item = gtk_menu_item_new_with_label ("Edit Tags");
	gtk_menu_shell_append (GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect (G_OBJECT(menu_item), "activate", G_CALLBACK(edit_row), NULL);
	gtk_widget_show (menu_item);

	menu_item = gtk_menu_item_new_with_label ("Open");
	gtk_menu_shell_append (GTK_MENU_SHELL(menu), menu_item);
	gtk_widget_set_sensitive(GTK_WIDGET(menu_item), FALSE);
	gtk_widget_show (menu_item);

	//-------

	//separator
	menu_item = gtk_separator_menu_item_new();
	gtk_menu_shell_append (GTK_MENU_SHELL(menu), menu_item);
	gtk_widget_show (menu_item);

	//-------

	GtkWidget* prefs = gtk_menu_item_new_with_label("Prefs");
	gtk_widget_show(prefs);
	gtk_container_add(GTK_CONTAINER(menu), prefs);

	GtkWidget* sub = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(prefs), sub);

	menu_item = gtk_check_menu_item_new_with_mnemonic("Add Recursively");
	gtk_menu_shell_append(GTK_MENU_SHELL(sub), menu_item);
	gtk_widget_show(menu_item);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item), app.add_recursive);
	g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(toggle_recursive_add), NULL);

	if(themes){
		GtkWidget* theme_menu = gtk_menu_item_new_with_label("Icon Themes");
		gtk_widget_show(theme_menu);
		gtk_container_add(GTK_CONTAINER(sub), theme_menu);

		GtkWidget* sub_menu = themes->data;
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(theme_menu), sub_menu);
	}

	return menu;
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

	dbg(0, "setting rowref=%p", prev_row_ref);
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
		gtk_cell_renderer_get_size(checkcell, GTK_WIDGET(view), NULL, NULL, NULL, (int*)&width, NULL);

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
		gtk_cell_renderer_get_size(checkcell, GTK_WIDGET(view), &cell_rect, NULL, NULL, (int*)&width, (int*)&height);
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
	dbg(0, "entry activated!");
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
			dbg(0, "'%s' already in string '%s'", new, existing);
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
	PF;

	int box_num = GPOINTER_TO_UINT(user_data); //box_num corresponds to the colour index.

	//convert to a pseudo uri string:
	sprintf(text, "colour:%i%c%c", box_num + 1, 13, 10); //1 based to avoid atoi problems.

	gtk_selection_data_set(data, GDK_SELECTION_TYPE_STRING, BITS_PER_CHAR_8, (guchar*)text, strlen(text));
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

  PF;

  static gulong handler1 =0; // the edit box "focus_out" handler.
  //static gulong handler2 =0; // the edit box RETURN key trap.
  
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
  
  dbg(0, "finished.");
  //busy = FALSE;
}


gboolean
on_directory_list_changed()
{
	/*
	the database has been modified, the directory list may have changed.
	Update all directory views. Atm there is only one.
	*/

	g_idle_add(dir_tree_update, NULL);
	return FALSE;
}


gboolean
toggle_recursive_add(GtkWidget *widget, gpointer user_data)
{
	PF;
	if(app.add_recursive) app.add_recursive = false; else app.add_recursive = true;
	return FALSE;
}


void
on_file_moved(GtkTreeIter iter)
{
	PF;

	gchar *tags;
	gchar *fpath;
	gchar *fname;
	gchar *length;
	gchar *samplerate;
	int channels;
	gchar *mimetype;
	gchar *notes;
	int id;
	gtk_tree_model_get(GTK_TREE_MODEL(app.store), &iter, COL_NAME, &fname, COL_FNAME, &fpath, COL_KEYWORDS, &tags, COL_LENGTH, &length, COL_SAMPLERATE, &samplerate, COL_CHANNELS, &channels, COL_MIMETYPE, &mimetype, COL_NOTES, &notes, COL_IDX, &id, -1);
}


extern char theme_name[];

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
		dbg (2, "group=%s.", groupname);
		if(!strcmp(groupname, "Samplecat")){
			/*
			gsize length;
			gchar** keys = g_key_file_get_keys(app.key_file, "Samplecat", &length, &error);
			*/

			#define num_keys 10
			//FIXME 64 is not long enough for directories
			char keys[num_keys][64] = {"database_host",          "database_user",          "database_pass",          "database_name",          "show_dir",          "window_height",          "window_width",         "icon_theme", "col1_width",                "filter"};
			char* loc[num_keys]     = {app.config.database_host, app.config.database_user, app.config.database_pass, app.config.database_name, app.config.show_dir, app.config.window_height, app.config.window_width, theme_name,  app.config.column_widths[0], app.search_phrase};

			int k;
			gchar* keyval;
			for(k=0;k<num_keys;k++){
				if((keyval = g_key_file_get_string(app.key_file, groupname, keys[k], &error))){
					snprintf(loc[k], 64, "%s", keyval);
                    dbg(2, "%s=%s", keys[k], keyval);
					g_free(keyval);
				}else{
					GERR_WARN;
					strcpy(loc[k], "");
				}
			}
			set_search_dir(app.config.show_dir);
		}
		else{ warnprintf("config_load(): cannot find Samplecat key group.\n"); return FALSE; }
		g_free(groupname);
	}else{
		printf("unable to load config file: %s.\n", error->message);
		g_error_free(error);
		error = NULL;
		config_new();
		config_save();
		return FALSE;
	}

	return TRUE;
}


gboolean
config_save()
{
	if(app.loaded){
		//update the search directory:
		g_key_file_set_value(app.key_file, "Samplecat", "show_dir", app.search_dir);

		//save window dimensions:
		gint width, height;
		char value[256];
		if(app.window && GTK_WIDGET_REALIZED(app.window)){
			gtk_window_get_size(GTK_WINDOW(app.window), &width, &height);
		} else {
			dbg (0, "couldnt get window size...", "");
			width  = MIN(atoi(app.config.window_width),  2048); //FIXME ?
			height = MIN(atoi(app.config.window_height), 2048);
		}
		snprintf(value, 255, "%i", width);
		g_key_file_set_value(app.key_file, "Samplecat", "window_width", value);
		snprintf(value, 255, "%i", height);
		g_key_file_set_value(app.key_file, "Samplecat", "window_height", value);

		g_key_file_set_value(app.key_file, "Samplecat", "icon_theme", theme_name);

		GtkTreeViewColumn* column = gtk_tree_view_get_column(GTK_TREE_VIEW(app.view), 1);
		int column_width = gtk_tree_view_column_get_width(column);
		snprintf(value, 255, "%i", column_width);
		g_key_file_set_value(app.key_file, "Samplecat", "col1_width", value);

		g_key_file_set_value(app.key_file, "Samplecat", "col1_height", value);

		g_key_file_set_value(app.key_file, "Samplecat", "filter", gtk_entry_get_text(GTK_ENTRY(app.search)));
	}

	GError *error = NULL;
	gsize length;
	gchar* string = g_key_file_to_data(app.key_file, &length, &error);
	if(error){
		dbg (0, "error saving config file: %s", error->message);
		g_error_free(error);
	}

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
	//g_key_file_has_group(GKeyFile *key_file, const gchar *group_name);

	GError *error = NULL;
	char data[256 * 256];
	sprintf(data, "# this is the default config file for the Samplecat application.\n# pls enter your database details.\n"
		"[Samplecat]\n"
		"database_host=localhost\n"
		"database_user=username\n"
		"database_pass=pass\n"
		"database_name=samplelib\n"
		);

	if(!g_key_file_load_from_data(app.key_file, data, strlen(data), G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, &error)){
		perr("error creating new key_file from data. %s\n", error->message);
		g_error_free(error);
		error = NULL;
		return;
	}

	printf("A default config file has been created. Please enter your database details in '%s'.\n", app.config_filename);
}


void
on_quit(GtkMenuItem *menuitem, gpointer user_data)
{
	//if(user_data) dbg(0, "exitcode=%i", GPOINTER_TO_INT(user_data));
	int exit_code = GPOINTER_TO_INT(user_data);
	if(exit_code > 1) exit_code = 0; //ignore invalid exit code.

	jack_close();

	if(app.loaded) config_save();

#if 0
	//disabled due to errors when quitting early.

	clear_store(); //does this make a difference to valgrind? no.

	gtk_main_quit();
#endif

	MYSQL* mysql;
	mysql=&app.mysql;
	mysql_close(mysql);

	dbg (1, "done.");
	exit(exit_code);
}


