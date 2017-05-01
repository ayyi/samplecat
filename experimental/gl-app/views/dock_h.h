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
#ifndef __views_dock_h_h__
#define __views_dock_h_h__
#include "panel.h"

typedef struct {
   PanelView     panel;  // dock children must be PanelViews. DockVHiew inherits from PanelView so it can be a DockVView child.
   GList*        panels; // list of type PanelView*
   struct {
      AGlActor*  actor;
      float      opacity;
   } handle;
   WfAnimatable* animatables[1];
} DockHView;

AGlActor* dock_h_view                (WaveformActor*);
AGlActor* dock_h_add_panel           (DockHView*, AGlActor*);
void      dock_h_move_panel_to_index (DockHView*, AGlActor*, int);
void      dock_h_move_panel_to_y     (DockHView*, AGlActor*, int);

#endif
