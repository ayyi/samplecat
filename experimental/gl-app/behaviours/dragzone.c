/*
 +----------------------------------------------------------------------+
 | This file is part of the Ayyi project. https://www.ayyi.org          |
 | copyright (C) 2013-2026 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#include "config.h"
#include <stdio.h>
#include <string.h>
#include "debug/debug.h"
#include "agl/utils.h"
#include "agl/event.h"
#include "dragzone.h"

extern Display* dpy;

#define SCROLL_TIMEOUT_MS 50

typedef struct
{
	guint  timer;
	int    speed;
	int    time;  // counts the number of consequtive window scrolls in order to set the speed.
	AGliPt xy;
} ScrollOp;

static ScrollOp scroll = {0, 1};

static void dragzone_behaviour_free  (AGlBehaviour*);
static bool dragzone_behaviour_event (AGlBehaviour*, AGlActor*, AGlEvent*, AGliPt);


static AGlBehaviourClass klass = {
	.event = dragzone_behaviour_event,
	.free = dragzone_behaviour_free
};


AGlBehaviourClass*
dragzone_get_class ()
{
	return &klass;
}


AGlBehaviour*
dragzone_behaviour_init (DragZoneBehaviour* behaviour, DragZoneBehaviourPrototype prototype)
{
	memcpy(behaviour, &prototype, sizeof(typeof(prototype)));
	((AGlBehaviour*)behaviour)->klass = &klass;

	return (AGlBehaviour*)behaviour;
}


void
dragzone_init (DragZoneClass* dzclass, DragZone* dz, DragZone prototype)
{
	void dragzone_abort_change (DragZone* zone)
	{
		*zone->val = zone->orig_val;
	}

	*dz = prototype;

	dzclass->abort = dragzone_abort_change;
	dz->class = dzclass;
}


void
dragzone_box (DragZone* vo, int line_width)
{
	agl_box(line_width, vo->region.x1, vo->region.y1, vo->region.x2 - vo->region.x1, vo->region.y2 - vo->region.y1);
}


static DragZone*
actor_pick_generic (AGlActor* actor, AGliPt xy)
{
	// xy is relative to the top/left of the actor.

	DragZoneBehaviour* behaviour = (DragZoneBehaviour*)agl_actor__find_behaviour(actor, dragzone_get_class());

	bool region_match (AGliRegion* r, int x, int y) // dupe
	{
		return x > r->x1 && x < r->x2 && y >= r->y1 && y < r->y2;
	}

	DragZone* object = NULL;

	for (int i=0;i<behaviour->n_zones;i++) {
		if (region_match(&behaviour->zones[i].region, xy.x, xy.y)) {
			return &behaviour->zones[i];
		}
	}
	return object;
}


#define SCROLL_MASK 5

static void
dragzone_behaviour_free (AGlBehaviour* b)
{
	// behaviour not allocated separately, ensure nothing is freed.
}

static bool
dragzone_behaviour_event (AGlBehaviour* behaviour, AGlActor* actor, AGlEvent* event, AGliPt xy)
{
	DragZoneBehaviour* dzb = (DragZoneBehaviour*)behaviour;

	bool handled = AGL_NOT_HANDLED;
	ActorPick pick = actor_pick_generic;

	static DragZone* zone;
	static int zone_index;
	static AGliPt offset;

	void viewobject_stop_timer (AGlActor* actor)
	{
		if (scroll.timer) {
			scroll = (ScrollOp){.timer = (g_source_remove(scroll.timer), 0), .speed = 1, .time  = 0};
		}
	}

	void viewobject_start_mouse_timer (AGlActor* actor, AGliPt xy)
	{
		bool viewobject_mouse_timeout (gpointer data)
		{
			bool stop = G_SOURCE_REMOVE;

			// Other versions of DragZone provide auto scrolling when going offscreen,
			// but it is not used here.

			return stop;
		}

		if (!scroll.timer) {
			scroll.timer = g_timeout_add(SCROLL_TIMEOUT_MS, (gpointer)viewobject_mouse_timeout, actor);
		}
		scroll.xy = xy;
	}

	switch (event->type){
		case AGL_BUTTON_PRESS:
			if (event->button.button == 1) {
				dbg (1, "BUTTON_PRESS");
				if (zone) pwarn("BUTTON_PRESS: previous object not cleared.");

				if ((zone = pick(actor, xy))) {
					offset = xy;
					offset.x -= zone->region.x1;
					offset.y -= zone->region.y1;
					agl_actor__grab(actor);

					for (zone_index=0;zone_index<dzb->n_zones;zone_index++) {
						if (&dzb->zones[zone_index] == zone) {
							break;
						}
					}
				}
			}
			break;
		case AGL_MOTION_NOTIFY:
			if (zone) {
				*zone->val = zone->class->xy_to_val(zone, zone_index, actor, xy, offset);

				if ((xy.x < SCROLL_MASK) || (xy.x > agl_actor__width(actor) - SCROLL_MASK)) {
					viewobject_start_mouse_timer(actor, xy);
				} else {
					viewobject_stop_timer(actor);
				}

				/* In other versions of DragZone, we have custom behaviour for different types
				switch (zone->model->vals[0].type) {
					case G_TYPE_INT:
						break;
				}
				*/

				agl_actor__invalidate(actor);
				handled = true;
			} else {
				Cursor cursor = None;
				DragZone* obj;
				if ((obj = pick(actor, xy))) {
					cursor = *obj->class->cursor;
#if 0
					if (obj->hover.text) emit("statusbar", obj->hover.text);
#endif
					handled = true;
				}
				XDefineCursor(dpy, actor->root->gl.egl.window, cursor);
			}
			break;
		case AGL_2BUTTON_PRESS:
			if (zone) {
				zone = NULL;
			}
			break;
		case AGL_BUTTON_RELEASE:
			viewobject_stop_timer(actor);
			if (zone) {
				if (true) {
					*zone->val = zone->class->xy_to_val(zone, zone_index, actor, xy, offset);
#if 0
					zone->class->change_done(zone, NULL, &pos, NULL, NULL);
#endif
					if (dzb->model->set) dzb->model->set(actor, dzb->model, zone_index, *zone->val, NULL, NULL);
					agl_actor__invalidate(actor);
				} else {
					abort_change(zone);
				}
				handled = true;
				XDefineCursor(dpy, actor->root->gl.egl.window, None);
			}
			zone = NULL;
			break;
		default:
			break;
	}
	return handled;
}


void
am_object_init (AMObject* object, int n_properties)
{
	object->vals = g_malloc0((n_properties + 1) * sizeof(struct TypedVal));
}


void
am_object_deinit (AMObject* object)
{
	g_free(object->vals);
}
