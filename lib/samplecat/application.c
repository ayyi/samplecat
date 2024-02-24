/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. https://ayyi.github.io/samplecat/    |
 | copyright (C) 2023-2024 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#include "config.h"
#include <stdlib.h>
#include <libgen.h>
#include <getopt.h>
#include "debug/debug.h"
#define __wf_private__
#include "wf/debug.h"
#include "list_store.h"
#include "samplecat.h"
#include "application.h"

#include "usage.c"

extern SamplecatApplication* app;

enum  {
	SAMPLECAT_APPLICATION_0_PROPERTY,
	SAMPLECAT_APPLICATION_NUM_PROPERTIES
};

enum  {
	SAMPLECAT_APPLICATION_NUM_SIGNALS
};

static gpointer samplecat_application_parent_class = NULL;

static void     samplecat_application_startup  (GApplication*);
static void     samplecat_application_activate (GApplication*);
static void     samplecat_application_finalize      (GObject*);
static GType    samplecat_application_get_type_once (void);

#define ADD_PLAYER(A) (app->players = g_list_append(app->players, A))


SamplecatApplication*
samplecat_application_construct (GType object_type)
{
	SamplecatApplication* self = (SamplecatApplication*) g_object_new (object_type, NULL);

	return self;
}

SamplecatApplication*
samplecat_application_new (void)
{
	return samplecat_application_construct (TYPE_SAMPLECAT_APPLICATION);
}

static const struct option long_options[] = {
  { "backend",          1, NULL, 'b' },
  { "player",           1, NULL, 'p' },
  { "no-gui",           0, NULL, 'G' },
  { "verbose",          1, NULL, 'v' },
  { "search",           1, NULL, 's' },
  { "cwd",              0, NULL, 'c' },
  { "add",              1, NULL, 'a' },
  { "layout",           1, NULL, 'l' },
  { "help",             0, NULL, 'h' },
  { "version",          0, NULL, 'V' },
};

static const char* const short_options = "b:Gv:s:a:p:chV";

static gboolean
samplecat_application_local_command_line (GApplication* self, gchar*** arguments, int* exit_status)
{
	SamplecatApplication* sa = (SamplecatApplication*)self;

	*exit_status = 0;

	int argc = g_strv_length (*arguments);

	bool player_opt = false;
	int opt;
	while ((opt = getopt_long (argc, *arguments, short_options, long_options, NULL)) != -1) {
		switch (opt) {
			case 'v':
				printf("using debug level: %s\n", optarg);
				int d = atoi(optarg);
				if (d < 0 || d > 5) { pwarn ("bad arg. debug=%i", d); } else _debug_ = wf_debug = d;
			#ifdef USE_AYYI
				ayyi.debug = _debug_;
			#endif
				break;
			case 'b':
				// if a particular backend is requested, and is available, reduce the backend list to just this one.
				dbg(1, "backend '%s' requested.", optarg);
				if (can_use(samplecat.model->backends, optarg)) {
					g_clear_pointer(&samplecat.model->backends, g_list_free);
					samplecat_model_add_backend(optarg);
					dbg(1, "n_backends=%i", g_list_length(samplecat.model->backends));
				} else {
					warnprintf("requested backend not available: '%s'\navailable backends:\n", optarg);
					GList* l = samplecat.model->backends;
					for (;l;l=l->next) {
						printf("  %s\n", (char*)l->data);
					}
					*exit_status = EXIT_FAILURE;
				}
				break;
			case 'p':
				if (can_use(app->players, optarg)) {
					g_clear_pointer(&app->players, g_list_free);
					ADD_PLAYER(g_strdup(optarg));
					player_opt = true;
				} else {
					warnprintf("requested player is not available: '%s'\navailable backends:\n", optarg);
					GList* l = app->players;
					for (;l;l=l->next) printf("  %s\n", (char*)l->data);
					*exit_status = EXIT_FAILURE;
				}
				break;
			case 'h':
				printf(usage, basename((*arguments)[0]));
				return 1;
			case 's':
				printf("search: %s\n", optarg);
				observable_string_set(samplecat.model->filters2.search, g_strdup(optarg));
				break;
			case 'c':
				app->temp_view = true;
				break;
			case 'a':
				dbg(1, "add=%s", optarg);
				app->args.add = remove_trailing_slash(g_strdup(optarg));
				break;
			case 'l':
				dbg(1, "layout=%s", optarg);
				app->args.layout = g_strdup(optarg);
				break;
			case 'G':
			case 'V':
				// already consumed
				break;
			case '?':
			default:
				printf("unknown option: %c\n", optopt);
				printf(usage, basename((*arguments)[0]));
				*exit_status = EXIT_FAILURE;
				break;
		}
	}

	// remove args that have been handled
	for (int i=0;(*arguments)[i];i++) {
		g_clear_pointer(&((*arguments)[i]), g_free);
	}

	if (player_opt) {
		g_strlcpy(app->config.auditioner, sa->players->data, 8);
	} else {
		if (app->config.auditioner[0]) {
			if (can_use(sa->players, app->config.auditioner)) {
				g_clear_pointer(&sa->players, g_list_free);
				ADD_PLAYER(app->config.auditioner);
			}
		}
	}

	if (app->args.add) {
		/* initial import from commandline */
#ifdef GTK4_TODO
		do_progress(0, 0);
#endif
		ScanResults results = {0,};
		if (g_file_test(app->args.add, G_FILE_TEST_IS_DIR)) {
			samplecat_application_scan(app->args.add, &results);
		} else {
			printf("Adding file: %s\n", app->args.add);
			samplecat_application_add_file(app->args.add, &results);
		}
#ifdef GTK4_TODO
		hide_progress();
#endif
	}

	if (!*exit_status)
		return G_APPLICATION_CLASS (samplecat_application_parent_class)->local_command_line (self, arguments, exit_status);
	else
		return 1;
}

