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
#include "test/runner.h"
#include "application.h"

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

