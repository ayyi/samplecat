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

typedef struct {
   AGlActor    actor;
   GList*      panels; // list of type PanelView*
} DockVView;

AGlActor* dock_v_view                (WaveformActor*);
AGlActor* dock_v_add_panel           (DockVView*, AGlActor*);
void      dock_v_move_panel_to_index (DockVView*, AGlActor*, int);
void      dock_v_move_panel_to_y     (DockVView*, AGlActor*, int);

#endif
