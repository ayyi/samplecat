/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2007-2020 Tim Orford <tim@orford.org>                  |
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
#include <string.h>
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <gtk/gtk.h>
#pragma GCC diagnostic warning "-Wdeprecated-declarations"
#include <glib-object.h>
#include "debug/debug.h"
#include <sample.h>
#include "support.h"
#include "model.h"
#include "db/db.h"
#include "samplecat.h"
#include "library.h"
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

static void     application_play_next      ();


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
	play->enable_effect = true;
	play->link_speed_pitch = true;
	play->effect_param[0] = 0.0; /* cent transpose [-100 .. 100] */
	play->effect_param[1] = 0.0; /* semitone transpose [-12 .. 12] */
	play->effect_param[2] = 0.0; /* octave [-3 .. 3] */
	play->playback_speed = 1.0;
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

	void on_filter_changed (Observable* filter, AMVal value, gpointer user_data)
	{
		application_search();
	}
	for(int i = 0; i < N_FILTERS; i++){
		observable_subscribe(samplecat.model->filters3[i], on_filter_changed, NULL);
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


#ifdef WITH_VALGRIND
void
application_free (Application* self)
{
	ConfigContext* ctx = &app->configctx;
	int i = 0;
	ConfigOption* option;
	while((option = ctx->options[i++])){
		g_value_unset(&option->val);
		g_clear_pointer(&ctx->options[i-1], g_free);
	}
}
#endif


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
}


static void
application_instance_init (Application* self)
{
	self->state = NONE;

	play = player_new();

	play->next = application_play_next;
	play->play_selected = application_play_selected;
}


static void
application_finalize (GObject* obj)
{
	G_OBJECT_CLASS (application_parent_class)->finalize(obj);
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
	player_stop();
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
			g_value_set_boolean(&option->val, play->config.loop);
		}
		ctx->options[7] = config_option_new_bool("loop_playback", get_loop_playback);
	}
}


static int  auditioner_nullC() {return 0;}
static void auditioner_null() {;}
static bool auditioner_nullS(Sample* s) {return true;}

static void
_set_auditioner ()
{
	if(!app->no_gui) printf("auditioner: "); fflush(stdout);

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
		play->auditioner = & a_jack;
		if (!play->auditioner->check()) {
			connected = true;
			printf("JACK playback\n");
		}
	}
#endif
#ifdef HAVE_AYYIDBUS
	if(!connected && can_use(app->players, "ayyi")){
		play->auditioner = & a_ayyidbus;
		if (!play->auditioner->check()) {
			connected = true;
			if(!app->no_gui) printf("ayyi_audition\n");
		}
	}
#endif
#ifdef HAVE_GPLAYER
	if(!connected && can_use(app->players, "cli")){
		play->auditioner = & a_gplayer;
		if (!play->auditioner->check()) {
			connected = true;
			printf("using CLI player\n");
		}
	}
#endif
	if (!connected) {
		printf("no playback support\n");
		play->auditioner = & a_null;
	}
}


