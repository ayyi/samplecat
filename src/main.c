/*

Copyright (C) Tim Orford 2007-2009

This software is licensed under the GPL. See accompanying file COPYING.

*/
#define __main_c__
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

#ifdef USE_AYYI
  #include "ayyi.h"
  #include "ayyi_model.h"
#endif
#include "file_manager.h"
#include "gqview_view_dir_tree.h"
#include "typedefs.h"
#ifdef USE_TRACKER
  #include "src/db/tracker_.h"
#endif
#ifdef USE_MYSQL
  #include "db/mysql.h"
#endif

#include "types.h"
#include "mimetype.h"
#include "dh-link.h"
#include "support.h"
#include "main.h"
#include "sample.h"
#include "audio.h"
#include "overview.h"
#include "cellrenderer_hypertext.h"
#include "dh_tree.h"
#include "listview.h"
#include "window.h"
#include "inspector.h"
#include "dnd.h"
#include "console_view.h"
#ifdef USE_GQVIEW_1_DIRTREE
  #include "filelist.h"
  #include "view_dir_list.h"
  #include "view_dir_tree.h"
#endif

#include "pixmaps.h"

#ifdef USE_SQLITE
  #include "db/sqlite.h"
#endif

#undef DEBUG_NO_THREADS

//#if defined(HAVE_STPCPY) && !defined(HAVE_mit_thread)
  #define strmov(A,B) stpcpy((A),(B))
//#endif

extern void       dir_init          ();
static void       delete_row        (GtkWidget*, gpointer);
static void       update_row        (GtkWidget*, gpointer);
static void       edit_row          (GtkWidget*, gpointer);
static GtkWidget* make_context_menu ();
static gboolean   can_use_backend   (const char*);

struct _app app;
struct _palette palette;
GList* mime_types; // list of MIME_type*
extern GList* themes; 

unsigned debug = 0;
static gboolean search_pending = false;

//strings for console output:
char white [16];
char red   [16];
char green [16];
char yellow[16];
char bold  [16];

static const struct option long_options[] = {
  { "backend",          1, NULL, 'b' },
  { "no-gui",           0, NULL, 'g' },
  { "verbose",          1, NULL, 'v' },
  { "search",           1, NULL, 's' },
  { "add",              1, NULL, 'a' },
  { "help",             0, NULL, 'h' },
};

static const char* const short_options = "b:gv:s:a:h";

static const char* const usage =
  "Usage: %s [ options ]\n"
  " -v --verbose   show more information.\n"
  " -b --backend   select which database type to use.\n"
  " -g --no-gui    run as command line app.\n"
  " -s --search    search using this phrase.\n"
  " -a --add       add these files.\n"
  " -h --help      show this usage information and quit.\n"
  "\n";


void
app_init()
{
	memset(&app, 0, sizeof(app));

	int i; for(i=0;i<PALETTE_SIZE;i++) app.colour_button[i] = NULL;
	app.colourbox_dirty = true;
	memset(app.config.colour, 0, PALETTE_SIZE * 8);

	app.config_filename = g_strdup_printf("%s/." PACKAGE, g_get_home_dir());
}


