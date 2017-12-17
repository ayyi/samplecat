/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2016-2017 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#ifndef __materials_ring_h__
#define __materials_ring_h__

#include "agl/utils.h"

typedef struct {
    AGlMaterial  material;

    char         chr;
    uint32_t     colour;
    uint32_t     bg;

    PangoLayout* layout;
} IconMaterial;

AGlMaterial* ring_new ();

#endif
