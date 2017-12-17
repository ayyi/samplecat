/**
* +----------------------------------------------------------------------+
* | This file is part of the Ayyi project. http://www.ayyi.org           |
* | copyright (C) 2013-2017 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
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

static void scrollbar_set_size      (AGlActor*);
static bool scrollbar_on_event      (AGlActor*, GdkEvent*, AGliPt);
static void hscrollbar_bar_position (AGlActor*, iRange*);
static void vscrollbar_bar_position (AGlActor*, iRange*);


typedef struct {
    AGlActor       actor;
    GtkOrientation orientation;
    AGliPt         grab_offset; // TODO this should probably be part of the actor_context
    float          opacity;
    WfAnimatable   animation;
} ScrollbarActor;

static struct Press {
    AGliPt     pt;
    AGliRegion viewport;
} press = {{0},};

static AGl* agl = NULL;
static AGlActorClass actor_class = {0, "Scrollbar", (AGlActorNew*)scrollbar_view};


AGlActorClass*
scrollbar_view_get_class ()
{
	return &actor_class;
}


AGlActor*
scrollbar_view(AGlActor* panel, GtkOrientation orientation)
{
	agl = agl_get_instance();

	bool arr_gl_scrollbar_draw_gl1(AGlActor* actor)
	{
		return true;
	}

	bool arr_gl_scrollbar_draw_h(AGlActor* actor)
	{
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
		return true;
	}

	bool scrollbar_draw_v(AGlActor* actor)
	{
		if(!actor->disabled){
#if 0
			if(((ScrollbarActor*)actor)->animation.val.f > 0.61){
				agl->shaders.plain->uniform.colour = 0xffffff66;
				agl_use_program((AGlShader*)agl->shaders.plain);
				agl_rect(0, -actor->parent->scrollable.y1, agl_actor__width(actor), agl_actor__height(actor));
			}
#endif
			iRange bar = {0,};
			vscrollbar_bar_position(actor, &bar);
			bar.start += -actor->parent->scrollable.y1;
			bar.end += -actor->parent->scrollable.y1;

			v_scrollbar_shader.uniform.colour = 0x6677ffff;
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
		if(((ScrollbarActor*)actor)->orientation == GTK_ORIENTATION_VERTICAL){
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
			actor->paint = agl->use_shaders ? arr_gl_scrollbar_draw_h : arr_gl_scrollbar_draw_gl1;
#endif
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
			.region = (AGliRegion){0, 0, 1, 1},
		},
		.orientation = orientation,
		// TODO fade in and out when leaving/entering window
		.opacity = 0.5,
		.animation = {
			.start_val.f = 0.6,
			.val.f       = 0.6,
			.type        = WF_FLOAT
		}
	);
	scrollbar->animation.model_val.f = &scrollbar->opacity;

	return (AGlActor*)scrollbar;
}


static void
scrollbar_set_size(AGlActor* actor)
{
	AGlActor* root = (AGlActor*)actor->root;

	if(actor->parent->region.x2 > 0/* && actor->region.x2 > 0*/){
		if(((ScrollbarActor*)actor)->orientation == GTK_ORIENTATION_VERTICAL){
			actor->region = (AGliRegion){
				.x1 = actor->parent->region.x2/* - V_SCROLLBAR_H_PADDING*/ - 2 * R - 8,
				.x2 = actor->parent->region.x2/* - V_SCROLLBAR_H_PADDING*/,
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
		int handle = MAX(10, agl_actor__height(parent) * h_pct);
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

	int x = xy.x;

	bool handled = false;

	switch (event->type){
		case GDK_MOTION_NOTIFY:
			if(actor_context.grabbed == actor){
				if(((ScrollbarActor*)actor)->orientation == GTK_ORIENTATION_VERTICAL){
					//int dy = xy.y - press.pt.y;
					//double scale = (actor->parent->scrollable.y2 - actor->parent->scrollable.y1) / (double)agl_actor__height(actor);
					//int new = ((int)-press.viewport.y1) + dy * scale;
#if 0
				}else{
					float pos = x - ((ScrollbarActor*)actor)->grab_offset.x;
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
			}
			break;
		case GDK_ENTER_NOTIFY:
			scrollbar->opacity = 1.0;
			agl_actor__start_transition(actor, g_list_append(NULL, &scrollbar->animation), animation_done, NULL);
			break;
		case GDK_LEAVE_NOTIFY:
			scrollbar->opacity = 0.6;
			agl_actor__start_transition(actor, g_list_append(NULL, &scrollbar->animation), animation_done, NULL);
			break;
		case GDK_BUTTON_PRESS:
			{
				press = (struct Press){.pt = xy};

				iRange bar = {0,};
				if(((ScrollbarActor*)actor)->orientation == GTK_ORIENTATION_VERTICAL){
					vscrollbar_bar_position (actor, &bar);
					if(xy.y > bar.start && xy.y < bar.end){
						actor_context.grabbed = actor;
						((ScrollbarActor*)actor)->grab_offset = (AGliPt){x - bar.start, xy.y - bar.start};
						handled = true;
					}
				}else{
					hscrollbar_bar_position (actor, &bar);
					if(x > bar.start && x < bar.end){
						actor_context.grabbed = actor;
						((ScrollbarActor*)actor)->grab_offset = (AGliPt){x - bar.start, xy.y - bar.start};
						handled = true;
					}
				}
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


