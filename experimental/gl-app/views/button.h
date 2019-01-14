/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2013-2019 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#ifndef __views_button_h__
#define __views_button_h__

#include "agl/actor.h"

typedef void      (*ButtonAction)   (AGlActor*, gpointer);
typedef bool      (*ButtonGetState) (AGlActor*, gpointer);

typedef struct {
    AGlActor       actor;
    guint*         icon;
    ButtonAction   action;
    ButtonGetState get_state;
    gboolean       disabled;
    float          bg_opacity;
    float          ripple_radius;
    WfAnimatable*  animatables[2];
    gpointer       user_data;
} ButtonActor;

AGlActorClass* button_get_class ();
AGlActor*      button           (int* icon, ButtonAction, ButtonGetState, gpointer);

#endif
