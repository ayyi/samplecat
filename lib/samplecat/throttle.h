/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. https://ayyi.github.io/samplecat/    |
 | copyright (C) 2025-2025 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#pragma once

#include <glib.h>

typedef struct
{
	void     (*fn)(void*);
	gpointer user_data;
	guint    id;
} Throttle;

void      throttle_queue (Throttle*);
void      throttle_clear (Throttle*);
