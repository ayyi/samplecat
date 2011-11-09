/*

Samplecat

Copyright (C) Tim Orford 2007-2011

This software is licensed under the GPL. See accompanying file COPYING.

*/
#define __main_c__
#include "config.h"
#define __USE_GNU
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixdata.h>

#ifdef __APPLE__
#include <gdk/gdkkeysyms.h>
#include <igemacintegration/gtkosxapplication.h>
#endif

#ifdef USE_AYYI
  #include "ayyi.h"
  #include "ayyi_model.h"
#endif
#include "file_manager.h"
#include "gqview_view_dir_tree.h"
#include "typedefs.h"
#ifdef USE_TRACKER
  #include "src/db/tracker.h"
#endif
#ifdef USE_MYSQL
  #include "db/mysql.h"
#endif
#include "mimetype.h"
#include "dh_link.h"
#include "dh_tree.h"

#include "support.h"
#include "main.h"
#include "sample.h"
#if !(defined USE_DBUS || defined USE_GAUDITION)
#include "audio.h"
#endif
#include "overview.h"
#include "cellrenderer_hypertext.h"
#include "listview.h"
#include "window.h"
#include "inspector.h"
#include "dnd.h"
#if (defined USE_DBUS || defined USE_GAUDITION)
  #include "auditioner.h"
#endif
#include "console_view.h"
#ifdef USE_GQVIEW_1_DIRTREE
  #include "filelist.h"
  #include "view_dir_list.h"
  #include "view_dir_tree.h"
#endif
#ifdef HAVE_FFTW3
#ifdef USE_OPENGL
#include "gl_spectrogram_view.h"
#else
#include "spectrogram_widget.h"
#endif
#endif

#include "audio_decoder/ad.h"

#include "pixmaps.h"

#ifdef USE_SQLITE
  #include "db/sqlite.h"
#endif

#undef DEBUG_NO_THREADS


extern void       dir_init                  ();
static void       update_row                (GtkWidget*, gpointer);
static void       edit_row                  (GtkWidget*, gpointer);
static GtkWidget* make_context_menu         ();
static gboolean   can_use_backend           (const char*);
static gboolean   toggle_recursive_add      (GtkWidget*, gpointer);
static gboolean   on_directory_list_changed ();
#ifdef NEVER
static void       on_file_moved             (GtkTreeIter);
#endif
static void      _set_search_dir            (char*);
static gboolean   dir_tree_update           (gpointer);
static bool       config_load               ();
static void       config_new                ();
static bool       config_save               ();
static void       menu_delete_row           (GtkMenuItem*, gpointer);


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

	app.config_filename = g_strdup_printf("%s/.config/" PACKAGE "/" PACKAGE, g_get_home_dir());
	app.cache_dir = g_build_filename(g_get_home_dir(), ".config", PACKAGE, "cache", NULL);
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
	ad_init();

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
				#ifdef USE_AYYI
				ayyi.debug = debug;
				#endif
				break;
			case 'b':
				//if a particular backend is requested, and is available, reduce the backend list to just this one.
				dbg(1, "backend '%s' requested.", optarg);
				if(can_use_backend(optarg)){
					list_clear(app.backends);
					ADD_BACKEND(optarg);
					dbg(1, "n_backends=%i", g_list_length(app.backends));
				}
				else{
					warnprintf("requested backend not available: '%s'\navailable backends:\n", optarg);
					GList* l = app.backends;
					for(;l;l=l->next){
						printf("  %s\n", (char*)l->data);
					}
				}
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

#ifdef __APPLE__
	GtkOSXApplication *osxApp = (GtkOSXApplication*) 
	g_object_new(GTK_TYPE_OSX_APPLICATION, NULL);
#endif

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
				do_search(app.args.search ? app.args.search : app.search_phrase, app.search_dir);
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

	app.store = listmodel__new();

	if(!app.no_gui) window_new(); 
	if(!app.no_gui) app.context_menu = make_context_menu();

