/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2007-2014 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#define __main_c__
#include "config.h"
#define __USE_GNU
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <getopt.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixdata.h>

#ifdef __APPLE__
#include <gdk/gdkkeysyms.h>
#include <igemacintegration/gtkosxapplication.h>
char * program_name;
#endif

#include "debug/debug.h"
#include "utils/ayyi_utils.h"
#include "utils/pixmaps.h"
#ifdef USE_AYYI
  #include "ayyi.h"
  #include "ayyi_model.h"
#endif
#include "file_manager.h"
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

#include "db/db.h"
#include "model.h"
#include "main.h"
#include "list_store.h"
#include "support.h"
#include "sample.h"
#include "overview.h"
#include "cellrenderer_hypertext.h"
#include "listview.h"
#include "window.h"
#include "colour_box.h"
#include "inspector.h"
#include "progress_dialog.h"
#include "player_control.h"
#include "dnd.h"
#include "icon_theme.h"
#include "application.h"
#ifdef HAVE_AYYIDBUS
  #include "auditioner.h"
#endif
#ifdef HAVE_JACK
  #include "jack_player.h"
#endif
#ifdef HAVE_GPLAYER
  #include "gplayer.h"
#endif
#include "console_view.h"
#ifdef USE_GQVIEW_1_DIRTREE
  #include "filelist.h"
  #include "view_dir_list.h"
  #include "view_dir_tree.h"
#endif

#include "audio_decoder/ad.h"

#ifdef USE_SQLITE
  #include "db/sqlite.h"
#endif

#undef DEBUG_NO_THREADS


static void       update_rows               (GtkWidget*, gpointer);
static GtkWidget* make_context_menu         ();
static gboolean   can_use                   (GList*, const char*);
static gboolean   toggle_recursive_add      (GtkWidget*, gpointer);
static gboolean   toggle_loop_playback      (GtkWidget*, gpointer);
static gboolean   on_directory_list_changed ();
static gboolean   dir_tree_update           (gpointer);
static bool       config_load               ();
static void       config_new                ();
static bool       config_save               ();
static void       menu_delete_row           (GtkMenuItem*, gpointer);
void              menu_play_stop            (GtkWidget*, gpointer);


Application*     app = NULL;
SamplecatBackend backend; 
Palette          palette;
GList*           mime_types; // list of MIME_type*
extern GList*    themes; 

static gboolean  search_pending = false;

//strings for console output:
char white [16];
char red   [16];
char green [16];
char yellow[16];
char bold  [16];

static const struct option long_options[] = {
  { "backend",          1, NULL, 'b' },
  { "player",           1, NULL, 'p' },
  { "no-gui",           0, NULL, 'g' },
  { "verbose",          1, NULL, 'v' },
  { "search",           1, NULL, 's' },
  { "add",              1, NULL, 'a' },
  { "help",             0, NULL, 'h' },
  { "version",          0, NULL, 'V' },
};

static const char* const short_options = "b:gv:s:a:p:hV";

static const char* const usage =
	"Usage: %s [OPTION]\n\n"
	"SampleCat is a program for cataloguing and auditioning audio samples.\n" 
	"\n"
	"Options:\n"
	"  -a, --add <file(s)>    add these files.\n"
	"  -b, --backend <name>   select which database type to use.\n"
	"  -g, --no-gui           run as command line app.\n"
	"  -h, --help             show this usage information and quit.\n"
	"  -p, --player <name>    select audio player.\n"
	"  -s, --search <txt>     search using this phrase.\n"
	"  -v, --verbose <level>  show more information.\n"
	"  -V, --version          print version and exit.\n"
	"\n"
	"Keyboard shortcuts:\n"
	"  a      add file to library\n"
	"  DEL    delete library item\n"
	"  p      play\n"
	"  CTL-l  open layout manager\n"
	"  CTL-q  quit\n"
	"  CTL-w  quit\n"
	"  +      waveform zoom in\n"
	"  -      waveform zoom out\n"
	"  LEFT   waveform scroll left\n"
	"  RIGHT  waveform scroll right\n"
	"\n"
	"Files:\n"
	"  samplecat stores configuration and caches data in\n"
	"  $HOME/.config/samplecat/\n"
	"\n"
	"Report bugs to <tim@orford.org>.\n"
	"Website and manual: <http://ayyi.github.io/samplecat/>\n"
	"\n";

