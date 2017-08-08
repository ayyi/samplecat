/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2007-2017 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#ifndef __analysis_waveform_h__
#define __analysis_waveform_h__
#include <gtk/gtk.h>
#include "samplecat/typedefs.h"

void       set_overview_colour (GdkColor*, GdkColor*, GdkColor*);
GdkPixbuf* make_overview       (Sample*);

#endif