static void
samplecat_application_startup (GApplication* base)
{
	G_APPLICATION_CLASS (samplecat_application_parent_class)->startup(base);
}

static void
samplecat_application_activate (GApplication* base)
{
	G_APPLICATION_CLASS (samplecat_application_parent_class)->activate(base);
}

static void
quit_mainloop (GApplication* application)
{
}

static void
shutdown (GApplication* application)
{
	return G_APPLICATION_CLASS (samplecat_application_parent_class)->shutdown (application);
}

static void
samplecat_application_class_init (SamplecatApplicationClass* klass, gpointer klass_data)
{
	samplecat_application_parent_class = g_type_class_peek_parent (klass);

	((GApplicationClass *) klass)->startup = samplecat_application_startup;
	((GApplicationClass *) klass)->activate = samplecat_application_activate;
	((GApplicationClass *) klass)->local_command_line = samplecat_application_local_command_line;
	((GApplicationClass *) klass)->quit_mainloop = quit_mainloop;
	((GApplicationClass *) klass)->shutdown = shutdown;
	G_OBJECT_CLASS (klass)->finalize = samplecat_application_finalize;
}

static void
samplecat_application_instance_init (SamplecatApplication* self, gpointer klass)
{
	app = self;

	self->cache_dir = g_build_filename (g_get_home_dir (), ".config", PACKAGE, "cache", NULL);
	self->configctx.dir = g_build_filename (g_get_home_dir(), ".config", PACKAGE, NULL);
	self->configctx.filename = g_strdup_printf("%s/.config/" PACKAGE "/" PACKAGE, g_get_home_dir());

	samplecat_init();

	app->configctx.options = g_malloc0(CONFIG_MAX * sizeof(ConfigOption*));

	void get_theme_name (ConfigOption* option)
	{
		g_value_set_string(&option->val, theme_name);
	}
	ConfigContext* ctx = &app->configctx;
	ctx->options[CONFIG_ICON_THEME] = config_option_new_string("icon_theme", get_theme_name);

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
}

static void
samplecat_application_finalize (GObject* obj)
{
	G_OBJECT_CLASS (samplecat_application_parent_class)->finalize (obj);
}

static GType
samplecat_application_get_type_once (void)
{
	static const GTypeInfo g_define_type_info = { sizeof (SamplecatApplicationClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) samplecat_application_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (SamplecatApplication), 0, (GInstanceInitFunc) samplecat_application_instance_init, NULL };
	return g_type_register_static (gtk_application_get_type (), "SamplecatApplication", &g_define_type_info, 0);
}

GType
samplecat_application_get_type (void)
{
	static volatile gsize samplecat_application_type_id__once = 0;
	if (g_once_init_enter ((gsize*)&samplecat_application_type_id__once)) {
		GType samplecat_application_type_id = samplecat_application_get_type_once ();
		g_once_init_leave (&samplecat_application_type_id__once, samplecat_application_type_id);
	}
	return samplecat_application_type_id__once;
}

/*
 *  Path must not contain trailing slash
 */
void
samplecat_application_scan (const char* path, ScanResults* results)
{
	g_return_if_fail(path);

	app->state = SCANNING;
	worker_go_slow = true;
	logger_log(samplecat.logger, "scanning...");

	samplecat_application_add_dir(path, results);

	gchar* fail_msg = results->n_failed ? g_strdup_printf(", %i failed", results->n_failed) : "";
	gchar* dupes_msg = results->n_dupes ? g_strdup_printf(", %i duplicates", results->n_dupes) : "";
	logger_log(samplecat.logger, "add finished: %i files added%s%s", results->n_added, fail_msg, dupes_msg);
	g_clear_pointer(&fail_msg, g_free);
	g_clear_pointer(&dupes_msg, g_free);

	app->state = NONE;
	worker_go_slow = false;
}


