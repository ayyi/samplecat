/*
 * (SLIK) SimpLIstic sKin functions
 * (C) 2004 John Ellis
 *
 * Author: John Ellis
 *
 * This software is released under the GNU General Public License (GNU GPL).
 * Please read the included file COPYING for more information.
 * This software comes with no warranty of any kind, use at your own risk!
 */


#ifndef UI_FILEOPS_H
#define UI_FILEOPS_H


#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>


#define CASE_SORT(a, b) ( (file_sort_case_sensitive) ? strcmp(a, b) : strcasecmp(a, b) )

extern gint file_sort_case_sensitive;

void print_term(const gchar *text_utf8);

gchar *path_to_utf8(const gchar *path);
gchar *path_from_utf8(const gchar *path);

const gchar *homedir(void);
gint stat_utf8(const gchar *s, struct stat *st);
gint isname(const gchar *s);
gint isfile(const gchar *s);
gint isdir(const gchar *s);
gint64 filesize(const gchar *s);
time_t filetime(const gchar *s);
gint filetime_set(const gchar *s, time_t tval);
gint access_file(const gchar *s, int mode);
gint unlink_file(const gchar *s);
gint symlink_utf8(const gchar *source, const gchar *target);
gint mkdir_utf8(const gchar *s, int mode);
gint rmdir_utf8(const gchar *s);
gint copy_file_attributes(const gchar *s, const gchar *t, gint perms, gint mtime);
gint copy_file(const gchar *s, const gchar *t);
gint move_file(const gchar *s, const gchar *t);
gint rename_file(const gchar *s, const gchar *t);
gchar *get_current_dir(void);

/* return True on success, it is up to you to free
 * the lists with path_list_free()
 */
gint path_list(const gchar *path, GList **files, GList **dirs);
void path_list_free(GList *list);
GList *path_list_copy(GList *list);

long checksum_simple(const gchar *path);


gchar *unique_filename(const gchar *path, const gchar *ext, const gchar *divider, gint pad);
gchar *unique_filename_simple(const gchar *path);

const gchar *filename_from_path(const gchar *path);
gchar *remove_level_from_path(const gchar *path);
gchar *concat_dir_and_file(const gchar *base, const gchar *name);

const gchar *extension_from_path(const gchar *path);
gchar *remove_extension_from_path(const gchar *path);

gint file_extension_match(const gchar *path, const gchar *ext);

/* warning note: this modifies path string! */
void parse_out_relatives(gchar *path);

gint file_in_path(const gchar *name);

#endif



