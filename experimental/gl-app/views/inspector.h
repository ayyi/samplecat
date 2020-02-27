/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2017-2020 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#ifndef __views_inspector_h__
#define __views_inspector_h__

#define INSPECTOR_RENDER_CACHE
#ifndef AGL_ACTOR_RENDER_CACHE
#undef INSPECTOR_RENDER_CACHE
#endif

typedef struct {
   AGlActor    actor;
   Sample*     sample;
#ifndef INSPECTOR_RENDER_CACHE
   int         scroll_offset;
#endif
   struct {
      int      n_rows;
      int      n_rows_visible;
   } cache;
} InspectorView;

AGlActorClass* inspector_view_get_class ();

AGlActor* inspector_view (gpointer);

#endif
