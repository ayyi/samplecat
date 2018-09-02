/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2016-2018 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#define __wf_private__
#include "config.h"
#undef USE_GTK
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
#include "waveform/shader.h"
#include "views/panel.h"
#include "views/dock_v.h"

#define _g_free0(var) (var = (g_free (var), NULL))
#define SPACING 15
#define DIVIDER 5
#define HANDLE_HEIGHT(P) (((PanelView*)P)->title ? PANEL_DRAG_HANDLE_HEIGHT : 0)

static AGl* agl = NULL;
static int instance_count = 0;
static AGlActorClass actor_class = {0, "Dock V", (AGlActorNew*)dock_v_view};


AGlActorClass*
dock_v_get_class()
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


static inline int
get_spacing(DockVView* dock)
{
	int n = 0;
	GList* l = dock->panels;
	for(;l;l=l->next){
		PanelView* p = l->data;
		if(l->next && !p->no_border) n++;
	}
	return n * SPACING;
}


AGlActor*
dock_v_view(gpointer _)
{
	instance_count++;

	_init();

	bool dock_v_paint(AGlActor* actor)
	{
		DockVView* dock = (DockVView*)actor;

		// dividing lines
		int y = 0;
		GList* l = dock->panels;
		for(;l;l=l->next){
			AGlActor* a = l->data;
			if(y){
				AGlActor* prev = l->prev->data;
				y = a->region.y1;
				if(!((PanelView*)prev)->no_border){
					agl_rect_((AGlRect){0, y - SPACING / 2 - 1, agl_actor__width(actor), 1});
				}
			}
			y = a->region.y2;
		}

		if(dock->handle.opacity > 0.0){
			float alpha = dock->animatables[0]->val.f;
			agl->shaders.plain->uniform.colour = (0x999999ff & 0xffffff00) + (uint32_t)(alpha * 0xff);
			agl_use_program((AGlShader*)agl->shaders.plain);
			agl_rect_((AGlRect){0, dock->handle.actor->region.y1 - SPACING / 2 - DIVIDER / 2 , agl_actor__width(actor), DIVIDER});
		}

		return true;
	}

	void dock_init(AGlActor* a)
	{
	}

	void dock_set_state(AGlActor* actor)
	{
		agl->shaders.plain->uniform.colour = 0x66666666;
	}

	void dock_v_set_size(AGlActor* actor)
	{
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

		// if the allocated size is correct, it should be preserved
		if(items[G_N_ELEMENTS(items) - 1].actor->region.y2 == actor->region.y2){
			int width = agl_actor__width(actor);
			GList* l = actor->children;
			for(;l;l=l->next){
				AGlActor* child = l->data;
				child->region.x2 = child->region.x1 + width;
				agl_actor__set_size(child);
			}

			return;
		}

		int vspace = agl_actor__height(actor) - get_spacing(dock);
		int n_flexible = g_list_length(dock->panels);
		for(i=0;i<G_N_ELEMENTS(items);i++){
			Item* item = &items[i];
			PanelView* panel = (PanelView*)item->actor;
			AGlActor* a = (AGlActor*)panel;
			a->region.x2 = panel->size_req.preferred.x > -1 ? panel->size_req.preferred.x : agl_actor__width(actor);

			if(panel->size_req.preferred.y > -1){
				item->height = panel->size_req.preferred.y + HANDLE_HEIGHT(panel);
				n_flexible --;
			}else if(panel->size_req.min.y > -1){
				item->height = panel->size_req.min.y + HANDLE_HEIGHT(panel);
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
				if(!item->height){
					item->height = each_unallocated;
				}
			}
			y += item->height + (panel->no_border ? 0 : SPACING);
		}
		y -= SPACING; // no spacing needed after last element

		if(y < agl_actor__height(actor)){
			// under allocated
			// TODO use distribute fn from dock_h.c
			int remaining = agl_actor__height(actor) - y;
			int n_resizable = 0;
			for(i=0;i<G_N_ELEMENTS(items);i++){
				Item* item = &items[i];
				PanelView* panel = (PanelView*)item->actor;
				if(item->height < panel->size_req.max.y){
					n_resizable ++;
				}
			}
			if(n_resizable){
				int each = remaining / n_resizable;
				for(i=0;i<G_N_ELEMENTS(items);i++){
					Item* item = &items[i];
					PanelView* panel = (PanelView*)item->actor;
					if(item->height < panel->size_req.max.y){
						item->height += each;
					}
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
			if(agl_actor__height(actor)){
				int gain = (1000 * y) / agl_actor__height(actor);
				for(i=0;i<G_N_ELEMENTS(items);i++){
					items[i].height = (1000 * items[i].height) / gain;
				}
			}
		}

		y = 0;
		for(l=dock->panels,i=0;l;l=l->next,i++){
			AGlActor* a = (AGlActor*)l->data;
			a->region.y1 = y;
			a->region.y2 = y + items[i].height;
			y += items[i].height + (((PanelView*)a)->no_border ? 0 : SPACING);
		}
	}

	bool dock_event(AGlActor* actor, GdkEvent* event, AGliPt xy)
	{
		DockVView* dock = (DockVView*)actor;

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
					int y = xy.y - actor->region.y1;

					AGlActor* a2 = dock->handle.actor;
					GList* l = g_list_find(dock->panels, a2);
					AGlActor* a1 = l->prev->data;

					int min_diff = -agl_actor__height(a1);
					int max_diff = agl_actor__height(a2);
					if(((PanelView*)a1)->size_req.min.y > -1){
						min_diff = ((PanelView*)a1)->size_req.min.y - agl_actor__height(a1) + PANEL_DRAG_HANDLE_HEIGHT;
					}
					if(((PanelView*)a1)->size_req.max.y > -1){
						max_diff = ((PanelView*)a1)->size_req.max.y - agl_actor__height(a1) + PANEL_DRAG_HANDLE_HEIGHT;
					}
					int diff = CLAMP(y - a2->region.y1, min_diff, max_diff);
					if(diff){
						a2->region.y1 = y;
						agl_actor__set_size(a2);
						agl_actor__invalidate(a2);

						a1->region.y2 += diff;
						agl_actor__set_size(a1);
						agl_actor__invalidate(a1);
					}
				}else{
					int y = 0;
					GList* l = dock->panels;
					AGlActor* f = NULL;
					for(;l;l=l->next){
						AGlActor* a = l->data;
						y = a->region.y1;
						if(ABS(y - xy.y) < SPACING){
							f = a;
							break;
						}
					}
					if(f){
						if(!dock->handle.opacity){
							//set_cursor(arrange->canvas->widget->window, CURSOR_H_DOUBLE_ARROW);
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
		DockVView* dock = (DockVView*)actor;

		g_list_free0(dock->panels);

		if(!--instance_count){
		}
	}

	DockVView* dock = AGL_NEW(DockVView,
		.panel = {
			.actor = {
				.class = &actor_class,
				.name = "Dock V",
				.program = (AGlShader*)agl->shaders.plain,
				.init = dock_init,
				.free = dock_free,
				.paint = dock_v_paint,
				.set_state = dock_set_state,
				.set_size = dock_v_set_size,
				.on_event = dock_event,
			},
			.size_req = {
				.min = {-1, -1},
				.preferred = {-1, -1},
				.max = {-1, -1}
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