int 
main(int argc, char** argv)
{
	//init console escape commands:
	sprintf(white,  "%c[0;39m", 0x1b);
	sprintf(red,    "%c[1;31m", 0x1b);
	sprintf(green,  "%c[1;32m", 0x1b);
	sprintf(yellow, "%c[1;33m", 0x1b);
	sprintf(bold,   "%c[1;39m", 0x1b);
	sprintf(err,    "%serror!%s", red, white);
	sprintf(warn,   "%swarning:%s", yellow, white);

	g_log_set_handler (NULL, G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION, log_handler, NULL);
	g_log_set_handler ("Gtk", G_LOG_LEVEL_CRITICAL | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION, log_handler, NULL);

	app_init();

	printf("%s"PACKAGE_NAME". Version "PACKAGE_VERSION"%s\n", yellow, white);

	#define ADD_BACKEND(A) app.backends = g_list_append(app.backends, A)
	ADD_BACKEND("mysql");
#ifdef USE_SQLITE
	ADD_BACKEND("sqlite");
#endif
#ifdef USE_TRACKER
	app.backends = g_list_append(app.backends, "tracker");
#endif

	int opt;
	while((opt = getopt_long (argc, argv, short_options, long_options, NULL)) != -1) {
		switch(opt) {
			case 'v':
				printf("using debug level: %s\n", optarg);
				int d = atoi(optarg);
				if(d<0 || d>5) { gwarn ("bad arg. debug=%i", d); } else debug = d;
				break;
			case 'b':
				//if a particular backend is requested, and is available, reduce the backend list to just this one.
				dbg(0, "backend '%s' requested.", optarg);
				if(can_use_backend(optarg)){
					list_clear(app.backends);
					//app.backends = g_list_append(app.backends, optarg);
					ADD_BACKEND(optarg);
					dbg(0, "n_backends=%i", g_list_length(app.backends));
				}
				else gwarn("requested backend not available: '%s'", optarg);
				break;
			case 'g':
				app.no_gui = true;
				break;
			case 'h':
				printf(usage, argv[0]);
				exit(EXIT_SUCCESS);
				break;
			case 's':
				printf("search=%s\n", optarg);
				app.args.search = g_strdup(optarg);
				break;
			case 'a':
				printf("add=%s\n", optarg);
				app.args.add = g_strdup(optarg);
				break;
			case '?':
				printf("unknown option: %c\n", optopt);
				break;
		}
	}

	config_load();

	g_thread_init(NULL);
	gdk_threads_init();
	app.mutex = g_mutex_new();
	gtk_init(&argc, &argv);

	type_init();
	pixmaps_init();
	dir_init();

#ifdef USE_MYSQL
	if(can_use_backend("mysql")){
		if(mysql__connect()){
			set_backend(BACKEND_MYSQL);
		}else{
			#ifdef QUIT_WITHOUT_DB
			on_quit(NULL, GINT_TO_POINTER(1));
			#endif
		}
	}
#endif
#ifdef USE_SQLITE
	if(can_use_backend("sqlite")){
		if(!sqlite__connect()){
			#ifdef QUIT_WITHOUT_DB
			on_quit(NULL, GINT_TO_POINTER(EXIT_FAILURE));
			#endif
		}else{
			if(BACKEND_IS_NULL) set_backend(BACKEND_SQLITE);
		}
	}
#endif
#ifdef USE_TRACKER
	if(BACKEND_IS_NULL && can_use_backend("tracker")){
		void on_tracker_init()
		{
			dbg(2, "...");
			set_backend(BACKEND_TRACKER);
			if(search_pending){
				do_search(app.search_phrase, app.search_dir);
			}
		}
		if(app.no_gui){
			tracker__init(on_tracker_init);
		}
		else {
			backend.pending = true;
			g_idle_add_full(G_PRIORITY_LOW, (GSourceFunc)tracker__init, on_tracker_init, NULL);
		}
	}
#endif

	if(app.args.add){
		add_file(app.args.add);
	}

	if(!app.no_gui) file_manager__init();

#ifndef DEBUG_NO_THREADS
	dbg(3, "creating overview thread...");
	GError *error = NULL;
	app.msg_queue = g_async_queue_new();
	if(!g_thread_create(overview_thread, NULL, false, &error)){
		perr("error creating thread: %s\n", error->message);
		g_error_free(error);
	}
#endif

	app.store = gtk_list_store_new(NUM_COLS, GDK_TYPE_PIXBUF,
	                               #ifdef USE_AYYI
	                               GDK_TYPE_PIXBUF,
	                               #endif
	                               G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);

	if(!app.no_gui) window_new(); 
	if(!app.no_gui) app.context_menu = make_context_menu();

	if(!backend.pending){ 
		do_search(app.search_phrase, app.search_dir);
	}else{
		search_pending = true;
	}

	if(app.no_gui) exit(EXIT_SUCCESS);

#ifdef USE_AYYI
	ayyi_client_init();
	ayyi_connect();
#endif

#ifdef USE_TRACKER
	if(!app.no_gui && BACKEND_IS_NULL) g_idle_add((GSourceFunc)tracker__init, NULL);
#else
	//if(BACKEND_IS_NULL) listview__show_db_missing();
#endif

	app.loaded = true;
	dbg(1, "loaded");
	message_panel__add_msg("hello", GTK_STOCK_INFO);

	gtk_main();
	exit(EXIT_SUCCESS);
}


