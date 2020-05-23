/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2020-2020 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#include "agl/debug.h"
#include "cache.h"

#ifdef AGL_ACTOR_RENDER_CACHE

typedef struct {
   CacheBehaviour* behaviour;
   AGlActor*       actor;
} BH;

static void cache_behaviour_init (AGlBehaviour*, AGlActor*);
static void cache_behaviour_free (AGlBehaviour*);

static AGlBehaviourClass klass = {
	.new = cache_behaviour,
	.free = cache_behaviour_free,
	.init = cache_behaviour_init
};


AGlBehaviourClass*
cache_get_class ()
{
	return &klass;
}


AGlBehaviour*
cache_behaviour ()
{
	return (AGlBehaviour*)AGL_NEW(CacheBehaviour,
		.behaviour = {
			.klass = &klass,
		},
		.dependencies = g_array_sized_new(FALSE, FALSE, sizeof(BH), 4)
	);
}


static void
cache_behaviour_free (AGlBehaviour* behaviour)
{
	g_array_free(((CacheBehaviour*)behaviour)->dependencies, true);
	g_free(behaviour);
}


static void
cache_behaviour_init (AGlBehaviour* behaviour, AGlActor* actor)
{
	actor->fbo = agl_fbo_new(agl_actor__width(actor), agl_actor__height(actor), 0, AGL_FBO_HAS_STENCIL);
	actor->cache.enabled = true;
}


void
cache_behaviour_add_dependency (CacheBehaviour* behaviour, AGlActor* actor, Observable* dependency)
{
	g_array_append_val(behaviour->dependencies, ((BH){
		.behaviour = behaviour,
		.actor = actor
	}));

	void on_event (Observable* observable, AMVal value, gpointer _data)
	{
		BH* bh = _data;

		if(bh->behaviour->on_invalidate)
			bh->behaviour->on_invalidate(bh->actor);

		agl_actor__invalidate(bh->actor);
	}

	// warning the GArray needs to be presized otherwise the BH address will change
	observable_subscribe_with_state(
		dependency,
		on_event,
		&g_array_index(behaviour->dependencies, BH, behaviour->dependencies->len - 1)
	);
}

#endif
