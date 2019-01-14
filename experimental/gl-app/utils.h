/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2013-2019 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#ifndef __sc_gl_utils_h__
#define __sc_gl_utils_h__

uint32_t  colour_mix         (uint32_t, uint32_t, float amount);
uint32_t  colour_lighter     (uint32_t, int amount);

#endif
