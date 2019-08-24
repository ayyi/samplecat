/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2016-2019 Tim Orford <tim@orford.org>                  |
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
#include <GL/gl.h>
#include "debug/debug.h"
#include "agl/ext.h"
#include "agl/utils.h"
#include "materials/icon_ring.h"
#include "application.h"
#include "shader.h"
#include "views/tabs.h"

#define _g_free0(var) (var = (g_free (var), NULL))

#define FONT "Droid Sans"
#define TAB_HEIGHT 30

static AGl* agl = NULL;
static int instance_count = 0;
static AGlActorClass actor_class = {0, "Tabs", (AGlActorNew*)tabs_view};
static int tab_width = 80;

static AGlMaterial* ring_material = NULL;
extern AGlMaterialClass ring_material_class;

static void tabs_select_tab (TabsView*, int);


AGlActorClass*
tabs_view_get_class ()
{
	return &actor_class;
}


static void
_init ()
{
	static bool init_done = false;

	if(!init_done){
		agl = agl_get_instance();
		agl_set_font_string("Roboto 10"); // initialise the pango context

		ring_material = ring_new();

		init_done = true;
	}
}


AGlActor*
tabs_view (gpointer _)
{
	instance_count++;

	_init();

	bool tabs_paint (AGlActor* actor)
	{
		TabsView* tabs = (TabsView*)actor;
		IconMaterial* icon = (IconMaterial*)ring_material;

		int x = 0;
		int i = 0;
		for(GList* l = tabs->tabs;l;l=l->next){
			TabsViewTab* tab = l->data;

			icon->bg = 0x000000ff;
			if(tabs->hover.animatable.val.f && i == tabs->hover.tab){
				agl->shaders.plain->uniform.colour = icon->bg = 0x33333300 + (int)(((float)0xff) * tabs->hover.animatable.val.f);
				agl_use_program((AGlShader*)agl->shaders.plain);
				agl_rect_((AGlRect){i * tab_width, -4, tab_width - 10, TAB_HEIGHT - 6});
			}

			//icon->active = i == tabs->active;
			icon->chr = tab->actor->name[0];
			icon->colour = (i == tabs->active) ? tab->actor->colour : 0x777777ff;

			glTranslatef(x, -1, 0);
			ring_material_class.render(ring_material);
			glTranslatef(-x, 1, 0);

			x += 22;
			agl_print(x, 0, 0, 0xffffffff, tab->name);
			x += tab_width - 22;

			i++;
		}

		agl->shaders.plain->uniform.colour = (app->style.fg & 0xffffff00) + 0xff;
		agl_use_program((AGlShader*)agl->shaders.plain);
		agl_rect_((AGlRect){tabs->active * tab_width, TAB_HEIGHT - 10, tab_width - 10, 2});

		// set content position
		if(ABS(tabs->x.val.f) > 0.01){
			// there should be 2 content panes to position
			for(GList* l = actor->children; l; l = l->next){
				AGlActor* a = l->data;
				if(agl_actor__width(a)){
					int w = agl_actor__width(a);
					a->region.x1 = tabs->x.val.f;
					a->region.x2 = a->region.x1 + w;
				}
			}
		}

		return true;
	}

	void tabs_init (AGlActor* a)
	{
	}

	void tabs_set_size (AGlActor* actor)
	{
		TabsView* tabs = (TabsView*)actor;

		TabsViewTab* tab = g_list_nth_data(tabs->tabs, tabs->active);
		if(tab){
			tab->actor->region = (AGliRegion){0, TAB_HEIGHT, agl_actor__width(actor), agl_actor__height(actor)};
		}
	}

	bool tabs_event (AGlActor* actor, GdkEvent* event, AGliPt xy)
	{
		TabsView* tabs = (TabsView*)actor;

		void end_hover(TabsView* tabs)
		{
			tabs->hover.opacity = 0.0;
			agl_actor__start_transition(actor, g_list_append(NULL, &tabs->hover.animatable), NULL, NULL);
		}

		switch(event->type){
			case GDK_BUTTON_PRESS:
			case GDK_BUTTON_RELEASE:;
				int active = xy.x / tab_width;
				dbg(1, "x=%i y=%i active=%i", xy.x, xy.y, active);
				tabs_select_tab(tabs, active);
				break;

			case GDK_LEAVE_NOTIFY:
				end_hover(tabs);
				return AGL_HANDLED;

			case GDK_MOTION_NOTIFY:;
				int tab = xy.x / tab_width;
				if(tab >= g_list_length(tabs->tabs)){
					end_hover(tabs);
				}else{
					if(!tabs->hover.animatable.val.f || tab != tabs->hover.tab){
						tabs->hover.tab = tab;
						tabs->hover.opacity = 1.0;
						agl_actor__start_transition(actor, g_list_append(NULL, &tabs->hover.animatable), NULL, NULL);
					}
				}
				break;

			default:
				break;
		}
		return AGL_HANDLED;
	}

	void tabs_free (AGlActor* actor)
	{
		TabsView* tabs = (TabsView*)actor;

		g_list_free_full(tabs->tabs, g_free);

		ring_material_class.free(ring_material);

		if(!--instance_count){
		}
	}

	TabsView* view = AGL_NEW(TabsView,
		.actor = {
			.class = &actor_class,
			.name = "Tabs",
			.init = tabs_init,
			.free = tabs_free,
			.paint = tabs_paint,
			.set_size = tabs_set_size,
			.on_event = tabs_event,
		},
		.active = -1
	);

	view->hover.animatable = (WfAnimatable){
		.model_val.f = &view->hover.opacity,
		.start_val.f = 0.0,
		.val.f       = 0.0,
		.type        = WF_FLOAT
	};

	view->x = (WfAnimatable){
		.model_val.f = &view->_x,
		.type        = WF_FLOAT
	};

	return (AGlActor*)view;
}


