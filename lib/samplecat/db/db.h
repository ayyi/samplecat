/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. https://ayyi.github.io/samplecat/    |
 | copyright (C) 2007-2026 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#pragma once

#include <stdbool.h>
#include <gdk/gdk.h>
#include "typedefs.h"

struct _SamplecatBackend
{
	gboolean   pending;

	void       (*init)             (void* config);

	bool       (*connect)          ();
	void       (*disconnect)       ();

	bool       (*search_iter_new)  (int* n_results);
	Sample*    (*search_iter_next) (unsigned long**);
	void       (*search_iter_free) ();

	void       (*dir_iter_new)     ();
	char*      (*dir_iter_next)    ();
	void       (*dir_iter_free)    ();

	int        (*insert)           (Sample*);
	bool       (*remove)           (int);
	bool       (*file_exists)      (const char*, int*);
	GList*     (*filter_by_audio)  (Sample*);

	bool       (*update_string)    (int, const char*, const char*);
	bool       (*update_int)       (int, const char*, const long int);
	bool       (*update_float)     (int, const char*, const float);
	bool       (*update_blob)      (int, const char*, const guint8*, const guint);

	int        n_results;
};

#include "samplecat/model.h"

#define BACKEND_IS_NULL (samplecat.model->backend.search_iter_new == NULL)

void       db_init                (void*);
bool       db_connect             ();
bool       samplecat_set_backend  (BackendType);

GdkPixbuf* blob_to_pixbuf         (const unsigned char* blob, guint len);

#ifdef USE_MYSQL
#include "samplecat/db/mysql.h"
#endif
