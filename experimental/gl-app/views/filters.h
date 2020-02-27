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
#ifndef __views_filters_h__
#define __views_filters_h__

typedef struct {
   AGlActor     actor;
   PangoLayout* title;
   struct {
      Observable*    filter;
      PangoLayout*   layout;
      int            position;
   }            filters[4];   // last item is for end position only
} FiltersView;

AGlActorClass*  filters_view_get_class ();
AGlActor*       filters_view           (gpointer);

#endif
