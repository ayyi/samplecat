/**
* +----------------------------------------------------------------------+
* | This file is part of the Ayyi project. http://ayyi.org               |
* | copyright (C) 2011-2017 Tim Orford <tim@orford.org>                  |
* | copyright (C) 2006, Thomas Leonard and others                        |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#ifndef __minibuffer_h__
#define __minibuffer_h__

void minibuffer_init   (void);
void create_minibuffer (AyyiLibfilemanager*);
void minibuffer_show   (AyyiLibfilemanager*, MiniType mini_type);
void minibuffer_hide   (AyyiLibfilemanager*);
void minibuffer_add    (AyyiLibfilemanager*, const gchar* leafname);

#endif
