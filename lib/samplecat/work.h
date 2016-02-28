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
#ifndef __samplecat_work_h__
#define __samplecat_work_h__

#include <gtk/gtk.h>
#include "typedefs.h"

void        request_analysis     (Sample*);

void        request_overview     (Sample*);
void        request_peaklevel    (Sample*);
void        request_ebur128      (Sample*);

#endif
