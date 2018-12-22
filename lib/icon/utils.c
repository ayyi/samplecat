/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2007-2018 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#include "config.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include "debug/debug.h"

#include "file_manager/mimetype.h" // for mime_type_clear


void
set_icon_theme(const char* name)
{
	mime_type_clear();

	dbg(1, "setting theme: %s.", theme_name);

	if(name && name[0]){
		if(!*theme_name) icon_theme = gtk_icon_theme_new(); // the old icon theme cannot be updated
		//ensure_valid_themes (icon_theme);
		gtk_icon_theme_has_icon (icon_theme, "image-jpg");

		g_strlcpy(theme_name, name, 64);
		gtk_icon_theme_set_custom_theme(icon_theme, theme_name);
#if 0
		gtk_icon_theme_list_icons (icon_theme, NULL);
#endif
	}

#if 0
	if(!strlen(theme_name))
		g_idle_add(check_default_theme, NULL);
#endif

	gtk_icon_theme_rescan_if_needed(icon_theme);
}

