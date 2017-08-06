/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2016-2017 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#ifndef __views_panel_h__
#define __views_panel_h__

typedef struct {
   AGlActor    actor;
   struct {
      AGliPt   min;
      AGliPt   preferred;
      AGliPt   max;
   }           size_req;
} PanelView;

AGlActorClass* panel_view_get_class ();
AGlActor*      panel_view           (WaveformActor*);

#define PANEL_DRAG_HANDLE_HEIGHT 8

#endif
