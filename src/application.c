/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2007-2016 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
* This file is partially based on vala/application.vala but is manually synced.
*
*/
#include "config.h"
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <glib-object.h>
#include "debug/debug.h"
#include <sample.h>
#include "support.h"
#include "model.h"
#include "db/db.h"
#include "samplecat.h"
#include "list_store.h"
#include "listview.h"
#include "progress_dialog.h"
#ifndef USE_GDL
#include "window.h"
#endif
#include "application.h"

extern char theme_name[64];

#ifdef HAVE_AYYIDBUS
extern Auditioner a_ayyidbus;
#endif
#ifdef HAVE_JACK
extern const Auditioner a_jack;
#endif
#ifdef HAVE_GPLAYER
extern const Auditioner a_gplayer;
#endif

#define CHECK_SIMILAR
#undef INTERACTIVE_IMPORT

extern void colour_box_init();

static gpointer application_parent_class = NULL;

enum  {
	APPLICATION_DUMMY_PROPERTY
};
static GObject* application_constructor    (GType, guint n_construct_properties, GObjectConstructParam*);
static void     application_finalize       (GObject*);
static void     application_set_auditioner (Application*);

static guint    play_timeout_id;


Application*
application_construct (GType object_type)
{
	Application* app = g_object_new (object_type, NULL);
	app->cache_dir = g_build_filename (g_get_home_dir(), ".config", PACKAGE, "cache", NULL);
	app->configctx.dir = g_build_filename (g_get_home_dir(), ".config", PACKAGE, NULL);
	return app;
}


Application*
application_new ()
{
	app = application_construct (TYPE_APPLICATION);

	colour_box_init();

	app->configctx.filename = g_strdup_printf("%s/.config/" PACKAGE "/" PACKAGE, g_get_home_dir());

#if (defined HAVE_JACK)
	app->enable_effect = true;
	app->link_speed_pitch = true;
	app->effect_param[0] = 0.0; /* cent transpose [-100 .. 100] */
	app->effect_param[1] = 0.0; /* semitone transpose [-12 .. 12] */
	app->effect_param[2] = 0.0; /* octave [-3 .. 3] */
	app->playback_speed = 1.0;
#endif

	samplecat_init();

	{
		ConfigContext* ctx = &app->configctx;
		app->configctx.options = g_malloc0(CONFIG_MAX * sizeof(ConfigOption*));

		void get_theme_name(ConfigOption* option)
		{
			g_value_set_string(&option->val, theme_name);
		}
		ctx->options[CONFIG_ICON_THEME] = config_option_new_string("icon_theme", get_theme_name);
	}

	void on_filter_changed(GObject* _filter, gpointer user_data)
	{
		//SamplecatFilter* filter = (SamplecatFilter*)_filter;
		application_search();
	}

	GList* l = samplecat.model->filters_;
	for(;l;l=l->next){
		g_signal_connect((SamplecatFilter*)l->data, "changed", G_CALLBACK(on_filter_changed), NULL);
	}

	application_set_auditioner(app);

	void icon_theme_changed(Application* application, char* theme, gpointer data){ application_search(); }
	g_signal_connect((gpointer)app, "icon-theme", G_CALLBACK(icon_theme_changed), NULL);

	void listmodel__sample_changed(SamplecatModel* m, Sample* sample, int prop, void* val, gpointer _app)
	{
		samplecat_list_store_on_sample_changed((SamplecatListStore*)samplecat.store, sample, prop, val);
	}
	g_signal_connect((gpointer)samplecat.model, "sample-changed", G_CALLBACK(listmodel__sample_changed), app);

	void log_message(GObject* o, char* message, gpointer _)
	{
		dbg(1, "---> %s", message);
		statusbar_print(1, PACKAGE_NAME". Version "PACKAGE_VERSION);
	}
	g_signal_connect(samplecat.logger, "message", G_CALLBACK(log_message), NULL);

	return app;
}


void
application_emit_icon_theme_changed (Application* self, const gchar* s)
{
	g_return_if_fail (self);
	g_return_if_fail (s);

	g_signal_emit_by_name (self, "icon-theme", s);
}


static GObject*
application_constructor (GType type, guint n_construct_properties, GObjectConstructParam* construct_properties)
{
	GObjectClass* parent_class = G_OBJECT_CLASS (application_parent_class);
	GObject* obj = parent_class->constructor (type, n_construct_properties, construct_properties);
	return obj;
}


