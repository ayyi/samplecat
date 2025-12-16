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

#include <stdint.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "utils/ayyi_utils.h"
#include "file_manager/mimetype.h"
#include "samplecat/typedefs.h"

#define HAS_ALPHA_FALSE 0
#define HAS_ALPHA_TRUE 1

#define SC_NEW(T, ...) ({T* obj = g_new0(T, 1); *obj = (T){__VA_ARGS__}; obj;})

#ifndef __ayyi_h__
#define g_error_clear(E) { if(E){ g_error_free(E); E = NULL; }}
#endif

#ifndef g_free0
#define g_free0(A) (A = (g_free(A), NULL))
#endif

#define set_pointer(P, A, FREE) { if (P) FREE(P); P = A; }

void         p_                        (int level, const char* format, ...);

gboolean     file_exists               (const char*);
time_t       file_mtime                (const char*);
gboolean     is_dir                    (const char*);
gboolean     dir_is_empty              (const char*);
void         file_extension            (const char*, char* extn);

bool         mimestring_is_unsupported (char*);
bool         mimetype_is_unsupported   (MIME_type*, char* mime_string);
GdkPixbuf*   get_iconbuf_from_mimetype (char* mimetype);

bool         ensure_config_dir         ();

uint8_t*     pixbuf_to_blob            (GdkPixbuf* in, guint* len);

bool         can_use                   (GList*, const char*);

void         samplerate_format         (char*, int samplerate);
void         bitrate_format            (char* str, int bitdepth);
void         bitdepth_format           (char* str, int bitdepth);
gchar*       dir_format                (char*);
gchar*       format_channels           (int);
void         format_smpte              (char*, int64_t);

float        gain2db                   (float);
char*        gain2dbstring             (float);

gchar*       str_replace               (const gchar* string, const gchar* search, const gchar* replace);

char*        remove_trailing_slash     (char* path);

