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
#ifndef __samplecat_typedefs_h__
#define __samplecat_typedefs_h__

#include "glib.h"

#ifndef __PRI64_PREFIX
#if (defined __X86_64__ || defined __LP64__)
# define __PRI64_PREFIX  "l"
#else
# define __PRI64_PREFIX  "ll"
#endif
#endif

#ifndef PRIu64
# define PRIu64   __PRI64_PREFIX "u"
#endif
#ifndef PRIi64
# define PRIi64   __PRI64_PREFIX "i"
#endif

typedef enum {
	BACKEND_NONE = 0,
	BACKEND_MYSQL,
	BACKEND_SQLITE,
	BACKEND_TRACKER,
} BackendType;

typedef struct _Samplecat         Samplecat;
typedef struct _SamplecatModel    SamplecatModel;
typedef struct _Sample            Sample;
typedef struct _SamplecatBackend  SamplecatBackend;
typedef struct _SamplecatDBConfig SamplecatDBConfig;
typedef struct _Logger            Logger;

typedef void   (*Callback)       (gpointer);
typedef void   (*SampleCallback) (Sample*, gpointer);
typedef void   (*ErrorCallback)  (GError*, gpointer);

struct _Samplecat {
   SamplecatModel*      model;
#ifdef __GTK_H__
   GtkListStore*        store;
#else
   gpointer             store;
#endif
   Logger*              logger;
};
#ifdef __samplecat_c__
Samplecat samplecat;
#else
extern Samplecat samplecat;
#endif

typedef struct {
   int n_added;
   int n_failed;
   int n_dupes;
} ScanResults;

typedef struct { double x1, y1, x2, y2; } DRect;
typedef struct { int start, end; } iRange;

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
