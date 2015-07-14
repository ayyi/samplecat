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
#ifndef __samplecat_typedefs_h__
#define __samplecat_typedefs_h__

typedef struct _Sample            Sample;

typedef void   (*Callback)       (gpointer);
typedef void   (*SampleCallback) (Sample*, gpointer);

#ifndef true
  #define true TRUE
#endif

#ifndef false
  #define false FALSE
#endif

#ifndef bool
  #define bool gboolean
#endif

#endif
