/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2019-2019 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#ifndef __key_behaviour_h__
#define __key_behaviour_h__

#include "glib.h"
#include "agl/actor.h"
#include "agl/behaviour.h"

typedef bool (ActorKeyHandler)(AGlActor*);

typedef struct
{
	int              key;
	ActorKeyHandler* handler;
} ActorKey;

typedef struct {
   AGlBehaviour behaviour;
   ActorKey     (*keys)[];
   GHashTable*  handlers;
} KeyBehaviour;

AGlBehaviourClass* key_get_class ();

AGlBehaviour* key_behaviour              ();
void          key_behaviour_init         (AGlBehaviour*, AGlActor*);
bool          key_behaviour_handle_event (AGlBehaviour*, AGlActor*, GdkEvent*);

#endif
