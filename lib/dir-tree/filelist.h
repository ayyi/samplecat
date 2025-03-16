/*
 * GQview
 * (C) 2004 John Ellis
 *
 * Author: John Ellis
 *
 * This software is released under the GNU General Public License (GNU GPL).
 * Please read the included file COPYING for more information.
 * This software comes with no warranty of any kind, use at your own risk!
 */


#ifndef FILELIST_H
#define FILELIST_H


typedef struct _FilterEntry FilterEntry;
struct _FilterEntry {
	gchar *key;
	gchar *description;
	gchar *extensions;
	gint enabled;
};

/* you can change, but not add or remove entries from the returned list */
GList *filter_get_list(void);
void filter_remove_entry(FilterEntry *fe);

void filter_add(const gchar *key, const gchar *description, const gchar *extensions, gint enabled);
void filter_add_unique(const gchar *description, const gchar *extensions, gint enabled);
void filter_add_defaults(void);
void filter_reset(void);
void filter_rebuild(void);

gint filter_name_exists(const gchar *name);

void filter_write_list(FILE *f);
void filter_parse(const gchar *text);

gint ishidden(const gchar *name) __attribute__ ((no_instrument_function));


GList *path_list_filter(GList *list, gint is_dir_list);

GList *path_list_sort(GList *list);
GList *path_list_recursive(const gchar *path);

gchar *text_from_size(gint64 size);
gchar *text_from_size_abrev(gint64 size);
const gchar *text_from_time(time_t t);

/* this expects a locale encoded path */
FileData *file_data_new(const gchar *path, struct stat *st);
#if 0
/* this expects a utf-8 path */
FileData *file_data_new_simple(const gchar *path);
#endif
void file_data_free(FileData *fd);

#if 0
GList *filelist_sort(GList *list, SortType method, gint ascend);
GList *filelist_insert_sort(GList *list, FileData *fd, SortType method, gint ascend);
#endif

gint filelist_read(const gchar *path, GList **files, GList **dirs);
void filelist_free(GList *list);

#endif


