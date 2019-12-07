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
#ifndef __icon_them_h__
#define __icon_them_h__

extern GList* themes;

GList*      icon_theme_init       ();
void        icon_theme_set_theme  (const char*);

const char* find_icon_theme       (const char* themes[]);

#endif