void
update_search_dir(gchar* uri)
{
	set_search_dir(uri);

	const gchar* text = app.search ? gtk_entry_get_text(GTK_ENTRY(app.search)) : "";
	do_search((gchar*)text, uri);
}


gboolean
dir_tree_update(gpointer data)
{
	/*
	refresh the directory tree. Called from an idle.

	note: destroying the whole node tree is wasteful - can we just make modifications to it?

	*/
	update_dir_node_list();

	dh_book_tree_reload((DhBookTree*)app.dir_treeview);

	return IDLE_STOP;
}


void
set_search_dir(char* dir)
{
	//this doesnt actually do the search. When calling, follow with do_search() if neccesary.

	//if string is empty, we show all directories?

	if(!dir){ perr("dir!\n"); return; }

	dbg (1, "dir=%s", dir);
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
	return row_num;
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
				if(add_file(filepath)) (*added_count)++;
				statusbar_print(1, "%i files added", *added_count);
			}
			// IS_DIR
			else if(app.add_recursive){
				scan_dir(filepath, added_count);
			}
		}
		g_dir_close(dir);
	}else{
		perr("cannot open directory. %s\n", error->message);
		g_error_free(error);
		error = NULL;
	}
}

void
do_search(char *search, char *dir)
{
	//fill the display with the results for the given search phrase.

	#define MAX_DISPLAY_ROWS 1000
	PF;

	if(BACKEND_IS_NULL) return;
	search_pending = false;

#ifdef USE_AYYI
	GdkPixbuf* ayyi_icon = NULL;
#endif

	int n_results = 0;
	if(backend.search_iter_new(search, dir, app.search_category, &n_results)){

		treeview_block_motion_handler(); //dont know exactly why but this prevents a segfault.

		listmodel__clear();

		//detach the model from the view to speed up row inserts:
		/*
		model = gtk_tree_view_get_model(GTK_TREE_VIEW(view));
		g_object_ref(model);
		gtk_tree_view_set_model(GTK_TREE_VIEW(view), NULL);
		*/ 

		if(
			#ifdef USE_MYSQL
			BACKEND_IS_MYSQL
			#else
			false
			#endif
			#ifdef USE_SQLITE
			|| BACKEND_IS_SQLITE
			#endif
			){
			int row_count = 0;
			unsigned long *lengths;
			SamplecatResult* result;
			while((result = backend.search_iter_next(&lengths)) && row_count < MAX_DISPLAY_ROWS){
				if(app.no_gui){
					if(!row_count) console__show_result_header();
					console__show_result(result);
				}else{
					listmodel__add_result(result);
				}
				row_count++;
			}
			if(app.no_gui) console__show_result_footer(row_count);

			backend.search_iter_free();

			if(0 && row_count < MAX_DISPLAY_ROWS){
				statusbar_print(1, "%i samples found.", row_count);
			}else if(!row_count){
				statusbar_print(1, "no samples found. filters: dir=%s", app.search_dir);
			}else{
				statusbar_print(1, "showing %i of %i samples", row_count, n_results);
			}
		}
		#ifdef USE_TRACKER
		else if(BACKEND_IS_TRACKER){
			int row_count = 0;
			SamplecatResult* result;
			while((result = tracker__search_iter_next()) && row_count < MAX_DISPLAY_ROWS){
				if(app.no_gui && row_count > 20) continue;
				listmodel__add_result(result);
				if(app.no_gui){
					if(!row_count) console__show_result_header();
					console__show_result(result);
				}
				row_count++;
			}
			backend.search_iter_free();

			if(0 && row_count < MAX_DISPLAY_ROWS){
				statusbar_print(1, "%i samples found.", row_count);
			}else{
				statusbar_print(1, "showing %i samples", row_count);
			}
		}
		#endif
		/*
		else{  // mysql_store_result() returned nothing
			if(mysql_field_count(mysql) > 0){
				// mysql_store_result() should have returned data
				printf( "Error getting records: %s\n", mysql_error(mysql));
			}
			else dbg(0, "failed to find any records (fieldcount<1): %s", mysql_error(mysql));
		}
		*/

		//treeview_unblock_motion_handler();  //causes a segfault even before it is run ??!!
	}
}


