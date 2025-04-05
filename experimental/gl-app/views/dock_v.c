/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. https://ayyi.github.io/samplecat/    |
 | copyright (C) 2016-2024 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#include "config.h"
#include "debug/debug.h"
#include "agl/ext.h"
#include "agl/shader.h"
#include "agl/event.h"
#include "views/panel.h"
#include "views/dock_v.h"

#define SPACING 15
#define DIVIDER 5
#define HANDLE_HEIGHT(P) (((PanelView*)P)->title ? PANEL_DRAG_HANDLE_HEIGHT : 0)

static void dock_free (AGlActor*);
static AGlActor* find_handle_by_y (DockVView*, float);

static AGl* agl = NULL;
static int instance_count = 0;
static AGlActorClass actor_class = {0, "Dock V", (AGlActorNew*)dock_v_view, dock_free};


AGlActorClass*
dock_v_get_class ()
{
	return &actor_class;
}


static void
_init ()
{
	if (!agl) {
		agl = agl_get_instance();
	}
}


static inline float
panel_spacing (PanelView* panel)
{
	return panel->no_border ? 4 : SPACING;
}


static inline int
get_total_spacing (DockVView* dock)
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
dock_v_view (gpointer _)
{
	instance_count++;

	_init();

	bool dock_v_paint (AGlActor* actor)
	{
		DockVView* dock = (DockVView*)actor;

		// dividing lines
		GList* l = dock->panels;
		int y = ((AGlActor*)l->data)->region.y2;
		l = l->next;
		for(;l;l=l->next){
			AGlActor* a = l->data;
			if(y){
				AGlActor* prev = l->prev->data;
				y = a->region.y1;
				if(!((PanelView*)prev)->no_border){
					agl_rect_((AGlRect){0, y - SPACING / 2 - 1, agl_actor__width(actor), 1});
				}
				y = a->region.y2;
			}
		}

		if(dock->handle.opacity > 0.0){
			float alpha = dock->handle.opacity;
			SET_PLAIN_COLOUR (agl->shaders.plain, (0x999999ff & 0xffffff00) + (uint32_t)(alpha * 0xff));
			agl_use_program((AGlShader*)agl->shaders.plain);
			agl_rect_((AGlRect){0, dock->handle.actor->region.y1 - SPACING / 2 - DIVIDER / 2 , agl_actor__width(actor), DIVIDER});
		}

		return true;
	}

	void dock_init (AGlActor* a)
	{
	}

	void dock_set_state (AGlActor* actor)
	{
		SET_PLAIN_COLOUR (agl->shaders.plain, 0x66666666);
	}

	void dock_v_layout (AGlActor* actor)
	{
		DockVView* dock = (DockVView*)actor;

		typedef struct {
			AGlActor* actor;
			int       height;
		} Item;
		Item items[g_list_length(dock->panels)];

		int req = 0;
		int i = 0;
		for(GList* l=dock->panels;l;l=l->next,i++){
			items[i] = (Item){l->data, 0};
			PanelView* panel = (PanelView*)items[i].actor;
			if(panel->size_req.preferred.y > -1) req += panel->size_req.preferred.y;
		}

		// fix any incorrect positioning that may have come from the config
		// (maybe move this)
		{
			// there is redundant information (position and height)
			// here we choose to preserve height and adjust position if inconsistent
			float y = 0;
			for(i=0;i<G_N_ELEMENTS(items);i++){
				Item* item = &items[i];
				PanelView* panel = (PanelView*)item->actor;
				AGlActor* a = (AGlActor*)panel;
				if(a->region.y1 != y){
					dbg(1, "%s: correcting panel position: %.0f (expected %.0f)", actor->name, a->region.y1, y);
					float height = agl_actor__height(a);
					a->region.y1 = y;
					a->region.y2 = y + height;
				}
				y += agl_actor__height(a) + panel_spacing(panel);
			}
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

		int vspace = agl_actor__height(actor) - get_total_spacing(dock);
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
			y += item->height + ((i < G_N_ELEMENTS(items) - 1) ? panel_spacing(panel) : 0);
		}

		if(y < agl_actor__height(actor)){
			// under allocated
			int remaining = agl_actor__height(actor) - y;
			int n_resizable = 0;
			for(i=0;i<G_N_ELEMENTS(items);i++){
				Item* item = &items[i];
				PanelView* panel = (PanelView*)item->actor;
				if(panel->size_req.max.y < 0 || item->height < panel->size_req.max.y){
					n_resizable ++;
				}
			}
			if(n_resizable){
				typedef struct {PanelView* panel; int h; int amount; bool full;} A; // TODO just use Item

				void distribute(A L[], int _to_distribute, int n_resizable, int iter)
				{
					g_return_if_fail(_to_distribute > 0);

					int to_distribute = _to_distribute;
					int each = to_distribute / n_resizable;
					int remainder = to_distribute % n_resizable;

					#define CHECK_FULL(A) if(height + amount == max){ A->full = true; n_resizable--; }

					int i; for(i=0;i<20;i++){
						A* a = &L[i];
						if(a->panel){
							if(!a->full){
								PanelView* panel = a->panel;
								int max = panel->size_req.max.y < 0 ? 10000 : panel->size_req.max.y;
								int height = a->h + a->amount;
								int amount = MIN(max - height, each);
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
					if(iter == 5) pwarn("failed to distribute");
				}
				A L[G_N_ELEMENTS(items) + 1];
				for(i=0;i<G_N_ELEMENTS(items);i++){
					Item* item = &items[i];
					L[i] = (A){(PanelView*)item->actor,item->height,};
				}
				L[i] = (A){NULL,};
				distribute(L, remaining, n_resizable, 0);

				for(i=0;i<G_N_ELEMENTS(items);i++){
					Item* item = &items[i];
					A* a = &L[i];
					item->height += a->amount;
				}
			}
		}
		else if(y > agl_actor__height(actor)){
			dbg(1, "%s: over allocated", actor->name);
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
				int gain = (1000 * y) / (int)agl_actor__height(actor);
				int y = 0;
				for(i=0;i<G_N_ELEMENTS(items)-1;i++){
					Item* item = &items[i];
					PanelView* panel = (PanelView*)item->actor;
					y += items[i].height = MAX((1000 * items[i].height) / gain, panel->size_req.min.y);
					y += panel_spacing(panel);
				}
				// last can be less than req.min
				items[G_N_ELEMENTS(items) - 1].height = (int)agl_actor__height(actor) - y;
			}
		}

		y = 0;
		GList* l = dock->panels;
		for(int i=0;l;l=l->next,i++){
			AGlActor* a = (AGlActor*)l->data;
			a->region.y1 = y;
			a->region.y2 = y + items[i].height;
			y += items[i].height + panel_spacing((PanelView*)a);
		}
	}

	bool dock_event (AGlActor* actor, AGlEvent* event, AGliPt xy)
	{
		DockVView* dock = (DockVView*)actor;

		void animation_done (WfAnimation* animation, gpointer user_data)
		{
		}

		switch (event->type) {
			case AGL_BUTTON_PRESS:
				agl_actor__grab(actor);
				//set_cursor(arrange->canvas->widget->window, CURSOR_H_DOUBLE_ARROW);
				return AGL_HANDLED;
			case AGL_MOTION_NOTIFY:
				if (actor_context.grabbed == actor) {
					int y = xy.y - actor->region.y1;

					AGlActor* a2 = dock->handle.actor;
					GList* l = g_list_find(dock->panels, a2);
					g_return_val_if_fail(l->prev, false);
					AGlActor* a1 = l->prev->data;

					int min_diff = -agl_actor__height(a1);
					int max_diff = agl_actor__height(a2);
					if (((PanelView*)a1)->size_req.min.y > -1) {
						min_diff = ((PanelView*)a1)->size_req.min.y - agl_actor__height(a1) + PANEL_DRAG_HANDLE_HEIGHT;
					}
					if (((PanelView*)a1)->size_req.max.y > -1) {
						max_diff = ((PanelView*)a1)->size_req.max.y - agl_actor__height(a1) + PANEL_DRAG_HANDLE_HEIGHT;
					}
					int diff = CLAMP(y - a2->region.y1, min_diff, max_diff);
					if (diff) {
						a2->region.y1 = y;
						agl_actor__set_size(a2);
						agl_actor__invalidate(a2);

						a1->region.y2 += diff;
						agl_actor__set_size(a1);
						agl_actor__invalidate(a1);
					}
				} else {
					AGlActor* f = find_handle_by_y(dock, xy.y);
					if (f) {
						if (!dock->handle.opacity) {
							//set_cursor(arrange->canvas->widget->window, CURSOR_H_DOUBLE_ARROW);
							dock->animatables[0]->target_val.f = 1.0;
							dock->handle.actor = f;
							agl_actor__start_transition(actor, g_list_append(NULL, dock->animatables[0]), animation_done, NULL);
						}
					} else {
						if (dock->handle.opacity) {
							dock->animatables[0]->target_val.f = 0.0;
							agl_actor__start_transition(actor, g_list_append(NULL, dock->animatables[0]), animation_done, NULL);
						}
					}
				}
				return AGL_HANDLED;
			case AGL_LEAVE_NOTIFY:
				dbg (1, "LEAVE_NOTIFY");
				if (dock->handle.opacity > 0.0) {
					dock->animatables[0]->target_val.f = 0.0;
					agl_actor__start_transition(actor, g_list_append(NULL, dock->animatables[0]), animation_done, NULL);
				}
				break;
			case AGL_BUTTON_RELEASE:
			default:
				break;
		}
		return AGL_NOT_HANDLED;
	}

	DockVView* dock = agl_actor__new(DockVView,
		.panel = {
			.actor = {
				.class = &actor_class,
				.program = (AGlShader*)agl->shaders.plain,
				.init = dock_init,
				.paint = dock_v_paint,
				.set_state = dock_set_state,
				.set_size = dock_v_layout,
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
		.val.f       = &dock->handle.opacity,
		.start_val.f = 0.0,
		.target_val.f= 0.0,
		.type        = WF_FLOAT
	);

	return (AGlActor*)dock;
}


static void
dock_free (AGlActor* actor)
{
	DockVView* dock = (DockVView*)actor;

	g_clear_pointer(&dock->panels, g_list_free);

	if(!--instance_count){
	}
}


AGlActor*
dock_v_add_panel (DockVView* dock, AGlActor* panel)
{
	dock->panels = g_list_append(dock->panels, panel);
	return agl_actor__add_child((AGlActor*)dock, panel);
}


static AGlActor*
find_handle_by_y (DockVView* dock, float pos)
{
	GList* l = dock->panels;
	for(;l;l=l->next){
		AGlActor* a = l->data;
		if(!((PanelView*)a)->no_border && pos > a->region.y2 && pos < a->region.y2 + SPACING){
			GList* l = g_list_find(dock->panels, a);
			return l->next ? l->next->data : NULL;
		}
	}
	return NULL;
}


void
dock_v_move_panel_to_index (DockVView* dock, AGlActor* panel, int i)
{
	dbg(1, "i=%i", i);

	dock->panels = g_list_remove(dock->panels, panel);
	dock->panels = g_list_insert(dock->panels, panel, i);
	agl_actor__set_size((AGlActor*)dock);
}


void
dock_v_move_panel_to_y (DockVView* dock, AGlActor* panel, int y)
{
	int find_index (DockVView* dock, int y)
	{
		int i = 0;
		for (GList* l=dock->panels;l;l=l->next,i++) {
			AGlActor* a = l->data;
			if(a->region.y1 > y) return i - 1;
		}
		return i;
	}

	int i = find_index(dock, y);
	dock_v_move_panel_to_index(dock, panel, i);
}


