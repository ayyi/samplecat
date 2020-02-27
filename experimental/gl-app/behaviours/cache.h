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
#ifndef __cache_behaviour_h__
#define __cache_behaviour_h__

#include "glib.h"
#include "agl/actor.h"
#include "samplecat/observable.h"

#ifdef AGL_ACTOR_RENDER_CACHE

typedef struct {
   AGlBehaviour behaviour;
   GArray*      dependencies;
   AGlActorFn   on_invalidate;
} CacheBehaviour;

AGlBehaviourClass* cache_get_class ();

AGlBehaviour* cache_behaviour                ();
void          cache_behaviour_add_dependency (CacheBehaviour*, AGlActor*, Observable*);

#endif
#endif
