/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. https://ayyi.github.io/samplecat/    |
 | copyright (C) 2007-2023 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "samplecat/typedefs.h"
#include "wf/promise.h"

#define AUDITIONER_DOMAIN "Auditioner"

typedef struct {
    int     (*check)      ();
    void    (*connect)    (ErrorCallback, gpointer);
    void    (*disconnect) ();
    bool    (*play)       (Sample*);
    void    (*play_all)   ();
    void    (*stop)       ();
/* extended API */
    int     (*pause)      (int);
    void    (*seek)       (double);
    guint   (*position)   ();
} Auditioner;

G_BEGIN_DECLS

typedef enum {
   PLAYER_INIT = 0,
   PLAYER_STOPPED,
   PLAYER_PAUSED,
   PLAYER_PLAY_PENDING,
   PLAYER_PLAYING
} PlayerState;


#define TYPE_PLAYER            (player_get_type ())
#define PLAYER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_PLAYER, Player))
#define PLAYER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_PLAYER, PlayerClass))
#define IS_PLAYER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_PLAYER))
#define IS_PLAYER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_PLAYER))
#define PLAYER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_PLAYER, PlayerClass))

typedef struct _PlayerClass PlayerClass;
typedef struct _PlayerPrivate PlayerPrivate;

struct _PlayerClass {
	GObjectClass parent_class;
};


typedef struct {
    GObject      parent_instance;
    PlayerPrivate* priv;
    Auditioner const* auditioner;

    AMPromise*   ready;

    PlayerState  state;
    Sample*      sample;
    GList*       queue;
    guint        position;

    void         (*next)();
    void         (*play_selected)();

#if (defined HAVE_JACK)
    gboolean     enable_effect;
    gboolean     effect_enabled;         // read-only set by jack_player.c
    float        effect_param[3];
    float        playback_speed;
    gboolean     link_speed_pitch;
#endif

    struct {
        bool     loop;
        char     jack_autoconnect[1024];
        char     jack_midiconnect[1024];
    }            config;
} Player;

GType   player_get_type             () G_GNUC_CONST;
Player* player_new                  ();
void    player_connect              (ErrorCallback, gpointer);
void    player_set_state            (PlayerState);
bool    player_play                 (Sample*);
void    player_stop                 ();
void    player_pause                ();
void    player_set_position         (gint64);
void    player_set_position_seconds (float);
void    player_on_play_finished     ();

bool    player_is_stopped           ();
bool    player_is_playing           ();

#define PLAYER_TYPE_STATE (player_state_get_type ())

GType   player_state_get_type       ();

G_END_DECLS

extern Player* play;