#ifdef __APPLE__
	gtk_window_set_title(GTK_WINDOW(app.window), "SampleCat");

	GtkWidget *menu_bar;
	menu_bar = gtk_menu_bar_new();
	/* Note: the default OSX menu bar already includes a 'quit' entry 
	 * connected to 'gtk_main_quit' by default. so we're fine.
	 */
	gtk_osxapplication_set_menu_bar(osxApp, GTK_MENU_SHELL(menu_bar));
#endif

	if(!backend.pending){ 
		do_search(app.args.search ? app.args.search : app.search_phrase, app.search_dir);
	}else{
		search_pending = true;
	}

	if(app.no_gui) exit(EXIT_SUCCESS);

#if (defined USE_DBUS || defined USE_GAUDITION)
	auditioner_connect();
#endif

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

#ifdef __APPLE__
	gtk_osxapplication_ready(osxApp);
#endif
	gtk_main();

#if (defined USE_DBUS || defined USE_GAUDITION)
	auditioner_stop(NULL);
#endif
	exit(EXIT_SUCCESS);
}


void
update_search_dir(gchar* uri)
{
	_set_search_dir(uri);

	const gchar* text = app.search ? gtk_entry_get_text(GTK_ENTRY(app.search)) : "";
	do_search((gchar*)text, uri);
}


static gboolean
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


static void
_set_search_dir(char* dir)
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
add_dir(const char* path, int* added_count)
{
	/*
	scan the directory and try and add any files we find.
	
	*/
	PF;

	char filepath[256];
	G_CONST_RETURN gchar *file;
	GError* error = NULL;
	GDir* dir;
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
				add_dir(filepath, added_count);
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
do_search(char* search, char* dir)
{
	//fill the display with the results for the given search phrase.

	PF;

	if(BACKEND_IS_NULL) return;
	search_pending = false;

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
			unsigned long* lengths;
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
			}else if(!row_count){
				statusbar_print(1, "no samples found. filters: dir=%s", app.search_dir);
			}else{
				statusbar_print(1, "showing %i samples", row_count);
			}
		}
		#endif

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


static void
create_nodes_for_path(gchar** path)
{
	gchar* make_uri(gchar** path, int n)
	{
		//result must be freed by caller

		gchar* a = NULL;
		int k; for(k=0;k<=n;k++){
			gchar* b = g_strjoin("/", a, path[k], NULL);
			if(a) g_free(a);
			a = b;
		}
		dbg(2, "uri=%s", a);
		return a;
	}

	GNode* node = app.dir_tree;
	int i; for(i=0;path[i];i++){
		if(strlen(path[i])){
			//dbg(0, "  testing %s ...", path[i]);
			gboolean found = false;
			GNode* n = g_node_first_child(node);
			if(n) do {
				DhLink* link = n->data;
				if(link){
					//dbg(0, "  %s / %s", path[i], link->name);
					if(!strcmp(link->name, path[i])){
						//dbg(0, "    found. node=%p", n);
						node = n;
						found = true;
						break;
					}
				}
			} while ((n = g_node_next_sibling(n)));

			if(!found){
				dbg(2, "  not found. inserting... (%s)", path[i]);
				gchar* uri = make_uri(path, i);
				if(strcmp(uri, g_get_home_dir())){
					DhLink* l = dh_link_new(DH_LINK_TYPE_PAGE, path[i], uri);
					g_free(uri);
					GNode* leaf = g_node_new(l);

					gint position = -1; //end
					g_node_insert(node, position, leaf);
					node = leaf;
				}
			}
		}
	}
}


