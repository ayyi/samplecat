/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2007-2019 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#include "debug/debug.h"
#include "samplecat/sample.h"
#include "samplecat/support.h"
#include "player.h"

Player* play = NULL;

static guint play_timeout_id = 0;

static gpointer player_parent_class = NULL;

enum  {
	PLAYER_PLAY_SIGNAL,
	PLAYER_STOP_SIGNAL,
	PLAYER_POSITION_SIGNAL,
	PLAYER_NUM_SIGNALS
};

static guint player_signals[PLAYER_NUM_SIGNALS] = {0};


Player*
player_construct (GType object_type)
{
	Player* self = (Player*) g_object_new (object_type, NULL);
	return self;
}


Player*
player_new ()
{
	return player_construct (TYPE_PLAYER);
}


static GObject*
player_constructor (GType type, guint n_construct_properties, GObjectConstructParam* construct_properties)
{
	GObjectClass* parent_class = G_OBJECT_CLASS (player_parent_class);
	GObject* obj = parent_class->constructor (type, n_construct_properties, construct_properties);

	return obj;
}


static void
player_class_init (PlayerClass* klass)
{
	player_parent_class = g_type_class_peek_parent (klass);
	G_OBJECT_CLASS (klass)->constructor = player_constructor;

	player_signals[PLAYER_PLAY_SIGNAL] = g_signal_new ("play", TYPE_PLAYER, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	player_signals[PLAYER_STOP_SIGNAL] = g_signal_new ("stop", TYPE_PLAYER, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	player_signals[PLAYER_POSITION_SIGNAL] = g_signal_new ("position", TYPE_PLAYER, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}


static void
player_instance_init (Player* player)
{
	player->ready = am_promise_new(player);
}


GType
player_get_type ()
{
	static volatile gsize player_type_id__volatile = 0;
	if (g_once_init_enter (&player_type_id__volatile)) {
		static const GTypeInfo g_define_type_info = { sizeof (PlayerClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) player_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (Player), 0, (GInstanceInitFunc) player_instance_init, NULL };
		GType player_type_id;
		player_type_id = g_type_register_static (G_TYPE_OBJECT, "Player", &g_define_type_info, 0);
		g_once_init_leave (&player_type_id__volatile, player_type_id);
	}
	return player_type_id__volatile;
}


void
player_connect (ErrorCallback callback, gpointer user_data)
{
	typedef struct {
		ErrorCallback callback;
		gpointer user_data;
	} C;

	void player_on_connected(GError* error, gpointer _)
	{
		C* c = _;

		am_promise_resolve(play->ready, NULL);
		if(c->callback) c->callback(error, c->user_data);

		g_free(c);
	}

	play->auditioner->connect(player_on_connected, SC_NEW(C, .callback = callback, .user_data = user_data));
}


bool
player_play (Sample* sample)
{
	bool play_update(gpointer _)
	{
		if(play->auditioner->position) play->position = play->auditioner->position();
		if(play->position != UINT_MAX) g_signal_emit_by_name (play, "position");

		return G_SOURCE_CONTINUE;
	}

	play->status = PLAY_PLAY_PENDING;

	if(play->auditioner->play(sample)){
		sample_unref0(play->sample);

		play->status = PLAY_PLAYING;
		play->sample = sample_ref(sample);
		play->position = UINT_MAX;

		g_signal_emit_by_name (play, "play");

		if(play->auditioner->position && !play_timeout_id) {
			GSource* source = g_timeout_source_new (50);
			g_source_set_callback (source, play_update, NULL, NULL);
			play_timeout_id = g_source_attach (source, NULL);
		}
		return true;
	}
	return false;
}


void
player_on_play_finished ()
{
	if (play_timeout_id) {
		g_source_destroy(g_main_context_find_source_by_id(NULL, play_timeout_id));
		play_timeout_id = 0;
	}

	if(_debug_) printf("PLAY STOP\n");

	g_signal_emit_by_name (play, "stop");

	sample_unref0(play->sample);
}


void
player_stop()
{
	PF;
	if (play->queue) {
		g_list_foreach(play->queue, (GFunc)sample_unref, NULL);
		g_list_free0(play->queue);
	}

	play->auditioner->stop();
	play->status = PLAY_STOPPED;

	player_on_play_finished();
}


void
player_set_position (gint64 frames)
{
	if(play->sample && play->sample->sample_rate){
		play->position = (frames * 1000) / play->sample->sample_rate;
		g_signal_emit_by_name (play, "position");
	}
}


void
player_set_position_seconds (float seconds)
{
	if(play->sample){
		play->position = seconds * 1000;
		g_signal_emit_by_name (play, "position");
	}
}


bool
player_is_stopped ()
{
	return play->status == PLAY_STOPPED;
}


bool
player_is_playing ()
{
   return play->status == PLAY_PLAYING;
}

