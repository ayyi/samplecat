/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2012-2020 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#define __wf_private__
#include "config.h"
#include <X11/keysym.h>
#include <gdk/gdkkeysyms.h>
#include "debug/debug.h"
#include "agl/utils.h"
#include "agl/fbo.h"
#include "agl/shader.h"
#include "agl/text/pango.h"
#include "samplecat.h"
#include "application.h"
#include "behaviours/cache.h"
#include "views/panel.h"
#include "views/search.h"

#define _g_object_unref0(var) ((var == NULL) ? NULL : (var = (g_object_unref (var), NULL)))

#define PADDING 2
#define BORDER 1
#define MARGIN_BOTTOM 5

static void search_free (AGlActor*);

static int instance_count = 0;
static AGlActorClass actor_class = {0, "Search", (AGlActorNew*)search_view, search_free};

static AGl* agl = NULL;
static char* font = NULL;

static int search_view_height (SearchView*);


AGlActorClass*
search_view_get_class ()
{
	static bool init_done = false;

	if(!init_done){
		agl = agl_get_instance();

		agl_actor_class__add_behaviour(&actor_class, cache_get_class());

		font = g_strdup_printf("%s 10", APP_STYLE.font);

		init_done = true;
	}

	return &actor_class;
}


AGlActor*
search_view (gpointer _)
{
	instance_count++;

	search_view_get_class();

	bool search_paint (AGlActor* actor)
	{
		SearchView* view = (SearchView*)actor;

		agl_enable_stencil(0, 0, actor->region.x2, actor->region.y2);
		if(!agl->use_shaders) agl_enable(AGL_ENABLE_BLEND); // disable textures

		// border
		agl->shaders.plain->uniform.colour = 0x6677ff77;
		agl_use_program((AGlShader*)agl->shaders.plain);
		int h = agl_actor__height(actor) - MARGIN_BOTTOM;
		agl_rect_((AGlRect){0, 0, agl_actor__width(actor), BORDER});
		agl_rect_((AGlRect){agl_actor__width(actor) - BORDER, 0, BORDER, h});
		agl_rect_((AGlRect){0, h, agl_actor__width(actor), BORDER});
		agl_rect_((AGlRect){0, 0, BORDER, h});

		if(actor->root->selected == actor){
			agl->shaders.plain->uniform.colour = 0x777777ff;
			agl_use_program((AGlShader*)agl->shaders.plain);
			agl_rect_((AGlRect){0, 0, agl_actor__width(actor), h});

			// cursor
  			PangoRectangle rect;
			pango_layout_get_cursor_pos(view->layout, view->cursor_pos, &rect, NULL);
			agl->shaders.plain->uniform.colour = 0xffffffff;
			agl_use_program((AGlShader*)agl->shaders.plain);
			agl_rect_((AGlRect){2 + PANGO_PIXELS(rect.x), 2, 1, h - 2});
		}

		agl_print_layout(4, BORDER + PADDING + 2, 0, view->layout_colour, view->layout);

		agl_disable_stencil();

		return true;
	}

	void search_init (AGlActor* a)
	{
		a->fbo = agl_fbo_new(agl_actor__width(a), agl_actor__height(a), 0, AGL_FBO_HAS_STENCIL);
		a->cache.enabled = true;

		PanelView* panel = (PanelView*)a->parent;
		panel->no_border = true;

		// The search panel is unusual in that it can only have one height
		// which is determined by the font size
		panel->size_req.min.y = panel->size_req.max.y = panel->size_req.preferred.y = search_view_height((SearchView*)a);
	}

	void search_layout (SearchView* view)
	{
		if(!view->layout){
			view->layout = pango_layout_new (agl_pango_get_context());
		}
		char* text = view->text ? view->text : samplecat.model->filters2.search->value.c;
		if(strlen(text)){
			view->layout_colour = 0xffffffff;
		}else{
			view->layout_colour = 0x555555ff;
			text = "Search";
		}
		pango_layout_set_text(view->layout, text, -1);

		agl_actor__invalidate((AGlActor*)view);
	}

	bool search_event (AGlActor* actor, GdkEvent* event, AGliPt xy)
	{
		SearchView* view = (SearchView*)actor;

		switch(event->type){
			case GDK_BUTTON_PRESS: {
					int index;
					pango_layout_xy_to_index (view->layout, xy.x * PANGO_SCALE, xy.y * PANGO_SCALE, &index, NULL);
					if(view->cursor_pos != index){
						view->cursor_pos = index;
						agl_actor__invalidate(actor);
					}
				}
				break;
			case GDK_KEY_PRESS:
				g_return_val_if_fail(actor->root->selected, AGL_NOT_HANDLED);
				dbg(0, "Keypress");
				if(!view->text){
					view->text = g_strdup(samplecat.model->filters2.search->value.c);
				}
				int val = ((GdkEventKey*)event)->keyval;
				switch(val){
					case XK_Left:
						view->cursor_pos = MAX(0, view->cursor_pos - 1);
						agl_actor__invalidate(actor);
						break;
					case XK_Right:
						if(view->text){
							view->cursor_pos = MIN(strlen(view->text), view->cursor_pos + 1);
							agl_actor__invalidate(actor);
						}
						break;
					case XK_Delete:
						dbg(0, "Delete");
						if(view->text && view->cursor_pos < strlen(view->text)){
							GString* s = g_string_new(view->text);
							g_string_erase(s, view->cursor_pos, 1);
							g_free(view->text);
							view->text = g_string_free(s, FALSE);

							search_layout(view);
						}
						break;
					case XK_BackSpace:
						dbg(0, "BackSpace");
						if(view->text && view->cursor_pos > 0){
							char a[64]; g_strlcpy(a, view->text, view->cursor_pos);
							char b[64]; g_strlcpy(b, view->text + view->cursor_pos, strlen(view->text) - view->cursor_pos + 2);
							g_free(view->text);
							view->text = g_strdup_printf("%s%s", a, b);
							view->cursor_pos --;

							search_layout(view);
						}
						break;
					case XK_Return:
						dbg(0, "RET");
						observable_set(samplecat.model->filters2.search, (AMVal){.c = view->text});
						break;
					default:
						;char str[2] = {val,};
						if(g_utf8_validate(str, 1, NULL)){
							g_return_val_if_fail(view->cursor_pos <= strlen(view->text), AGL_HANDLED);
							char* a = g_strndup(view->text, view->cursor_pos);
							char* b = g_strndup(view->text + view->cursor_pos, strlen(view->text) - view->cursor_pos);

							view->text = g_strconcat(a, str, b, NULL);

							g_free(a);
							g_free(b);

							view->cursor_pos ++;
							search_layout(view);
						}
						break;
				}
				break;
			default:
				break;
		}
		return AGL_HANDLED;
	}

	SearchView* view = agl_actor__new(SearchView,
		.actor = {
			.class = &actor_class,
			.name = actor_class.name,
			.init = search_init,
			.paint = search_paint,
			.on_event = search_event
		}
	);

	CacheBehaviour* cache = (CacheBehaviour*)((AGlActor*)view)->behaviours[0];
	cache->on_invalidate = (AGlActorFn)search_layout;
	cache_behaviour_add_dependency(cache, (AGlActor*)view, samplecat.model->filters2.search);

	search_layout(view);

	return (AGlActor*)view;
}


static void
search_free (AGlActor* actor)
{
	SearchView* view = (SearchView*)actor;

	_g_object_unref0(view->layout);

	if(!--instance_count){
	}

	g_free(actor);
}


static int
search_view_height (SearchView* view)
{
	g_return_val_if_fail(view->layout, 22);

	PangoRectangle logical_rect;
	pango_layout_get_pixel_extents(view->layout, NULL, &logical_rect);
	return logical_rect.height - logical_rect.y + 2 * PADDING + 2 * BORDER + MARGIN_BOTTOM;
}

