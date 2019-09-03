/**
* +----------------------------------------------------------------------+
* | This file is part of the Ayyi project. http://www.ayyi.org           |
* | copyright (C) 2013-2019 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#include "config.h"
#include "debug/debug.h"
#include "samplecat/typedefs.h"
#include "scrollbar.h"
#include "shader.h"

#define R 2
#define Rf 2.0
#define V_SCROLLBAR_H_PADDING 2
#define V_SCROLLBAR_V_PADDING 3
#define H_SCROLLBAR_H_PADDING 3
#define H_SCROLLBAR_V_PADDING 2

static void scrollbar_set_size       (AGlActor*);
static bool scrollbar_on_event       (AGlActor*, GdkEvent*, AGliPt);
static void scrollbar_start_activity (AGlActor*, bool);
static void hscrollbar_bar_position  (AGlActor*, iRange*);
static void vscrollbar_bar_position  (AGlActor*, iRange*);


static struct Press {
    AGliPt     pt;
    AGliPt     offset; // TODO this should probably be part of the actor_context
} press = {{0},};

static guint activity = 0;

static AGl* agl = NULL;
static AGlActorClass actor_class = {0, "Scrollbar", (AGlActorNew*)scrollbar_view};


AGlActorClass*
scrollbar_view_get_class ()
{
	return &actor_class;
}


static void
scrollbar_on_scroll (Observable* observable, int row, gpointer scrollbar)
{
	scrollbar_start_activity((AGlActor*)scrollbar, false);
}


AGlActor*
scrollbar_view (AGlActor* panel, AGlOrientation orientation)
{
	agl = agl_get_instance();

	bool arr_gl_scrollbar_draw_gl1(AGlActor* actor)
	{
		return true;
	}

	bool arr_gl_scrollbar_draw_h(AGlActor* actor)
	{
#if 0
		if(!actor->disabled){
			iRange bar;
			hscrollbar_bar_position(actor, &bar);

			if(((ScrollbarActor*)actor)->animation.val.f > 0.61){
				agl->shaders.plain->uniform.colour = 0xffffff66;
				agl_use_program((AGlShader*)agl->shaders.plain);
				agl_rect(0, 1, agl_actor__width(actor), agl_actor__height(actor) - 1);
			}

#if 0 // TODO define this shader
			h_scrollbar_shader.uniform.colour = 0xffffff00 + (int)(((ScrollbarActor*)actor)->animation.val.f * 0xff);
			h_scrollbar_shader.uniform.bg_colour = v_scrollbar_shader.uniform.colour & 0xffffff00;
			h_scrollbar_shader.uniform.centre1 = (AGliPt){bar.start + 4, 6};
			h_scrollbar_shader.uniform.centre2 = (AGliPt){bar.end   - 4, 6};
			agl_use_program((AGlShader*)&h_scrollbar_shader);
#endif

			agl_rect(H_SCROLLBAR_H_PADDING + bar.start, H_SCROLLBAR_V_PADDING, bar.end - bar.start, 11);
		}
#endif
		return true;
	}

	bool scrollbar_draw_v(AGlActor* actor)
	{
		ScrollbarActor* scrollbar = (ScrollbarActor*)actor;

		if(!actor->disabled){
			if(scrollbar->handle.animation.val.f < 0.05){
				return true;
			}

			if(scrollbar->trough.animation.val.f > 0.05){
				agl->shaders.plain->uniform.colour = 0xffffff00 + (int)(255.0f * scrollbar->trough.animation.val.f);
				agl_use_program((AGlShader*)agl->shaders.plain);
				agl_rect(0, -actor->parent->scrollable.y1, agl_actor__width(actor), agl_actor__height(actor));
			}

			iRange bar = {0,};
			vscrollbar_bar_position(actor, &bar);
			bar.start += -actor->parent->scrollable.y1;
			bar.end += -actor->parent->scrollable.y1;

			v_scrollbar_shader.uniform.colour = 0x6677ff00 + (int)(255.0f * scrollbar->handle.animation.val.f);
			v_scrollbar_shader.uniform.bg_colour = v_scrollbar_shader.uniform.colour & 0xffffff00;
			v_scrollbar_shader.uniform.centre1 = (AGliPt){6, bar.start + 4};
			v_scrollbar_shader.uniform.centre2 = (AGliPt){6, bar.end - 4};
			agl_use_program((AGlShader*)&v_scrollbar_shader);

			agl_rect(V_SCROLLBAR_H_PADDING, bar.start, agl_actor__width(actor) - 2 * V_SCROLLBAR_H_PADDING, bar.end - bar.start);
		}
		return true;
	}

	void scrollbar_init(AGlActor* actor)
	{
		if(((ScrollbarActor*)actor)->orientation == AGL_ORIENTATION_VERTICAL){
			if(!v_scrollbar_shader.shader.program){
				agl_create_program(&v_scrollbar_shader.shader);
				v_scrollbar_shader.uniform.radius = 3;
			}

			actor->paint = agl->use_shaders ? scrollbar_draw_v : agl_actor__null_painter;
		}else{
#if 0
			if(!h_scrollbar_shader.shader.program){
				agl_create_program(&h_scrollbar_shader.shader);
				h_scrollbar_shader.uniform.radius = 3;
			}
#endif
			actor->paint = agl->use_shaders ? arr_gl_scrollbar_draw_h : arr_gl_scrollbar_draw_gl1;
		}
	}

	ScrollbarActor* scrollbar = AGL_NEW(ScrollbarActor,
		.actor = {
			.class = &actor_class,
			.name = "Scrollbar",
			.init = scrollbar_init,
			.set_size = scrollbar_set_size,
			.paint = agl_actor__null_painter,
			.on_event = scrollbar_on_event,
		},
		.orientation = orientation,
		.scroll = observable_new(),
		.handle = {
			.opacity = 0.5,
			.animation = {
				.start_val.f = 0.7,
				.val.f       = 0.7,
				.type        = WF_FLOAT
			}
		},
		.trough = {
			.opacity = 0.0,
			.animation = {
				.start_val.f = 0.0,
				.val.f       = 0.0,
				.type        = WF_FLOAT
			},
		}
	);
	scrollbar->handle.animation.model_val.f = &scrollbar->handle.opacity;
	scrollbar->trough.animation.model_val.f = &scrollbar->trough.opacity;

	observable_subscribe(scrollbar->scroll, scrollbar_on_scroll, scrollbar);

	return (AGlActor*)scrollbar;
}


static void
scrollbar_set_size(AGlActor* actor)
{
	AGlActor* root = (AGlActor*)actor->root;

	if(actor->parent->region.x2 > 0/* && actor->region.x2 > 0*/){
		if(((ScrollbarActor*)actor)->orientation == AGL_ORIENTATION_VERTICAL){
			actor->region = (AGliRegion){
				.x1 = agl_actor__width(actor->parent)/* - V_SCROLLBAR_H_PADDING*/ - 2 * R - 8,
				.x2 = agl_actor__width(actor->parent)/* - V_SCROLLBAR_H_PADDING*/,
				.y1 = V_SCROLLBAR_V_PADDING,
				.y2 = agl_actor__height(actor->parent) - V_SCROLLBAR_V_PADDING
			};
		}else{
			int offset = 0;

			actor->region = (AGliRegion){
				.x1 = 0,
				.x2 = root->region.x2,
				.y1 = root->region.y2 - H_SCROLLBAR_V_PADDING - 2 * R - 5 + offset,
				.y2 = root->region.y2                                     + offset
			};
		}
	}
}


