/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. https://ayyi.github.io/samplecat/    |
 | copyright (C) 2016-2025 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#pragma once

#include "samplecat/throttle.h"

#ifndef DIR_NODE
#  define DIR_NODE struct { void* a; bool b; void* c; int d }
#endif

typedef struct {
   AGlActor    actor;
   int         selection;
   int         scroll_offset;
   GArray*     nodes;
   DIR_NODE    root;
   Throttle    change;
   struct {
      int      n_rows;
      int      n_rows_visible;
   } cache;
} DirectoriesView;

AGlActorClass* directories_view_get_class ();

AGlActor* directories_view (gpointer);
