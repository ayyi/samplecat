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
#ifndef __samplecat_types_h__
#define __samplecat_types_h__

#include <glib.h>
#include "typedefs.h"

#define PALETTE_SIZE 17

struct _palette {
	guint red[8];
	guint grn[8];
	guint blu[8];
};

struct _ScanResults {
   int n_added;
   int n_failed;
   int n_dupes;
};

#define HOMOGENOUS 1
#define NON_HOMOGENOUS 0

#endif
