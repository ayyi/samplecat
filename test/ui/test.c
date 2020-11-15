/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2020-2020 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/

#include "config.h"
#include <getopt.h>
#include "gdl/gdl-dock-item.h"
#include "gdl/gdl-dock-master.h"
#include "gdk/gdkkeysyms.h"
#include "debug/debug.h"
#include "icon_theme.h"
#include "file_manager/pixmaps.h"
#include "test/runner.h"
#include "support.h"
#include "panels/library.h"
#include "application.h"
#include "window.h"

static bool search_pending = false;

Application* app = NULL;

#include "utils.c"
#include "list.c"


int
application_main (int argc, char** argv)
{
	g_log_set_default_handler(log_handler, NULL);

	gtk_init_check(&argc, &argv);

	app = application_new();
	SamplecatModel* model = samplecat.model;

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

	GBytes* gtkrc = g_resources_lookup_data ("/samplecat/resources/gtkrc", 0, NULL);
	gtk_rc_parse_string (g_bytes_get_data(gtkrc, 0));
	g_bytes_unref (gtkrc);

	bool player_opt = false;

	type_init();

	if(config_load(&app->configctx, &app->config)){
		g_signal_emit_by_name (app, "config-loaded");
	}

	// Pick a valid icon theme
	// Preferably from the config file, otherwise from the hardcoded list
	const char* themes[] = {NULL, "oxygen", "breeze", NULL};
	themes[0] = g_value_get_string(&app->configctx.options[CONFIG_ICON_THEME]->val);
	const char* theme = find_icon_theme(themes[0] ? &themes[0] : &themes[1]);
	icon_theme_set_theme(theme);

	db_init(
#ifdef USE_MYSQL
		&app->config.mysql
#else
		NULL
#endif
	);

	if (app->config.database_backend && can_use(model->backends, app->config.database_backend)) {
		g_clear_pointer(&model->backends, g_list_free);
		samplecat_model_add_backend(app->config.database_backend);
	}

	if (!player_opt && app->config.auditioner) {
		if(can_use(app->players, app->config.auditioner)){
			g_clear_pointer(&app->players, g_list_free);
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

	if (!db_connect()) {
		g_warning("cannot connect to any database.");
#ifdef QUIT_WITHOUT_DB
		on_quit(NULL, GINT_TO_POINTER(EXIT_FAILURE));
#endif
	}

#ifndef DEBUG_NO_THREADS
	worker_thread_init();
#endif

	if(!app->no_gui) window_new();

	if(!samplecat.model->backend.pending){
		application_search();
		search_pending = false;
	}else{
		search_pending = true;
	}

#ifdef USE_AYYI
	ayyi_client_init();
	g_idle_add((GSourceFunc)ayyi_connect, NULL);
#endif

	application_set_ready();

	statusbar_print(2, PACKAGE_NAME". Version "PACKAGE_VERSION);

	return 0;
}


void
on_quit ()
{
	app->temp_view = true;

	if(app->loaded && !app->temp_view){
		config_save(&app->configctx);
		application_quit(app); // emit signal
	}

	if(play->auditioner){
#ifdef HAVE_AYYIDBUS
		extern Auditioner a_ayyidbus;
		if(play->auditioner != & a_ayyidbus){
#else
		if(true){
#endif
			play->auditioner->stop();
		}
		play->auditioner->disconnect();
	}

	if(samplecat.model->backend.disconnect) samplecat.model->backend.disconnect();

#ifdef WITH_VALGRIND
	application_free(app);
	mime_type_clear();
#if 0
	application_free(app);
#endif
	mime_type_clear();
#endif

	dbg (1, "done");
}


void
setup ()
{
	TEST.n_tests = G_N_ELEMENTS(tests);

	application_main (0, NULL);
}


void
teardown ()
{
	dbg(1, "sending CTL-Q ...");

	send_key(app->window->window, GDK_KEY_q, GDK_CONTROL_MASK);
}

