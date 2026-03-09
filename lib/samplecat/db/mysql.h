/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
 | copyright (C) 2007-2026 Tim Orford <tim@orford.org> and others       |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#pragma once

#include "mysql/mysql.h"
#include "db.h"

#define BACKEND_IS_MYSQL (backend.search_iter_new == mysql__search_iter_new)

void      mysql__set_as_backend   (SamplecatBackend*, SamplecatMysqlConfig*);


struct _SamplecatMysqlConfig
{
	char      host[64];
	char      user[64];
	char      pass[64];
	char      name[64];
};


#if NEVER
gboolean  mysql__is_connected     ();
int       mysql__update_path      (const char* old_path, const char* new_path);
//void      mysql__iter_to_result   (Sample*);
#endif