static void
application_set_auditioner (Application* a)
{
	// The gui is allowed to load before connecting the audio.
	// Connecting the audio can sometimes be very slow.

	// TODO In the case of jack_player, starting jack can block the gui
	//      so this needs to be made properly asynchronous.

	void set_auditioner_on_connected (GError* error, gpointer _)
	{
		if(error){
			statusbar_print(1, "Player: %s", error->message);
		}
	}

	gboolean set_auditioner_on_idle (gpointer data)
	{
		if(!((Application*)data)->no_gui){
			_set_auditioner();

			player_connect(set_auditioner_on_connected, data);
		}
		return G_SOURCE_REMOVE;
	}

	g_idle_add_full(G_PRIORITY_LOW, set_auditioner_on_idle, a, NULL);
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


/*
 *  Path must not contain trailing slash
 */
void
application_scan (const char* path, ScanResults* results)
{
	g_return_if_fail(path);

	app->state = SCANNING;
	worker_go_slow = true;
	statusbar_print(1, "scanning...");

	application_add_dir(path, results);

	gchar* fail_msg = results->n_failed ? g_strdup_printf(", %i failed", results->n_failed) : "";
	gchar* dupes_msg = results->n_dupes ? g_strdup_printf(", %i duplicates", results->n_dupes) : "";
	statusbar_print(1, "add finished: %i files added%s%s", results->n_added, fail_msg, dupes_msg);
	if(results->n_failed) g_free(fail_msg);
	if(results->n_dupes) g_free(dupes_msg);

	app->state = NONE;
	worker_go_slow = false;
}


/*
 *  uri must be "unescaped" before calling this fn. Method string must be removed.
 */
bool
application_add_file (const char* path, ScanResults* result)
{
	/* check if file already exists in the store
	 * -> don't add it again
	 */
	if(samplecat.model->backend.file_exists(path, NULL)) {
		if(!app->no_gui) statusbar_print(1, "duplicate: %s", path);
		if(_debug_) g_warning("duplicate file: %s", path);
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
			if (_debug_) pwarn("cannot add file: file-type is not supported");
			statusbar_print(1, "cannot add file: file-type is not supported");
		}
		return false;
	}

	if(_debug_ && app->no_gui){ printf("%s\n", path); fflush(stdout); }

	if(!sample_get_file_info(sample)){
		pwarn("cannot add file: reading file info failed. type=%s", sample->mimetype);
		if(!app->no_gui) statusbar_print(1, "cannot add file: reading file info failed");
		sample_unref(sample);
		return false;
	}

#ifdef CHECK_SIMILAR
	/* check if same file already exists with different path */
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
application_add_dir (const char* path, ScanResults* result)
{
	PF;

	char filepath[PATH_MAX];
	const gchar* file;
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
	if(play->status == PLAY_PAUSED){
		if(play->auditioner->playpause){
			play->auditioner->playpause(false);
		}
		play->status = PLAY_PLAYING;
		return;
	}

	if(sample) dbg(1, "%s", sample->name);

	if(player_play(sample)){
		if(play->queue)
			statusbar_print(1, "playing 1 of %i ...", g_list_length(play->queue));
		else
			statusbar_print(1, "");
#ifndef USE_GDL
		if(app->view_options[SHOW_PLAYER].value) show_player(true);
#endif

	}else{
		statusbar_print(1, "File not playable");
	}
}


void
application_pause()
{
	if(play->auditioner->playpause){
		play->auditioner->playpause(true);
	}
	play->status = PLAY_PAUSED;
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


#define ADD_TO_QUEUE(S) \
	(app->play.queue = g_list_append(app->play.queue, sample_ref(S)))
#define REMOVE_FROM_QUEUE(S) \
	(play->queue = g_list_remove(play->queue, S), sample_unref(S))


void
application_play_all ()
{
	if(play->queue){
		pwarn("already playing");
		return;
	}

	gboolean foreach_func(GtkTreeModel* model, GtkTreePath* path, GtkTreeIter* iter, gpointer user_data)
	{
		//ADD_TO_QUEUE(samplecat_list_store_get_sample_by_path(path));
		play->queue = g_list_append(play->queue, samplecat_list_store_get_sample_by_path(path)); // there is a ref already added, so another one is not needed when adding to the queue.
		return G_SOURCE_CONTINUE;
	}
	gtk_tree_model_foreach(GTK_TREE_MODEL(samplecat.store), foreach_func, NULL);

	if(play->queue){
		if(play->auditioner->play_all) play->auditioner->play_all(); // TODO remove this fn.
		application_play_next();
	}
}


void
application_play_next ()
{
	PF;
	if(play->queue){
		Sample* next = play->queue->data;
		REMOVE_FROM_QUEUE(next);
		dbg(1, "%s", next->full_path);

		if(play->sample){
			play->position = UINT32_MAX;
			g_signal_emit_by_name (play, "position");
		}

		application_play(next);
	}else{
		dbg(1, "play_all finished.");
		player_stop();
	}
}


void
application_play_path (const char* path)
{
	if(play->sample) player_stop();

	Sample* s = sample_new_from_filename((char*)path, false);
	g_assert(s->ref_count == 1); // will be unreffed and freed in application_on_play_finished()
	if(s){
		application_play(s);
	}
}

