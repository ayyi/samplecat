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
#ifndef __views_tabs_h__
#define __views_tabs_h__
#include <GL/gl.h>

typedef struct {
   const char* name;
   AGlActor*   actor;
} TabsViewTab;

typedef struct {
   AGlActor    actor;
   GList*      tabs;   // type TabsViewTab*
   int         active;
} TabsView;

AGlActor* tabs_view          (WaveformActor*);
void      tabs_view__add_tab (TabsView*, const char*, AGlActor*);

#endif