static void
hscrollbar_bar_position (AGlActor* actor, iRange* pos)
{
	*pos = (iRange){0,};
}


static void
vscrollbar_bar_position (AGlActor* actor, iRange* pos)
{
	AGlActor* parent = actor->parent;

	int parent_size = parent->scrollable.y2 - parent->scrollable.y1;
	if(agl_actor__height(actor) < parent_size){
#ifdef DEBUG
		if(agl_actor__height(parent) - parent->scrollable.y1 > parent_size) gwarn("scroll out of range");
#endif
		double h_pct = agl_actor__height(parent) / (double)(parent_size/* - agl_actor__height(actor)*/);
		double y_pct = - parent->scrollable.y1 / (double)(parent_size - agl_actor__height(parent));
		int handle = MAX(20, agl_actor__height(parent) * h_pct);
		pos->start = y_pct * (agl_actor__height(actor) - handle);
		pos->end = pos->start + handle;
	}else{
		*pos = (iRange){0,}; // no scrollbar needed
	}
}


static bool
scrollbar_on_event(AGlActor* actor, GdkEvent* event, AGliPt xy)
{
	void animation_done (WfAnimation* animation, gpointer user_data)
	{
	}

	ScrollbarActor* scrollbar = (ScrollbarActor*)actor;

	bool handled = false;

	switch (event->type){
		case GDK_MOTION_NOTIFY:
			if(actor_context.grabbed == actor){
				if(scrollbar->orientation == AGL_ORIENTATION_VERTICAL){
					int dy = xy.y - press.pt.y;
					iRange bar = {0,};
					vscrollbar_bar_position (actor, &bar);
					int useable_bar_range = agl_actor__height(actor) - (bar.end - bar.start);
					double scale = (actor->parent->scrollable.y2 - actor->parent->scrollable.y1) / (double)useable_bar_range;
					int new = (press.pt.y - press.offset.y + dy);
					new = MIN(new, useable_bar_range);

					event->type = GDK_SCROLL; // custom event for use by the parent scrollable
					((GdkEventMotion*)event)->y = new * scale;
#if 0
				}else{
					float pos = x - press.offset.x;
					/*
					iRange bar; hscrollbar_bar_position (actor, &bar);
					int total = agl_actor__width(((AGlActor*)actor->root)) - (bar.end - bar.start) - 0 * SCROLLBAR_H_PADDING;
					float pct = MIN(1.0, pos / total);
					*/
					int dx = x - press.pt.x;
					double vp_size = arr_gl_pos2px_(arrange, &am_object_val(&song->loc[AM_LOC_END]).sp) + 20.0;
					double scale = vp_size / (double)agl_actor__width(actor);
					arrange->canvas->scroll_to(arrange, ((int)-press.viewport.x1) + dx * scale, -1);
#endif
				}
			}else{
				iRange bar = {0,};
				vscrollbar_bar_position (actor, &bar);
				scrollbar->handle.opacity = (xy.y > bar.start && xy.y < bar.end) ? 1.0 : 0.7;
				agl_actor__start_transition(actor, g_list_append(NULL, &scrollbar->handle.animation), animation_done, NULL);
			}
			break;
		case GDK_ENTER_NOTIFY:
			scrollbar->trough.opacity = 0.1;
			agl_actor__start_transition(actor, g_list_append(NULL, &scrollbar->trough.animation), animation_done, NULL);

			iRange bar = {0,};
			vscrollbar_bar_position (actor, &bar);
			scrollbar_start_activity(actor, (xy.y > bar.start && xy.y < bar.end));
			break;
		case GDK_LEAVE_NOTIFY:
			scrollbar_start_activity(actor, false);

			scrollbar->trough.opacity = 0.0;
			agl_actor__start_transition(actor, g_list_append(NULL, &scrollbar->trough.animation), animation_done, NULL);

			scrollbar->handle.opacity = 0.7;
			agl_actor__start_transition(actor, g_list_append(NULL, &scrollbar->handle.animation), animation_done, NULL);
			break;
		case GDK_BUTTON_PRESS:
			{
				iRange bar = {0,};
				if(((ScrollbarActor*)actor)->orientation == AGL_ORIENTATION_VERTICAL){
					vscrollbar_bar_position (actor, &bar);
					if(xy.y >= bar.start && xy.y <= bar.end){
						actor_context.grabbed = actor;
						handled = true;
					}
				}else{
					hscrollbar_bar_position (actor, &bar);
					if(xy.x > bar.start && xy.x < bar.end){
						actor_context.grabbed = actor;
						handled = true;
					}
				}

				press = (struct Press){
					.pt = xy,
					.offset = (AGliPt){xy.x - bar.start, xy.y - bar.start}
				};
			}
			break;
		case GDK_BUTTON_RELEASE:
			{
				actor_context.grabbed = NULL;
				handled = true;
			}
			break;
		default:
			break;
	}
	return handled;
}


static bool
go_inactive(gpointer _view)
{
	AGlActor* actor = _view;
	ScrollbarActor* scrollbar = _view;

	if(agl_actor__is_hovered(actor)){
		return G_SOURCE_CONTINUE;
	}

	activity = 0;

	scrollbar->handle.opacity = 0.0;
	agl_actor__start_transition(actor, g_list_append(NULL, &scrollbar->handle.animation), NULL, NULL);

	return G_SOURCE_REMOVE;
}


static void
scrollbar_start_activity(AGlActor* actor, bool hovered)
{
	ScrollbarActor* scrollbar = (ScrollbarActor*)actor;

	scrollbar->handle.opacity = hovered ? 1.0 : 0.7;
	agl_actor__start_transition(actor, g_list_append(NULL, &scrollbar->handle.animation), NULL, NULL);

	if(activity){
		g_source_remove(activity);
	}
	activity = g_timeout_add(5000, go_inactive, actor);
}