gboolean
row_set_tags_from_id(int id, GtkTreeRowReference* row_ref, const char* tags_new)
{
	if(!id){ perr("bad arg: id (%i)\n", id); return false; }
	g_return_val_if_fail(row_ref, false);
	dbg(0, "id=%i", id);

	if(!backend.update_keywords(id, tags_new)){
		statusbar_print(1, "database error. keywords not updated");
		return false;
	}

	//update the store:
	GtkTreePath *path;
	if((path = gtk_tree_row_reference_get_path(row_ref))){
		GtkTreeIter iter;
		gtk_tree_model_get_iter(GTK_TREE_MODEL(app.store), &iter, path);
		gtk_tree_path_free(path);

		gtk_list_store_set(app.store, &iter, COL_KEYWORDS, tags_new, -1);
	}
	else { perr("cannot get row path: id=%i.\n", id); return false; }

	return true;
}


void
update_dir_node_list()
{
	/*
	builds a node list of directories listed in the database.
	*/

	if(BACKEND_IS_NULL) return;

	backend.dir_iter_new();

	if(app.dir_tree) g_node_destroy(app.dir_tree);
	app.dir_tree = g_node_new(NULL);

	if(1/*result*/){// there are rows
		DhLink* link = dh_link_new(DH_LINK_TYPE_PAGE, "all directories", "");
		GNode* leaf = g_node_new(link);
		g_node_insert(app.dir_tree, -1, leaf);

		char* u;
		while((u = backend.dir_iter_next())){

			link = dh_link_new(DH_LINK_TYPE_PAGE, u, u);
			leaf = g_node_new(link);

			gint position = -1;//end
			g_node_insert(app.dir_tree, position, leaf);
		}
	}

	backend.dir_iter_free();
	dbg(2, "size=%i", g_node_n_children(app.dir_tree));
}


gboolean
new_search(GtkWidget *widget, gpointer userdata)
{
	gwarn("deprecated function");
	const gchar* text = gtk_entry_get_text(GTK_ENTRY(app.search));
	
	do_search((gchar*)text, app.search_dir);
	return false;
}


gboolean
mimestring_is_unsupported(char* mime_string)
{
	MIME_type* mime_type = mime_type_lookup(mime_string);
	return mimetype_is_unsupported(mime_type, mime_string);
}


gboolean
mimetype_is_unsupported(MIME_type* mime_type, char* mime_string)
{
	//TODO remove 2nd arg

	char types[][16] = {"application", "image", "text", "video"};
	int i; for(i=0;i<G_N_ELEMENTS(types);i++){
		if(!strcmp(mime_type->media_type, types[i])){
			return true;
		}
	}

	char unsupported[][64] = {"audio/mpeg", "audio/x-tta", "audio/x-speex", "audio/x-musepack"};
	for(i=0;i<G_N_ELEMENTS(unsupported);i++){
		if(!strcmp(mime_string, unsupported[i])){
			return true;
		}
	}
	dbg(1, "mimetype ok: %s", mime_string);
	return false;
}


