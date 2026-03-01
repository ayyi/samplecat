/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. https://ayyi.github.io/samplecat/    |
 | copyright (C) 2007-2026 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#include "config.h"
#include <glib.h>
#include "samplecat/application.h"
#include "palette.h"

extern SamplecatApplication* app;

gboolean
palette_rgba_from_index (gint idx, GdkRGBA* rgba)
{
	if (!rgba) return FALSE;
	if (idx < 0 || idx >= PALETTE_SIZE) return FALSE;
	const char *hex = app->config.colour[idx];
	if (!hex || !strlen (hex)) return FALSE;
	char buf[16];
	g_snprintf (buf, sizeof buf, "#%s", hex);
	return gdk_rgba_parse (rgba, buf);
}

