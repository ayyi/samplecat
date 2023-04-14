/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
 | copyright (C) 2007-2023 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#include "config.h"
#include <stdio.h>
#include <string.h>
#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixdata.h>
#include <cairo.h>
#include "debug/debug.h"
#include "wf/waveform.h"
#include "waveform/pixbuf.h"
#include "decoder/ad.h"
#include "samplecat/support.h"
#include "samplecat/sample.h"

#define OVERVIEW_WIDTH (200)
#define OVERVIEW_HEIGHT (20)

static struct {
	uint32_t bg;
	uint32_t base;
	uint32_t text;
} colour = {
	0,
	0,
	0xddddddff,
};


#if 0
static void
pixbuf_clear(GdkPixbuf* pixbuf, GdkColor* colour)
{
	guint32 colour_rgba = ((colour->red/256)<< 24) | ((colour->green/256)<<16) | ((colour->blue/256)<<8) | (0x60);
	gdk_pixbuf_fill(pixbuf, colour_rgba);
}
#endif


void
set_overview_colour (uint32_t text_colour, uint32_t base_colour, uint32_t bg_colour)
{
	colour.bg = bg_colour;
	colour.base = base_colour;
	colour.text = text_colour;
}


GdkPixbuf*
make_overview (Sample* sample)
{
	WfDecoder d = {{0,}};
	if (!ad_open(&d, sample->full_path)) return NULL;
	ad_close (&d);

	dbg(1, "NEW OVERVIEW");

	GdkPixbuf* pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, HAS_ALPHA_FALSE, BITS_PER_CHAR_8, OVERVIEW_WIDTH, OVERVIEW_HEIGHT);
	Waveform* waveform = waveform_load_new (sample->full_path);
	waveform_peak_to_pixbuf (waveform, pixbuf, NULL, colour.text, colour.base, true);
	g_object_unref (waveform);

	return sample->overview = pixbuf;
}
