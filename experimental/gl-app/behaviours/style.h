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
#ifndef __behvr_style_h__
#define __behvr_style_h__

#include "agl/behaviour.h"

typedef struct {
    AGlBehaviour behaviour;

    uint32_t     bg;
    uint32_t     bg_alt;
    uint32_t     bg_selected;
    uint32_t     fg;
    uint32_t     text;
    uint32_t     selection;
    char*        font;
} StyleBehaviour;

AGlBehaviourClass* style_get_class   ();
AGlBehaviour*      style             ();

#define STYLE (*(StyleBehaviour*)((AGlActor*)actor->root)->behaviours[0])

#endif
