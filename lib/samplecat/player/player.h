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
#ifndef __player_h__
#define __player_h__

#include <glib-object.h>
#include "samplecat/typedefs.h"
#include "waveform/promise.h"

#define AUDITIONER_DOMAIN "Auditioner"

typedef struct {
    int     (*check)      ();
    void    (*connect)    (ErrorCallback, gpointer);
    void    (*disconnect) ();
    bool    (*play)       (Sample*);
    void    (*play_all)   ();
    void    (*stop)       ();
/* extended API */
    int     (*playpause)  (int);
    void    (*seek)       (double);
    guint   (*position)   ();
} Auditioner;

typedef enum {
   PLAY_STOPPED = 0,
   PLAY_PAUSED,
   PLAY_PLAY_PENDING,
   PLAY_PLAYING
} PlayStatus;


G_BEGIN_DECLS

#define TYPE_PLAYER (player_get_type ())
#define PLAYER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_PLAYER, Player))
#define PLAYER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_PLAYER, PlayerClass))
#define IS_PLAYER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_PLAYER))
#define IS_PLAYER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_PLAYER))
#define PLAYER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_PLAYER, PlayerClass))

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

    PlayStatus   status;
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
Player* player_construct            (GType);
void    player_connect              (ErrorCallback, gpointer);
bool    player_play                 (Sample*);
void    player_stop                 ();
void    player_set_position         (gint64);
void    player_set_position_seconds (float);
void    player_on_play_finished     ();

bool    player_is_stopped           ();
bool    player_is_playing           ();

G_END_DECLS

extern Player* play;

#endif