int 
main(int argc, char** argv)
{
#ifdef __APPLE__
	program_name = argv[0];
#endif
	//init console escape commands:
	sprintf(white,  "%c[0;39m", 0x1b);
	sprintf(red,    "%c[1;31m", 0x1b);
	sprintf(green,  "%c[1;32m", 0x1b);
	sprintf(yellow, "%c[1;33m", 0x1b);
	sprintf(bold,   "%c[1;39m", 0x1b);
	sprintf(err,    "%serror!%s", red, white);
	sprintf(warn,   "%swarning:%s", yellow, white);

	g_log_set_default_handler(log_handler, NULL);

	if (!g_thread_supported()) g_thread_init(NULL);
	gdk_threads_init();
	gtk_init_check(&argc, &argv);

	app = application_new();
	colour_box_init();
	memset(&backend, 0, sizeof(struct _backend)); 

#define ADD_BACKEND(A) app->backends = g_list_append(app->backends, A)

#ifdef USE_MYSQL
	ADD_BACKEND("mysql");
#endif
#ifdef USE_SQLITE
	ADD_BACKEND("sqlite");
#endif
#ifdef USE_TRACKER
	ADD_BACKEND("tracker");
#endif

#define ADD_PLAYER(A) app->players = g_list_append(app->players, A)

#ifdef HAVE_JACK
	ADD_PLAYER("jack");
#endif
#ifdef HAVE_AYYIDBUS
	ADD_PLAYER("ayyi");
#endif
#ifdef HAVE_GPLAYER
	ADD_PLAYER("cli");
#endif
	ADD_PLAYER("null");

	gboolean player_opt = false;
	int opt;
	while((opt = getopt_long (argc, argv, short_options, long_options, NULL)) != -1) {
		switch(opt) {
			case 'v':
				printf("using debug level: %s\n", optarg);
				int d = atoi(optarg);
				if(d<0 || d>5) { gwarn ("bad arg. debug=%i", d); } else _debug_ = d;
				#ifdef USE_AYYI
				ayyi.debug = _debug_;
				#endif
				break;
			case 'b':
				//if a particular backend is requested, and is available, reduce the backend list to just this one.
				dbg(1, "backend '%s' requested.", optarg);
				if(can_use(app->backends, optarg)){
					list_clear(app->backends);
					ADD_BACKEND(optarg);
					dbg(1, "n_backends=%i", g_list_length(app->backends));
				}
				else{
					warnprintf("requested backend not available: '%s'\navailable backends:\n", optarg);
					GList* l = app->backends;
					for(;l;l=l->next){
						printf("  %s\n", (char*)l->data);
					}
					exit(EXIT_FAILURE);
				}
				break;
			case 'p':
				if(can_use(app->players, optarg)){
					list_clear(app->players);
					ADD_PLAYER(optarg);
					player_opt=true;
				} else{
					warnprintf("requested player is not available: '%s'\navailable backends:\n", optarg);
					GList* l = app->players;
					for(;l;l=l->next) printf("  %s\n", (char*)l->data);
					exit(EXIT_FAILURE);
				}
				break;
			case 'g':
				app->no_gui = true;
				break;
			case 'h':
				printf(usage, basename(argv[0]));
				exit(EXIT_SUCCESS);
				break;
			case 's':
				printf("search=%s\n", optarg);
				app->args.search = g_strdup(optarg);
				break;
			case 'a':
				printf("add=%s\n", optarg);
				app->args.add = g_strdup(optarg);
				break;
			case 'V':
				printf ("%s %s\n\n",basename(argv[0]), PACKAGE_VERSION);
				printf(
					"Copyright (C) 2007-2014 Tim Orford\n"
					"Copyright (C) 2011 Robin Gareus\n"
					"This is free software; see the source for copying conditions.  There is NO\n"
					"warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\n"
					);
				exit(EXIT_SUCCESS);
				break;
			case '?':
			default:
				printf("unknown option: %c\n", optopt);
				printf(usage, basename(argv[0]));
				exit(EXIT_FAILURE);
				break;
		}
	}

	printf("%s"PACKAGE_NAME". Version "PACKAGE_VERSION"%s\n", yellow, white);

	type_init();
	app->model = samplecat_model_new();

	config_load();

	if (app->config.database_backend && can_use(app->backends, app->config.database_backend)) {
		list_clear(app->backends);
		ADD_BACKEND(app->config.database_backend);
	}

	if (!player_opt && app->config.auditioner) {
		if(can_use(app->players, app->config.auditioner)){
			list_clear(app->players);
			ADD_PLAYER(app->config.auditioner);
		}
	}

#ifdef __APPLE__
	GtkOSXApplication *osxApp = (GtkOSXApplication*) 
	g_object_new(GTK_TYPE_OSX_APPLICATION, NULL);
#endif
	app->gui_thread = pthread_self();

	icon_theme_init();
	pixmaps_init();
	ad_init();
	set_auditioner();
	app->store = listmodel__new();
	if(app->no_gui) console__init();

#if 0
	void store_content_changed(GtkListStore* store, gpointer data)
	{
		PF0;
	}
	g_signal_connect(G_OBJECT(app->store), "content-changed", G_CALLBACK(store_content_changed), NULL);
#endif

	gboolean db_connected = false;
#ifdef USE_MYSQL
	if(can_use(app->backends, "mysql")){
		mysql__init(app->model, &app->config.mysql);
		db_connected = samplecat_set_backend(BACKEND_MYSQL);
	}
#endif
#ifdef USE_SQLITE
	if(!db_connected && can_use(app->backends, "sqlite") && ensure_config_dir()){
		if(sqlite__connect()){
			db_connected = samplecat_set_backend(BACKEND_SQLITE);
		}
	}
#endif

	if (!db_connected) {
		g_warning("cannot connect to any database.\n");
#ifdef QUIT_WITHOUT_DB
		on_quit(NULL, GINT_TO_POINTER(EXIT_FAILURE));
#endif
	}

#ifdef USE_TRACKER
	if(BACKEND_IS_NULL && can_use(app->backends, "tracker")){
		void on_tracker_init()
		{
			dbg(2, "...");
			samplecat_set_backend(BACKEND_TRACKER);

			//hide unsupported inspector notes
			GtkWidget* notes = app->inspector->text;
#if 1 
			// may not work -- it could re-appear ?! show_fields()??
			if(notes) gtk_widget_hide(notes);
#else // THIS NEEDS TESTING:
			g_object_ref(notes); //stops gtk deleting the unparented widget.
			gtk_container_remove(GTK_CONTAINER(notes->parent), notes);
#endif

			if(notes) gtk_widget_hide(notes);

			if(search_pending){
				do_search(app->args.search ? app->args.search : app->model->filters.phrase, app->model->filters.dir);
			}
		}
		if(app->no_gui){
			tracker__init(on_tracker_init);
		}
		else {
			backend.pending = true;
			g_idle_add_full(G_PRIORITY_LOW, (GSourceFunc)tracker__init, on_tracker_init, NULL);
		}
	}
#endif

	if(app->args.add){
		/* initial import from commandline */
		do_progress(0, 0);
		add_file(app->args.add);
		hide_progress();
	}

#ifndef DEBUG_NO_THREADS
	dbg(3, "creating overview thread...");
	GError* error = NULL;
	app->msg_queue = g_async_queue_new();
	if(!g_thread_create(overview_thread, NULL, false, &error)){
		perr("error creating thread: %s\n", error->message);
		g_error_free(error);
	}
#endif

	if(!app->no_gui) window_new(); 
	if(!app->no_gui) app->context_menu = make_context_menu();

#ifdef __APPLE__
	GtkWidget* menu_bar;
	menu_bar = gtk_menu_bar_new();
	/* Note: the default OSX menu bar already includes a 'quit' entry 
	 * connected to 'gtk_main_quit' by default. so we're fine.
	 */
	gtk_osxapplication_set_menu_bar(osxApp, GTK_MENU_SHELL(menu_bar));
#endif

	if(!backend.pending){ 
		do_search(app->args.search ? app->args.search : app->model->filters.phrase, app->model->filters.dir);
	}else{
		search_pending = true;
	}

	if(app->no_gui) exit(EXIT_SUCCESS);

	app->auditioner->connect();

#ifdef USE_AYYI
	ayyi_client_init();
	g_idle_add((GSourceFunc)ayyi_connect, NULL);
#endif

#ifdef USE_TRACKER
	if(!app->no_gui && BACKEND_IS_NULL) g_idle_add((GSourceFunc)tracker__init, NULL);
#else
	//if(BACKEND_IS_NULL) listview__show_db_missing();
#endif

	app->loaded = true;
	dbg(1, "loaded");
#ifndef USE_GDL
	message_panel__add_msg("hello", GTK_STOCK_INFO);
#endif
	statusbar_print(2, PACKAGE_NAME". Version "PACKAGE_VERSION);

#ifdef __APPLE__
	gtk_osxapplication_ready(osxApp);
#endif
	gtk_main();

	menu_play_stop(NULL,NULL);
	app->auditioner->disconnect();

	exit(EXIT_SUCCESS);
}


