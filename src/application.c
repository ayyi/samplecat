/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. https://ayyi.github.io/samplecat/    |
 | copyright (C) 2007-2025 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#include "config.h"
#include <string.h>
#include <gtk/gtk.h>
#include "debug/debug.h"
#include "gtk/icon_theme.h"
#include "file_manager/pixmaps.h"
#include "types.h"
#include "sample.h"
#include "support.h"
#include "model.h"
#include "db/db.h"
#include "samplecat.h"
#include "library.h"
#include "progress_dialog.h"
#include "window.h"
#include "gtk/utils.h"
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
static void     application_play_selected  ();
static void     pause_toggle_activate      (GSimpleAction* action, GVariant* parameter, gpointer app);
static void     next_activate              ();

#ifdef GTK4_TODO
static void
clear_filter (GSimpleAction* action, GVariant* parameter, gpointer app)
{
}
#endif


static GActionEntry app_entries[] =
{
	{ "play-all", application_play_all, },
	{ "player-stop", player_stop, },
	{ "player-play", application_play_selected, },
	{ "player-pause", pause_toggle_activate},
	{ "player-next", next_activate},
#ifdef GTK4_TODO
	{ "clear-filter", clear_filter, }
#endif
};


Application*
application_construct (GType object_type)
{
	Application* app = g_object_new (object_type, "application-id", "org.ayyi.samplecat", "flags", G_APPLICATION_NON_UNIQUE, NULL);
	app->gui_thread = pthread_self();

	return app;
}


Application*
application_new ()
{
	PF;

	Application* self = application_construct (TYPE_APPLICATION);

	colour_box_init();

#if (defined HAVE_JACK)
	play->enable_effect = true;
	play->link_speed_pitch = true;
	play->effect_param[0] = 0.0; /* cent transpose [-100 .. 100] */
	play->effect_param[1] = 0.0; /* semitone transpose [-12 .. 12] */
	play->effect_param[2] = 0.0; /* octave [-3 .. 3] */
	play->playback_speed = 1.0;
#endif

	return self;
}


#ifdef WITH_VALGRIND
void
application_free (Application* self)
{
	ConfigContext* ctx = &app->configctx;
	int i = 0;
	ConfigOption* option;
	while ((option = ctx->options[i++])) {
		g_value_unset(&option->val);
		g_clear_pointer(&ctx->options[i-1], g_free);
	}
}
#endif


