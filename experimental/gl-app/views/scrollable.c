/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2017-2020 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
/*
 * Previously the scrollbar actor was a child actor but this doesnt work for
 * caching which requires that the scrollbar is a sibling or parent.
 * The scrollbar drawing could be done by the this object but it would
 * require better z-order draw ordering from the scene graph (not just siblings)
 */
#define __wf_private__
#include "config.h"
#undef USE_GTK
#include "debug/debug.h"
#include "agl/ext.h"
#include "agl/utils.h"
#include "agl/actor.h"
#include "samplecat/typedefs.h"
#include "shader.h"
#include "views/scrollbar.h"
#include "views/scrollable.h"

#define R 4
#define V_SCROLLBAR_H_PADDING 2
#define V_SCROLLBAR_V_PADDING 3
#define H_SCROLLBAR_H_PADDING 3
#define H_SCROLLBAR_V_PADDING 2

#define agl_actor__scrollable_height(A) (A->scrollable.y2 - A->scrollable.y1)
#define max_scroll_offset (agl_actor__scrollable_height(((AGlActor*)actor->children->data)) - agl_actor__height(actor))

static AGl* agl = NULL;
static AGlActorClass actor_class = {0, "Scrollable", (AGlActorNew*)scrollable_view};
static int instance_count = 0;

static void scrollable_set_scroll_position (AGlActor*, int);


AGlActorClass*
scrollable_view_get_class ()
{
	return &actor_class;
}


static void
_init()
{
	static bool init_done = false;

	if(!init_done){
		agl = agl_get_instance();
		init_done = true;
	}
}


AGlActor*
scrollable_view(gpointer _)
{
	instance_count++;

	_init();

	void scrollable_init(AGlActor* actor)
	{
		agl_actor__add_child(actor, scrollbar_view(NULL, AGL_ORIENTATION_VERTICAL));
	}

	void scrollable_set_size(AGlActor* actor)
	{
		// first scrollable view child takes up all available space

		AGlActor* child = actor->children->data;
		child->region = (AGlfRegion){0, 0, agl_actor__width(actor), agl_actor__height(actor)};
		agl_actor__set_size(child);

		scrollable_set_scroll_position(actor, -1);
	}

	bool scrollable_event(AGlActor* actor, GdkEvent* event, AGliPt xy)
	{
		ScrollableView* view = (ScrollableView*)actor;

		switch(event->type){
			case GDK_BUTTON_PRESS:
				switch(event->button.button){
					case 4:
						scrollable_set_scroll_position(actor, view->scroll_offset - 10);
						agl_actor__invalidate(actor);
						break;
					case 5:
						if(agl_actor__scrollable_height(((AGlActor*)actor->children->data)) > agl_actor__height(actor)){
							scrollable_set_scroll_position(actor, view->scroll_offset + 10);
							agl_actor__invalidate(actor);
						}
						break;
				}
				break;
			default:
				break;
		}
		return AGL_HANDLED;
	}

	ScrollableView* view = AGL_NEW(ScrollableView,
		.actor = {
			.class = &actor_class,
			.name = actor_class.name,
			.init = scrollable_init,
			.paint = agl_actor__null_painter,
			.set_size = scrollable_set_size,
			.on_event = scrollable_event,
		}
	);

	return (AGlActor*)view;
}


/*
 *  Use -1 to preserve existing offset
 */
static void
scrollable_set_scroll_position (AGlActor* actor, int scroll_offset)
{
	ScrollableView* view = (ScrollableView*)actor;
	AGlActor* child = actor->children->data;

	scroll_offset = MAX(0, scroll_offset < 0 ? view->scroll_offset : scroll_offset); // TODO why doesnt CLAMP do this?

	g_return_if_fail(max_scroll_offset > -1);
	view->scroll_offset = CLAMP(scroll_offset, 0, max_scroll_offset);
	int h = agl_actor__scrollable_height(child);
#ifdef AGL_ACTOR_RENDER_CACHE
	actor->scrollable.y1 = child->scrollable.y1 = child->cache.offset.y = - view->scroll_offset;
#else
	actor->scrollable.y1 = child->scrollable.y1 = - view->scroll_offset;
#endif
	actor->scrollable.y2 = child->scrollable.y2 = actor->scrollable.y1 + h;
}

