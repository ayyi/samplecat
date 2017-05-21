/**
* +----------------------------------------------------------------------+
* | This file is part of the Ayyi project. http://ayyi.org               |
* | copyright (C) 2011-2017 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#ifndef __fm_support_h__
#define __fm_support_h__
#include "stdint.h"
#include <sys/stat.h>
#include "file_manager/typedefs.h"

#define PRETTY_SIZE_LIMIT 10000
#define TIME_FORMAT "%T %d %b %Y"

char*         pathdup               (const char* path);
const guchar* make_path             (const char* dir, const char* leaf);
const char*   our_host_name         (void);
const char*   our_host_name_for_dnd (void);
const char*   user_name             (uid_t);
const char*   group_name            (gid_t);

const char*   format_size           (off_t);
const char*   pretty_permissions    (mode_t);
char*         pretty_time           (const time_t*);

guchar*       shell_escape          (const guchar*);

char*         readlink_dup          (const char* path);
gchar*        to_utf8               (const gchar* src);
char*         md5_hash              (const char*);
void          destroy_glist         (GList**);
void          null_g_free           (gpointer);

CollateKey*   collate_key_new       (const guchar* name);
void          collate_key_free      (CollateKey*);
int           collate_key_cmp       (const CollateKey*, const CollateKey*, gboolean caps_first);

int           stat_with_timeout     (const char* path, struct stat*);
GPtrArray*    list_dir              (const guchar* path);
gint          strcmp2               (gconstpointer a, gconstpointer b);
#if 0
gboolean     file_exists(const char *path);
gboolean     is_dir(const char *path);
gboolean     dir_is_empty(const char *path);
void         file_extension(const char* path, char* extn);
#endif
gboolean     is_sub_dir            (const char* sub_obj, const char* parent);
guchar*      file_copy             (const guchar* from, const guchar* to);
void         file_move             (const char* path, const char* dest);
char*        fork_exec_wait        (const char** argv);

gchar*       uri_text_from_list    (GList*, gint* len, gint plain_text);
GList*       uri_list_from_text    (gchar* data, gint files_only);
gchar*       uri_text_escape       (const gchar* text);
void         uri_text_decode       (gchar* text);

GdkPixbuf*   create_spotlight_pixbuf(GdkPixbuf *src, GdkColor*);

gchar*       path_to_utf8          (const gchar *path);

void         fm__escape_for_menu   (char*);

#endif