static gboolean
dir_tree_update(gpointer data)
{
	/*
	refresh the directory tree. Called from an idle.

	note: destroying the whole node tree is wasteful - can we just make modifications to it?

	*/
	update_dir_node_list();

	dh_book_tree_reload((DhBookTree*)app->dir_treeview);

	return IDLE_STOP;
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

	char filepath[PATH_MAX];
	G_CONST_RETURN gchar *file;
	GError* error = NULL;
	GDir* dir;
	if((dir = g_dir_open(path, 0, &error))){
		while((file = g_dir_read_name(dir))){
			if(file[0]=='.') continue;
			snprintf(filepath, PATH_MAX, "%s%c%s", path, G_DIR_SEPARATOR, file);
			filepath[PATH_MAX-1]='\0';
			if (do_progress(0,0)) break;

			if(!g_file_test(filepath, G_FILE_TEST_IS_DIR)){
				if(add_file(filepath)) (*added_count)++;
				statusbar_print(1, "%i files added", *added_count);
			}
			// IS_DIR
			else if(app->add_recursive){
				add_dir(filepath, added_count);
			}
		}
		//hide_progress(); ///no: keep window open until last recursion.
		g_dir_close(dir);
	}else{
		perr("cannot open directory. %s\n", error->message);
		g_error_free(error);
		error = NULL;
	}
}

