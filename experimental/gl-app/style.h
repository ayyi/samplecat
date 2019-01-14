/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2007-2019 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#ifndef __style_h__
#define __style_h__

#include "config.h"

typedef struct
{
    uint32_t bg;
    uint32_t bg_alt;
    uint32_t bg_selected;
    uint32_t fg;
    uint32_t text;
    uint32_t selection;
    char*    font;
} Style;

#endif
