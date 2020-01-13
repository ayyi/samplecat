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
#ifndef __views_overlay_h__
#define __views_overly_h__

#include "agl/actor.h"

typedef struct {
   AGlActor    actor;
   AGliRegion  insert_pt;
} OverlayView;

AGlActorClass* overlay_view_get_class ();
AGlActor*      overlay_view           (gpointer);
void           overlay_set_insert_pos (OverlayView*, AGliRegion);

#endif
