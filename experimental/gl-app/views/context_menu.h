/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2019-2019 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#ifndef __views_context_menu_h__
#define __views_context_menu_h__
#include "agl/actor.h"
#include "waveform/promise.h"
#include "../glx.h"

typedef struct {
    int code;
    int modifier;
} Key;

typedef struct {
    char* title;
    char* icon;
    Key key;
    void (*action)(gpointer);
    bool (*show_icon)(gpointer);
    gpointer user_data;
} MenuItem;

typedef struct {
    int len;
    MenuItem items[];
} Menu;

AGlWindow* context_menu_open_new (AGlScene*, AGliPt, Menu*, AMPromise*);
AGlActor*  context_menu          (gpointer);

#endif
