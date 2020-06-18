/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://samplecat.orford.org          |
* | copyright (C) 2007-2020 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/

#ifndef __samplecat_colorbox_h__
#define __samplecat_colorbox_h__

#include <gtk/gtk.h>

void       colour_box_init      ();
GtkWidget* colour_box_new       (GtkWidget* parent);
gboolean   colour_box_add       (GdkColor*);
void       colour_box_colourise ();

#endif
