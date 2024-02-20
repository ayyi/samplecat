/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. https://ayyi.github.io/samplecat/    |
 | copyright (C) 2017-2024 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#include "config.h"
#include <X11/keysym.h>
#include "debug/debug.h"
#include "agl/actor.h"
#include "agl/fbo.h"
#include "agl/text/pango.h"
#include "agl/behaviours/cache.h"
#include "samplecat.h"
#include "shader.h"
#include "behaviours/panel.h"
#include "views/filters.h"

#define _g_object_unref0(var) ((var == NULL) ? NULL : (var = (g_object_unref (var), NULL)))

#define PADDING 1
#define BORDER 1

static void filters_free (AGlActor*);

static AGl* agl = NULL;
static int instance_count = 0;
static AGlActorClass actor_class = {0, "Filters", (AGlActorNew*)filters_view, filters_free};
static int mouse = 0;


AGlActorClass*
filters_view_get_class ()
{
	if (!agl) {
		agl = agl_get_instance();

		agl_actor_class__add_behaviour(&actor_class, panel_get_class());
		agl_actor_class__add_behaviour(&actor_class, cache_get_class());
	}

	return &actor_class;
}


AGlActor*
filters_view (gpointer _)
{
	instance_count++;

	filters_view_get_class();

	bool filters_paint (AGlActor* actor)
	{
		FiltersView* view = (FiltersView*)actor;

		int x = 0;
		int y = 4;

		agl_print_layout(x, y, 0, 0xffffffff, view->title);
		PangoRectangle logical_rect;
		pango_layout_get_pixel_extents(view->title, NULL, &logical_rect);
		x += logical_rect.width + 10;

		int n_filters = 0;
		int i; for(i=0;i<G_N_ELEMENTS(view->filters) - 1;i++){
			Observable* filter = view->filters[i].filter;
			char* val = filter->value.c;
			if(val && strlen(val)){
				if(!view->filters[i].layout){
					view->filters[i].layout = pango_layout_new (agl_pango_get_context());
				}
				char* text = strcmp(((NamedObservable*)filter)->name, "search")
					? g_strdup_printf("%s: %s", ((NamedObservable*)filter)->name, val)
					: g_strdup_printf("\"%s\"", val);
				pango_layout_set_text(view->filters[i].layout, text, -1);
				g_free(text);

				PangoRectangle logical_rect;
				pango_layout_get_pixel_extents(view->filters[i].layout, NULL, &logical_rect);
				int width = logical_rect.width;

				#define BTN_OFFSET 6
				AGlfPt size = (AGlfPt){width + 22., 23.};
				bool hovered = (actor->root->hovered == actor) && mouse > x - BTN_OFFSET && mouse < x + size.x - 4;

				if(hovered){
					button_shader.uniform.colour = 0xffffffff;
					button_shader.uniform.bg_colour = 0x000000ff;
					button_shader.uniform.fill_colour = 0x003366ff;
					button_shader.uniform.btn_size = size;
					agl_use_program((AGlShader*)&button_shader);
					agl_translate(&button_shader.shader, x - BTN_OFFSET, 0);
					agl_rect (0, 0, size.x, size.y);
					agl_translate(&button_shader.shader, -(x - BTN_OFFSET), 0);

					agl_print(x + width + 4, y, 0, 0xffffffff, "x");
				}
				agl_print_layout(view->filters[i].position = x, y, 0, /*hovered ? 0xff0000ff : */0xffffffff, view->filters[i].layout);

				x += width + 16;
				view->filters[i+1].position = x;

				n_filters++;
			}
		}

		if(!n_filters) agl_print(x, y, 0, 0x777777ff, "No filters");

		return true;
	}

	void filters_init (AGlActor* a)
	{
		FiltersView* view = (FiltersView*)a;

		if (!button_shader.shader.program) {
			agl_create_program(&button_shader.shader);
			button_shader.uniform.radius = 2;
		}

		view->title = pango_layout_new (agl_pango_get_context());
		pango_layout_set_text(view->title, "Filters:", -1);

		view->filters[0].filter = samplecat.model->filters2.search;
		view->filters[1].filter = samplecat.model->filters2.dir;
		view->filters[2].filter = samplecat.model->filters2.category;
	}

	void filters_layout (AGlActor* actor)
	{
	}

	bool filters_event (AGlActor* actor, GdkEvent* event, AGliPt xy)
	{
		FiltersView* view = (FiltersView*)actor;

		int pick (FiltersView* view, int x)
		{
			for(int i=0;i<G_N_ELEMENTS(view->filters)-1;i++){
				iRange r = {view->filters[i].position, view->filters[i + 1].position};
				if(r.start && x > r.start -4 && x < r.end + 16){
					return i;
				}
			}
			return -1;
		}

		switch (event->type) {
			case GDK_BUTTON_PRESS:
			case GDK_BUTTON_RELEASE:
				switch (event->button.button) {
					case 1:;
						int j = pick(view, xy.x);
						dbg(0, "click! pick=%i", j);
						if (j > -1) {
							if(event->type == GDK_BUTTON_RELEASE)
								observable_string_set(view->filters[j].filter, g_strdup(""));
							return AGL_HANDLED;
						}
				}
				break;
			case GDK_MOTION_NOTIFY:
				mouse = xy.x;
				agl_actor__invalidate(actor);
				break;
			case GDK_LEAVE_NOTIFY:
				agl_actor__invalidate(actor);
				break;
			default:
				break;
		}
		return AGL_NOT_HANDLED;
	}

	FiltersView* view = agl_actor__new(FiltersView,
		.actor = {
			.class = &actor_class,
			.init = filters_init,
			.paint = filters_paint,
			.set_size = filters_layout,
			.on_event = filters_event,
		}
	);

	CacheBehaviour* cache = (CacheBehaviour*)((AGlActor*)view)->behaviours[1];

	cache_behaviour_add_dependency(cache, (AGlActor*)view, samplecat.model->filters2.search);
	cache_behaviour_add_dependency(cache, (AGlActor*)view, samplecat.model->filters2.dir);
	cache_behaviour_add_dependency(cache, (AGlActor*)view, samplecat.model->filters2.category);

	return (AGlActor*)view;
}


static void
filters_free (AGlActor* actor)
{
	FiltersView* view = (FiltersView*)actor;

	for (int i=0;i<3;i++) if (view->filters[i].layout) _g_object_unref0(view->filters[i].layout);

	if (!--instance_count) {
	}

	g_free(actor);
}