static void
application_activate (GApplication* base)
{
	PF;

	application_set_ready ();

	G_APPLICATION_CLASS (application_parent_class)->activate(base);

	// Pick a valid icon theme
	// Preferably from the config file, otherwise from the hardcoded list
	const char* themes[] = {NULL, "oxygen", "breeze", NULL};
	themes[0] = g_value_get_string(&app->configctx.options[CONFIG_ICON_THEME]->val);
	const char* theme = find_icon_theme(themes[0] ? &themes[0] : &themes[1]);
	icon_theme_set_theme(theme);

    GtkCssProvider* provider = gtk_css_provider_new ();
#if 0
    gtk_css_provider_load_from_resource (provider, "/samplecat/resources/style.css");
#else
    gtk_css_provider_load_from_resource (provider, "/samplecat/resources/dark.css");
#endif
	gtk_style_context_add_provider_for_display (gdk_display_get_default(), GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	g_object_unref (provider);

	pixmaps_init();
	window_new (GTK_APPLICATION (base), NULL);
	statusbar_print(2, PACKAGE_NAME" "PACKAGE_VERSION);
	application_search();

#ifdef USE_PROFILING
	extern void instrumentation_print ();
	if (_debug_ > 1) instrumentation_print();
#endif
}


static GObject*
application_constructor (GType type, guint n_construct_properties, GObjectConstructParam* construct_properties)
{
	return G_OBJECT_CLASS (application_parent_class)->constructor (type, n_construct_properties, construct_properties);
}


static void
application_class_init (ApplicationClass* klass)
{
	application_parent_class = g_type_class_peek_parent (klass);

	G_OBJECT_CLASS (klass)->constructor = application_constructor;
	G_OBJECT_CLASS (klass)->finalize = application_finalize;

	((GApplicationClass*)klass)->activate = application_activate;

	g_signal_new ("config_loaded", TYPE_APPLICATION, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	g_signal_new ("search_starting", TYPE_APPLICATION, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	g_signal_new ("icon_theme", TYPE_APPLICATION, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__STRING, G_TYPE_NONE, 1, G_TYPE_STRING);
	g_signal_new ("on_quit", TYPE_APPLICATION, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	g_signal_new ("theme_changed", TYPE_APPLICATION, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	g_signal_new ("layout_changed", TYPE_APPLICATION, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

	types_init();
}


static void
application_instance_init (Application* self)
{
	play = player_new();

	play->next = application_play_next;
	play->play_selected = application_play_selected;

	g_action_map_add_action_entries (G_ACTION_MAP (self), app_entries, G_N_ELEMENTS (app_entries), self);

	gtk_application_set_accels_for_action (GTK_APPLICATION (self), "app.play", (const char*[]){ "<Ctrl>P", NULL });
	{
		GAction* action = g_action_map_lookup_action (G_ACTION_MAP(app), "play-all");
		if (action) {
			g_simple_action_set_enabled (G_SIMPLE_ACTION(action), false);
		}
	}

	void on_filter_changed (Observable* filter, AGlVal value, gpointer user_data)
	{
		application_search();
	}
	for (int i = 0; i < N_FILTERS; i++) {
		agl_observable_subscribe (samplecat.model->filters3[i], on_filter_changed, NULL);
	}

	application_set_auditioner(self);

	void icon_theme_changed(Application* application, char* theme, gpointer data){ application_search(); }
	g_signal_connect((gpointer)app, "icon-theme", G_CALLBACK(icon_theme_changed), NULL);

	void listmodel__sample_modified (SamplecatModel* m, Sample* sample, int prop, void* val, gpointer _app)
	{
		samplecat_list_store_on_sample_modified((SamplecatListStore*)samplecat.store, sample, prop, val);
	}
	g_signal_connect((gpointer)samplecat.model, "sample-changed", G_CALLBACK(listmodel__sample_modified), app);

	void log_message (GObject* o, char* message, gpointer _)
	{
		dbg(1, "---> %s", message);
		statusbar_print(1, PACKAGE_NAME". Version "PACKAGE_VERSION);
	}
	g_signal_connect(samplecat.logger, "message", G_CALLBACK(log_message), NULL);

	void on_player_state_change (GObject* o, GParamSpec*, gpointer _)
	{
		dbg(1, "---> %s", g_enum_to_string (PLAYER_TYPE_STATE, play->state));
		statusbar_print(1, "%s", g_enum_to_string (PLAYER_TYPE_STATE, play->state));

		g_simple_action_set_enabled (G_SIMPLE_ACTION(g_action_map_lookup_action (G_ACTION_MAP(app), "player-stop")), play->state > PLAYER_STOPPED);
		g_simple_action_set_enabled (G_SIMPLE_ACTION(g_action_map_lookup_action (G_ACTION_MAP(app), "player-play")), play->state > PLAYER_UNAVAILABLE && play->state < PLAYER_PLAY_PENDING);
		GAction* action = g_action_map_lookup_action (G_ACTION_MAP(app), "play-all");
		if (action) {
			g_simple_action_set_enabled (G_SIMPLE_ACTION(action), true);
		}
		g_simple_action_set_enabled (G_SIMPLE_ACTION(g_action_map_lookup_action (G_ACTION_MAP(app), "player-pause")), play->state != PLAYER_STOPPED);
	}
	g_signal_connect(play, "notify::state", G_CALLBACK(on_player_state_change), NULL);

	void menu_on_audio_ready (gpointer _, gpointer __)
	{
#ifdef GTK4_TODO
		if (play->auditioner->seek) {
			gtk_widget_show(widgets.loop_playback);
		}
#endif
	}
	am_promise_add_callback(play->ready, menu_on_audio_ready, NULL);

#ifdef DEBUG
	char** actions = gtk_application_list_action_descriptions (GTK_APPLICATION(self));
	if (!actions) dbg(0, "no actions found with accelerators");
	printf("actions:\n");
	char* action;
	for (int i=0;(action = actions[i]);i++) {
		printf("  * %s\n", action);
	}
	g_strfreev(actions);
#endif
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
	if (g_once_init_enter ((gsize*)&application_type_id__volatile)) {
		static const GTypeInfo g_define_type_info = { sizeof (ApplicationClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) application_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (Application), 0, (GInstanceInitFunc) application_instance_init, NULL };
		GType application_type_id = g_type_register_static (samplecat_application_get_type (), "Application", &g_define_type_info, 0);
		g_once_init_leave (&application_type_id__volatile, application_type_id);
	}
	return application_type_id__volatile;
}


void
application_quit (Application* app)
{
	PF;
	player_stop();
	g_signal_emit_by_name (app, "on-quit");
}


void
application_set_ready ()
{
	((Application*)app)->loaded = true;
	dbg(1, "loaded");

	// make config saveable
	// note that this is not done until the application is fully loaded
	{
		ConfigContext* ctx = &app->configctx;

		void get_width (ConfigOption* option)
		{
			gint width = 0;
			GtkWidget* window = (GtkWidget*)gtk_application_get_active_window(GTK_APPLICATION(app));
			if (window && gtk_widget_get_realized(window)) {
				width = gtk_widget_get_width(window);
			}
			g_value_set_int(&option->val, width);
		}
		ctx->options[CONFIG_WINDOW_WIDTH] = config_option_new_int("window_width", get_width, 20, 4096);

		void get_height (ConfigOption* option)
		{
			GtkWidget* window = (GtkWidget*)gtk_application_get_active_window(GTK_APPLICATION(app));

			gint height = 0;
			if (window && gtk_widget_get_realized(window)) {
				height = gtk_widget_get_height(window);
			}
			g_value_set_int(&option->val, height);
		}
		ctx->options[CONFIG_WINDOW_HEIGHT] = config_option_new_int("window_height", get_height, 20, 4096);

		void get_col1_width (ConfigOption* option)
		{
			GtkWidget* library = find_widget_by_type_deep((GtkWidget*)find_panel("Library"), GTK_TYPE_TREE_VIEW);
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
			GtkTreeViewColumn* column = gtk_tree_view_get_column(GTK_TREE_VIEW(library), 1);
#pragma GCC diagnostic warning "-Wdeprecated-declarations"
			g_value_set_int(&option->val, gtk_tree_view_column_get_width(column));
		}
		ctx->options[CONFIG_COL1_WIDTH] = config_option_new_int("col1_width", get_col1_width, 10, 1024);

		void get_auditioner (ConfigOption* option)
		{
			if (strlen(app->config.auditioner))
				g_value_set_string(&option->val, app->config.auditioner);
		}
		ctx->options[4] = config_option_new_string("auditioner", get_auditioner);

		void get_colours (ConfigOption* option)
		{
			for (int i=1;i<PALETTE_SIZE;i++) {
				char keyname[32];
				snprintf(keyname, 32, "colorkey%02d", i);
				g_key_file_set_value(app->configctx.key_file, "Samplecat", keyname, app->config.colour[i]);
			}
		}
		ctx->options[5] = config_option_new_manual(get_colours);

		void get_add_recursive (ConfigOption* option)
		{
			g_value_set_boolean(&option->val, app->config.add_recursive);
		}
		ctx->options[6] = config_option_new_bool("add_recursive", get_add_recursive);

		void get_loop_playback (ConfigOption* option)
		{
			g_value_set_boolean(&option->val, play->config.loop);
		}
		ctx->options[7] = config_option_new_bool("loop_playback", get_loop_playback);
	}
}


static int  auditioner_nullC() { return 0; }
static void auditioner_null() {;}
static bool auditioner_nullS(Sample* s) { return true; }

static void
_set_auditioner ()
{
	printf("auditioner: "); fflush(stdout);

	const static Auditioner a_null = {
		&auditioner_nullC,
		&auditioner_null,
		&auditioner_null,
		&auditioner_nullS,
		NULL,
		&auditioner_null,
		NULL,
	};

	bool connected = false;
#ifdef HAVE_JACK
	if (!connected && can_use(app->players, "jack")) {
		play->auditioner = & a_jack;
		if (!play->auditioner->check()) {
			connected = true;
			printf("JACK playback\n");
		}
	}
#endif
#ifdef HAVE_AYYIDBUS
	if (!connected && can_use(app->players, "ayyi")) {
		play->auditioner = & a_ayyidbus;
		if (!play->auditioner->check()) {
			connected = true;
			printf("ayyi_audition\n");
		}
	}
#endif
#ifdef HAVE_GPLAYER
	if (!connected && can_use(app->players, "cli")) {
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
		if (error) {
			statusbar_print(1, "Player: %s", error->message);
		}
	}

	gboolean set_auditioner_on_idle (gpointer data)
	{
		if (!((Application*)data)->no_gui) {
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
application_search ()
{
	PF;

	if (BACKEND_IS_NULL) return;

	g_signal_emit_by_name(app, "search-starting");

	samplecat_list_store_do_search((SamplecatListStore*)samplecat.store);
}


GList*
application_get_selection ()
{
	return listview__get_selection();
}


void
application_play (Sample* sample)
{
	if (play->state == PLAYER_PAUSED) {
		if (play->auditioner->pause) {
			play->auditioner->pause(false);
		}
		player_set_state(PLAYER_PLAYING);
		return;
	}

	if (sample) dbg(1, "%s", sample->name);

	if (player_play(sample)) {
		if (play->queue)
			statusbar_print(1, "playing 1 of %i ...", g_list_length(play->queue));
		else
			statusbar_print(1, "");

	} else {
		statusbar_print(1, "File not playable");
	}
}


static void
application_play_selected ()
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


static void
pause_toggle_activate (GSimpleAction* action, GVariant* parameter, gpointer app)
{
	play->state == PLAYER_PAUSED
		? application_play(NULL)
		: player_pause();
}


static void
next_activate ()
{
}


#define ADD_TO_QUEUE(S) \
	(app->play.queue = g_list_append(app->play.queue, sample_ref(S)))
#define REMOVE_FROM_QUEUE(S) \
	(play->queue = g_list_remove(play->queue, S), sample_unref(S))


void
application_play_all ()
{
	if (play->queue) {
		pwarn("already playing");
		return;
	}

	gboolean foreach_func (GtkTreeModel* model, GtkTreePath* path, GtkTreeIter* iter, gpointer user_data)
	{
		//ADD_TO_QUEUE(samplecat_list_store_get_sample_by_path(path));
		play->queue = g_list_append(play->queue, samplecat_list_store_get_sample_by_path(path)); // there is a ref already added, so another one is not needed when adding to the queue.
		return G_SOURCE_CONTINUE;
	}
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	gtk_tree_model_foreach(GTK_TREE_MODEL(samplecat.store), foreach_func, NULL);
#pragma GCC diagnostic warning "-Wdeprecated-declarations"

	if (play->queue) {
		if (play->auditioner->play_all) play->auditioner->play_all(); // TODO remove this fn.
		application_play_next();
	}
}


void
application_play_next ()
{
	PF;
	if (play->queue) {
		Sample* next = play->queue->data;
		REMOVE_FROM_QUEUE(next);
		dbg(1, "%s", next->full_path);

		if (play->sample) {
			play->position = UINT32_MAX;
			g_signal_emit_by_name (play, "position");
		}

		application_play(next);
	} else {
		dbg(1, "play_all finished.");
		player_stop();
	}
}


void
application_play_path (const char* path)
{
	if (play->sample) player_stop();

	Sample* s = sample_new_from_filename((char*)path, false);
	g_assert(s->ref_count == 1); // will be unreffed and freed in application_on_play_finished()
	if (s) {
		application_play(s);
	}
}

