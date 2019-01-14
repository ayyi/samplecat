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
#ifndef __views_player_h__
#define __views_player_h__

#include "glib.h"
#include "agl/actor.h"

#define PLAYER_ICON_SIZE 24

AGlActorClass* player_view_get_class ();

AGlActor* player_view (gpointer);

#endif
