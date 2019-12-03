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
#include "key.h"

static AGlBehaviourClass klass = {
	.new = key_behaviour,
	.init = key_behaviour_init
};


AGlBehaviourClass*
key_get_class ()
{
	return &klass;
}


AGlBehaviour*
key_behaviour ()
{
	KeyBehaviour* a = AGL_NEW(KeyBehaviour,
		.behaviour = {
			.klass = &klass,
		},
	);

	return (AGlBehaviour*)a;
}


void
key_behaviour_init (AGlBehaviour* behaviour, AGlActor* actor)
{
	KeyBehaviour* kb = (KeyBehaviour*)behaviour;

	kb->handlers = g_hash_table_new(g_int_hash, g_int_equal);
	int i = 0; while(true){
		ActorKey* key = &(*kb->keys)[i];
		if(i > 100 || !key->key) break;
		g_hash_table_insert(kb->handlers, &key->key, key->handler);
		i++;
	}
}


bool
key_behaviour_handle_event (AGlBehaviour* behaviour, AGlActor* actor, GdkEvent* event)
{
	KeyBehaviour* kb = (KeyBehaviour*)behaviour;

	GdkEventKey* e = (GdkEventKey*)event;
	int keyval = e->keyval;

	ActorKeyHandler* handler = g_hash_table_lookup(kb->handlers, &keyval);
	return handler
		? handler(actor)
		: AGL_NOT_HANDLED;
}