void
update_dir_node_list()
{
	/*
	builds a node list of directories listed in the latest database result.
	*/

	if(BACKEND_IS_NULL) return;

	backend.dir_iter_new();

	if(app.dir_tree) g_node_destroy(app.dir_tree);
	app.dir_tree = g_node_new(NULL);

	DhLink* link = dh_link_new(DH_LINK_TYPE_PAGE, "all directories", "");
	GNode* leaf = g_node_new(link);
	g_node_insert(app.dir_tree, -1, leaf);

	GNode* lookup_node_by_path(gchar** path)
	{
		//find a node containing the given path

		//dbg(0, "path=%s", path[0]);
		struct _closure {
			gchar** path;
			GNode*  insert_at;
		};
		gboolean traverse_func(GNode *node, gpointer data)
		{
			//test if the given node is a match for the closure path.

			DhLink* link = node->data;
			struct _closure* c = data;
			gchar** path = c->path;
			//dbg(0, "%s", link ? link->name : NULL);
			if(link){
				c->insert_at = node;
				//dbg(0, "%s 0=%s", link->name, path[0]);
				gchar** v = g_strsplit(link->name, "/", 64);
				int i; for(i=0;v[i];i++){
					if(strlen(v[i])){
						if(!strcmp(v[i], path[i])){
							//dbg(0, "found! %s at depth %i", v[i], i);
							//dbg(0, " %s", v[i]);
						}else{
							//dbg(0, "no match. failed at %i: %s", i, path[i]);
							c->insert_at = NULL;
							break;
						}
					}
				}
				g_strfreev(v);
			}
			return false; //continue
		}

		struct _closure* c = g_new0(struct _closure, 1);
		c->path = path;

		g_node_traverse(app.dir_tree, G_IN_ORDER, G_TRAVERSE_ALL, 64, traverse_func, c);
		GNode* ret = c->insert_at;
		g_free(c);
		return ret;
	}

	char* u;
	while((u = backend.dir_iter_next())){

		dbg(2, "%s", u);
		gchar** v = g_strsplit(u, "/", 64);
		int i = 0;
		while(v[i]){
			//dbg(0, "%s", v[i]);
			i++;
		}
		GNode* node = lookup_node_by_path(v);
		if(node){
			dbg(3, "node found. not adding.");
		}else{
			create_nodes_for_path(v);
		}
		g_strfreev(v);
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
	g_return_val_if_fail(mime_type, true);

	/* XXX - actually ffmpeg can read audio-tracks in video-files,
	 * application/ogg, application/annodex, application/zip may contain audio
	 * ...
	 */
	if(strcmp(mime_type->media_type, "audio")){
		return true;
	}

	char unsupported[][64] = {
		"audio/csound", 
		"audio/midi", 
		"audio/prs.sid",
		"audio/telephone-event",
		"audio/tone",
		"audio/x-tta", 
		"audio/x-speex",
		"audio/x-musepack"
	};
	int i;
	for(i=0;i<G_N_ELEMENTS(unsupported);i++){
		if(!strcmp(mime_string, unsupported[i])){
			return true;
		}
	}
	dbg(2, "mimetype ok: %s", mime_string);
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

	Sample* sample = sample_new(); //free'd after db and store are updated.
	g_strlcpy(sample->filename, path, 256);

	gchar* filedir = g_path_get_dirname(path);
	gchar* filename = g_path_get_basename(path);

	MIME_type* mime_type = type_from_path(filename);
	char mime_string[64];
	snprintf(mime_string, 64, "%s/%s", mime_type->media_type, mime_type->subtype);

	if(mimetype_is_unsupported(mime_type, mime_string)){
		printf("cannot add file: file type \"%s\" not supported.\n", mime_string);
		statusbar_print(1, "cannot add file: %s files not supported", mime_string);
		ok = false;
		goto out;
	}

	if(sample_get_file_info(sample)){

		sample->id = backend.insert(sample, mime_type);

		listview__add_item(sample);

		request_overview(sample);
		request_peaklevel(sample);

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
on_overview_done(gpointer _sample)
{
	PF;
	Sample* sample = _sample;
	g_return_val_if_fail(sample, false);
	if(!sample->pixbuf){ dbg(1, "overview creation failed (no pixbuf).\n"); return false; }

	backend.update_pixbuf(sample);

	if(sample->row_ref){
		listmodel__set_overview(sample->row_ref, sample->pixbuf);
	} else pwarn("rowref not set!\n");

	sample_unref(sample);
	return IDLE_STOP;
}


gboolean
on_peaklevel_done(gpointer _sample)
{
	Sample* sample = _sample;
	dbg(1, "peaklevel=%.2f id=%i rowref=%p", sample->peak_level, sample->id, sample->row_ref);

	backend.update_peaklevel(sample->id, sample->peak_level);

	if(sample->row_ref){
		listmodel__set_peaklevel(sample->row_ref, sample->peak_level);
	} else pwarn("rowref not set!\n");

	sample_unref(sample);
	return IDLE_STOP;
}


void
delete_selected_rows()
{
	GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(app.view));

	GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(app.view));
	GList* selectionlist = gtk_tree_selection_get_selected_rows(selection, &(model));
	if(!selectionlist){ perr("no files selected?\n"); return; }
	dbg(1, "%i rows selected.", g_list_length(selectionlist));

	GList* selected_row_refs = NULL;

	//get row refs for each selected row:
	GList* l = selectionlist;
	for(;l;l=l->next){
		GtkTreePath* treepath_selection = l->data;

		GtkTreeRowReference* row_ref = gtk_tree_row_reference_new(GTK_TREE_MODEL(app.store), treepath_selection);
		selected_row_refs = g_list_prepend(selected_row_refs, row_ref);
	}
	g_list_free(selectionlist);

	int n = 0;
	GtkTreePath* path;
	GtkTreeIter iter;
	l = selected_row_refs;
	for(;l;l=l->next){
		GtkTreeRowReference* row_ref = l->data;
		if((path = gtk_tree_row_reference_get_path(row_ref))){

			if(gtk_tree_model_get_iter(model, &iter, path)){
				gchar *fname;
				int id;
				gtk_tree_model_get(model, &iter, COL_NAME, &fname, COL_IDX, &id, -1);

				if(!backend.delete(id)) return;

				//update the store:
				gtk_list_store_remove(app.store, &iter);
				n++;

			} else perr("bad iter!\n");
		} else perr("cannot get path from row_ref!\n");
	}
	g_list_free(selected_row_refs); //FIXME free the row_refs?

	statusbar_print(1, "%i rows deleted", n);
	on_directory_list_changed();
}


static void
menu_delete_row(GtkMenuItem* widget, gpointer user_data)
{
	delete_selected_rows();
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

	int i, id; gchar *fname; gchar *fdir; gchar *mimetype; float peak_level;
	GdkPixbuf* iconbuf;
	gboolean online;
	for(i=0;i<g_list_length(selectionlist);i++){
		GtkTreePath *treepath = g_list_nth_data(selectionlist, i);

		gtk_tree_model_get_iter(model, &iter, treepath);
		gtk_tree_model_get(model, &iter, COL_NAME, &fname, COL_FNAME, &fdir, COL_MIMETYPE, &mimetype, COL_IDX, &id, COL_PEAKLEVEL, &peak_level, -1);

		//gchar path[256];
		//snprintf(path, 256, "%s/%s", fdir, fname);
		gchar* path = g_strdup_printf("%s/%s", fdir, fname);
		if(file_exists(path)){
			online = 1;

			MIME_type* mime_type = mime_type_lookup(mimetype);
			type_to_icon(mime_type);
			if (!mime_type->image) dbg(0, "no icon.");
			iconbuf = mime_type->image->sm_pixbuf;

			dbg(0, "peak_level=%.2f", peak_level);
			if(peak_level < 0.00001){
				//TODO check dates first?
				dbg(0, "no peak level. regenerating...");
				Sample* sample = sample_new_from_model(treepath);
				//TODO need to update inspector once finished
				request_peaklevel(sample);
			}
		}else{
			online = 0;
			iconbuf = NULL;
		}
		gtk_list_store_set(app.store, &iter, COL_ICON, iconbuf, -1);

		if(backend.update_online(id, online)){
			statusbar_print(1, "online status updated (%s)", online ? "online" : "not online");
		}else
			statusbar_print(1, "error! online status not updated");

		g_free(path);
	}
	g_list_free(selectionlist);
	//g_date_free(date);
}


static void
menu_play_all(GtkWidget* widget, gpointer user_data)
{
	dbg(0, "...");
#if (defined USE_DBUS || defined USE_GAUDITION)
	auditioner_play_all();
#endif
}


static void
menu_play_stop(GtkWidget* widget, gpointer user_data)
{
	dbg(0, "...");
#if (defined USE_DBUS || defined USE_GAUDITION)
	auditioner_stop(NULL);
#endif
}


static void
edit_row(GtkWidget* widget, gpointer user_data)
{
	//currently this only works for the The tags cell.	
	PF;
	GtkTreeView* treeview = GTK_TREE_VIEW(app.view);

	GtkTreeSelection* selection = gtk_tree_view_get_selection(treeview);
	if(!selection){ perr("cannot get selection.\n");/* return;*/ }
	GtkTreeModel* model = GTK_TREE_MODEL(app.store);
	GList* selectionlist = gtk_tree_selection_get_selected_rows(selection, &(model));
	if(!selectionlist){ perr("no files selected?\n"); return; }

	GtkTreePath* treepath;
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
	{"Delete",         G_CALLBACK(menu_delete_row), GTK_STOCK_DELETE,     true},
	{"Update",         G_CALLBACK(update_row),      GTK_STOCK_REFRESH,    true},
	{"Play All",       G_CALLBACK(menu_play_all),   GTK_STOCK_MEDIA_PLAY, true},
	{"Stop Playback",  G_CALLBACK(menu_play_stop),  GTK_STOCK_MEDIA_STOP, true},
	{"Edit tags",      G_CALLBACK(edit_row),   GTK_STOCK_EDIT,        true},
	{"Open",           G_CALLBACK(edit_row),   GTK_STOCK_OPEN,       false},
	{"Open Directory", G_CALLBACK(NULL),       GTK_STOCK_OPEN,        true},
	{"",                                                                  },
	{"View",           G_CALLBACK(NULL),       GTK_STOCK_PREFERENCES, true},
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

	{
		GList* last = g_list_last(menu_items);
		GList* prev = last->prev;
		GtkWidget* view = prev->data;

		GtkWidget* sub = gtk_menu_new();
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(view), sub);

		void set_view_toggle_state(GtkMenuItem* item, struct _view_option* option)
		{
			option->value = !option->value;
			gulong sig_id = g_signal_handler_find(item, G_SIGNAL_MATCH_FUNC, 0, 0, 0, option->on_toggle, NULL);
			if(sig_id){
				g_signal_handler_block(item, sig_id);
				gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), option->value);
				g_signal_handler_unblock(item, sig_id);
			}
			dbg(2, "value=%i", option->value);
		}

		void toggle_view_spectrogram(GtkMenuItem* item, gpointer userdata)
		{
			struct _view_option* option = &app.view_options[SHOW_SPECTROGRAM];
			option->value = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(item));
			dbg(2, "on=%i", option->value);

			show_spectrogram(option->value);
		}

		void toggle_view_filemanager(GtkMenuItem* item, gpointer userdata)
		{
			PF;
			struct _view_option* option = &app.view_options[SHOW_SPECTROGRAM];
			option->value = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(item));
			show_widget_if(app.fm_view, option->value);
			show_widget_if(app.dir_treeview2->widget, option->value);
		}

		struct _view_option* option = &app.view_options[SHOW_FILEMANAGER];
		option->name = "Filemanager";
		option->value = true;
		option->on_toggle = toggle_view_filemanager;

		option = &app.view_options[SHOW_SPECTROGRAM];
		option->name = "Spectrogram";
		option->on_toggle = toggle_view_spectrogram;

		int i; for(i=0;i<MAX_VIEW_OPTIONS;i++){
			struct _view_option* option = &app.view_options[i];
			GtkWidget* menu_item = gtk_check_menu_item_new_with_mnemonic(option->name);
			gtk_menu_shell_append(GTK_MENU_SHELL(sub), menu_item);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item), option->value);
			option->value = !option->value; //toggle before it gets toggled back.
			set_view_toggle_state((GtkMenuItem*)menu_item, option);
			g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(option->on_toggle), NULL);
		}
	}

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


