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
#include "panel.h"

static void panel_behaviour_init (AGlBehaviour*, AGlActor*);

static AGlBehaviourClass klass = {
	.new = panel_behaviour,
	.init = panel_behaviour_init
};


AGlBehaviourClass*
panel_get_class ()
{
	return &klass;
}


AGlBehaviour*
panel_behaviour ()
{
	PanelBehaviour* a = AGL_NEW(PanelBehaviour,
		.behaviour = {
			.klass = &klass,
		},
	);

	return (AGlBehaviour*)a;
}


static void
panel_behaviour_init (AGlBehaviour* behaviour, AGlActor* actor)
{
}
