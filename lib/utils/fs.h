/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2020-2020 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/

#ifndef ___utils_fs_h__
#define ___utils_fs_h__

#include <stdbool.h>

void with_dir (const char* path, GError**, bool (*fn)(GDir*, const char*, gpointer), gpointer);

#endif