static void
application_class_init (ApplicationClass* klass)
{
	application_parent_class = g_type_class_peek_parent (klass);
	G_OBJECT_CLASS (klass)->constructor = application_constructor;
	G_OBJECT_CLASS (klass)->finalize = application_finalize;
	g_signal_new ("config_loaded", TYPE_APPLICATION, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	g_signal_new ("search_starting", TYPE_APPLICATION, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	g_signal_new ("icon_theme", TYPE_APPLICATION, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__STRING, G_TYPE_NONE, 1, G_TYPE_STRING);
	g_signal_new ("on_quit", TYPE_APPLICATION, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	g_signal_new ("theme_changed", TYPE_APPLICATION, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	g_signal_new ("layout_changed", TYPE_APPLICATION, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	g_signal_new ("audio_ready", TYPE_APPLICATION, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	g_signal_new ("play_start", TYPE_APPLICATION, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	g_signal_new ("play_stop", TYPE_APPLICATION, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	g_signal_new ("play_position", TYPE_APPLICATION, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}


static void
application_instance_init (Application* self)
{
	self->state = NONE;
}


static void
application_finalize (GObject* obj)
{
	G_OBJECT_CLASS (application_parent_class)->finalize (obj);
}


GType
application_get_type ()
{
	static volatile gsize application_type_id__volatile = 0;
	if (g_once_init_enter (&application_type_id__volatile)) {
		static const GTypeInfo g_define_type_info = { sizeof (ApplicationClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) application_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (Application), 0, (GInstanceInitFunc) application_instance_init, NULL };
		GType application_type_id;
		application_type_id = g_type_register_static (G_TYPE_OBJECT, "Application", &g_define_type_info, 0);
		g_once_init_leave (&application_type_id__volatile, application_type_id);
	}
	return application_type_id__volatile;
}


void
application_quit(Application* app)
{
	PF;
	application_stop();
	g_signal_emit_by_name (app, "on-quit");
}


void
application_set_ready()
{
	app->loaded = true;
	dbg(1, "loaded");

	// make config saveable
	// note that this is not done until the application is fully loaded
	{
		ConfigContext* ctx = &app->configctx;

		void get_width(ConfigOption* option)
		{
			gint width = 0;
			if(app->window && GTK_WIDGET_REALIZED(app->window)){
				gtk_window_get_size(GTK_WINDOW(app->window), &width, NULL);
			}
			g_value_set_int(&option->val, width);
		}
		ctx->options[CONFIG_WINDOW_WIDTH] = config_option_new_int("window_width", get_width, 20, 4096);

		void get_height(ConfigOption* option)
		{
			gint height = 0;
			if(app->window && GTK_WIDGET_REALIZED(app->window)){
				gtk_window_get_size(GTK_WINDOW(app->window), NULL, &height);
			}
			g_value_set_int(&option->val, height);
		}
		ctx->options[CONFIG_WINDOW_HEIGHT] = config_option_new_int("window_height", get_height, 20, 4096);

		void get_col1_width(ConfigOption* option)
		{
			GtkTreeViewColumn* column = gtk_tree_view_get_column(GTK_TREE_VIEW(app->libraryview->widget), 1);
			g_value_set_int(&option->val, gtk_tree_view_column_get_width(column));
		}
		ctx->options[CONFIG_COL1_WIDTH] = config_option_new_int("col1_width", get_col1_width, 10, 1024);

		void get_auditioner(ConfigOption* option)
		{
			if(strlen(app->config.auditioner))
				g_value_set_string(&option->val, app->config.auditioner);
		}
		ctx->options[4] = config_option_new_string("auditioner", get_auditioner);

		void get_colours(ConfigOption* option)
		{
			int i;
			for (i=1;i<PALETTE_SIZE;i++) {
				char keyname[32];
				snprintf(keyname, 32, "colorkey%02d", i);
				g_key_file_set_value(app->configctx.key_file, "Samplecat", keyname, app->config.colour[i]);
			}
		}
		ctx->options[5] = config_option_new_manual(get_colours);

		void get_add_recursive(ConfigOption* option)
		{
			g_value_set_boolean(&option->val, app->config.add_recursive);
		}
		ctx->options[6] = config_option_new_bool("add_recursive", get_add_recursive);

		void get_loop_playback(ConfigOption* option)
		{
			g_value_set_boolean(&option->val, app->config.loop_playback);
		}
		ctx->options[7] = config_option_new_bool("loop_playback", get_loop_playback);
	}
}


static int  auditioner_nullC() {return 0;}
static void auditioner_null() {;}
static bool auditioner_nullS(Sample* s) {return true;}

static void
_set_auditioner()
{
	printf("auditioner: "); fflush(stdout);
	const static Auditioner a_null = {
		&auditioner_nullC,
		&auditioner_null,
		&auditioner_null,
		&auditioner_nullS,
		NULL,
		&auditioner_null,
		NULL, NULL, NULL
	};

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


static void
application_set_auditioner(Application* a)
{
	// The gui is allowed to load before connecting the audio.
	// Connecting the audio can sometimes be very slow.

	// TODO starting jack can block the gui so this needs to be moved to another thread.

	void set_auditioner_on_connected(gpointer _)
	{
		g_signal_emit_by_name (app, "audio-ready");
	}

	bool set_auditioner_on_idle(gpointer data)
	{
		_set_auditioner();

		app->auditioner->connect(set_auditioner_on_connected, data);

		return G_SOURCE_REMOVE;
	}

	if(!a->no_gui) // TODO too early for this flag to be set ?
		g_idle_add_full(G_PRIORITY_LOW, set_auditioner_on_idle, NULL, NULL);
}


/**
 * fill the display with the results matching the current set of filters.
 */
void
application_search()
{
	PF;

	if(BACKEND_IS_NULL) return;

	g_signal_emit_by_name(app, "search-starting");

	samplecat_list_store_do_search((SamplecatListStore*)samplecat.store);
}


void
application_scan(const char* path, ScanResults* results)
{
	// path must not contain trailing slash

	g_return_if_fail(path);

	app->state = SCANNING;
	statusbar_print(1, "scanning...");

	application_add_dir(path, results);

	gchar* fail_msg = results->n_failed ? g_strdup_printf(", %i failed", results->n_failed) : "";
	gchar* dupes_msg = results->n_dupes ? g_strdup_printf(", %i duplicates", results->n_dupes) : "";
	statusbar_print(1, "add finished: %i files added%s%s", results->n_added, fail_msg, dupes_msg);
	if(results->n_failed) g_free(fail_msg);
	if(results->n_dupes) g_free(dupes_msg);
	app->state = NONE;
}


bool
application_add_file(const char* path, ScanResults* result)
{
	/*
	 *  uri must be "unescaped" before calling this fn. Method string must be removed.
	 */

	/* check if file already exists in the store
	 * -> don't add it again
	 */
	if(samplecat.model->backend.file_exists(path, NULL)) {
		if(!app->no_gui) statusbar_print(1, "duplicate: not re-adding a file already in db.");
		g_warning("duplicate file: %s", path);
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

	if(!app->no_gui) dbg(1, "%s", path);

	if(BACKEND_IS_NULL) return false;

	Sample* sample = sample_new_from_filename((char*)path, false);
	if (!sample) {
		if (app->state != SCANNING){
			if (_debug_) gwarn("cannot add file: file-type is not supported");
			statusbar_print(1, "cannot add file: file-type is not supported");
		}
		return false;
	}

	if(app->no_gui){ printf("%s\n", path); fflush(stdout); }

	if(!sample_get_file_info(sample)){
		gwarn("cannot add file: reading file info failed. type=%s", sample->mimetype);
		if(!app->no_gui) statusbar_print(1, "cannot add file: reading file info failed");
		sample_unref(sample);
		return false;
	}
#ifdef CHECK_SIMILAR
	/* check if /same/ file already exists w/ different path */
	GList* existing;
	if((existing = samplecat.model->backend.filter_by_audio(sample))) {
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
#endif /* END check for similar files on import */

	sample->online = 1;
	sample->id = samplecat.model->backend.insert(sample);
	if (sample->id < 0) {
		sample_unref(sample);
		return false;
	}

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
application_add_dir(const char* path, ScanResults* result)
{
	PF;

	char filepath[PATH_MAX];
	G_CONST_RETURN gchar* file;
	GError* error = NULL;
	GDir* dir;

	if((dir = g_dir_open(path, 0, &error))){
		while((file = g_dir_read_name(dir))){
			if(file[0] == '.') continue;

			snprintf(filepath, PATH_MAX, "%s%c%s", path, G_DIR_SEPARATOR, file);
			filepath[PATH_MAX - 1] = '\0';
			if (do_progress(0, 0)) break;

			if(!g_file_test(filepath, G_FILE_TEST_IS_DIR)){
				if(g_file_test(filepath, G_FILE_TEST_IS_SYMLINK) && !g_file_test(filepath, G_FILE_TEST_IS_REGULAR)){
					dbg(0, "ignoring dangling symlink: %s", filepath);
				}else{
					if(application_add_file(filepath, result)){
						if(!app->no_gui) statusbar_print(1, "%i files added", result->n_added);
					}
				}
			}
			// IS_DIR
			else if(app->config.add_recursive){
				application_add_dir(filepath, result);
			}
		}
		//hide_progress(); ///no: keep window open until last recursion.
		g_dir_close(dir);
	}else{
		result->n_failed++;

		if(error->code == 2)
			pwarn("%s\n", error->message); // permission denied
		else
			perr("cannot open directory. %i: %s\n", error->code, error->message);
		g_error_free0(error);
	}
}


void
application_play(Sample* sample)
{
	bool play_update(gpointer _)
	{
		if(app->auditioner->position) app->play.position = app->auditioner->position();
		if(app->play.position != UINT_MAX) g_signal_emit_by_name (app, "play-position");
		return TIMER_CONTINUE;
	}

	if(app->play.status == PLAY_PAUSED){
		if(app->auditioner->playpause){
			app->auditioner->playpause(false);
		}
		app->play.status = PLAY_PLAYING;
		// TODO
		return;
	}

	if(sample) dbg(1, "%s", sample->name);
  	app->play.status = PLAY_PLAY_PENDING;

	if(app->auditioner->play(sample)){
		sample_unref0(app->play.sample);

  		app->play.status = PLAY_PLAYING;
		app->play.sample = sample_ref(sample);
		app->play.position = UINT_MAX;

		if(app->play.queue)
			statusbar_print(1, "playing 1 of %i ...", g_list_length(app->play.queue));
		else
			statusbar_print(1, "");
#ifndef USE_GDL
		if(app->view_options[SHOW_PLAYER].value) show_player(true);
#endif
		g_signal_emit_by_name (app, "play-start");

		if(app->auditioner->position && !play_timeout_id) {
			GSource* source = g_timeout_source_new (50);
			g_source_set_callback (source, play_update, NULL, NULL);
			play_timeout_id = g_source_attach (source, NULL);
		}
	}else{
		statusbar_print(1, "File not playable");
	}
}


void
application_stop()
{
	PF;
	if (app->play.queue) {
		g_list_foreach(app->play.queue, (GFunc)sample_unref, NULL);
		g_list_free0(app->play.queue);
	}

	app->auditioner->stop();
  	app->play.status = PLAY_STOPPED;

	application_on_play_finished();
}


void
application_pause()
{
	if(app->auditioner->playpause){
		app->auditioner->playpause(true);
	}
  	app->play.status = PLAY_PAUSED;
}


void
application_play_selected()
{
	PF;
	Sample* sample = samplecat.model->selection;
	if (!sample) {
		statusbar_print(1, "Not playing: nothing selected");
		return;
	}
	dbg(0, "MIDI PLAY %s", sample->full_path);

	application_play(sample);
}

#define ADD_TO_QUEUE(S) (app->play.queue = g_list_append(app->play.queue, sample_ref(S)))
#define REMOVE_FROM_QUEUE(S) (app->play.queue = g_list_remove(app->play.queue, S), sample_unref(S))

void
application_play_all ()
{
	if(app->play.queue){
		pwarn("already playing");
		return;
	}

	bool foreach_func(GtkTreeModel* model, GtkTreePath* path, GtkTreeIter* iter, gpointer user_data)
	{
		//ADD_TO_QUEUE(sample_get_from_model(path));
		app->play.queue = g_list_append(app->play.queue, sample_get_from_model(path)); // there is a ref already added, so another one is not needed when adding to the queue.
		return false; // continue
	}
	gtk_tree_model_foreach(GTK_TREE_MODEL(samplecat.store), foreach_func, NULL);

	if(app->play.queue){
		if(app->auditioner->play_all) app->auditioner->play_all(); // TODO remove this fn.
		application_play_next();
	}
}


void
application_play_next ()
{
	PF;
	if(app->play.queue){
		Sample* next = app->play.queue->data;
		REMOVE_FROM_QUEUE(next);
		dbg(1, "%s", next->full_path);

		if(app->play.sample){
			app->play.position = UINT32_MAX;
			g_signal_emit_by_name (app, "play-position");
		}

		application_play(next);
	}else{
		dbg(1, "play_all finished.");
		application_stop();
	}
}


void
application_play_path (const char* path)
{
	if(app->play.sample) application_stop();

	Sample* s = sample_new_from_filename((char*)path, false);
	g_assert(s->ref_count == 1); // will be unreffed and freed in application_on_play_finished()
	if(s){
		application_play(s);
	}
}


void
application_set_position (gint64 frames)
{
	if(app->play.sample && app->play.sample->sample_rate){
		app->play.position = (frames * 1000) / app->play.sample->sample_rate;
		g_signal_emit_by_name (app, "play-position");
	}
}


void
application_on_play_finished ()
{
	if (play_timeout_id) {
		g_source_destroy(g_main_context_find_source_by_id(NULL, play_timeout_id));
		play_timeout_id = 0;
	}

	if(_debug_) printf("PLAY STOP\n");

	g_signal_emit_by_name (app, "play-stop");

	sample_unref0(app->play.sample);
}

