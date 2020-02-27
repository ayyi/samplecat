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
#ifndef __views_scrollbar_h__
#define __views_scrollbar_h__
#include "agl/actor.h"
#include "agl/observable.h"

typedef struct {
    AGlActor       actor;
    AGlOrientation orientation;
    AGlObservable* scroll;
    struct {
      float        opacity;
      WfAnimatable animation;
    }              handle;
    struct {
      float        opacity;
      WfAnimatable animation;
    }              trough;
} ScrollbarActor;


AGlActorClass* scrollbar_view_get_class ();

AGlActor* scrollbar_view (AGlActor*, AGlOrientation);

#endif
