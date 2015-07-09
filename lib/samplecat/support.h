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
#ifndef __samplecat_support_h__
#define __samplecat_support_h__
#include <stdint.h>
#include <gtk/gtk.h>
#include "utils/ayyi_utils.h"
#include "utils/mime_type.h"
#include "samplecat/typedefs.h"

#define HAS_ALPHA_FALSE 0
#define HAS_ALPHA_TRUE 1

gboolean     file_exists               (const char*);
time_t       file_mtime                (const char*);
gboolean     is_dir                    (const char*);
gboolean     dir_is_empty              (const char*);
void         file_extension            (const char*, char* extn);

bool         mimestring_is_unsupported (char*);
bool         mimetype_is_unsupported   (MIME_type*, char* mime_string);

bool         ensure_config_dir         ();

void         colour_get_float          (GdkColor*, float* r, float* g, float* b, const unsigned char alpha);

uint8_t*     pixbuf_to_blob            (GdkPixbuf* in, guint* len);

bool         can_use                   (GList*, const char*);

void         samplerate_format         (char*, int samplerate);
void         bitrate_format            (char* str, int bitdepth);
void         bitdepth_format           (char* str, int bitdepth);
gchar*       dir_format                (char*);
gchar*       format_channels           (int);
void         format_smpte              (char*, int64_t);

#endif
