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
#include "glib.h"
#include "debug/debug.h"
#include "utils/fs.h"

void
with_dir (const char* path, GError** error, bool (*fn)(GDir*, const char*, gpointer), gpointer user_data)
{
	g_return_if_fail(path && path[0]);
	if(!g_file_test(path, G_FILE_TEST_IS_DIR)) return;
	GDir* dir = g_dir_open(path, 0, error);
	if(dir && !(error && *error)) {
		const gchar* filename;
		while((filename = g_dir_read_name(dir))){
			if(fn(dir, filename, user_data) == G_SOURCE_REMOVE)
				break;
		}
		g_dir_close(dir);
	}
}


bool
with_fp (const char* filename, const char* mode, bool (*fn)(FILE*, gpointer), gpointer user_data)
{
	FILE* fp = fopen(filename, mode);
	if (!fp) {
		pwarn("cannot open config file (%s).", filename);
		return false;
	}

	bool ok = fn(fp, user_data);

	fclose(fp);

	return ok;
}
