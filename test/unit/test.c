/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2020-2020 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/

#include "config.h"
#include <getopt.h>
#ifdef USE_GDL
#include "gdl/gdl-dock-item.h"
#endif
#include "gdk/gdkkeysyms.h"
#include "debug/debug.h"
#include "icon_theme.h"
#include "file_manager/pixmaps.h"
#include "test/runner.h"
#include "support.h"
#include "panels/library.h"
#include "application.h"
#include "window.h"

#include "utils.c"
#include "list.c"


void
setup ()
{
	type_init();

	TEST.n_tests = G_N_ELEMENTS(tests);
}


void
teardown ()
{
}

