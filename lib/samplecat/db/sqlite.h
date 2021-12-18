/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
 | copyright (C) 2007-2022 Tim Orford <tim@orford.org> and others       |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#pragma once

#define BACKEND_IS_SQLITE (backend.search_iter_new == sqlite__search_iter_new)

void     sqlite__set_as_backend   (SamplecatBackend*);
int      sqlite__connect          ();