gboolean
add_file(char* path)
{
	/*
	uri must be "unescaped" before calling this fn. Method string must be removed.
	*/

	dbg(1, "%s", path);
	if(BACKEND_IS_NULL) return false;
	gboolean ok = true;

	sample* sample = sample_new(); //free'd after db and store are updated.
	snprintf(sample->filename, 256, "%s", path);

	char length_ms[64];
	char samplerate_s[32];
	gchar* filedir = g_path_get_dirname(path);
	gchar* filename = g_path_get_basename(path);

	MIME_type* mime_type = type_from_path(filename);
	char mime_string[64];
	snprintf(mime_string, 64, "%s/%s", mime_type->media_type, mime_type->subtype);

#if 0
	char extn[64];
	file_extension(filename, extn);
	if(!strcmp(extn, "rfl")){
		printf("cannot add file: file type \"%s\" not supported.\n", extn);
		statusbar_print(1, "cannot add file: rfl files not supported.");
		ok = false;
		goto out;
	}
#endif

	sample_set_type_from_mime_string(sample, mime_string);

	if(mimetype_is_unsupported(mime_type, mime_string)){
		printf("cannot add file: file type \"%s\" not supported.\n", mime_string);
		statusbar_print(1, "cannot add file: %s files not supported", mime_string);
		ok = false;
		goto out;
	}

	if(get_file_info(sample)){
		format_time_int(length_ms, sample->length);
		samplerate_format(samplerate_s, sample->sample_rate);

		sample->id = backend.insert(sample, mime_type);

		GtkTreeIter iter;
		gtk_list_store_append(app.store, &iter);

		//store a row reference:
		GtkTreePath* treepath;
		if((treepath = gtk_tree_model_get_path(GTK_TREE_MODEL(app.store), &iter))){
			sample->row_ref = gtk_tree_row_reference_new(GTK_TREE_MODEL(app.store), treepath);
			gtk_tree_path_free(treepath);
		}else perr("failed to make treepath from inserted iter.\n");

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
		g_async_queue_push(app.msg_queue, sample); //notify the overview thread.

		on_directory_list_changed();

	}else{
		ok = false;
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

	return false;
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
		int e = sf_error(NULL);
		dbg(0, "error=%i", e);
		return false;
	}

	char chanwidstr[64];
	if     (sfinfo.channels==1) snprintf(chanwidstr, 64, "mono");
	else if(sfinfo.channels==2) snprintf(chanwidstr, 64, "stereo");
	else                        snprintf(chanwidstr, 64, "channels=%i", sfinfo.channels);
	if(debug) printf("%iHz %s frames=%i\n", sfinfo.samplerate, chanwidstr, (int)sfinfo.frames);
	sample->channels    = sfinfo.channels;
	sample->sample_rate = sfinfo.samplerate;
	sample->length      = (sfinfo.frames * 1000) / sfinfo.samplerate;

	if(sample->channels<1 || sample->channels>100){ dbg(0, "bad channel count: %i", sample->channels); return false; }
	if(sf_close(sffile)) perr("bad file close.\n");

	return true;
}


gboolean
on_overview_done(gpointer data)
{
	PF;
	sample* sample = data;
	g_return_val_if_fail(sample, false);
	if(!sample->pixbuf){ perr("overview creation failed (no pixbuf).\n"); return false; }

	backend.update_pixbuf(sample);

	if(sample->row_ref){
		GtkTreePath* treepath;
		if((treepath = gtk_tree_row_reference_get_path(sample->row_ref))){ //it's possible the row may no longer be visible.
			GtkTreeIter iter;
			if(gtk_tree_model_get_iter(GTK_TREE_MODEL(app.store), &iter, treepath)){
				if(GDK_IS_PIXBUF(sample->pixbuf)){
					gtk_list_store_set(app.store, &iter, COL_OVERVIEW, sample->pixbuf, -1);
				}else perr("pixbuf is not a pixbuf.\n");

			}else perr("failed to get row iter. row_ref=%p\n", sample->row_ref);
			gtk_tree_path_free(treepath);
		} else dbg(2, "no path for rowref. row_ref=%p\n", sample->row_ref); //this is not an error. The row may not be part of the current search.
	} else pwarn("rowref not set!\n");

	sample_free(sample);
	return false; //source should be removed.
}


