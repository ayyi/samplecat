/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. https://ayyi.github.io/samplecat/    |
 | copyright (C) 2019-2024 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#pragma once

#include "agl/behaviour.h"

typedef struct {
    AGlBehaviour behaviour;

    uint32_t     bg;
    uint32_t     bg_alt;
    uint32_t     bg_selected;
    uint32_t     fg;
    uint32_t     text;
    uint32_t     selection;
    const char*  family;
    const char*  font;
} StyleBehaviour;

AGlBehaviourClass* style_get_class   ();
AGlBehaviour*      style             ();

#define STYLE (*(StyleBehaviour*)((AGlActor*)actor->root)->behaviours[0])

#if 1
#define FONT_SIZE 10
#define LINE_HEIGHT 20
#else
#define FONT_SIZE 15
#define LINE_HEIGHT 28
#endif
