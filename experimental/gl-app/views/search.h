/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2015-2016 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#ifndef __views_search_h__
#define __views_search_h__

typedef struct {
   AGlActor     actor;
   int          cursor_pos;
   char*        text;
   PangoLayout* layout;
} SearchView;

AGlActor* search_view        (WaveformActor*);
int       search_view_height (SearchView*);

#endif
