/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2019-2019 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug/debug.h"
#include "samplecat.h"
#include "application.h"
#include "views/panel.h"
#include "views/button.h"
#include "views/player.h"

static AGl* agl = NULL;
static int instance_count = 0;
static AGlActorClass actor_class = {0, "Player", (AGlActorNew*)player_view};

static void player_init  (AGlActor*);
static bool player_paint (AGlActor*);

char* icons[] = {
	"media-playback-stop",
	"media-playback-start",
};
static guint textures[2];
static guint t_idx = 0;


AGlActorClass*
player_view_get_class ()
{
	return &actor_class;
}


static void
_init ()
{
	static bool init_done = false;

	if(!init_done){
		agl = agl_get_instance();

		init_done = true;
	}
}


static void
action_stop (AGlActor* button, gpointer _)
{
	player_stop();
}


static void
action_play (AGlActor* button, gpointer _)
{
	application_play_selected();
}


typedef struct {
    ButtonAction action;
    ButtonGetState state;
} Buttons;

Buttons buttons[] = {
	{action_stop, player_is_stopped},
	{action_play, player_is_playing}
};


AGlActor*
player_view (gpointer _)
{
	instance_count++;

	_init();

	AGlActor* view = SC_NEW(AGlActor,
		.class = &actor_class,
		.name = "Player",
		.colour = 0xaaff33ff,
		.init = player_init,
		.paint = player_paint,
	);

	AGlActor* b[2];
	for(int i=0;i<2;i++){
		agl_actor__add_child(view, b[i] = button(&textures[i], buttons[i].action, buttons[i].state, NULL));
		b[i]->region = (AGliRegion){
			.x1 = (PLAYER_ICON_SIZE + 4) * i,
			.x2 = (PLAYER_ICON_SIZE + 4) * i + PLAYER_ICON_SIZE,
			.y2 = PLAYER_ICON_SIZE,
		};
	}

	return view;
}


static GdkPixbuf*
get_icon(const char* name)
{
	GError* error = NULL;

	GdkPixbuf* pixbuf = gtk_icon_theme_load_icon(icon_theme, name, PLAYER_ICON_SIZE, GTK_ICON_LOOKUP_USE_BUILTIN, &error);

	return pixbuf;
}


static guint
icon_to_texture(GdkPixbuf* icon)
{
	dbg(0, "icon: pixbuf=%ix%i %ibytes/px", gdk_pixbuf_get_width(icon), gdk_pixbuf_get_height(icon), gdk_pixbuf_get_n_channels(icon));
	glBindTexture   (GL_TEXTURE_2D, textures[t_idx++]);
	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D    (GL_TEXTURE_2D, 0, GL_RGBA, gdk_pixbuf_get_width(icon), gdk_pixbuf_get_height(icon), 0, GL_RGBA, GL_UNSIGNED_BYTE, gdk_pixbuf_get_pixels(icon));
	gl_warn("texture bind");

	g_object_unref(icon);

	return textures[t_idx - 1];
}


static void
player_init (AGlActor* actor)
{
	if(actor->parent){
		AGlActor* parent = actor->parent;
		PanelView* panel = (PanelView*)parent;
		if(!strcmp(parent->name, "Panel")){
			panel->title = NULL;
			panel->no_border = true;
			panel->size_req.min.y = panel->size_req.max.y = panel->size_req.preferred.y = PLAYER_ICON_SIZE;
		}
	}

	glGenTextures(G_N_ELEMENTS(textures), textures);
	for(int i=0;i<G_N_ELEMENTS(icons);i++){
		GdkPixbuf* icon = get_icon(icons[i]);
		dbg(0, "icon=%p", icon);
		if(icon){
			guint t = icon_to_texture(icon);
			dbg(0, "texture=%u", t);
		}
	}
}


static bool
player_paint (AGlActor* actor)
{
	return true;
}