/**
 * fill the display with the results for the given search phrase.
 */
void
do_search(char* search, char* dir)
{
	PF;

	if(BACKEND_IS_NULL) return;
	search_pending = false;

	if (search) g_strlcpy(app->model->filters.phrase, search, 256); //the search phrase is now always taken from the model.
	if (!dir) dir = app->model->filters.dir;

	int n_results = 0;
	if(!backend.search_iter_new(search, dir, app->model->filters.category, &n_results)) {
		return;
	}

	if(app->libraryview)
		listview__block_motion_handler(); // TODO make private to listview.

	listmodel__clear();

	int row_count = 0;
	unsigned long* lengths;
	Sample* result;
	while((result = backend.search_iter_next(&lengths)) && row_count < MAX_DISPLAY_ROWS){
		Sample* s = sample_dup(result);
		//listmodel__add_result(s);
		samplecat_list_store_add((SamplecatListStore*)app->store, s);
		sample_unref(s);
		row_count++;
	}

	backend.search_iter_free();

	if(0 && row_count < MAX_DISPLAY_ROWS){
		statusbar_print(1, "%i samples found.", row_count);
	}else if(!row_count){
		statusbar_print(1, "no samples found. filters: dir=%s", app->model->filters.dir);
	}else if (n_results <0) {
		statusbar_print(1, "showing %i sample(s)", row_count);
	}else{
		statusbar_print(1, "showing %i of %i sample(s)", row_count, n_results);
	}

	samplecat_list_store_do_search((SamplecatListStore*)app->store);
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

	GNode* node = app->dir_tree;
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
					GNode* leaf = g_node_new(l);

					gint position = -1; //end
					g_node_insert(node, position, leaf);
					node = leaf;
				}
				if (uri) g_free(uri);
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

	if(app->dir_tree) g_node_destroy(app->dir_tree);
	app->dir_tree = g_node_new(NULL);

	DhLink* link = dh_link_new(DH_LINK_TYPE_PAGE, "all directories", "");
	GNode* leaf = g_node_new(link);
	g_node_insert(app->dir_tree, -1, leaf);

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

		g_node_traverse(app->dir_tree, G_IN_ORDER, G_TRAVERSE_ALL, 64, traverse_func, c);
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
	dbg(2, "size=%i", g_node_n_children(app->dir_tree));
}



gboolean
add_file(char* path)
{
	/*
	uri must be "unescaped" before calling this fn. Method string must be removed.
	*/

#if 1
	/* check if file already exists in the store
	 * -> don't add it again
	 */
	if(backend.file_exists(path, NULL)) {
		statusbar_print(1, "duplicate: not re-adding a file already in db.");
		gwarn("duplicate file: %s", path);
		//TODO ask what to do
		Sample* s = sample_get_by_filename(path);
		if (s) {
			sample_refresh(s, false);
			sample_unref(s);
		} else {
			dbg(0, "Sample found in DB but not in model.");
		}
		return false;
	}
#endif

	dbg(1, "%s", path);
	if(BACKEND_IS_NULL) return false;

	Sample* sample = sample_new_from_filename(path, false);
	if (!sample) {
		gwarn("cannot add file: file-type is not supported");
		statusbar_print(1, "cannot add file: file-type is not supported");
		return false;
	}

	if(!sample_get_file_info(sample)){
		gwarn("cannot add file: reading file info failed");
		statusbar_print(1, "cannot add file: reading file info failed");
		sample_unref(sample);
		return false;
	}
#if 1
	/* check if /same/ file already exists w/ different path */
	GList* existing;
	if((existing = backend.filter_by_audio(sample))) {
		GList* l = existing; int i;
#ifdef INTERACTIVE_IMPORT
		GString* note = g_string_new("Similar or identical file(s) already present in database:\n");
#endif
		for(i=1;l;l=l->next, i++){
			/* TODO :prompt user: ask to delete one of the files
			 * - import/update the remaining file(s)
			 */
			dbg(0, "found similar or identical file: %s", l->data);
#ifdef INTERACTIVE_IMPORT
			if (i < 10)
				g_string_append_printf(note, "%d: '%s'\n", i, (char*) l->data);
#endif
		}
#ifdef INTERACTIVE_IMPORT
		if (i>9)
			g_string_append_printf(note, "..\n and %d more.", i - 9);
		g_string_append_printf(note, "Add this file: '%s' ?", sample->full_path);
		if (do_progress_question(note->str) != 1) {
			// 0, aborted: -> whole add_file loop is aborted on next do_progress() call.
			// 1, OK
			// 2, cancled: -> only this file is skipped
			sample_unref(sample);
			g_string_free(note, true);
			return false;
		}
		g_string_free(note, true);
		g_list_foreach(existing, (GFunc)g_free, NULL);
		g_list_free(existing);
#endif /* END interactive import */
	}
#endif /* END check for similar files on import */

	sample->online=1;
	sample->id = backend.insert(sample);
	if (sample->id < 0) {
		sample_unref(sample);
		return false;
	}

	samplecat_list_store_add((SamplecatListStore*)app->store, sample);

	on_directory_list_changed();
	sample_unref(sample);
	return true;
}


