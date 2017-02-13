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
#include "samplecat.h"
#include "views/panel.h"
#include "views/dock_v.h"

#define _g_free0(var) (var = (g_free (var), NULL))

#define FONT "Droid Sans"

static AGl* agl = NULL;
static int instance_count = 0;


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
dock_v_view(WaveformActor* _)
{
	instance_count++;

	_init();

	bool dock_v_paint(AGlActor* actor)
	{
		/*
		DockVView* view = (DockVView*)actor;
		*/

		return true;
	}

	void dock_init(AGlActor* a)
	{
	}

	void dock_set_size(AGlActor* actor)
	{
		#define SPACING 2
		DockVView* dock = (DockVView*)actor;

		typedef struct {
			AGlActor* actor;
			int       height;
		} Item;
		Item items[g_list_length(dock->panels)];

		int req = 0;
		GList* l = dock->panels;
		int i = 0;
		for(;l;l=l->next,i++){
			items[i] = (Item){l->data, 0};
			PanelView* panel = (PanelView*)items[i].actor;
			if(panel->size_req.preferred.y > -1) req += panel->size_req.preferred.y;
		}

		int vspace = agl_actor__height(actor) - SPACING * g_list_length(dock->panels);
		int n_flexible = g_list_length(dock->panels);
		for(i=0;i<G_N_ELEMENTS(items);i++){
			Item* item = &items[i];
			PanelView* panel = (PanelView*)item->actor;
			AGlActor* a = (AGlActor*)panel;
			a->region.x2 = panel->size_req.preferred.x > -1 ? panel->size_req.preferred.x : agl_actor__width(actor);

			if(panel->size_req.preferred.y > -1){
				item->height = panel->size_req.preferred.y + PANEL_DRAG_HANDLE_HEIGHT;
				n_flexible --;
			}
			vspace -= item->height;
		}

		int each_unallocated = n_flexible ? vspace / n_flexible : 0;
		int y = 0;
		for(i=0;i<G_N_ELEMENTS(items);i++){
			Item* item = &items[i];
			PanelView* panel = (PanelView*)item->actor;

			if(each_unallocated > 0){
				if(panel->size_req.preferred.y < 0){
					item->height = each_unallocated;
				}
			}
			y += item->height + SPACING;
		}

		if(y < agl_actor__height(actor)){
			int remaining = agl_actor__height(actor) - y;
			int n_resizable = 0;
			for(i=0;i<G_N_ELEMENTS(items);i++){
				Item* item = &items[i];
				PanelView* panel = l->data;
				if(item->height < panel->size_req.max.y){
					n_resizable ++;
				}
			}
			int each = remaining / n_resizable;
			for(i=0;i<G_N_ELEMENTS(items);i++){
				Item* item = &items[i];
				PanelView* panel = (PanelView*)item->actor;
				if(item->height < panel->size_req.max.y){
					item->height += each;
				}
			}
		}
		else if(y > agl_actor__height(actor)){
			dbg(0, "over allocated");
			/*
			int over = y - agl_actor__height(actor);
			if(n_flexible){
				int flexible_height = 0;
				for(i=0;i<G_N_ELEMENTS(items);i++){
					Item* item = &items[i];
					PanelView* panel = (PanelView*)item->actor;
					if(panel->size_req.preferred.y < 0){
						flexible_height += item->height;
					}
				}
				dbg(0, "tot flexible_height=%i", flexible_height);
			}
			*/
			int gain = (1000 * y) / agl_actor__height(actor);
			for(i=0;i<G_N_ELEMENTS(items);i++){
				items[i].height = (1000 * items[i].height) / gain;
			}
		}

		y = 0;
		for(l=dock->panels,i=0;l;l=l->next,i++){
			AGlActor* a = (AGlActor*)l->data;
			a->region.y1 = y;
			a->region.y2 = y + items[i].height;
			y += items[i].height + SPACING;
		}
	}

	bool dock_event(AGlActor* actor, GdkEvent* event, AGliPt xy)
	{
		return !AGL_HANDLED;
	}

	void dock_free(AGlActor* actor)
	{
		DockVView* dock = (DockVView*)actor;

		g_list_free0(dock->panels);

		if(!--instance_count){
		}
	}

	DockVView* view = WF_NEW(DockVView,
		.actor = {
#ifdef AGL_DEBUG_ACTOR
			.name = "Dock V",
#endif
			.init = dock_init,
			.free = dock_free,
			.paint = dock_v_paint,
			.set_size = dock_set_size,
			.on_event = dock_event,
		}
	);

	return (AGlActor*)view;
}


AGlActor*
dock_v_add_panel(DockVView* dock, AGlActor* panel)
{
	dock->panels = g_list_append(dock->panels, panel);
	agl_actor__add_child((AGlActor*)dock, panel);
	return panel;
}


void
dock_v_move_panel_to_index (DockVView* dock, AGlActor* panel, int i)
{
	dbg(0, "i=%i", i);
	dock->panels = g_list_remove(dock->panels, panel);
	dock->panels = g_list_insert(dock->panels, panel, i);
	agl_actor__set_size((AGlActor*)dock);
}


void
dock_v_move_panel_to_y (DockVView* dock, AGlActor* panel, int y)
{
	int find_index(DockVView* dock, int y)
	{
		//int i; for(i=0;i<G_N_ELEMENTS(dock->items);i++){
		//	AGlActor* a = dock->items[i]->actor;
		GList* l = dock->panels;
		//AGlActor* prev = NULL;
		int i = 0;
		for(;l;l=l->next){
			//PanelView* tab = l->data;
			AGlActor* a = l->data;
			dbg(0, "  %i", a->region.y1);
			if(a->region.y1 > y) return i;
			i++;
		}
		return i;
	}

	dbg(0, "y=%i", y);
	int i = find_index(dock, y);
	dock_v_move_panel_to_index(dock, panel, i);
}


