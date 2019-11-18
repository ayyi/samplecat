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
#ifndef __icon_utils_h__
#define __icon_utils_h__

#include "file_manager/typedefs.h"

const char* find_icon_theme (const char* themes[]);
void        set_icon_theme  (const char* name);

guint       get_icon_texture_by_mimetype (MIME_type*);
guint       get_icon_texture_by_name (const char*, int size);

#endif