gboolean
on_overview_done(gpointer _sample)
{
	PF;
	Sample* sample = _sample;
	g_return_val_if_fail(sample, false);
	if(!sample->overview){ dbg(1, "overview creation failed (no pixbuf).\n"); return false; }
	listmodel__update_sample(sample, COL_OVERVIEW, NULL);
	sample_unref(sample);
	return IDLE_STOP;
}


gboolean
on_peaklevel_done(gpointer _sample)
{
	Sample* sample = _sample;
	dbg(1, "peaklevel=%.2f id=%i", sample->peaklevel, sample->id);
	listmodel__update_sample(sample, COL_PEAKLEVEL, NULL);
	sample_unref(sample);
	return IDLE_STOP;
}


gboolean
on_ebur128_done(gpointer _sample)
{
	Sample* sample = _sample;
	listmodel__update_sample(sample, COLX_EBUR, NULL);
	sample_unref(sample);
	return IDLE_STOP;
}


void
delete_selected_rows()
{
	GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(app->libraryview->widget));

	GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(app->libraryview->widget));
	GList* selectionlist = gtk_tree_selection_get_selected_rows(selection, &(model));
	if(!selectionlist){ perr("no files selected?\n"); return; }
	dbg(1, "%i rows selected.", g_list_length(selectionlist));

	GList* selected_row_refs = NULL;

	//get row refs for each selected row before the list is modified:
	GList* l = selectionlist;
	for(;l;l=l->next){
		GtkTreePath* treepath_selection = l->data;

		GtkTreeRowReference* row_ref = gtk_tree_row_reference_new(GTK_TREE_MODEL(app->store), treepath_selection);
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
				gchar* fname;
				int id;
				gtk_tree_model_get(model, &iter, COL_NAME, &fname, COL_IDX, &id, -1);

				if(!backend.remove(id)) return;

				//update the store:
				gtk_list_store_remove(app->store, &iter);
				n++;

			} else perr("bad iter!\n");
			gtk_tree_path_free(path);
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


/** sync the selected catalogue row with the filesystem. */
static void
update_rows(GtkWidget* widget, gpointer user_data)
{
	PF;
	GtkTreeModel* model = GTK_TREE_MODEL(app->store);
	gboolean force_update = (GPOINTER_TO_INT(user_data)==2)?true:false; // NOTE - linked to order in _menu_def[]

	GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(app->libraryview->widget));
	GList* selectionlist = gtk_tree_selection_get_selected_rows(selection, &(model));
	if(!selectionlist){ perr("no files selected?\n"); return; }
	dbg(2, "%i rows selected.", g_list_length(selectionlist));

	int i;
	for(i=0;i<g_list_length(selectionlist);i++){
		GtkTreePath* treepath = g_list_nth_data(selectionlist, i);
		Sample* sample = sample_get_from_model(treepath);
		if(do_progress(0, 0)) break; // TODO: set progress title to "updating"
		sample_refresh(sample, force_update);
		statusbar_print(1, "online status updated (%s)", sample->online ? "online" : "not online");
		sample_unref(sample);
	}
	hide_progress();
	//g_list_free(selectionlist);
#warning TODO selectionlist should be freed.
	/*
	g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (list);
	*/
}


static void
menu_play_all(GtkWidget* widget, gpointer user_data)
{
	app->auditioner->play_all();
}


void
menu_play_stop(GtkWidget* widget, gpointer user_data)
{
	app->auditioner->stop();
}


static MenuDef _menu_def[] = {
	{"Delete",         G_CALLBACK(menu_delete_row),         GTK_STOCK_DELETE,      true},
	{"Update",         G_CALLBACK(update_rows),             GTK_STOCK_REFRESH,     true},
#if 0 // what? is the same as above
	{"Force Update",   G_CALLBACK(update_rows),             GTK_STOCK_REFRESH,     true},
#endif
	{"Reset Colours",  G_CALLBACK(listview__reset_colours), GTK_STOCK_OK, true},
	{"Edit tags",      G_CALLBACK(listview__edit_row),      GTK_STOCK_EDIT,        true},
	{"Open",           G_CALLBACK(listview__edit_row),      GTK_STOCK_OPEN,       false},
	{"Open Directory", G_CALLBACK(NULL),                    GTK_STOCK_OPEN,        true},
	{"",                                                                       },
	{"Play All",       G_CALLBACK(menu_play_all),           GTK_STOCK_MEDIA_PLAY,  true},
	{"Stop Playback",  G_CALLBACK(menu_play_stop),          GTK_STOCK_MEDIA_STOP,  true},
	{"",                                                                       },
	{"View",           G_CALLBACK(NULL),                    GTK_STOCK_PREFERENCES, true},
	{"Prefs",          G_CALLBACK(NULL),                    GTK_STOCK_PREFERENCES, true},
};

