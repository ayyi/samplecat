/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. https://ayyi.github.io/samplecat/    |
 | copyright (C) 2016-2026 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#pragma once

#include "agl/actor.h"
#include "agl/behaviours/sizeable.h"
#include "action.h"

typedef struct {
   AGlActor    actor;
   char*       title;
   bool        no_border;
   SizeRequest size_req;
   PangoLayout*layout;
   Actions     actions;
} PanelView;

AGlActorClass* panel_view_get_class ();
AGlActor*      panel_view           (gpointer);

#define PANEL_DRAG_HANDLE_HEIGHT 22
