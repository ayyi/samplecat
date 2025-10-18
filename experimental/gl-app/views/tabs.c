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
#include "agl/utils.h"
#include "agl/fbo.h"
#include "agl/text/renderer.h"
#include "agl/behaviours/cache.h"
#include "materials/icon_ring.h"
#include "behaviours/style.h"
#include "shader.h"
#include "views/tabs.h"

extern void agl_actor__render_from_fbo (AGlActor*);

#define TAB_HEIGHT (LINE_HEIGHT + 10)
#define GAP 30

static void tabs_free (AGlActor*);

static AGl* agl = NULL;
static int instance_count = 0;
static AGlActorClass actor_class = {0, "Tabs", (AGlActorNew*)tabs_view, tabs_free};
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
	if (!agl) {
		agl = agl_get_instance();
		agl_actor_class__add_behaviour(&actor_class, cache_get_class());

		ring_material = ring_new();
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
		TabTransition* slide = &tabs->slide;
		IconMaterial* icon = (IconMaterial*)ring_material;

		int x = 0;
		int i = 0;
		for (GList* l = tabs->tabs;l;l=l->next) {
			TabsViewTab* tab = l->data;

			icon->bg = 0x000000ff;
			if (tabs->hover.animatable.val.f && i == tabs->hover.tab) {
				PLAIN_COLOUR2 (agl->shaders.plain) = icon->bg = 0x33333300 + (int)(((float)0xff) * tabs->hover.opacity);
				agl_use_program (agl->shaders.plain);
				agl_rect_((AGlRect){i * tab_width, -0, tab_width - 10, TAB_HEIGHT - 6});
			}

			//icon->active = i == tabs->active;
			icon->chr = tab->actor->name[0];
			icon->colour = (i == tabs->active) ? tab->actor->colour : 0x777777ff;

			builder()->offset.x += x;
			builder()->offset.y += 3;
			ring_material_class.render(ring_material);
			builder()->offset.x -= x;
			builder()->offset.y -= 3;

			x += 22;
			agl_print(x, 4, 0, 0xffffffff, tab->name);
			x += tab_width - 22;

			i++;
		}

		PLAIN_COLOUR2 (agl->shaders.plain) = (STYLE.fg & 0xffffff00) + 0xff;
		agl_use_program ((AGlShader*)agl->shaders.plain);
		agl_rect_ ((AGlRect){tabs->active * tab_width, TAB_HEIGHT - 6, tab_width - 10, 2});

		// set content position
		if (ABS(slide->x) > 0.01) {

			/*
			 *  The child actors are rendered directly, not by the scene graph.
			 *  This is a workaround for the fact that you cannot apply a stencil to children.
			 *  (may not still be true)
			 */
			AGlActor* items[] = {slide->prev, slide->next};
			float x = slide->x;
			float w = agl_actor__width(actor);
			for (int i=0; i<G_N_ELEMENTS(items); i++) {
				AGlActor* a = items[i];
#ifdef AGL_ACTOR_RENDER_CACHE
				if (!a->cache.valid) {
					float x2 = a->region.x2;
					a->region.x2 = a->region.x1 + agl_actor__width(actor);

					if (!a->fbo) {
						AGlBehaviour* cache = agl_actor__add_behaviour (a, cache_behaviour());
						agl_behaviour_init (cache, a);
					}
					agl_actor__paint (a);
					a->region.x2 = x2;
				}

				if (a->cache.valid) {
					AGlfPt offset = builder()->offset;
					AGlfRegion region = a->region;

					builder()->offset.y += TAB_HEIGHT;

					if (i != tabs->active) {
						a->region.x2 = x;
						a->cache.offset.x = -(w + (tabs->active == 1 ? GAP : 0) - x);
					} else {
						builder()->offset.x += x + w + (tabs->active == 0 ? GAP : 0);

						a->region.x2 = a->region.x1 - x;
					}

					agl_actor__render_from_fbo(a);

					a->region = region;
					builder()->offset = offset;
#else
				if (false) {
#endif
				} else {
					a->region.x1 = x;
					a->region.x2 = a->region.x1 + w;
				}
				if (x < 0) x += w; else x -= w;
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
		if (tab) {
			tab->actor->region = (AGlfRegion){0, TAB_HEIGHT, agl_actor__width(actor), agl_actor__height(actor)};
		}
	}

	bool tabs_event (AGlActor* actor, GdkEvent* event, AGliPt xy)
	{
		TabsView* tabs = (TabsView*)actor;

		void end_hover (TabsView* tabs)
		{
			tabs->hover.animatable.target_val.f = 0.0;
			agl_actor__start_transition(actor, g_list_append(NULL, &tabs->hover.animatable), NULL, NULL);
		}

		switch (event->type) {
			case GDK_BUTTON_PRESS:
			case GDK_BUTTON_RELEASE:;
				int active = xy.x / tab_width;
				dbg(1, "x=%i y=%i active=%i", xy.x, xy.y, active);
				tabs_select_tab(tabs, active);
				return AGL_HANDLED;

			case GDK_LEAVE_NOTIFY:
				end_hover(tabs);
				return AGL_HANDLED;

			case GDK_MOTION_NOTIFY:;
				int tab = xy.x / tab_width;
				if (xy.y > TAB_HEIGHT || tab >= g_list_length(tabs->tabs)) {
					end_hover(tabs);
				} else {
					if (!tabs->hover.animatable.val.f || tab != tabs->hover.tab) {
						tabs->hover.tab = tab;
						tabs->hover.animatable.target_val.f = 1.0;
						agl_actor__start_transition(actor, g_list_append(NULL, &tabs->hover.animatable), NULL, NULL);
					}
				}
				return AGL_HANDLED;

			default:
				break;
		}
		return AGL_NOT_HANDLED;
	}

	TabsView* view = agl_actor__new(TabsView,
		.actor = {
			.class = &actor_class,
			.init = tabs_init,
			.paint = tabs_paint,
			.set_size = tabs_set_size,
			.on_event = tabs_event,
		},
		.active = -1,
	);

	view->hover.animatable = (WfAnimatable){
		.val.f       = &view->hover.opacity,
		.start_val.f = 0.0,
		.target_val.f= 0.0,
		.type        = WF_FLOAT
	};

	view->slide = (TabTransition){
		.animatable = {
			.val.f = &view->slide.x,
			.type  = WF_FLOAT
		}
	};

	return (AGlActor*)view;
}


static void
tabs_free (AGlActor* actor)
{
	TabsView* tabs = (TabsView*)actor;

	g_list_free_full(tabs->tabs, g_free);

	if (!--instance_count) {
		ring_material_class.free(ring_material);
	}
}


void
tabs_view__add_tab (TabsView* tabs, const char* name, AGlActor* child)
{
	TabsViewTab* tab = AGL_NEW(TabsViewTab, .actor=child, .name=name);

	tabs->tabs = g_list_append(tabs->tabs, tab);
	agl_actor__add_child((AGlActor*)tabs, child);

	child->region.x2 = 0; // hide until active

	if (tabs->active < 0) {
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
		TabTransition* slide = &tabs->slide;

		int i = 0;
		for (GList* l = actor->children; l; l = l->next, i++) {
			AGlActor* a = l->data;
			if (i == tabs->active) {
				a->region.x1 = 0;
			}
		}

		slide->next->region.x1 = 0;
		slide->next->region.x2 = agl_actor__width(actor);
		slide->prev->region.x2 = 0; // hide

		slide->next->cache.offset = (AGliPt){0,};
		slide->prev->cache.offset = (AGliPt){0,};
	}

	int n_tabs = g_list_length(tabs->tabs);

	if (active < n_tabs && active != tabs->active) {
		TabTransition* slide = &tabs->slide;
		TabsViewTab* prev = NULL;
		TabsViewTab* next = g_list_nth_data(tabs->tabs, active);
		slide->next = next->actor;

		if (tabs->active > -1) {
			prev = g_list_nth_data(tabs->tabs, tabs->active);
			prev->actor->region.x2 = 0;
			slide->prev = prev->actor;
		}

		int direction = (active > tabs->active) * 2 - 1;

		tabs->active = active;
		next->actor->region = (AGlfRegion){0, TAB_HEIGHT, agl_actor__width(actor), agl_actor__height(actor)};
		agl_actor__set_size(next->actor);
		agl_actor__invalidate(actor);

		if (n_tabs > 1 && next && prev) {
			slide->x = (agl_actor__width(actor) + GAP) * direction;
			slide->animatable.target_val.f = 0;
			// prevent the scene graph from rendering the panels - the tabs container will render them
			slide->next->region.x2 = -10000;
			slide->prev->region.x2 = -10000;

			agl_actor__start_transition(actor, g_list_append(NULL, &tabs->slide), slide_done, tabs);
		}
	}
}
