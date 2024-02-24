/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
 | copyright (C) 2007-2023 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#define __main_c__
#include "config.h"
#include <libgen.h>
#include <gdk-pixbuf/gdk-pixdata.h>

#ifdef __APPLE__
#include <gdk/gdkkeysyms.h>
#include <igemacintegration/gtkosxapplication.h>
char * program_name;
#endif

#include "debug/debug.h"
#ifdef USE_AYYI
  #include "ayyi.h"
  #include "ayyi_model.h"
#endif
#ifdef USE_TRACKER
  #include "db/tracker.h"
#endif

#include "db/db.h"
#include "support.h"
#include "progress_dialog.h"
#include "application.h"
#include "application-nogui.h"

#undef DEBUG_NO_THREADS

#ifdef GTK4_TODO
void on_quit (GtkMenuItem*, gpointer);
#endif

SamplecatApplication* app = NULL;

int 
main (int argc, char** argv)
{
#ifdef __APPLE__
	program_name = argv[0];
#endif

	g_log_set_default_handler(log_handler, NULL);

	printf("%s"PACKAGE_NAME" "PACKAGE_VERSION"%s\n", yellow, ayyi_white);

	bool want_gui = true;
	for (int i=1;i<argc;i++) {
		if (!strcmp(argv[i], "--no-gui") || !strcmp(argv[i], "-G"))
			want_gui = false;
		if (!strcmp(argv[i], "--version") || !strcmp(argv[i], "-V")) {
			printf ("%s %s\n\n", basename(argv[0]), PACKAGE_VERSION);
			printf(
				"Copyright (C) 2007-2023 Tim Orford\n"
				"Copyright (C) 2011 Robin Gareus\n"
				"This is free software; see the source for copying conditions.  There is NO\n"
				"warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\n"
				);
			return EXIT_SUCCESS;
		}
	}

	app = want_gui ? (SamplecatApplication*)application_new() : (SamplecatApplication*)no_gui_application_new();

	if (config_load(&app->configctx, &app->config)) {
		if (want_gui)
			g_signal_emit_by_name (app, "config-loaded");
	}

	db_init(
#ifdef USE_MYSQL
		&app->config.mysql
#else
		NULL
#endif
	);

	if (app->config.database_backend[0] && can_use(samplecat.model->backends, app->config.database_backend)) {
		g_clear_pointer(&samplecat.model->backends, g_list_free);
		samplecat_model_add_backend(app->config.database_backend);
	}

#ifdef __APPLE__
	GtkOSXApplication* osxApp = (GtkOSXApplication*)
	g_object_new(GTK_TYPE_OSX_APPLICATION, NULL);
#endif

	if (!db_connect()) {
		g_warning("cannot connect to any database.");
#ifdef QUIT_WITHOUT_DB
		on_quit(NULL, GINT_TO_POINTER(EXIT_FAILURE));
#endif
	}

#ifdef USE_TRACKER
	if (BACKEND_IS_NULL && can_use(samplecat.model->backends, "tracker")) {
		void on_tracker_init ()
		{
			dbg(2, "...");
			samplecat_set_backend(BACKEND_TRACKER);

			// hide unsupported inspector notes
			GtkWidget* notes = app->inspector->text;
			if (notes) gtk_widget_hide(notes);

			if (search_pending) {
				application_search();
				search_pending = false;
			}
		}
		if (app->no_gui) {
			tracker__init(on_tracker_init);
		} else {
			backend.pending = true;
			g_idle_add_full(G_PRIORITY_LOW, (GSourceFunc)tracker__init, on_tracker_init, NULL);
		}
	}
#endif

	if (app->temp_view) {
		g_autofree gchar* dir = g_get_current_dir();
		g_strlcpy(app->config.browse_dir, dir, PATH_MAX);
	}

#ifndef DEBUG_NO_THREADS
	worker_thread_init();
#endif

#ifdef __APPLE__
	GtkWidget* menu_bar = gtk_menu_bar_new();
	/* Note: the default OSX menu bar already includes a 'quit' entry 
	 * connected to 'gtk_main_quit' by default. so we're fine.
	 */
	gtk_osxapplication_set_menu_bar(osxApp, GTK_MENU_SHELL(menu_bar));
#endif

#ifdef USE_AYYI
	ayyi_client_init();
	g_idle_add((GSourceFunc)ayyi_connect, NULL);
#endif

#ifdef USE_TRACKER
	if (want_gui && BACKEND_IS_NULL) g_idle_add((GSourceFunc)tracker__init, NULL);
#else
	//if(BACKEND_IS_NULL) listview__show_db_missing();
#endif

	application_set_ready();

#ifdef __APPLE__
	gtk_osxapplication_ready(osxApp);
#endif
	int status = g_application_run (G_APPLICATION (app), argc, argv);

	dbg(1, "application quit %i", status);

#ifdef GTK4_TODO
	play->auditioner->stop();
	play->auditioner->disconnect();
#endif

	exit(status);
}


#ifdef GTK4_TODO
void
on_quit (GtkMenuItem* menuitem, gpointer user_data)
{
	int exit_code = GPOINTER_TO_INT(user_data);
	if (exit_code > 1) exit_code = 0; // ignore invalid exit code.

	if (app->loaded && !app->temp_view) {
		config_save(&app->configctx);
		application_quit(app); // emit signal
	}

	if (play->auditioner) {
#ifdef HAVE_AYYIDBUS
		extern Auditioner a_ayyidbus;
		if (play->auditioner != & a_ayyidbus) {
#else
		if (true) {
#endif
			play->auditioner->stop();
		}
		play->auditioner->disconnect();
	}

	if (samplecat.model->backend.disconnect) samplecat.model->backend.disconnect();

#ifdef WITH_VALGRIND
	application_free(app);
	mime_type_clear();

#if 0
	clear_store(); //does this make a difference to valgrind? no.
#endif
#endif

	dbg (1, "done.");
	exit(exit_code);
}
#endif