#if 0
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
#endif


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
			dbg(1, "'%s' already in string '%s'", new, existing);
			found = true;
			break;
		}
		word_index++;
	}

	g_strfreev(split);
	return found;
}


static gboolean
on_directory_list_changed()
{
	/*
	the database has been modified, the directory list may have changed.
	Update all directory views. Atm there is only one.
	*/

	g_idle_add(dir_tree_update, NULL);
	return false;
}


static gboolean
toggle_recursive_add(GtkWidget *widget, gpointer user_data)
{
	PF;
	if(app.add_recursive) app.add_recursive = false; else app.add_recursive = true;
	return false;
}


#ifdef NEVER
static void
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
#endif


extern char theme_name[];

static bool
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

			#define num_keys 11
			//FIXME 64 is not long enough for directories
			char keys[num_keys][64] = {"database_host",          "database_user",          "database_pass",          "database_name",          "show_dir",          "window_height",          "window_width",         "icon_theme", "col1_width",                "filter",          "browse_dir"};
			char* loc[num_keys]     = {app.config.database_host, app.config.database_user, app.config.database_pass, app.config.database_name, app.config.show_dir, app.config.window_height, app.config.window_width, theme_name,  app.config.column_widths[0], app.search_phrase, app.config.browse_dir};

			int k;
			gchar* keyval;
			for(k=0;k<num_keys;k++){
				if((keyval = g_key_file_get_string(app.key_file, groupname, keys[k], &error))){
					snprintf(loc[k], 64, "%s", keyval);
					dbg(2, "%s=%s", keys[k], keyval);
					g_free(keyval);
				}else{
					if(error->code == 3) g_error_clear(error)
					else { GERR_WARN; }
					strcpy(loc[k], "");
				}
			}
			_set_search_dir(app.config.show_dir);
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


