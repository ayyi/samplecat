/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2013-2021 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#include "config.h"
#undef USE_GTK
#include "stdint.h"
#include "glib.h"
#include "utils.h"


uint32_t
colour_mix (uint32_t a, uint32_t b, float _amount)
{
	int amount = _amount * 0xff;
	int red0 = a                >> 24;
	int grn0 = (a & 0x00ff0000) >> 16;
	int blu0 = (a & 0x0000ff00) >>  8;
	int   a0 = (a & 0x000000ff);
	int red1 = b                >> 24;
	int grn1 = (b & 0x00ff0000) >> 16;
	int blu1 = (b & 0x0000ff00) >>  8;
	int   a1 = (b & 0x000000ff);
	return
		(((red0 * (0xff - amount) + red1 * amount) / 256) << 24) +
		(((grn0 * (0xff - amount) + grn1 * amount) / 256) << 16) +
		(((blu0 * (0xff - amount) + blu1 * amount) / 256) <<  8) +
		(a0 * (0xff - amount) + a1 * amount) / 256;
}


uint32_t
colour_lighter (uint32_t colour, int amount)
{
	// return a colour slightly lighter than the one given

	int fixed = 0x2 * amount; // 0x10 is just visible on black with amount=2
	float multiplier = 1.0;// + amount / 60.0;

	int red   = MIN(((colour & 0xff000000) >> 24) * multiplier + fixed, 0xff);
	/*
	printf("%s(): red: %x %x %x\n", __func__, (colour & 0xff000000) >> 24,
                     (int)(((colour & 0xff000000) >> 24) * 1.2 + 0x600),
					 red
					 );
	*/
	int green = MIN(((colour & 0x00ff0000) >> 16) * multiplier + fixed, 0xff);
	int blue  = MIN(((colour & 0x0000ff00) >>  8) * multiplier + fixed, 0xff);
	int alpha = colour & 0x000000ff;

	//dbg(0, "%08x --> %08x", colour, red << 24 | green << 16 | blue << 8 | alpha);
	return red << 24 | green << 16 | blue << 8 | alpha;
}


