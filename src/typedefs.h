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
#ifndef __typedefs_h__
#define __typedefs_h__
#include "samplecat/typedefs.h"

#define OVERVIEW_WIDTH (200)
#define OVERVIEW_HEIGHT (20)

#ifndef PATH_MAX
#define PATH_MAX (1024)
#endif

typedef struct _menu_def          MenuDef;
typedef struct _libraryview       LibraryView;
typedef struct _inspector         Inspector;
typedef struct _inspector_priv    InspectorPriv;
typedef struct _PlayCtrl          PlayCtrl;
typedef struct _view_option       ViewOption;
typedef struct _accel             Accel;
typedef struct _ScanResults       ScanResults;
typedef struct _GimpActionGroup   GimpActionGroup;

#endif