static bool
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

		AyyiLibfilemanager* fm = file_manager__get_signaller();
		struct _Filer* f = fm->file_window;
		if(f){
			g_key_file_set_value(app.key_file, "Samplecat", "browse_dir", f->real_path);
		}
	}

	GError *error = NULL;
	gsize length;
	gchar* string = g_key_file_to_data(app.key_file, &length, &error);
	if(error){
		dbg (0, "error saving config file: %s", error->message);
		g_error_free(error);
	}

	if(ensure_config_dir()){

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
	}
	else errprintf("cannot create config directory.");
	g_free(string);
	return true;
}


static void
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
on_quit(GtkMenuItem* menuitem, gpointer user_data)
{
	//if(user_data) dbg(0, "exitcode=%i", GPOINTER_TO_INT(user_data));
	int exit_code = GPOINTER_TO_INT(user_data);
	if(exit_code > 1) exit_code = 0; //ignore invalid exit code.

#if !(defined USE_DBUS || defined USE_GAUDITION)
	playback_stop();
#endif

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
			backend.update_peaklevel = mysql__update_peaklevel;
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
			backend.update_peaklevel = sqlite__update_peaklevel;
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
			backend.update_peaklevel = tracker__update_peaklevel;
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


void
observer__item_selected(Result* result)
{
	//TODO move these to idle functions

	inspector_update_from_result(result);

#ifdef HAVE_FFTW3
	if(app.spectrogram){
		char* path = g_strdup_printf("%s/%s", result->dir, result->sample_name);
#ifdef USE_OPENGL
		gl_spectrogram_set_file((GlSpectrogram*)app.spectrogram, path);
#else
		spectrogram_widget_set_file ((SpectrogramWidget*)app.spectrogram, path);
#endif
		g_free(path);
	}
#endif
}


