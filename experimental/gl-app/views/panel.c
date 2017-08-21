/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2016-2017 Tim Orford <tim@orford.org>                  |
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
#include <glib.h>
#include <GL/gl.h>
#include "agl/ext.h"
#include "agl/utils.h"
#include "agl/actor.h"
#include "waveform/shader.h"
#include "debug/debug.h"
#include "samplecat/typedefs.h"
#include "views/dock_v.h"
#include "views/panel.h"

#define FONT "Droid Sans"

static AGl* agl = NULL;
static AGlActorClass actor_class = {0, "Panel", (AGlActorNew*)panel_view};
static int instance_count = 0;
static AGliPt origin = {0,};
static AGliPt mouse = {0,};


AGlActorClass*
panel_view_get_class ()
{
	return &actor_class;
}


static void
_init()
{
	static bool init_done = false;

	if(!init_done){
		agl = agl_get_instance();
		agl_set_font_string("Roboto 10"); // initialise the pango context

		init_done = true;
	}
}


AGlActor*
panel_view(gpointer _)
{
	instance_count++;

	_init();

	bool panel_paint(AGlActor* actor)
	{
#ifdef SHOW_PANEL_BACKGROUND
		agl->shaders.plain->uniform.colour = 0xffff0033;
		agl_use_program((AGlShader*)agl->shaders.plain);
		agl_rect_((AGlRect){0, 0, agl_actor__width(actor), agl_actor__height(actor)});
#endif

		if(actor_context.grabbed == actor){
			AGliPt offset = {mouse.x - origin.x, mouse.y - origin.y};
			agl->shaders.plain->uniform.colour = 0x6677ff77;
			agl_use_program((AGlShader*)agl->shaders.plain);
			agl_box(1, offset.x, offset.y, agl_actor__width(actor), agl_actor__height(actor));
		}

		return true;
	}

	void panel_init(AGlActor* actor)
	{
	}

	void panel_set_size(AGlActor* actor)
	{
		// single child takes all space of panel
		if(g_list_length(actor->children) == 1){
			AGlActor* child = actor->children->data;
			child->region = (AGliRegion){0, PANEL_DRAG_HANDLE_HEIGHT, agl_actor__width(actor), agl_actor__height(actor)};
			agl_actor__set_size(child);
		}
	}

	bool panel_event(AGlActor* actor, GdkEvent* event, AGliPt xy)
	{
		switch(event->type){
			case GDK_BUTTON_PRESS:
				dbg(1, "PRESS %i", xy.y - actor->region.y1);
				if(xy.y < PANEL_DRAG_HANDLE_HEIGHT){
					actor_context.grabbed = actor;
					origin = mouse = xy;
				}
				break;
			case GDK_BUTTON_RELEASE:
				agl_actor__invalidate(actor);
				dbg(1, "RELEASE y=%i", xy.y);
				if(actor_context.grabbed){
					dock_v_move_panel_to_y((DockVView*)actor->parent, actor, xy.y);
					actor_context.grabbed = NULL;
				}
				break;
			case GDK_MOTION_NOTIFY:
				if(actor_context.grabbed == actor){
					mouse = xy;
					agl_actor__invalidate(actor);
				}
				break;
			default:
				break;
		}
		return AGL_HANDLED;
	}

	void panel_free(AGlActor* actor)
	{
		if(!--instance_count){
		}
	}

	PanelView* view = AGL_NEW(PanelView,
		.actor = {
			.class = &actor_class,
			.name = "Panel",
			.init = panel_init,
			.free = panel_free,
			.paint = panel_paint,
			.set_size = panel_set_size,
			.on_event = panel_event,
		},
		.size_req = {
			.min = {-1, -1},
			.preferred = {-1, -1},
			.max = {-1, -1}
		}
	);

	return (AGlActor*)view;
}


