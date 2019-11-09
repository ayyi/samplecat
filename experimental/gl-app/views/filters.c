/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2017-2017 Tim Orford <tim@orford.org>                  |
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
#include <X11/keysym.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <GL/gl.h>
#include "debug/debug.h"
#include "agl/actor.h"
#include "agl/fbo.h"
#include "agl/pango_render.h"
#include "samplecat.h"
#include "shader.h"
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
filters_view(gpointer _)
{
	instance_count++;

	_init();

	bool filters_paint(AGlActor* actor)
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
			SamplecatFilter* filter = view->filters[i].filter;
			char* val = filter->value;
			if(val && strlen(val)){
				if(!view->filters[i].layout){
					PangoGlRendererClass* PGRC = g_type_class_peek(PANGO_TYPE_GL_RENDERER);
					view->filters[i].layout = pango_layout_new (PGRC->context);
				}
				char* text = strcmp(filter->name, "search")
					? g_strdup_printf("%s: %s", filter->name, val)
					: g_strdup_printf("\"%s\"", val);
				pango_layout_set_text(view->filters[i].layout, text, -1);
				g_free(text);

				PangoRectangle logical_rect;
				pango_layout_get_pixel_extents(view->filters[i].layout, NULL, &logical_rect);
				int width = logical_rect.width;

				#define BTN_OFFSET 6
				AGliPt size = (AGliPt){width + 22, 23};
				bool hovered = (actor->root->hovered == actor) && mouse > x - BTN_OFFSET && mouse < x + size.x - 4;

				if(hovered){
					button_shader.uniform.colour = 0xffffffff;
					button_shader.uniform.bg_colour = 0x000000ff;
					button_shader.uniform.fill_colour = 0x003366ff;
					button_shader.uniform.btn_size = size;
					agl_use_program((AGlShader*)&button_shader);
					glTranslatef(x - BTN_OFFSET, 0, 0);
					agl_irect(0, 0, size.x, size.y);
					glTranslatef(-(x - BTN_OFFSET), 0, 0);

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

	void filters_init(AGlActor* a)
	{
		FiltersView* view = (FiltersView*)a;

		if(!button_shader.shader.program){
			agl_create_program(&button_shader.shader);
			button_shader.uniform.radius = 2;
		}

#ifdef AGL_ACTOR_RENDER_CACHE
		a->fbo = agl_fbo_new(agl_actor__width(a), agl_actor__height(a), 0, AGL_FBO_HAS_STENCIL);
		a->cache.enabled = true;
#endif

		PangoGlRendererClass* PGRC = g_type_class_peek(PANGO_TYPE_GL_RENDERER);
		view->title = pango_layout_new (PGRC->context);
		pango_layout_set_text(view->title, "Filters:", -1);

		view->filters[0].filter = samplecat.model->filters.search;
		view->filters[1].filter = samplecat.model->filters.dir;
		view->filters[2].filter = samplecat.model->filters.category;
	}

	void filters_size(AGlActor* actor)
	{
	}

	bool filters_event(AGlActor* actor, GdkEvent* event, AGliPt xy)
	{
		FiltersView* view = (FiltersView*)actor;

		int pick(FiltersView* view, int x)
		{
			int i; for(i=0;i<G_N_ELEMENTS(view->filters)-1;i++){
				iRange r = {view->filters[i].position, view->filters[i + 1].position};
				if(r.start && x > r.start -4 && x < r.end + 16){
					return i;
				}
			}
			return -1;
		}

		switch(event->type){
			case GDK_BUTTON_PRESS:
			case GDK_BUTTON_RELEASE:
				switch(event->button.button){
					case 1:;
						int j = pick(view, xy.x);
						dbg(0, "click! pick=%i", j);
						if(j > -1){
							if(event->type == GDK_BUTTON_RELEASE) samplecat_filter_set_value(view->filters[j].filter, "");
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

	FiltersView* view = AGL_NEW(FiltersView,
		.actor = {
			.class = &actor_class,
			.name = "Search",
			.init = filters_init,
			.paint = filters_paint,
			.set_size = filters_size,
			.on_event = filters_event,
		}
	);

	void filters_on_filter_changed(GObject* _filter, gpointer _actor)
	{
		PF;
		agl_actor__invalidate((AGlActor*)_actor);
	}
	g_signal_connect(samplecat.model->filters.search, "changed", G_CALLBACK(filters_on_filter_changed), view);
	g_signal_connect(samplecat.model->filters.dir, "changed", G_CALLBACK(filters_on_filter_changed), view);
	g_signal_connect(samplecat.model->filters.category, "changed", G_CALLBACK(filters_on_filter_changed), view);

	return (AGlActor*)view;
}


static void
filters_free (AGlActor* actor)
{
	FiltersView* view = (FiltersView*)actor;

	int i; for(i=0;i<3;i++) if(view->filters[i].layout) _g_object_unref0(view->filters[i].layout);

	if(!--instance_count){
	}

	g_free(actor);
}

