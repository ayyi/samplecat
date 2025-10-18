/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. https://samplecat.orford.org         |
 | copyright (C) 2025-2025 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#include "throttle.h"


void
throttle_queue (Throttle* throttle)
{
	g_return_if_fail (throttle);

	gboolean throttle_run (gpointer self)
	{
		Throttle* throttle = self;

		throttle->fn (throttle->user_data);
		throttle->id = 0;

		return G_SOURCE_REMOVE;
	}

	if (!throttle->id) {
		throttle->id = g_idle_add(throttle_run, throttle);
	}
}


void
throttle_clear (Throttle* throttle)
{
	if (throttle->id) {
		g_source_remove(throttle->id);
		throttle->id = 0;
	}
}
