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
#include "config.h"
#undef USE_GTK
#include "glib.h"
#include "debug/debug.h"
#include "agl/actor.h"
#include "samplecat/support.h"
#include "state.h"

static void state_init (AGlBehaviour*, AGlActor*);
static void state_free (AGlBehaviour*);

typedef struct
{
    AGlBehaviourClass class;
} StateBehaviourClass;

static StateBehaviourClass klass = {
	.class = {
		.new = state,
		.free = state_free,
		.init = state_init
	}
};


AGlBehaviourClass*
state_get_class ()
{
	return (AGlBehaviourClass*)&klass;
}


AGlBehaviour*
state ()
{
	StateBehaviour* behaviour = AGL_NEW(StateBehaviour,
		.behaviour = {
			.klass = (AGlBehaviourClass*)&klass
		}
	);

	return (AGlBehaviour*)behaviour;
}


static void
state_free (AGlBehaviour* behaviour)
{
	StateBehaviour* state = (StateBehaviour*)behaviour;
	ParamArray* params = state->params;
	if(params){
		g_free0(state->params);
	}
	g_free(state);
}


static void
state_init (AGlBehaviour* behaviour, AGlActor* actor)
{
}


bool
state_set_named_parameter (AGlActor* actor, char* name, char* value)
{
	for(int i = 0; i < AGL_ACTOR_N_BEHAVIOURS; i++){
		AGlBehaviour* behaviour = actor->behaviours[i];
		if(!behaviour)
			break;

		if(behaviour->klass == state_get_class()){
			StateBehaviour* b = (StateBehaviour*)behaviour;
			ParamArray* params = b->params;
			for(int i = 0; i< params->size; i++){
				ConfigParam* param = &params->params[i];
				if(!strcmp(name, param->name)){
					switch(param->utype){
						case G_TYPE_STRING:
							param->set.c(actor, value);
							param->val.c = g_strdup(value);
							return true;
						default:
							break;
					}
				}
			}
		}
	}
	return false;
}

