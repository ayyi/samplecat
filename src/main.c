/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2007-2015 Tim Orford <tim@orford.org>                  |
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
#include "waveform/utils.h"
#include "utils/ayyi_utils.h"
#include "utils/pixmaps.h"
#include "file_manager.h"
#include "samplecat.h"
#ifdef USE_AYYI
  #include "ayyi.h"
  #include "ayyi_model.h"
#endif
#include "typedefs.h"
#ifdef USE_TRACKER
  #include "db/tracker.h"
#endif
#ifdef USE_MYSQL
  #include "db/mysql.h"
#endif
#include "dh_link.h"

#include "db/db.h"
#include "model.h"
#include "list_store.h"
#include "support.h"
#include "sample.h"
#include "cellrenderer_hypertext.h"
#include "listview.h"
#include "window.h"
#include "colour_box.h"
#include "progress_dialog.h"
#include "dnd.h"
#include "icon_theme.h"
#include "application.h"
#include "console_view.h"

#undef DEBUG_NO_THREADS

void on_quit (GtkMenuItem*, gpointer);

Application*     app = NULL;
Palette          palette;

extern char theme_name[64];

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
  { "cwd",              0, NULL, 'c' },
  { "add",              1, NULL, 'a' },
  { "help",             0, NULL, 'h' },
  { "version",          0, NULL, 'V' },
};

static const char* const short_options = "b:gv:s:a:p:chV";

static const char* const usage =
	"Usage: %s [OPTIONS]\n\n"
	"SampleCat is a program for cataloguing and auditioning audio samples.\n" 
	"\n"
	"Options:\n"
	"  -a, --add <file(s)>    add these files/directories.\n"
	"  -b, --backend <name>   select which database type to use.\n"
	"  -g, --no-gui           run as command line app.\n"
	"  -h, --help             show this usage information and quit.\n"
	"  -p, --player <name>    select audio player (jack, ayyi, cli).\n"
	"  -s, --search <txt>     search using this phrase.\n"
	"  -c, --cwd              show contents of current directory (temporary view, state not saved).\n"
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
	"    $HOME/.config/samplecat/\n"
	"    $HOME/.cache/peak/\n"
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

#ifndef HAVE_GTK_2_12
	if (!g_thread_supported()) g_thread_init(NULL);
