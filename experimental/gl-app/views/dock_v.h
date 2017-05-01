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
#ifndef __views_dock_v_h__
#define __views_dock_v_h__
#include "panel.h"

typedef struct {
   PanelView     panel;  // dock children must be PanelViews. DockVView inherits from PanelView so it can be a DockHView child.
   GList*        panels; // list of type PanelView*
   struct {
      AGlActor*  actor;
      float      opacity;
   } handle;
   WfAnimatable* animatables[1];
} DockVView;

AGlActor* dock_v_view                (WaveformActor*);
AGlActor* dock_v_add_panel           (DockVView*, AGlActor*);
void      dock_v_move_panel_to_index (DockVView*, AGlActor*, int);
void      dock_v_move_panel_to_y     (DockVView*, AGlActor*, int);

#endif
