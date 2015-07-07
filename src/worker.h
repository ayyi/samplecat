/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2007-2015 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#ifndef __samplecat_worker_h__
#define __samplecat_worker_h__

#include <gtk/gtk.h>
#include "typedefs.h"

enum MsgType
{
	MSG_TYPE_OVERVIEW,
	MSG_TYPE_PEAKLEVEL,
	MSG_TYPE_EBUR128,
} MsgType;

void        worker_thread_init   (SamplecatModel*);
void        worker_register      (Callback);

void        request_overview     (Sample*);
void        request_peaklevel    (Sample*);
void        request_ebur128      (Sample*);

#endif
