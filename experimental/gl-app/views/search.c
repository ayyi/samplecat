/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2012-2021 Tim Orford <tim@orford.org>                  |
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
#include "agl/behaviours/key.h"
#include "agl/text/text_input.h"
#include "samplecat.h"
#include "application.h"
#include "behaviours/cache.h"
#include "behaviours/state.h"
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

static ActorKeyHandler search_enter;

static ActorKey keys[] = {
	{XK_Return,   search_enter},
	{XK_KP_Enter, search_enter},
	{0,}
};

#define KEYS(A) ((KeyBehaviour*)(A)->behaviours[2])


AGlActorClass*
search_view_get_class ()
{
	static bool init_done = false;

	if(!init_done){
		agl = agl_get_instance();

		agl_actor_class__add_behaviour(&actor_class, cache_get_class());
		agl_actor_class__add_behaviour(&actor_class, state_get_class());
		agl_actor_class__add_behaviour(&actor_class, key_get_class());

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
		agl_enable_stencil(0, 0, actor->region.x2, actor->region.y2);
		if(!agl->use_shaders) agl_enable(AGL_ENABLE_BLEND); // disable textures

		// border
		PLAIN_COLOUR2 (agl->shaders.plain) = 0x6677ff77;
		agl_use_program (agl->shaders.plain);
		int h = agl_actor__height(actor) - MARGIN_BOTTOM;
		agl_rect_((AGlRect){0, 0, agl_actor__width(actor), BORDER});
		agl_rect_((AGlRect){agl_actor__width(actor) - BORDER, 0, BORDER, h});
		agl_rect_((AGlRect){0, h, agl_actor__width(actor), BORDER});
		agl_rect_((AGlRect){0, 0, BORDER, h});

		if(actor->root->selected == actor){
			PLAIN_COLOUR2 (agl->shaders.plain) = 0x777777ff;
			agl_use_program((AGlShader*)agl->shaders.plain);
			agl_rect_((AGlRect){0, 0, agl_actor__width(actor), h});
		}

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

	void search_size (AGlActor* actor)
	{
		((AGlActor*)actor->children->data)->region = (AGlfRegion){4, BORDER + PADDING - 2, agl_actor__width(actor) - 8, 18};
	}

	void search_layout (SearchView* view)
	{
		AGlActor* actor = (AGlActor*)view;

		text_input_set_text((TextInput*)actor->children->data, samplecat.model->filters2.search->value.c);

		agl_actor__invalidate(actor);
	}

	SearchView* view = agl_actor__new(SearchView,
		.actor = {
			.class = &actor_class,
			.name = actor_class.name,
			.init = search_init,
			.paint = search_paint,
			.set_size = search_size,
		}
	);

	AGlActor* input = agl_actor__add_child((AGlActor*)view, text_input(NULL));
	text_input_set_placeholder((TextInput*)input, "Search");
	agl_observable_set(((TextInput*)input)->font, 10);

	CacheBehaviour* cache = (CacheBehaviour*)((AGlActor*)view)->behaviours[0];
	cache->on_invalidate = (AGlActorFn)search_layout;
	cache_behaviour_add_dependency(cache, (AGlActor*)view, samplecat.model->filters2.search);

	CacheBehaviour* state = (CacheBehaviour*)((AGlActor*)view)->behaviours[1];
	((StateBehaviour*)state)->is_container = false;

	KEYS((AGlActor*)view)->keys = &keys;

	search_layout(view);

	return (AGlActor*)view;
}


static void
search_free (AGlActor* actor)
{
	if(!--instance_count){
	}

	g_free(actor);
}


static int
search_view_height (SearchView* view)
{
	return text_input_get_height((TextInput*)((AGlActor*)view)->children->data) + 2 * PADDING + 2 * BORDER + MARGIN_BOTTOM;
}


static bool
search_enter (AGlActor* actor, GdkModifierType modifiers)
{
	const gchar* text = text_input_get_text((TextInput*)actor->children->data);

	observable_set(samplecat.model->filters2.search, (AMVal){.c = (char*)text});

	return AGL_HANDLED;
}

