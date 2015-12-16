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
#ifndef __views_list_h__
#define __views_list_h__
#include <GL/gl.h>

typedef struct {
   AGlActor    actor;
   int         selection;
   int         scroll_offset;
} ListView;

AGlActor* list_view             (WaveformActor*);
void      list_view_select      (ListView*, int);

#endif
