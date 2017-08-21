/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2017-2017 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#ifndef __views_scrollable_h__
#define __views_scrollable_h__

typedef struct {
   AGlActor    actor;
   int         scroll_offset;
} ScrollableView;

AGlActorClass* scrollable_view_get_class ();
AGlActor*      scrollable_view           (gpointer);

#endif
