/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2016-2018 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
* | DockH                                                                |
* | A container with 2 or more children arranged in a horizontal row     |
* +----------------------------------------------------------------------+
*
*/
#define __wf_private__
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <gdk/gdkkeysyms.h>
#include <GL/gl.h>
#include "agl/ext.h"
#include "agl/utils.h"
#include "agl/actor.h"
#include "waveform/waveform.h"
#include "waveform/peakgen.h"
#include "waveform/shader.h"
#include "samplecat.h"
#include "views/dock_h.h"

#define _g_free0(var) (var = (g_free (var), NULL))
#define SPACING 15
#define DIVIDER 5

static AGl* agl = NULL;
static int instance_count = 0;
static AGlActorClass actor_class = {0, "Dock H", (AGlActorNew*)dock_h_view};


AGlActorClass*
dock_h_get_class()
{
	return &actor_class;
}


static void
_init()
{
	static bool init_done = false;

	if(!init_done){
		agl = agl_get_instance();

		init_done = true;
	}
}


AGlActor*
dock_h_view(WaveformActor* _)
{
	instance_count++;

	_init();

	bool dock_h_paint(AGlActor* actor)
	{
		DockHView* dock = (DockHView*)actor;

		int x = 0;
		GList* l = dock->panels;
		for(;l;l=l->next){
			AGlActor* a = l->data;
			if(x){
				x = a->region.x1;
				agl_rect_((AGlRect){x - SPACING / 2 - 1, 0, 1, agl_actor__height(actor)});
			}
			x = a->region.x2;
		}

		if(dock->handle.opacity > 0.0){
			float alpha = dock->animatables[0]->val.f;
			agl->shaders.plain->uniform.colour = (0x999999ff & 0xffffff00) + (uint32_t)(alpha * 0xff);
			agl_use_program((AGlShader*)agl->shaders.plain);
			agl_rect_((AGlRect){dock->handle.actor->region.x1 - SPACING / 2 - DIVIDER / 2, 0, DIVIDER, agl_actor__height(actor)});
		}

		return true;
	}

	void dock_init(AGlActor* a)
	{
		void panel_init(AGlActor* actor)
		{
			PanelView* panel = (PanelView*)actor;

			panel->size_req.preferred = (AGliPt){-1, -1};
			panel->size_req.max = (AGliPt){-1, -1};
		}

		panel_init(a);
	}

	void dock_set_state(AGlActor* actor)
	{
		agl->shaders.plain->uniform.colour = 0x66666666;
	}

	void dock_h_set_size(AGlActor* actor)
	{
		DockHView* dock = (DockHView*)actor;
		int height = agl_actor__height(actor);

		typedef struct {
			AGlActor* actor;
			int       width;
		} Item;
		Item items[g_list_length(dock->panels)];

		int req = 0;
		GList* l = dock->panels;
		int i = 0;
		for(;l;l=l->next,i++){
			items[i] = (Item){l->data, 0};
			PanelView* panel = (PanelView*)items[i].actor;
			if(panel->size_req.preferred.x > -1) req += panel->size_req.preferred.x;
		}

		// if the allocated horizontal size is correct, it should be preserved
		if(items[G_N_ELEMENTS(items) - 1].actor->region.x2 == agl_actor__width(actor)){
			dbg(2, "width already correct: %i", agl_actor__width(actor));

			GList* l = actor->children;
			for(;l;l=l->next){
				AGlActor* child = l->data;
				child->region.y2 = height;
				agl_actor__set_size(child);
			}
			return;
		}

		int hspace = agl_actor__width(actor) - SPACING * (g_list_length(dock->panels) - 1);
		int n_flexible = g_list_length(dock->panels);
		for(i=0;i<G_N_ELEMENTS(items);i++){
			Item* item = &items[i];
			PanelView* panel = (PanelView*)item->actor;

			if(panel->size_req.preferred.x > -1){
				item->width = panel->size_req.preferred.x + PANEL_DRAG_HANDLE_HEIGHT;
				n_flexible --;
			}
			hspace -= item->width;
		}

		int each_unallocated = n_flexible ? hspace / n_flexible : 0;
		int x = 0;
		for(i=0;i<G_N_ELEMENTS(items);i++){
			Item* item = &items[i];
			PanelView* panel = (PanelView*)item->actor;

			if(each_unallocated > 0){
				if(panel->size_req.preferred.x < 0){ // -1 means no preference
							// TODO should be += ?
					item->width = each_unallocated;
				}
			}
			x += item->width + SPACING;
		}
		x -= SPACING; // no spacing needed after last element

		if(x < agl_actor__width(actor)){
			// under-allocated
			int remaining = agl_actor__width(actor) - x;
			int n_resizable = 0;
			for(i=0;i<G_N_ELEMENTS(items);i++){
				Item* item = &items[i];
				PanelView* panel = (PanelView*)item->actor;
				if(panel->size_req.max.x < 0 || item->width < panel->size_req.max.x){
					n_resizable ++;
				}
			}
			if(n_resizable){
				typedef struct {PanelView* panel; int w; int amount; bool full;} A; // TODO just use Item

				void distribute(A L[], int _to_distribute, int n_resizable, int iter)
				{
					g_return_if_fail(_to_distribute > 0);

					int to_distribute = _to_distribute;
					int each = to_distribute / n_resizable;
					int remainder = to_distribute % n_resizable;

					#define CHECK_FULL(A) if(width + amount == max){ A->full = true; n_resizable--; }

					int i; for(i=0;i<20;i++){
						A* a = &L[i];
						if(a->panel){
							if(!a->full){
								PanelView* panel = a->panel;
								int max = panel->size_req.max.x < 0 ? 10000 : panel->size_req.max.x;
								int width = a->w + a->amount;
								int amount = MIN(max - width, each);
								//dbg(0, "   panel=%s width=%i max=%i possible=%i amount=%i (%i)", ((AGlActor*)a->panel)->name, width, max, max - width, amount, width + amount);
								CHECK_FULL(a)
								else if(remainder){
									amount++;
									remainder--;
									CHECK_FULL(a)
								}
								to_distribute -= amount;
								a->amount += amount;
							}
						} else break;
					}
					if(to_distribute && (to_distribute != _to_distribute) && iter++ < 5) distribute(L, to_distribute, n_resizable, iter);
					if(iter == 5) gwarn("failed to distribute");
				}
				A L[G_N_ELEMENTS(items) + 1];
				for(i=0;i<G_N_ELEMENTS(items);i++){
					Item* item = &items[i];
					L[i] = (A){(PanelView*)item->actor,item->width,};
				}
				L[i] = (A){NULL,};
				distribute(L, remaining, n_resizable, 0);

				for(i=0;i<G_N_ELEMENTS(items);i++){
					Item* item = &items[i];
					A* a = &L[i];
					item->width += a->amount;
				}
			}
		}
		else if(x > agl_actor__width(actor)){
			dbg(0, "over allocated");
			/*
			int over = y - agl_actor__height(actor);
			if(n_flexible){
				int flexible_height = 0;
				for(i=0;i<G_N_ELEMENTS(items);i++){
					Item* item = &items[i];
					PanelView* panel = (PanelView*)item->actor;
					if(panel->size_req.preferred.y < 0){
						flexible_height += item->width;
					}
				}
				dbg(0, "tot flexible_height=%i", flexible_height);
			}
			*/
			int gain = (1000 * x) / agl_actor__width(actor);
			for(i=0;i<G_N_ELEMENTS(items);i++){
				items[i].width = (1000 * items[i].width) / gain;
			}
		}

		x = 0;
		for(l=dock->panels,i=0;l;l=l->next,i++){
			AGlActor* a = (AGlActor*)l->data;
			Item* item = &items[i];
			PanelView* panel = (PanelView*)item->actor;
			a->region = (AGliRegion){
				.x1 = x,
				.y1 = 0,
				.x2 = x + items[i].width,
				.y2 = panel->size_req.preferred.y > -1 ? panel->size_req.preferred.y : height
			};
			x += items[i].width + SPACING;
		}
		dbg(2, "-> total=%i / %i", x - SPACING, agl_actor__width(actor));

		// copynpaste - PanelView set_size
		// single child takes all space of panel
		if(g_list_length(actor->children) == 1){
			AGlActor* child = actor->children->data;
			child->region = (AGliRegion){0, PANEL_DRAG_HANDLE_HEIGHT, agl_actor__width(actor), height};
			agl_actor__set_size(child);
		}
	}

	bool dock_h_event(AGlActor* actor, GdkEvent* event, AGliPt xy)
	{
		DockHView* dock = (DockHView*)actor;

		void animation_done (WfAnimation* animation, gpointer user_data)
		{
		}

		switch (event->type){
			case GDK_BUTTON_PRESS:
				agl_actor__grab(actor);
				//set_cursor(arrange->canvas->widget->window, CURSOR_H_DOUBLE_ARROW);
				break;
			case GDK_MOTION_NOTIFY:
				if(actor_context.grabbed == actor){
					AGlActor* a2 = dock->handle.actor;
					GList* l = g_list_find(dock->panels, a2);
					AGlActor* a1 = l->prev->data;

					int min_diff = -agl_actor__width(a1);
					int max_diff = 1000;
					if(((PanelView*)a1)->size_req.min.x > -1){
						min_diff = ((PanelView*)a1)->size_req.min.x - agl_actor__width(a1);
					}
					if(((PanelView*)a1)->size_req.max.x > -1){
						max_diff = ((PanelView*)a1)->size_req.max.x - agl_actor__width(a1);
					}
					int diff = CLAMP(xy.x - a2->region.x1, min_diff, max_diff);
					if(diff){
						a2->region.x1 += diff;
						agl_actor__set_size(a2);
						agl_actor__invalidate(a2);

						a1->region.x2 += diff;
						agl_actor__set_size(a1);
						agl_actor__invalidate(a1);
					}
				}else{
					int x = 0;
					GList* l = dock->panels;
					AGlActor* f = NULL;
					for(;l;l=l->next){
						AGlActor* a = l->data;
						x = a->region.x1;
						if(ABS(x - xy.x) < SPACING){
							f = a;
							break;
						}
					}
					if(f){
						if(!dock->handle.opacity){
							dock->handle.opacity = 1.0;
							dock->handle.actor = f;
							agl_actor__start_transition(actor, g_list_append(NULL, dock->animatables[0]), animation_done, NULL);
						}
					}else{
						if(dock->handle.opacity){
							dock->handle.opacity = 0.0;
							agl_actor__start_transition(actor, g_list_append(NULL, dock->animatables[0]), animation_done, NULL);
						}
					}
				}
				break;
			case GDK_LEAVE_NOTIFY:
				dbg (1, "LEAVE_NOTIFY");
				if(dock->handle.opacity > 0.0){
					dock->handle.opacity = 0.0;
					agl_actor__start_transition(actor, g_list_append(NULL, dock->animatables[0]), animation_done, NULL);
				}
				break;
			case GDK_BUTTON_RELEASE:
			default:
				break;
		}
		return AGL_HANDLED;
	}

	void dock_free(AGlActor* actor)
	{
		DockHView* dock = (DockHView*)actor;

		g_list_free0(dock->panels);

		if(!--instance_count){
		}
	}

	DockHView* dock = WF_NEW(DockHView,
		.panel = {
			.actor = {
				.class = &actor_class,
				.name = "Dock H",
				.program = (AGlShader*)agl->shaders.plain,
				.init = dock_init,
				.free = dock_free,
				.paint = dock_h_paint,
				.set_state = dock_set_state,
				.set_size = dock_h_set_size,
				.on_event = dock_h_event,
			}
		}
	);

	dock->animatables[0] = AGL_NEW(WfAnimatable,
		.model_val.f = &dock->handle.opacity,
		.start_val.f = 0.0,
		.val.f       = 0.0,
		.type        = WF_FLOAT
	);

	return (AGlActor*)dock;
}


AGlActor*
dock_h_add_panel(DockHView* dock, AGlActor* panel)
{
	dock->panels = g_list_append(dock->panels, panel);
	agl_actor__add_child((AGlActor*)dock, panel);
	return panel;
}


void
dock_h_move_panel_to_index (DockHView* dock, AGlActor* panel, int i)
{
	dbg(0, "i=%i", i);
	dock->panels = g_list_remove(dock->panels, panel);
	dock->panels = g_list_insert(dock->panels, panel, i);
	agl_actor__set_size((AGlActor*)dock);
}


void
dock_h_move_panel_to_y (DockHView* dock, AGlActor* panel, int y)
{
	int find_index(DockHView* dock, int y)
	{
		GList* l = dock->panels;
		int i = 0;
		for(;l;l=l->next){
			AGlActor* a = l->data;
			dbg(0, "  %i", a->region.y1);
			if(a->region.y1 > y) return i;
			i++;
		}
		return i;
	}

	dbg(0, "y=%i", y);
	int i = find_index(dock, y);
	dock_h_move_panel_to_index(dock, panel, i);
}


