/*
 +----------------------------------------------------------------------+
 | This file is part of the Ayyi project. http://ayyi.org               |
 | copyright (C) 2011-2023 Tim Orford <tim@orford.org>                  |
 | copyright (C) 2006, Thomas Leonard and others                        |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#pragma once

#undef USE_MINIBUFFER

void minibuffer_init   (void);
void create_minibuffer (AyyiFilemanager*);
void minibuffer_show   (AyyiFilemanager*, MiniType mini_type);
void minibuffer_hide   (AyyiFilemanager*);
void minibuffer_add    (AyyiFilemanager*, const gchar* leafname);
