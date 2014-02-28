/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2007-2014 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#ifndef __auditioner_h__
#define __auditioner_h__

#include "typedefs.h"

int  auditioner_check      ();
void auditioner_connect    (Callback, gpointer);
void auditioner_disconnect ();
void auditioner_play       (Sample*);
void auditioner_play_path  (const char*);
void auditioner_stop       ();
void auditioner_toggle     (Sample*);
void auditioner_play_all   ();

#endif