static void
delete_row(GtkWidget *widget, gpointer user_data)
{
	//widget is likely to be a popupmenu.

	GtkTreeIter iter;
	GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(app.view));

	GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(app.view));
	GList* selectionlist = gtk_tree_selection_get_selected_rows(selection, &(model));
	if(!selectionlist){ perr("no files selected?\n"); return; }
	dbg(1, "%i rows selected.", g_list_length(selectionlist));

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

				if(!backend.delete(id)) return;

				//update the store:
				gtk_list_store_remove(app.store, &iter);
			} else perr("bad iter! i=%i (<%i)\n", i, g_list_length(selectionlist));
		} else perr("cannot get path from row_ref!\n");
	}
	g_list_free(selected_row_refs); //FIXME free the row_refs?

	statusbar_print(1, "%i rows deleted", i);
	on_directory_list_changed();
}


static void
update_row(GtkWidget *widget, gpointer user_data)
{
	//sync the catalogue row with the filesystem.

	PF;
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(app.view));

	GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(app.view));
	GList* selectionlist = gtk_tree_selection_get_selected_rows(selection, &(model));
	if(!selectionlist){ perr("no files selected?\n"); return; }
	dbg(2, "%i rows selected.", g_list_length(selectionlist));

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
			online = 1;

			MIME_type* mime_type = mime_type_lookup(mimetype);
			type_to_icon(mime_type);
			if (!mime_type->image) dbg(0, "no icon.");
			iconbuf = mime_type->image->sm_pixbuf;

		}else{
			online = 0;
			iconbuf = NULL;
		}
		gtk_list_store_set(app.store, &iter, COL_ICON, iconbuf, -1);

		if(backend.update_online(id, online)){
			statusbar_print(1, "online status updated (%s)", online ? "online" : "not online");
		}else
			statusbar_print(1, "error! online status not updated");
	}
	g_list_free(selectionlist);
	//g_date_free(date);
}


static void
edit_row(GtkWidget *widget, gpointer user_data)
{
	//currently this only works for the The tags cell.	
	PF;
	GtkTreeView* treeview = GTK_TREE_VIEW(app.view);

	GtkTreeSelection* selection = gtk_tree_view_get_selection(treeview);
	if(!selection){ perr("cannot get selection.\n");/* return;*/ }
	GtkTreeModel* model = GTK_TREE_MODEL(app.store);
	GList* selectionlist = gtk_tree_selection_get_selected_rows(selection, &(model));
	if(!selectionlist){ perr("no files selected?\n"); return; }

	GtkTreePath *treepath;
	if((treepath = g_list_nth_data(selectionlist, 0))){
		GtkTreeIter iter;
		if(gtk_tree_model_get_iter(GTK_TREE_MODEL(app.store), &iter, treepath)){
			gchar* path_str = gtk_tree_model_get_string_from_iter(GTK_TREE_MODEL(app.store), &iter);
			dbg(2, "path=%s", path_str);

			GtkTreeViewColumn* focus_column = app.col_tags;
			GtkCellRenderer*   focus_cell   = app.cell_tags;
			//g_signal_handlers_block_by_func(app.view, cursor_changed, self);
			gtk_widget_grab_focus(app.view);
			gtk_tree_view_set_cursor_on_cell(GTK_TREE_VIEW(app.view), treepath,
			                                 focus_column, //GtkTreeViewColumn *focus_column - this needs to be set for start_editing to work.
			                                 focus_cell,   //the cell to be edited.
			                                 START_EDITING);
			//g_signal_handlers_unblock_by_func(treeview, cursor_changed, self);


			g_free(path_str);
		} else perr("cannot get iter.\n");
		gtk_tree_path_free(treepath);
	}
	g_list_free(selectionlist);
}


