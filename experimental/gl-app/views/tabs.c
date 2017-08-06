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
#define __wf_private__
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <GL/gl.h>
#include "agl/ext.h"
#include "agl/utils.h"
#include "agl/actor.h"
#include "waveform/waveform.h"
#include "waveform/peakgen.h"
#include "waveform/shader.h"
#include "waveform/actors/text.h"
#include "samplecat.h"
#include "views/tabs.h"

#define _g_free0(var) (var = (g_free (var), NULL))

#define FONT "Droid Sans"
#define TAB_HEIGHT 22

static AGl* agl = NULL;
static int instance_count = 0;
static AGlActorClass actor_class = {0, "Tabs", (AGlActorNew*)tabs_view};
static int tab_width = 50;


AGlActorClass*
tabs_view_get_class ()
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
tabs_view(WaveformActor* _)
{
	instance_count++;

	_init();

	bool tabs_paint(AGlActor* actor)
	{
		TabsView* tabs = (TabsView*)actor;

		GList* l = tabs->tabs;
		int x = 0;
		for(;l;l=l->next){
			TabsViewTab* tab = l->data;
			agl_print(x, 0, 0, 0xffffffff, tab->name);
			x += tab_width;
		}

		agl->shaders.plain->uniform.colour = 0x777777ff;
		agl_use_program((AGlShader*)agl->shaders.plain);
		agl_rect_((AGlRect){tabs->active * tab_width, TAB_HEIGHT - 4, tab_width, 2});

		return true;
	}

	void tabs_init(AGlActor* a)
	{
	}

	void tabs_set_size(AGlActor* actor)
	{
		TabsView* tabs = (TabsView*)actor;

		TabsViewTab* tab = g_list_nth_data(tabs->tabs, tabs->active);
		if(tab){
			tab->actor->region = (AGliRegion){0, TAB_HEIGHT, agl_actor__width(actor), agl_actor__height(actor)};
		}
	}

	bool tabs_event(AGlActor* actor, GdkEvent* event, AGliPt xy)
	{
		TabsView* tabs = (TabsView*)actor;

		switch(event->type){
			case GDK_BUTTON_PRESS:
			case GDK_BUTTON_RELEASE:;
				/*
				agl_actor__invalidate(actor);
				int row = (xy.y - actor->region.y1) / row_height;
				*/
				int active = xy.x / tab_width;
				dbg(0, "x=%i y=%i active=%i", xy.x, xy.y - actor->region.y1, active);
				if(active < g_list_length(tabs->tabs) && active != tabs->active){
					TabsViewTab* prev = g_list_nth_data(tabs->tabs, tabs->active);
					TabsViewTab* next = g_list_nth_data(tabs->tabs, active);

#ifdef AGL_DEBUG_ACTOR
					dbg(0, "selecting... %s", prev->actor->name);
#endif
					prev->actor->region.x2 = 0;

					tabs->active = active;
					next->actor->region = (AGliRegion){0, 20, agl_actor__width(actor), agl_actor__height(actor)};
					agl_actor__set_size(next->actor);
					agl_actor__invalidate(actor);
				}
				break;
			default:
				break;
		}
		return AGL_HANDLED;
	}

	void tabs_free(AGlActor* actor)
	{
		TabsView* tabs = (TabsView*)actor;

		g_list_free_full(tabs->tabs, g_free);

		if(!--instance_count){
		}
	}

	TabsView* view = WF_NEW(TabsView,
		.actor = {
			.class = &actor_class,
			.name = "Tabs",
			.init = tabs_init,
			.free = tabs_free,
			.paint = tabs_paint,
			.set_size = tabs_set_size,
			.on_event = tabs_event,
		}
	);

	return (AGlActor*)view;
}


void
tabs_view__add_tab(TabsView* tabs, const char* name, AGlActor* child)
{
	TabsViewTab* tab = WF_NEW(TabsViewTab, .actor=child, .name=name);

	tabs->tabs = g_list_append(tabs->tabs, tab);
	agl_actor__add_child((AGlActor*)tabs, child);
}


