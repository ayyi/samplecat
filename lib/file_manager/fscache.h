/**
* +----------------------------------------------------------------------+
* | This file is part of the Ayyi project. http://ayyi.org               |
* | copyright (C) 2011-2017 Tim Orford <tim@orford.org>                  |
* | copyright (C) 2006, Thomas Leonard and others                        |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#ifndef __fscache_h__
#define __fscache_h__

#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <glib.h>
#include <glib-object.h>
#include "file_manager/typedefs.h"

typedef GObject *(*GFSLoadFunc)(const char* pathname, gpointer user_data);
typedef void (*GFSUpdateFunc)(gpointer object, const char* pathname, gpointer user_data);
typedef enum {
    FSCACHE_LOOKUP_CREATE,   // Load if missing. Update as needed
    FSCACHE_LOOKUP_ONLY_NEW, // Return NULL if not present AND uptodate
    FSCACHE_LOOKUP_PEEK,     // Lookup; don't load or update
    FSCACHE_LOOKUP_INIT,     // Internal use
    FSCACHE_LOOKUP_INSERT,   // Internal use
} FSCacheLookup;

GFSCache* g_fscache_new         (GFSLoadFunc, GFSUpdateFunc, gpointer);
void      g_fscache_destroy     (GFSCache*);
gpointer  g_fscache_lookup      (GFSCache*, const char* pathname);
gpointer  g_fscache_lookup_full (GFSCache*, const char* pathname, FSCacheLookup lookup_type, gboolean* found);
void      g_fscache_may_update  (GFSCache*, const char* pathname);
void      g_fscache_update      (GFSCache*, const char* pathname);
void      g_fscache_purge       (GFSCache*, gint age);

void      g_fscache_insert      (GFSCache*, const char* pathname, gpointer obj, gboolean update_details);

#endif
