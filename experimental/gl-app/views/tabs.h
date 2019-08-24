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
#ifndef __views_tabs_h__
#define __views_tabs_h__
#include "agl/actor.h"

typedef struct {
   const char* name;
   AGlActor*   actor;
} TabsViewTab;

typedef struct {
   AGlActor    actor;
   GList*      tabs;   // type TabsViewTab*
   int         active;
   struct {
      int          tab;
      float        opacity;
      WfAnimatable animatable;
   }            hover;

   float       _x;
   WfAnimatable x;     // position of active tab content
} TabsView;

AGlActorClass* tabs_view_get_class ();

AGlActor* tabs_view          (gpointer);
void      tabs_view__add_tab (TabsView*, const char*, AGlActor*);

#endif