static GtkWidget*
make_context_menu()
{
	GtkWidget* menu = gtk_menu_new();

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

		void set_view_toggle_state(GtkMenuItem* item, ViewOption* option)
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

		void toggle_view(GtkMenuItem* item, gpointer _option)
		{
			PF;
			ViewOption* option = (ViewOption*)_option;
			option->on_toggle(option->value = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(item)));
		}

		int i; for(i=0;i<MAX_VIEW_OPTIONS;i++){
			ViewOption* option = &app->view_options[i];
			GtkWidget* menu_item = gtk_check_menu_item_new_with_mnemonic(option->name);
			gtk_menu_shell_append(GTK_MENU_SHELL(sub), menu_item);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item), option->value);
			option->value = !option->value; //toggle before it gets toggled back.
			set_view_toggle_state((GtkMenuItem*)menu_item, option);
			g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(toggle_view), option);
		}
	}

	GtkWidget* menu_item = gtk_check_menu_item_new_with_mnemonic("Add Recursively");
	gtk_menu_shell_append(GTK_MENU_SHELL(sub), menu_item);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item), app->add_recursive);
	g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(toggle_recursive_add), NULL);

	if (app->auditioner->seek) {
		menu_item = gtk_check_menu_item_new_with_mnemonic("Loop Playback");
		gtk_menu_shell_append(GTK_MENU_SHELL(sub), menu_item);
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item), app->loop_playback);
		g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(toggle_loop_playback), NULL);
	}

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
toggle_loop_playback(GtkWidget* widget, gpointer user_data)
{
	PF;
	if(app->loop_playback) app->loop_playback = false; else app->loop_playback = true;
	return false;
}


static gboolean
toggle_recursive_add(GtkWidget* widget, gpointer user_data)
{
	PF;
	if(app->add_recursive) app->add_recursive = false; else app->add_recursive = true;
	return false;
}

extern char theme_name[64];

