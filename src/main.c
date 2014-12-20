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

#include "db/db.h"
#include "model.h"
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
#include "console_view.h"
#include "audio_decoder/ad.h"

#include "main.h"

#undef DEBUG_NO_THREADS


static bool      config_load  ();
static void      config_new   ();
static bool      config_save  ();

Application*     app = NULL;
SamplecatBackend backend; 
Palette          palette;
GList*           mime_types; // list of MIME_type*

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
	"  -a, --add <file(s)>    add these files/directories.\n"
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

#ifndef HAVE_GTK_2_12
	if (!g_thread_supported()) g_thread_init(NULL);
#endif
	gdk_threads_init();
	gtk_init_check(&argc, &argv);

	printf("%s"PACKAGE_NAME". Version "PACKAGE_VERSION"%s\n", yellow, white);

	app = application_new();
	SamplecatModel* model = app->model;

	colour_box_init();
	memset(&backend, 0, sizeof(struct _backend)); 

#define ADD_BACKEND(A) model->backends = g_list_append(model->backends, A)

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
				if(can_use(model->backends, optarg)){
					list_clear(model->backends);
					ADD_BACKEND(optarg);
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
				printf("search=%s\n", optarg);
				app->args.search = g_strdup(optarg);
				break;
			case 'a':
				dbg(1, "add=%s", optarg);
				app->args.add = remove_trailing_slash(g_strdup(optarg));
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

	type_init();

	config_load();

	db_init(app->model,
#ifdef USE_MYSQL
		&app->config.mysql
#else
		NULL
#endif
	);

	if (app->config.database_backend && can_use(model->backends, app->config.database_backend)) {
		list_clear(model->backends);
		ADD_BACKEND(app->config.database_backend);
	}

	if (!player_opt && app->config.auditioner) {
		if(can_use(app->players, app->config.auditioner)){
			list_clear(app->players);
			ADD_PLAYER(app->config.auditioner);
		}
	}

#ifdef __APPLE__
	GtkOSXApplication* osxApp = (GtkOSXApplication*) 
	g_object_new(GTK_TYPE_OSX_APPLICATION, NULL);
#endif
	app->gui_thread = pthread_self();

	icon_theme_init();
	pixmaps_init();
	ad_init();
	app->store = listmodel__new();
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
			if(app->no_gui) app->add_recursive = true; // TODO take from config
			application_scan(app->args.add, &results);
		}else{
			printf("Adding file: %s\n", app->args.add);
			application_add_file(app->args.add, &results);
		}
		hide_progress();
	}

#ifndef DEBUG_NO_THREADS
	overview_thread_init(app->model);
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

	if(!backend.pending){ 
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

	app->auditioner->stop();
	app->auditioner->disconnect();

	exit(EXIT_SUCCESS);
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

				if(!samplecat_model_remove(app->model, id)) return;

				gtk_list_store_remove(app->store, &iter);
				n++;

			} else perr("bad iter!\n");
			gtk_tree_path_free(path);
		} else perr("cannot get path from row_ref!\n");
	}
	g_list_free(selected_row_refs); //FIXME free the row_refs?

	statusbar_print(1, "%i rows deleted", n);
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
#ifdef USE_MYSQL
#define num_keys (16)
#else
#define num_keys (12)
#endif
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
			ADD_CONFIG_KEY (app->config.window_height,    "window_height");
			ADD_CONFIG_KEY (app->config.window_width,     "window_width");
			ADD_CONFIG_KEY (theme_name,                   "icon_theme");
			ADD_CONFIG_KEY (app->config.column_widths[0], "col1_width");
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

			gchar* keyval;
			for(k=0;k<(num_keys+PALETTE_SIZE-1);k++){
				if((keyval = g_key_file_get_string(app->key_file, groupname, keys[k], &error))){
					if(loc[k]){
						size_t keylen = siz[k];
						snprintf(loc[k], keylen, "%s", keyval); loc[k][keylen-1] = '\0';
					}
					dbg(2, "%s=%s", keys[k], keyval);
					g_free(keyval);
				}else{
					if(error->code == 3) g_error_clear(error)
					else { GERR_WARN; }
					if (!loc[k] || strlen(loc[k])==0) strcpy(loc[k], "");
				}
			}

			if((keyval = g_key_file_get_string(app->key_file, groupname, "show_dir", &error))){
				samplecat_model_set_search_dir (app->model, app->config.show_dir);
			}
			if((keyval = g_key_file_get_string(app->key_file, groupname, "filter", &error))){
				app->model->filters.search->value = g_strdup(keyval);
			}

#ifndef USE_GDL
			app->view_options[SHOW_PLAYER]      = (ViewOption){"Player",      show_player,      strcmp(app->config.show_player, "false")};
			app->view_options[SHOW_FILEMANAGER] = (ViewOption){"Filemanager", show_filemanager, true};
			app->view_options[SHOW_WAVEFORM]    = (ViewOption){"Waveform",    show_waveform,    !strcmp(app->config.show_waveform, "true")};
#ifdef HAVE_FFTW3
			app->view_options[SHOW_SPECTROGRAM] = (ViewOption){"Spectrogram", show_spectrogram, !strcmp(app->config.show_spectrogram, "true")};
#endif
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

	g_signal_emit_by_name (app, "config-loaded");
	return true;
}


static bool
config_save()
{
	if(app->loaded){
		//update the search directory:
		g_key_file_set_value(app->key_file, "Samplecat", "show_dir", app->model->filters.dir->value ? app->model->filters.dir->value : "");

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

		g_key_file_set_value(app->key_file, "Samplecat", "filter", app->model->filters.search->value);

		AyyiLibfilemanager* fm = file_manager__get_signaller();
		if(fm){
			struct _Filer* f = fm->file_window;
			if(f){
				g_key_file_set_value(app->key_file, "Samplecat", "browse_dir", f->real_path);
			}
		}

#ifndef USE_GDL
		g_key_file_set_value(app->key_file, "Samplecat", "show_player", app->view_options[SHOW_PLAYER].value ? "true" : "false");
		g_key_file_set_value(app->key_file, "Samplecat", "show_waveform", app->view_options[SHOW_WAVEFORM].value ? "true" : "false");
#ifdef HAVE_FFTW3
		g_key_file_set_value(app->key_file, "Samplecat", "show_spectrogram", app->view_options[SHOW_SPECTROGRAM].value ? "true" : "false");
#endif
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

	app->auditioner->stop();
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