void
tabs_view__add_tab (TabsView* tabs, const char* name, AGlActor* child)
{
	TabsViewTab* tab = AGL_NEW(TabsViewTab, .actor=child, .name=name);

	tabs->tabs = g_list_append(tabs->tabs, tab);
	agl_actor__add_child((AGlActor*)tabs, child);

	child->region.x2 = 0; // hide until active

	if(tabs->active < 0){
		tabs_select_tab(tabs, 0);
	}
}


static void
tabs_select_tab (TabsView* tabs, int active)
{
	AGlActor* actor = (AGlActor*)tabs;

	void slide_done (WfAnimation* animation, gpointer _)
	{
		TabsView* tabs = _;
		AGlActor* actor = (AGlActor*)tabs;
		dbg(0, "%s", actor->name);
		// hide non-active
		int i = 0;
		for(GList* l = actor->children; l; l = l->next, i++){
			AGlActor* a = l->data;
			if(i == tabs->active){
				a->region.x1 = 0;
				a->region.x2 = agl_actor__width(actor);
			}else{
				a->region.x1 = a->region.x2 = 0;
			}
		}
	}

	if(active < g_list_length(tabs->tabs) && active != tabs->active){
		TabsViewTab* prev;
		TabsViewTab* next = g_list_nth_data(tabs->tabs, active);

		if(tabs->active > -1){
			prev = g_list_nth_data(tabs->tabs, tabs->active);
			prev->actor->region.x2 = 0;
		}

		tabs->active = active;
		next->actor->region = (AGliRegion){0, TAB_HEIGHT, agl_actor__width(actor), agl_actor__height(actor)};
		agl_actor__set_size(next->actor);
		agl_actor__invalidate(actor);

		// TODO for smooth animation, ensure children are cached
		//      ... or even better, just cache the result and scroll it
		tabs->x.val.f = -100;
		tabs->_x = 0;
		agl_actor__start_transition(actor, g_list_append(NULL, &tabs->x), slide_done, tabs);
	}
}
