/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2007-2016 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#ifndef __db_track_h__
#define __db_track_h__

#include "db/db.h"

#define BACKEND_IS_TRACKER (backend.search_iter_new == tracker__search_iter_new)

gboolean tracker__init             ();
void     tracker__disconnect       ();

int      tracker__insert           (Sample*);
gboolean tracker__delete_row       (int id);
gboolean tracker__file_exists      (const char*, int *id);
GList *  tracker__filter_by_audio  (Sample *s);

gboolean tracker__search_iter_new  (int* n_results);
Sample * tracker__search_iter_next (unsigned long** lengths);
void     tracker__search_iter_free ();

void     tracker__dir_iter_new     ();
char*    tracker__dir_iter_next    ();
void     tracker__dir_iter_free    ();


gboolean tracker__update_string    (int id, const char*, const char*);
gboolean tracker__update_int       (int id, const char*, const long int);
gboolean tracker__update_float     (int id, const char*, const float);
gboolean tracker__update_blob      (int id, const char*, const guint8*, const guint);

#endif
