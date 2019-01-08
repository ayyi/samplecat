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
#include <gtk/gtk.h>
#include "typedefs.h"

#define PALETTE_SIZE 17

struct _menu_def
{
	char*      label;
	GCallback  callback;
	char*      stock_id;
	gboolean   sensitive;
};

struct _auditioner {
	int     (*check)      ();
	void    (*connect)    (ErrorCallback, gpointer);
	void    (*disconnect) ();
	bool    (*play)       (Sample*);
	void    (*play_all)   ();
	void    (*stop)       ();
/* extended API */
	int     (*playpause)  (int);
	void    (*seek)       (double);
	guint   (*position)   ();
};

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

typedef enum {
   PLAY_STOPPED = 0,
   PLAY_PAUSED,
   PLAY_PLAY_PENDING,
   PLAY_PLAYING
} PlayStatus;

#define HOMOGENOUS 1
#define NON_HOMOGENOUS 0

#endif