static MenuDef _menu_def[] = {
    {"Delete",         G_CALLBACK(delete_row), GTK_STOCK_DELETE,      true},
    {"Update",         G_CALLBACK(update_row), GTK_STOCK_REFRESH,     true},
    {"Edit tags",      G_CALLBACK(edit_row),   GTK_STOCK_EDIT,        true},
    {"Open",           G_CALLBACK(edit_row),   GTK_STOCK_OPEN,       false},
    {"Open Directory", G_CALLBACK(NULL),       GTK_STOCK_OPEN,        true},
    {"",                                                                  },
    {"Prefs",          G_CALLBACK(NULL),       GTK_STOCK_PREFERENCES, true},
};

static GtkWidget*
make_context_menu()
{
	GtkWidget *menu = gtk_menu_new();

	add_menu_items_from_defn(menu, _menu_def, G_N_ELEMENTS(_menu_def));

	GList* menu_items = gtk_container_get_children((GtkContainer*)menu);

	GtkWidget* prefs = g_list_last(menu_items)->data;

	GtkWidget* sub = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(prefs), sub);

	GtkWidget* menu_item = gtk_check_menu_item_new_with_mnemonic("Add Recursively");
	gtk_menu_shell_append(GTK_MENU_SHELL(sub), menu_item);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item), app.add_recursive);
	g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(toggle_recursive_add), NULL);

	if(themes){
		GtkWidget* theme_menu = gtk_menu_item_new_with_label("Icon Themes");
		gtk_container_add(GTK_CONTAINER(sub), theme_menu);

		GtkWidget* sub_menu = themes->data;
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(theme_menu), sub_menu);
	}

	gtk_widget_show_all(menu);
	return menu;
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

	if(colm == NULL) return false; // not found

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
			return true;
		}

		colm_x += width;
	}

	g_list_free(cells);
	return false; // not found
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

		//if(x >= colm_x && x < (colm_x + width)){
		//if(y >= colm_y && y < (colm_y + height)){
		if(y >= cell_rect.y && y < (cell_rect.y + cell_rect.height)){
			*cell = checkcell;
			g_list_free(cells);
			return true;
		}

		//colm_y += height;
	}

	g_list_free(cells);
	printf("not found in column. cell_height=%i\n", cell_rect.height);
	return false; // not found
}


gboolean
keyword_is_dupe(char* new, char* existing)
{
	//return true if the word 'new' is contained in the 'existing' string.

	if(!existing) return false;

	//FIXME make case insensitive.
	//gint g_ascii_strncasecmp(const gchar *s1, const gchar *s2, gsize n);
	//gchar*      g_utf8_casefold(const gchar *str, gssize len);

	gboolean found = false;

	//split the existing keyword string into separate words.
	gchar** split = g_strsplit(existing, " ", 0);
	int word_index = 0;
	while(split[word_index]){
		if(!strcmp(split[word_index], new)){
			dbg(0, "'%s' already in string '%s'", new, existing);
			found = true;
			break;
		}
		word_index++;
	}

	g_strfreev(split);
	return found;
}


gboolean
on_directory_list_changed()
{
	/*
	the database has been modified, the directory list may have changed.
	Update all directory views. Atm there is only one.
	*/

	g_idle_add(dir_tree_update, NULL);
	return false;
}


