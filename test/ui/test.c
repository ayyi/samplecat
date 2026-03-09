/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. https://ayyi.github.io/samplecat/    |
 | copyright (C) 2020-2026 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#include "config.h"
#include <getopt.h>
#include <glib/gstdio.h>
#include <gdk/gdkkeysyms.h>
#include "gdl/gdl-dock-item.h"
#include "gdl/master.h"
#include "debug/debug.h"
#include "gtk/icon_theme.h"
#include "file_manager/pixmaps.h"
#include "test/runner.h"
#include "support.h"
#include "panels/library.h"
#include "application.h"
#include "window.h"

static bool search_pending = false;
static char* home;

SamplecatApplication* app = NULL;

#include "utils.c"
#include "list.c"

static void set_home_dir (char** argv);


int
application_main (int argc, char** argv)
{
#ifndef HAVE_GTK_4_16
	g_setenv ("GDK_DEBUG", "gl-glx", false);
#endif
	g_setenv ("GDK_BACKEND", "x11", false);

	g_log_set_default_handler (log_handler, NULL);

	set_home_dir (argv);

	app = SAMPLECAT_APPLICATION(application_new());
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

#ifdef GTK4_TODO
	GBytes* gtkrc = g_resources_lookup_data ("/samplecat/resources/gtkrc", 0, NULL);
	gtk_rc_parse_string (g_bytes_get_data(gtkrc, 0));
	g_bytes_unref (gtkrc);
#endif

	bool player_opt = false;

	if (config_load(&app->configctx, &app->config)) {
		sprintf(app->config.auditioner, "cli");
#ifdef USE_SQLITE
		sprintf(app->config.database_backend, "sqlite");
#endif
		g_signal_emit_by_name (app, "config-loaded");
	}

	icon_theme_init();

	// Pick a valid icon theme
	// Preferably from the config file, otherwise from the hardcoded list
	const char* themes[] = {NULL, "oxygen", "breeze", NULL};
	themes[0] = g_value_get_string(&app->configctx.options[CONFIG_ICON_THEME]->val);
	const char* theme = find_icon_theme(themes[0] ? &themes[0] : &themes[1]);
	icon_theme_set_theme(theme);

	pixmaps_init();

	db_init(
		false ? NULL
#ifdef USE_MYSQL
			: config_is_mysql() ? (void*)&app->config.mysql
#endif
#ifdef USE_SQLITE
			: !strcmp(app->config.database_backend, "sqlite") ? (void*)&app->config.sqlite
#endif
			: NULL
	);

	if (can_use(model->backends, app->config.database_backend)) {
		g_clear_pointer(&model->backends, g_list_free);
		samplecat_model_add_backend(app->config.database_backend);
	}

	if (!player_opt) {
		if (can_use(app->players, app->config.auditioner)) {
			g_clear_pointer(&app->players, g_list_free);
			ADD_PLAYER(app->config.auditioner);
		}
	}

	if (player_opt) g_strlcpy(app->config.auditioner, app->players->data, 8);

	if (!db_connect()) {
		g_warning("cannot connect to any database.");
#ifdef QUIT_WITHOUT_DB
		on_quit(NULL, GINT_TO_POINTER(EXIT_FAILURE));
#endif
	}

#ifdef USE_SQLITE
	{
		char* files[] = {"piano.wav", "short.wav"};
		ScanResults results = {0,};
		for (int i=0;i<G_N_ELEMENTS(files);i++) {
			g_autofree char* path = find_wav (files[i]);
			samplecat_application_add_file (path, &results);
		}
	}
#endif

#ifndef DEBUG_NO_THREADS
	worker_thread_init();
#endif

	if (!samplecat.model->backend.pending) {
		application_search();
		search_pending = false;
	} else {
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

	if (APPLICATION(app)->loaded && !app->temp_view) {
		config_save(&app->configctx);
		application_quit(APPLICATION(app)); // emit signal
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
	application_free((Application*)app);
	mime_type_clear();
#if 0
	application_free(app);
#endif
	mime_type_clear();
#endif

	dbg (1, "done");
}


void
setup (char* argv[])
{
	TEST.n_tests = G_N_ELEMENTS(tests);
	TEST.before_each = (ReadyTest)window_is_open;

	application_main (0, argv);
}


void
teardown ()
{
	dbg(1, "sending CTL-Q ...");

#ifdef GTK4_TODO
	send_key(app->window->window, GDK_KEY_q, GDK_CONTROL_MASK);
#endif

	g_free(home);
}


/*
 *  Set HOME so that tests can use their own config
 */
static void
set_home_dir (char** argv)
{
	home = g_build_filename(g_get_tmp_dir(), PACKAGE, NULL);
	g_setenv ("HOME", home, true);
	g_autofree char* configdir = g_build_filename(home, ".config", NULL);
	g_setenv ("XDG_CONFIG_HOME", configdir, true);

#ifdef USE_SQLITE
	char* sqlite_db = g_strdup_printf("%s/.config/samplecat/samplecat.sqlite", home);
	g_unlink (sqlite_db);
#endif
}
