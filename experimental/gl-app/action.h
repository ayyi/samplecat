/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2007-2019 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#ifndef __action_h__
#define __action_h__

#include "glib.h"

#if 0
typedef struct {
    int key; // TODO use correct type
    void (*fn) ();
} Action;
#endif

typedef struct {
    GHashTable* actions;
} Actions;

#endif
