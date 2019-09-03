/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2016-2019 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#ifndef __selectable_h__
#define __selectable_h__

#include "glib.h"
#include "utils/observable.h"
#include "agl/actor.h"

typedef struct _Behaviour Behaviour;

typedef void (*BehaviourInit) (Behaviour*, AGlActor*);

struct _Behaviour {
   BehaviourInit init;
};

typedef struct {
   Behaviour   behaviour;
   Observable* observable;
   ObservableFn on_select;
} SelectBehaviour;

Behaviour* selectable      ();
void       selectable_init (Behaviour*, AGlActor*);

#endif
