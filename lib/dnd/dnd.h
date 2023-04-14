/*
 +----------------------------------------------------------------------+
 | This file is part of the Ayyi project. http://ayyi.org               |
 | copyright (C) 2011-2020 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#pragma once

enum {
  TARGET_APP_COLLECTION_MEMBER,
  TARGET_URI_LIST,
  TARGET_TEXT_PLAIN
};

#ifdef __dnd_c__
#ifdef GTK4_TODO
GtkTargetEntry dnd_file_drag_types[] = {
	{ "text/uri-list", 0, TARGET_URI_LIST },
	{ "text/plain", 0, TARGET_TEXT_PLAIN }
};
#endif
gint dnd_file_drag_types_count = 2;
#else
#ifdef GTK4_TODO
extern GtkTargetEntry dnd_file_drag_types[2];
#endif
extern gint dnd_file_drag_types_count;
#endif
