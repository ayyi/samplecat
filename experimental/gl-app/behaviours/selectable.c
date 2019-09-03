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
#include "selectable.h"


Behaviour*
selectable ()
{
	SelectBehaviour* a = g_new0(SelectBehaviour, 1);
	a->observable = observable_new();

	return (Behaviour*)a;
}


void
selectable_init (Behaviour* behaviour, AGlActor* actor)
{
	SelectBehaviour* selectable = (SelectBehaviour*)behaviour;

	observable_subscribe (selectable->observable, selectable->on_select, actor);
}
