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
#include "debug/debug.h"
#include "icon/utils.h"
#include "samplecat.h"
#include "application.h"
#include "behaviours/panel.h"
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


AGlActorClass*
player_view_get_class ()
{
	static bool init_done = false;

	if(!init_done){
		agl = agl_get_instance();

		actor_class.behaviour_classes[0] = panel_get_class();

		init_done = true;
	}

	return &actor_class;
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

	player_view_get_class();

	AGlActor* view = agl_actor__new(AGlActor,
		.class = &actor_class,
		.name = "Player",
		.colour = 0xaaff33ff,
		.init = player_init,
		.paint = player_paint,
	);

	AGlActor* b[2];
	for(int i=0;i<2;i++){
		agl_actor__add_child(view, b[i] = button((int*)&textures[i], buttons[i].action, buttons[i].state, NULL));
		b[i]->region = (AGlfRegion){
			.x1 = (PLAYER_ICON_SIZE + 4) * i,
			.x2 = (PLAYER_ICON_SIZE + 4) * i + PLAYER_ICON_SIZE,
			.y2 = PLAYER_ICON_SIZE,
		};
	}

	return view;
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
		textures[i] = get_icon_texture_by_name (icons[i], PLAYER_ICON_SIZE);
	}
}


static bool
player_paint (AGlActor* actor)
{
	return true;
}

