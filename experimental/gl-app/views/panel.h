/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2016-2019 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#ifndef __views_panel_h__
#define __views_panel_h__

#include "agl/actor.h"
#include "action.h"

typedef struct {
   AGlActor    actor;
   char*       title;
   bool        no_border;
   struct {
      AGliPt   min;
      AGliPt   preferred;
      AGliPt   max;
   }           size_req;
   PangoLayout*layout;
   Actions     actions;
} PanelView;

AGlActorClass* panel_view_get_class ();
AGlActor*      panel_view           (gpointer);

#define PANEL_DRAG_HANDLE_HEIGHT 22

#endif