/*
 *  uri must be "unescaped" before calling this fn. Method string must be removed.
 */
bool
samplecat_application_add_file (const char* path, ScanResults* result)
{
	if (BACKEND_IS_NULL) return false;

	/* check if file already exists in the store
	 * -> don't add it again
	 */
	if (samplecat.model->backend.file_exists(path, NULL)) {
		logger_log(samplecat.logger, "duplicate: %s", path);
		if (_debug_) g_warning("duplicate file: %s", path);
		Sample* s = sample_get_by_filename(path);
		if (s) {
			//sample_refresh(s, false);
			samplecat_model_refresh_sample (samplecat.model, s, false);
			sample_unref(s);
		} else {
			dbg(1, "sample found in db but not in model.");
		}
		result->n_dupes++;
		return false;
	}

	logger_log(samplecat.logger, "%s", path);

	Sample* sample = sample_new_from_filename((char*)path, false);
	if (!sample) {
		if (app->state != SCANNING){
			if (_debug_) pwarn("cannot add file: file-type is not supported");
			logger_log(samplecat.logger, "cannot add file: file-type is not supported");
		}
		return false;
	}

	if (!sample_get_file_info(sample)) {
		pwarn("cannot add file: reading file info failed. type=%s", sample->mimetype);
		logger_log(samplecat.logger, "cannot add file: reading file info failed");
		sample_unref(sample);
		return false;
	}

#ifdef CHECK_SIMILAR
	/* check if same file already exists with different path */
	GList* existing;
	if ((existing = samplecat.model->backend.filter_by_audio(sample))) {
		GList* l = existing; int i;
#ifdef INTERACTIVE_IMPORT
		GString* note = g_string_new("Similar or identical file(s) already present in database:\n");
#endif
		for (i=1;l;l=l->next, i++) {
			/* TODO :prompt user: ask to delete one of the files
			 * - import/update the remaining file(s)
			 */
			dbg(1, "found similar or identical file: %s", l->data);
#ifdef INTERACTIVE_IMPORT
			if (i < 10)
				g_string_append_printf(note, "%d: '%s'\n", i, (char*) l->data);
#endif
		}
#ifdef INTERACTIVE_IMPORT
		if (i > 9)
			g_string_append_printf(note, "..\n and %d more.", i - 9);

		g_string_append_printf(note, "Add this file: '%s' ?", sample->full_path);
		if (do_progress_question(note->str) != 1) {
			// 0, aborted: -> whole add_file loop is aborted on next do_progress() call.
			// 1, OK
			// 2, cancelled: -> only this file is skipped
			sample_unref(sample);
			g_string_free(note, true);
			return false;
		}
		g_string_free(note, true);
#endif /* END interactive import */
		g_list_foreach(existing, (GFunc)g_free, NULL);
		g_list_free(existing);
	}
#endif // END CHECK_SIMILAR

	sample->online = 1;
	sample->id = samplecat.model->backend.insert(sample);
	if (sample->id < 0) {
		sample_unref(sample);
		return false;
	}
	dbg(1, "       %s", sample->name);
	dbg(1, "       %s", sample->sample_dir);
	dbg(1, "       %s", sample->full_path);

	samplecat_list_store_add((SamplecatListStore*)samplecat.store, sample);

	samplecat_model_add(samplecat.model);

	result->n_added++;

	sample_unref(sample);
	return true;
}


/**
 *	Scan the directory and try and add any files found.
 */
void
samplecat_application_add_dir (const char* path, ScanResults* result)
{
	PF;

	char filepath[PATH_MAX];
	const gchar* file;
	GError* error = NULL;
	GDir* dir;

	if ((dir = g_dir_open(path, 0, &error))) {
		while ((file = g_dir_read_name(dir))) {
			if (file[0] == '.') continue;

			snprintf(filepath, PATH_MAX, "%s%c%s", path, G_DIR_SEPARATOR, file);
			filepath[PATH_MAX - 1] = '\0';

			if (!g_file_test(filepath, G_FILE_TEST_IS_DIR)) {
				if (g_file_test(filepath, G_FILE_TEST_IS_SYMLINK) && !g_file_test(filepath, G_FILE_TEST_IS_REGULAR)) {
					dbg(0, "ignoring dangling symlink: %s", filepath);
				} else {
					if (samplecat_application_add_file(filepath, result)) {
						logger_log(samplecat.logger, "%i files added", result->n_added);
					}
				}
			}
			// IS_DIR
			else if (app->config.add_recursive) {
				samplecat_application_add_dir(filepath, result);
			}

			g_main_context_dispatch (NULL);
		}
		g_dir_close(dir);
	} else {
		result->n_failed++;

		if (error->code == 2)
			pwarn("%s\n", error->message); // permission denied
		else
			perr("cannot open directory. %i: %s\n", error->code, error->message);
		g_clear_pointer(&error, g_error_free);
	}
}
