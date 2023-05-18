/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. https://ayyi.github.io/samplecat/    |
 | copyright (C) 2007-2023 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#pragma once

#include <glib.h>
#include "typedefs.h"

#define PALETTE_SIZE 17

struct _palette {
	guint red[8];
	guint grn[8];
	guint blu[8];
};

extern GType AYYI_TYPE_COLOUR;

void types_init ();
