/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. https://ayyi.github.io/samplecat/    |
 | copyright (C) 2007-2024 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#pragma once

#include <X11/keysym.h>

typedef void (KeyHandler)(void);

typedef struct
{
	int         key;
	KeyHandler* handler;
} KeyItem;
typedef KeyItem Key;

typedef struct
{
	guint          timer;
	KeyHandler*    handler;
} KeyHold;