gboolean
toggle_recursive_add(GtkWidget *widget, gpointer user_data)
{
	PF;
	if(app.add_recursive) app.add_recursive = false; else app.add_recursive = true;
	return false;
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
		if(debug) printf("ini file loaded.\n");
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
		else{ pwarn("cannot find Samplecat key group.\n"); return false; }
		g_free(groupname);
	}else{
		printf("unable to load config file: %s.\n", error->message);
		g_error_free(error);
		error = NULL;
		config_new();
		config_save();
		return false;
	}

	return true;
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
		statusbar_print(1, "cannot open config file for writing (%s).", app.config_filename);
		return false;
	}
	if(fprintf(fp, "%s", string) < 0){
		errprintf("error writing data to config file (%s).\n", app.config_filename);
		statusbar_print(1, "error writing data to config file (%s).", app.config_filename);
	}
	fclose(fp);
	g_free(string);
	return true;
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
		"show_dir=\n"
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

	backend.disconnect();

	dbg (1, "done.");
	exit(exit_code);
}


void
set_backend(BackendType type)
{
	backend.pending = false;

	switch(type){
		case BACKEND_MYSQL:
			#ifdef USE_MYSQL
			backend.search_iter_new  = mysql__search_iter_new;
			backend.search_iter_next = mysql__search_iter_next_;
			backend.search_iter_free = mysql__search_iter_free;
			backend.dir_iter_new     = mysql__dir_iter_new;
			backend.dir_iter_next    = mysql__dir_iter_next;
			backend.dir_iter_free    = mysql__dir_iter_free;
			backend.insert           = mysql__insert;
			backend.delete           = mysql__delete_row;
			backend.disconnect       = mysql__disconnect;
			backend.update_colour    = mysql__update_colour;
			backend.update_keywords  = mysql__update_keywords;
			backend.update_notes     = mysql__update_notes;
			backend.update_pixbuf    = mysql__update_pixbuf;
			backend.update_online    = mysql__update_online;
			printf("backend is mysql.\n");
			#endif
			break;
		case BACKEND_SQLITE:
			#ifdef USE_SQLITE
			backend.search_iter_new  = sqlite__search_iter_new;
			backend.search_iter_next = sqlite__search_iter_next;
			backend.search_iter_free = sqlite__search_iter_free;
			backend.dir_iter_new     = sqlite__dir_iter_new;
			backend.dir_iter_next    = sqlite__dir_iter_next;
			backend.dir_iter_free    = sqlite__dir_iter_free;
			backend.insert           = sqlite__insert;
			backend.delete           = sqlite__delete_row;
			backend.disconnect       = sqlite__disconnect;
			backend.update_colour    = sqlite__update_colour;
			backend.update_keywords  = sqlite__update_keywords;
			backend.update_notes     = sqlite__update_notes;
			backend.update_pixbuf    = sqlite__update_pixbuf;
			backend.update_online    = sqlite__update_online;
			printf("backend is sqlite.\n");
			#endif
			break;
		case BACKEND_TRACKER:
			#ifdef USE_TRACKER
			backend.search_iter_new  = tracker__search_iter_new;
			backend.search_iter_free = tracker__search_iter_free;
			backend.dir_iter_new     = tracker__dir_iter_new;
			backend.dir_iter_next    = tracker__dir_iter_next;
			backend.dir_iter_free    = tracker__dir_iter_free;
			backend.insert           = tracker__insert;
			backend.delete           = tracker__delete_row;
			backend.update_colour    = tracker__update_colour;
			backend.update_keywords  = tracker__update_keywords;
			backend.update_pixbuf    = tracker__update_pixbuf;
			backend.update_online    = tracker__update_online;
			backend.disconnect       = tracker__disconnect;
			printf("backend is tracker.\n");

			//hide unsupported inspector notes
			GtkWidget* notes = app.inspector->text;
			if(notes) gtk_widget_hide(notes);

			#endif
			break;
		default:
			break;
	}
}


static gboolean
can_use_backend(const char* backend)
{
	GList* l = app.backends;
	for(;l;l=l->next){
		if(!strcmp(l->data, backend)){
			return true;
		}
	}
	return false;
}


