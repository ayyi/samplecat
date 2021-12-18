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
#include "config.h"
#include "debug/debug.h"
#include "graph_debug.h"

extern bool agl_actor__is_onscreen (AGlActor*);
extern bool agl_actor__is_cached   (AGlActor*);

static AGlScene* target = NULL;
AGlWindow* window = NULL;

static bool graph_debug_paint (AGlActor*);


static gboolean
on_timeout (gpointer _)
{
	agl_actor__invalidate((AGlActor*)window->scene);

	return G_SOURCE_CONTINUE;
}


AGlWindow*
graph_debug_window (AGlScene* _scene)
{
	target = _scene;

	AGliPt size = {320, 120};
	window = agl_window("Graph debug", 0, 0, size.x, size.y, false);

	agl_actor__add_child(
		(AGlActor*)window->scene,
		agl_actor__new(
			AGlActor,
			.paint = graph_debug_paint,
			.region = {0, 0, 320, 120}
		)
	);

	g_timeout_add(5000, on_timeout, NULL);

	return window;
}


static bool
graph_debug_paint (AGlActor* actor)
{
	static int indent; indent = 1;

	void _print (AGlActor* actor, int* y)
	{
		g_return_if_fail(actor);

		bool is_onscreen = agl_actor__is_onscreen(actor);
		char* offscreen = is_onscreen ? "" : " OFFSCREEN";
		char* zero_size = agl_actor__width(actor) ? "" : " ZEROSIZE";
#ifdef AGL_ACTOR_RENDER_CACHE
		char* negative_size = (agl_actor__width(actor) < 1 || agl_actor__height(actor) < 1) ? " NEGATIVESIZE" : "";
		char* disabled = agl_actor__is_disabled(actor) ?  " DISABLED" :  "";
#endif

		char scrollablex[32] = {0};
		if(actor->scrollable.x1 || actor->scrollable.x2)
			sprintf(scrollablex, " scrollable.x(%i,%i)", actor->scrollable.x1, actor->scrollable.x2);
		char scrollabley[32] = {0};
		if(actor->scrollable.y1 || actor->scrollable.y2)
			sprintf(scrollabley, " scrollable.y(%i,%i)", actor->scrollable.y1, actor->scrollable.y2);

#ifdef AGL_ACTOR_RENDER_CACHE
		uint32_t colour = !agl_actor__width(actor) || !is_onscreen
			? 0x555555ff
			: agl_actor__is_cached(actor)
				? 0x999999ff
				: 0xffffffff;
		AGliPt offset = agl_actor__find_offset(actor);
		if(actor->name) agl_print_with_cursor(indent * 20, y, 0, colour, "%s:%s%s%s%s cache(%i,%i) region(%.0f,%.0f,%.0f,%.0f) offset(%i,%i)%s%s", actor->name, offscreen, zero_size, negative_size, disabled, actor->cache.enabled, actor->cache.valid, actor->region.x1, actor->region.y1, actor->region.x2, actor->region.y2, offset.x, offset.y, scrollablex, scrollabley);
#else
		if(actor->name) agl_printf_with_cursor(indent * 20, y, 0, 0xffffffff, "%s", actor->name);
#endif
		if(!actor->name) agl_print_with_cursor(indent * 20, y, 0, 0xffffffff, "%s%s (%.0f,%.0f)\n", offscreen, zero_size, actor->region.x1, actor->region.y1);
		indent++;
		for(GList* l=actor->children;l;l=l->next){
			AGlActor* child = l->data;
			_print(child, y);
		}
		indent--;
	}

	int y = 0;
	_print((AGlActor*)target, &y);

	return true;
}
