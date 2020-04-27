/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2007-2020 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#ifndef __libsamplecat_worker_h__
#define __libsamplecat_worker_h__

#include "typedefs.h"

void    worker_thread_init   ();
void    worker_register      (Callback);
void    worker_add_job       (Sample*, SampleCallback work, SampleCallback done, gpointer);

#endif