static bool
config_load()
{
#ifdef USE_MYSQL
	strcpy(app->config.mysql.name, "samplecat");
#endif
	strcpy(app->config.show_dir, "");

	int i;
	for (i=0;i<PALETTE_SIZE;i++) {
		//currently these are overridden anyway
		snprintf(app->config.colour[i], 7, "%s", "000000");
	}

	GError* error = NULL;
	app->key_file = g_key_file_new();
	if(g_key_file_load_from_file(app->key_file, app->config_filename, G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, &error)){
		p_(1, "config loaded.");
		gchar* groupname = g_key_file_get_start_group(app->key_file);
		dbg (2, "group=%s.", groupname);
		if(!strcmp(groupname, "Samplecat")){
#define num_keys (18)
#define ADD_CONFIG_KEY(VAR, NAME) \
			strcpy(keys[i], NAME); \
			loc[i] = VAR; \
			siz[i] = G_N_ELEMENTS(VAR); \
			i++;

			char  keys[num_keys+(PALETTE_SIZE-1)][64];
			char*  loc[num_keys+(PALETTE_SIZE-1)];
			size_t siz[num_keys+(PALETTE_SIZE-1)];

			i=0;
			ADD_CONFIG_KEY (app->config.database_backend, "database_backend");
#ifdef USE_MYSQL
			ADD_CONFIG_KEY (app->config.mysql.host,       "mysql_host");
			ADD_CONFIG_KEY (app->config.mysql.user,       "mysql_user");
			ADD_CONFIG_KEY (app->config.mysql.pass,       "mysql_pass");
			ADD_CONFIG_KEY (app->config.mysql.name,       "mysql_name");
#endif
			ADD_CONFIG_KEY (app->config.show_dir,         "show_dir");
			ADD_CONFIG_KEY (app->config.window_height,    "window_height");
			ADD_CONFIG_KEY (app->config.window_width,     "window_width");
			ADD_CONFIG_KEY (theme_name,                   "icon_theme");
			ADD_CONFIG_KEY (app->config.column_widths[0], "col1_width");
			ADD_CONFIG_KEY (app->model->filters.phrase,   "filter");
			ADD_CONFIG_KEY (app->config.browse_dir,       "browse_dir");
			ADD_CONFIG_KEY (app->config.show_player,      "show_player");
			ADD_CONFIG_KEY (app->config.show_waveform,    "show_waveform");
			ADD_CONFIG_KEY (app->config.show_spectrogram, "show_spectrogram");
			ADD_CONFIG_KEY (app->config.auditioner,       "auditioner");
			ADD_CONFIG_KEY (app->config.jack_autoconnect, "jack_autoconnect");
			ADD_CONFIG_KEY (app->config.jack_midiconnect, "jack_midiconnect");

			int k;
			for (k=0;k<PALETTE_SIZE-1;k++) {
				char tmp[16]; snprintf(tmp, 16, "colorkey%02d", k+1);
				ADD_CONFIG_KEY(app->config.colour[k+1], tmp)
			}

			for(k=0;k<(num_keys+PALETTE_SIZE-1);k++){
				gchar* keyval;
				if((keyval = g_key_file_get_string(app->key_file, groupname, keys[k], &error))){
					size_t keylen = siz[k];
					snprintf(loc[k], keylen, "%s", keyval); loc[k][keylen-1] = '\0';
					dbg(2, "%s=%s", keys[k], keyval);
					g_free(keyval);
				}else{
					if(error->code == 3) g_error_clear(error)
					else { GERR_WARN; }
					if (!loc[k] || strlen(loc[k])==0) strcpy(loc[k], "");
				}
			}
			samplecat_model_set_search_dir (app->model, app->config.show_dir);

			app->view_options[SHOW_PLAYER]      = (ViewOption){"Player",      show_player,      strcmp(app->config.show_player, "false")};
			app->view_options[SHOW_FILEMANAGER] = (ViewOption){"Filemanager", show_filemanager, true};
			app->view_options[SHOW_WAVEFORM]    = (ViewOption){"Waveform",    show_waveform,    !strcmp(app->config.show_waveform, "true")};
#ifdef HAVE_FFTW3
			app->view_options[SHOW_SPECTROGRAM] = (ViewOption){"Spectrogram", show_spectrogram, !strcmp(app->config.show_spectrogram, "true")};
#endif
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

	for (i=0;i<PALETTE_SIZE;i++) {
		if (strcmp(app->config.colour[i], "000000")) {
			app->colourbox_dirty = false;
			break;
		}
	}
	return true;
}


static bool
config_save()
{
	if(app->loaded){
		//update the search directory:
		g_key_file_set_value(app->key_file, "Samplecat", "show_dir", app->model->filters.dir ? app->model->filters.dir : "");

		//save window dimensions:
		gint width, height;
		char value[256];
		if(app->window && GTK_WIDGET_REALIZED(app->window)){
			gtk_window_get_size(GTK_WINDOW(app->window), &width, &height);
		} else {
			dbg (0, "couldnt get window size...", "");
			width  = MIN(atoi(app->config.window_width),  2048); //FIXME ?
			height = MIN(atoi(app->config.window_height), 2048);
		}
		snprintf(value, 255, "%i", width);
		g_key_file_set_value(app->key_file, "Samplecat", "window_width", value);
		snprintf(value, 255, "%i", height);
		g_key_file_set_value(app->key_file, "Samplecat", "window_height", value);

		g_key_file_set_value(app->key_file, "Samplecat", "icon_theme", theme_name);

		GtkTreeViewColumn* column = gtk_tree_view_get_column(GTK_TREE_VIEW(app->libraryview->widget), 1);
		int column_width = gtk_tree_view_column_get_width(column);
		snprintf(value, 255, "%i", column_width);
		g_key_file_set_value(app->key_file, "Samplecat", "col1_width", value);

		g_key_file_set_value(app->key_file, "Samplecat", "col1_height", value);

		g_key_file_set_value(app->key_file, "Samplecat", "filter", gtk_entry_get_text(GTK_ENTRY(app->search)));

		AyyiLibfilemanager* fm = file_manager__get_signaller();
		if(fm){
			struct _Filer* f = fm->file_window;
			if(f){
				g_key_file_set_value(app->key_file, "Samplecat", "browse_dir", f->real_path);
			}
		}

		g_key_file_set_value(app->key_file, "Samplecat", "show_player", app->view_options[SHOW_PLAYER].value ? "true" : "false");
		g_key_file_set_value(app->key_file, "Samplecat", "show_waveform", app->view_options[SHOW_WAVEFORM].value ? "true" : "false");
#ifdef HAVE_FFTW3
		g_key_file_set_value(app->key_file, "Samplecat", "show_spectrogram", app->view_options[SHOW_SPECTROGRAM].value ? "true" : "false");
#endif

		int i;
		for (i=1;i<PALETTE_SIZE;i++) {
			char keyname[32];
			snprintf(keyname,32, "colorkey%02d", i);
			g_key_file_set_value(app->key_file, "Samplecat", keyname, app->config.colour[i]);
		}
	}

	GError* error = NULL;
	gsize length;
	gchar* string = g_key_file_to_data(app->key_file, &length, &error);
	if(error){
		dbg (0, "error saving config file: %s", error->message);
		g_error_free(error);
	}

	if(ensure_config_dir()){

		FILE* fp;
		if(!(fp = fopen(app->config_filename, "w"))){
			errprintf("cannot open config file for writing (%s).\n", app->config_filename);
			statusbar_print(1, "cannot open config file for writing (%s).", app->config_filename);
			return false;
		}
		if(fprintf(fp, "%s", string) < 0){
			errprintf("error writing data to config file (%s).\n", app->config_filename);
			statusbar_print(1, "error writing data to config file (%s).", app->config_filename);
		}
		fclose(fp);

		application_quit(app);
	}
	else errprintf("cannot create config directory.");
	g_free(string);
	return true;
}


static void
config_new()
{
	//g_key_file_has_group(GKeyFile *key_file, const gchar *group_name);

	GError* error = NULL;
	char data[256 * 256];
	sprintf(data, "# this is the default config file for the Samplecat application.\n# pls enter your database details.\n"
		"[Samplecat]\n"
		"database_backend=sqlite\n"
		"mysql_host=localhost\n"
		"mysql_user=username\n"
		"mysql_pass=pass\n"
		"mysql_name=samplelib\n"
		"show_dir=\n"
		"auditioner=\n"
		"jack_autoconnect=system:playback_\n"
		"jack_midiconnect=DISABLED\n"
		);

	if(!g_key_file_load_from_data(app->key_file, data, strlen(data), G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, &error)){
		perr("error creating new key_file from data. %s\n", error->message);
		g_error_free(error);
		error = NULL;
		return;
	}

	printf("A default config file has been created. Please enter your database details in '%s'.\n", app->config_filename);
}


void
on_quit(GtkMenuItem* menuitem, gpointer user_data)
{
	int exit_code = GPOINTER_TO_INT(user_data);
	if(exit_code > 1) exit_code = 0; //ignore invalid exit code.

	if(app->loaded) config_save();

	menu_play_stop(NULL,NULL);
	app->auditioner->disconnect();

	if (backend.disconnect) backend.disconnect();

#if 0
	//disabled due to errors when quitting early.

	clear_store(); //does this make a difference to valgrind? no.

	gtk_main_quit();
#endif

	dbg (1, "done.");
	exit(exit_code);
}


int  auditioner_nullC() {return 0;}
void auditioner_null() {;}
void auditioner_nullP(const char *p) {;}
void auditioner_nullS(Sample *s) {;}

void
set_auditioner() /* tentative - WIP */
{
	printf("auditioner backend: "); fflush(stdout);
	const static Auditioner a_null = {
		&auditioner_nullC,
		&auditioner_null,
		&auditioner_null,
		&auditioner_nullP,
		&auditioner_nullS,
		&auditioner_nullS,
		&auditioner_null,
		&auditioner_null,
		NULL, NULL, NULL, NULL
	};
#ifdef HAVE_JACK
  const static Auditioner a_jack = {
		&jplay__check,
		&jplay__connect,
		&jplay__disconnect,
		&jplay__play_path,
		&jplay__play,
		&jplay__toggle,
		&jplay__play_all,
		&jplay__stop,
		&jplay__play_selected,
		&jplay__pause,
		&jplay__seek,
		&jplay__getposition
	};
#endif
#ifdef HAVE_AYYIDBUS
	const static Auditioner a_ayyidbus = {
		&auditioner_check,
		&auditioner_connect,
		&auditioner_disconnect,
		&auditioner_play_path,
		&auditioner_play,
		&auditioner_toggle,
		&auditioner_play_all,
		&auditioner_stop,
		NULL,
		NULL,
		NULL,
		NULL
	};
#endif
#ifdef HAVE_GPLAYER
	const static Auditioner a_gplayer = {
		&gplayer_check,
		&gplayer_connect,
		&gplayer_disconnect,
		&gplayer_play_path,
		&gplayer_play,
		&gplayer_toggle,
		&gplayer_play_all,
		&gplayer_stop,
		NULL,
		NULL,
		NULL,
		NULL
	};
#endif

	gboolean connected = false;
#ifdef HAVE_JACK
	if(!connected && can_use(app->players, "jack")){
		app->auditioner = & a_jack;
		if (!app->auditioner->check()) {
			connected = true;
			printf("JACK playback.\n");
		}
	}
#endif
#ifdef HAVE_AYYIDBUS
	if(!connected && can_use(app->players, "ayyi")){
		app->auditioner = & a_ayyidbus;
		if (!app->auditioner->check()) {
			connected = true;
			printf("ayyi_audition.\n");
		}
	}
#endif
#ifdef HAVE_GPLAYER
	if(!connected && can_use(app->players, "cli")){
		app->auditioner = & a_gplayer;
		if (!app->auditioner->check()) {
			connected = true;
			printf("using CLI player.\n");
		}
	}
#endif
	if (!connected) {
		printf("no playback support.\n");
		app->auditioner = & a_null;
	}
}


static gboolean
can_use (GList* l, const char* d)
{
	for(;l;l=l->next){
		if(!strcmp(l->data, d)){
			return true;
		}
	}
	return false;
}


