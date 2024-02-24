/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. https://ayyi.github.io/samplecat/    |
 | copyright (C) 2007-2024 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#pragma once

#include "samplecat/typedefs.h"

#define OVERVIEW_WIDTH (200)
#define OVERVIEW_HEIGHT (20)

#ifndef PATH_MAX
#define PATH_MAX (1024)
#endif

typedef struct _libraryview       LibraryView;
typedef struct _Inspector         Inspector;
typedef struct _view_option       ViewOption;
typedef struct _GimpActionGroup   GimpActionGroup;