#endif
	gdk_threads_init();
	gtk_init_check(&argc, &argv);

	printf("%s"PACKAGE_NAME". Version "PACKAGE_VERSION"%s\n", yellow, white);

	app = application_new();
	SamplecatModel* model = samplecat.model;

	colour_box_init();

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
				if(d<0 || d>5) { gwarn ("bad arg. debug=%i", d); } else _debug_ = wf_debug = d;
				#ifdef USE_AYYI
				ayyi.debug = _debug_;
				#endif
				break;
			case 'b':
				//if a particular backend is requested, and is available, reduce the backend list to just this one.
				dbg(1, "backend '%s' requested.", optarg);
				if(can_use(model->backends, optarg)){
					list_clear(model->backends);
					samplecat_model_add_backend(optarg);
					dbg(1, "n_backends=%i", g_list_length(model->backends));
				}
				else{
					warnprintf("requested backend not available: '%s'\navailable backends:\n", optarg);
					GList* l = model->backends;
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
					player_opt = true;
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
				printf("search: %s\n", optarg);
				app->args.search = g_strdup(optarg);
				break;
			case 'c':
				app->temp_view = true;
				break;
			case 'a':
				dbg(1, "add=%s", optarg);
				app->args.add = remove_trailing_slash(g_strdup(optarg));
				break;
			case 'V':
				printf ("%s %s\n\n", basename(argv[0]), PACKAGE_VERSION);
				printf(
					"Copyright (C) 2007-2015 Tim Orford\n"
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

	type_init();

	config_load(&app->configctx, &app->config);
	g_signal_emit_by_name (app, "config-loaded");

	db_init(
#ifdef USE_MYSQL
		&app->config.mysql
#else
		NULL
#endif
	);

	if (app->config.database_backend && can_use(model->backends, app->config.database_backend)) {
		list_clear(model->backends);
		samplecat_model_add_backend(app->config.database_backend);
	}

	if (!player_opt && app->config.auditioner) {
		if(can_use(app->players, app->config.auditioner)){
			list_clear(app->players);
			ADD_PLAYER(app->config.auditioner);
		}
	}

	if(player_opt) g_strlcpy(app->config.auditioner, app->players->data, 8);

#ifdef __APPLE__
	GtkOSXApplication* osxApp = (GtkOSXApplication*) 
	g_object_new(GTK_TYPE_OSX_APPLICATION, NULL);
#endif
	app->gui_thread = pthread_self();

	icon_theme_init();
	pixmaps_init();

	if(app->no_gui) console__init();


	if (!db_connect()) {
		g_warning("cannot connect to any database.\n");
#ifdef QUIT_WITHOUT_DB
		on_quit(NULL, GINT_TO_POINTER(EXIT_FAILURE));
#endif
	}

#ifdef USE_TRACKER
	if(BACKEND_IS_NULL && can_use(model->backends, "tracker")){
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
				application_search();
				search_pending = false;
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
		ScanResults results = {0,};
		if(g_file_test(app->args.add, G_FILE_TEST_IS_DIR)){
			application_scan(app->args.add, &results);
		}else{
			printf("Adding file: %s\n", app->args.add);
			application_add_file(app->args.add, &results);
		}
		hide_progress();
	}

	if(app->temp_view){
		gchar* dir = g_get_current_dir();
		g_strlcpy(app->config.browse_dir, dir, PATH_MAX);
		g_free(dir);
	}

#ifndef DEBUG_NO_THREADS
	worker_thread_init();
#endif

	if(!app->no_gui) window_new(); 

#ifdef __APPLE__
	GtkWidget* menu_bar;
	menu_bar = gtk_menu_bar_new();
	/* Note: the default OSX menu bar already includes a 'quit' entry 
	 * connected to 'gtk_main_quit' by default. so we're fine.
	 */
	gtk_osxapplication_set_menu_bar(osxApp, GTK_MENU_SHELL(menu_bar));
#endif

	if(!samplecat.model->backend.pending){
		application_search();
		search_pending = false;
	}else{
		search_pending = true;
	}

	if(app->no_gui) exit(EXIT_SUCCESS);

#ifdef USE_AYYI
	ayyi_client_init();
	g_idle_add((GSourceFunc)ayyi_connect, NULL);
#endif

#ifdef USE_TRACKER
	if(!app->no_gui && BACKEND_IS_NULL) g_idle_add((GSourceFunc)tracker__init, NULL);
#else
	//if(BACKEND_IS_NULL) listview__show_db_missing();
#endif

	application_set_ready();

#ifndef USE_GDL
	message_panel__add_msg("hello", GTK_STOCK_INFO);
#endif
	statusbar_print(2, PACKAGE_NAME". Version "PACKAGE_VERSION);

#ifdef __APPLE__
	gtk_osxapplication_ready(osxApp);
#endif
	gtk_main();

	app->auditioner->stop();
	app->auditioner->disconnect();

	exit(EXIT_SUCCESS);
}


void
on_quit(GtkMenuItem* menuitem, gpointer user_data)
{
	int exit_code = GPOINTER_TO_INT(user_data);
	if(exit_code > 1) exit_code = 0; //ignore invalid exit code.

	if(app->loaded && !app->temp_view){
		config_save(&app->configctx);
		application_quit(app); // emit signal
	}

	app->auditioner->stop();
	app->auditioner->disconnect();

	if(samplecat.model->backend.disconnect) samplecat.model->backend.disconnect();

#if 0
	//disabled due to errors when quitting early.

	clear_store(); //does this make a difference to valgrind? no.

	gtk_main_quit();
#endif

	dbg (1, "done.");
	exit(exit_code);
}


