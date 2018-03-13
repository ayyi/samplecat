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

#include <gtk/gtk.h>
#include "file_manager/file_manager.h"
#include "file_manager/support.h"
#include "view_dir_tree.h"
#include "utils/ayyi_utils.h"

#include "gqview.h"
#include "filelist.h"

#include "ui_fileops.h"


/*
 *-----------------------------------------------------------------------------
 * file filtering
 *-----------------------------------------------------------------------------
 */

//static GList *filter_list = NULL;
static GList *extension_list = NULL;

gint ishidden(const gchar *name)
{
	if (name[0] != '.') return FALSE;
	if (name[1] == '\0' || (name[1] == '.' && name[2] == '\0')) return FALSE;
	return TRUE;
}

#if 0
static FilterEntry *filter_entry_new(const gchar *key, const gchar *description,
				     const gchar *extensions, gint enabled)
{
	FilterEntry *fe;

	fe = g_new0(FilterEntry, 1);
	fe->key = g_strdup(key);
	fe->description = g_strdup(description);
	fe->extensions = g_strdup(extensions);
	fe->enabled = enabled;
	
	return fe;
}

static void filter_entry_free(FilterEntry *fe)
{
	if (!fe) return;

	g_free(fe->key);
	g_free(fe->description);
	g_free(fe->extensions);
	g_free(fe);
}

GList *filter_get_list(void)
{
	return filter_list;
}

void filter_remove_entry(FilterEntry *fe)
{
	if (!g_list_find(filter_list, fe)) return;

	filter_list = g_list_remove(filter_list, fe);
	filter_entry_free(fe);
}

static gint filter_key_exists(const gchar *key)
{
	GList *work;

	if (!key) return FALSE;

	work = filter_list;
	while (work)
		{
		FilterEntry *fe = work->data;
		work = work->next;

		if (strcmp(fe->key, key) == 0) return TRUE;
		}

	return FALSE;
}

void filter_add(const gchar *key, const gchar *description, const gchar *extensions, gint enabled)
{
	filter_list = g_list_append(filter_list, filter_entry_new(key, description, extensions, enabled));
}

void filter_add_unique(const gchar *description, const gchar *extensions, gint enabled)
{
	gchar *key;
	gint n;

	key = g_strdup("user0");
	n = 1;
	while (filter_key_exists(key))
		{
		g_free(key);
		if (n > 999) return;
		key = g_strdup_printf("user%d", n);
		n++;
		}

	filter_add(key, description, extensions, enabled);
	g_free(key);
}

static void filter_add_if_missing(const gchar *key, const gchar *description, const gchar *extensions, gint enabled)
{
	GList *work;

	if (!key) return;

	work = filter_list;
	while (work)
		{
		FilterEntry *fe = work->data;
		work = work->next;
		if (fe->key && strcmp(fe->key, key) == 0) return;
		}

	filter_add(key, description, extensions, enabled);
}

void filter_reset(void)
{
	GList *work;

	work = filter_list;
	while (work)
		{
		FilterEntry *fe = work->data;
		work = work->next;
		filter_entry_free(fe);
		}

	g_list_free(filter_list);
	filter_list = NULL;
}

void filter_add_defaults(void)
{
	GSList *list, *work;

	list = gdk_pixbuf_get_formats();
	work = list;
	while (work)
		{
		GdkPixbufFormat *format;
		gchar *name;
		gchar *desc;
		gchar **extensions;
		GString *filter = NULL;
		gint i;
		
		format = work->data;
		work = work->next;

		name = gdk_pixbuf_format_get_name(format);
		desc = gdk_pixbuf_format_get_description(format);
		extensions = gdk_pixbuf_format_get_extensions(format);

		i = 0;
		while (extensions[i])
			{
			if (!filter)
				{
				filter = g_string_new(".");
				filter = g_string_append(filter, extensions[i]);
				}
			else
				{
				filter = g_string_append(filter, ";.");
				filter = g_string_append(filter, extensions[i]);
				}
			i++;
			}

		if (debug) printf("loader reported [%s] [%s] [%s]\n", name, desc, filter->str);

		filter_add_if_missing(name, desc, filter->str, TRUE);

		g_free(name);
		g_free(desc);
		g_strfreev(extensions);
		g_string_free(filter, TRUE);
		}
	g_slist_free(list);

	/* add defaults even if gdk-pixbuf does not have them, but disabled */
	filter_add_if_missing("jpeg", "JPEG group", ".jpg;.jpeg;.jpe", FALSE);
	filter_add_if_missing("png", "Portable Network Graphic", ".png", FALSE);
	filter_add_if_missing("tiff", "Tiff", ".tif;.tiff", FALSE);
	filter_add_if_missing("pnm", "Packed Pixel formats", ".pbm;.pgm;.pnm;.ppm", FALSE);
	filter_add_if_missing("gif", "Graphics Interchange Format", ".gif", FALSE);
	filter_add_if_missing("xbm", "X bitmap", ".xbm", FALSE);
	filter_add_if_missing("xpm", "X pixmap", ".xpm", FALSE);
	filter_add_if_missing("bmp", "Bitmap", ".bmp", FALSE);
	filter_add_if_missing("ico", "Icon file", ".ico;.cur", FALSE);
	filter_add_if_missing("ras", "Raster", ".ras", FALSE);
	filter_add_if_missing("svg", "Scalable Vector Graphics", ".svg", FALSE);
}

static GList *filter_to_list(const gchar *extensions)
{
	GList *list = NULL;
	const gchar *p;

	if (!extensions) return NULL;

	p = extensions;
	while (*p != '\0')
		{
		const gchar *b;
		gint l = 0;

		b = p;
		while (*p != '\0' && *p != ';')
			{
			p++;
			l++;
			}
		list = g_list_append(list, g_strndup(b, l));
		if (*p == ';') p++;
		}

	return list;
}

void filter_rebuild(void)
{
	GList *work;

	path_list_free(extension_list);
	extension_list = NULL;

	work = filter_list;
	while (work)
		{
		FilterEntry *fe;

		fe = work->data;
		work = work->next;

		if (fe->enabled)
			{
			GList *ext;

			ext = filter_to_list(fe->extensions);
			if (ext) extension_list = g_list_concat(extension_list, ext);
			}
		}
}
#endif

gint filter_name_exists(const gchar *name)
{
	GList *work;
	if (!extension_list || file_filter_disable) return TRUE;

	work = extension_list;
	while (work)
		{
		gchar *filter = work->data;
		gint lf = strlen(filter);
		gint ln = strlen(name);
		if (ln >= lf)
			{
			if (strncasecmp(name + ln - lf, filter, lf) == 0) return TRUE;
			}
		work = work->next;
		}

	return FALSE;
}

#if 0
void filter_write_list(FILE *f)
{
	GList *work;

	work = filter_list;
	while (work)
		{
		FilterEntry *fe = work->data;
		work = work->next;

		fprintf(f, "filter_ext: \"%s%s\" \"%s\" \"%s\"\n", (fe->enabled) ? "" : "#",
			fe->key, fe->extensions,
			(fe->description) ? fe->description : "");
		}
}

void filter_parse(const gchar *text)
{
	const gchar *p;
	gchar *key;
	gchar *ext;
	gchar *desc;
	gint enabled = TRUE;

	if (!text || text[0] != '"') return;

	key = quoted_value(text);
	if (!key) return;

	p = text;
	p++;
	while (*p != '"' && *p != '\0') p++;
	if (*p != '"')
		{
		g_free(key);
		return;
		}
	p++;
	while (*p != '"' && *p != '\0') p++;
	if (*p != '"')
		{
		g_free(key);
		return;
		}

	ext = quoted_value(p);

	p++;
	while (*p != '"' && *p != '\0') p++;
	if (*p == '"') p++;
	while (*p != '"' && *p != '\0') p++;

	if (*p == '"')
		{
		desc = quoted_value(p);
		}
	else
		{
		desc = NULL;
		}

	if (key && key[0] == '#')
		{
		gchar *tmp;
		tmp = g_strdup(key + 1);
		g_free(key);
		key = tmp;

		enabled = FALSE;
		}

	if (key && strlen(key) > 0 && ext) filter_add(key, desc, ext, enabled);

	g_free(key);
	g_free(ext);
	g_free(desc);
}

GList *path_list_filter(GList *list, gint is_dir_list)
{
	GList *work;

	if (!is_dir_list && file_filter_disable && show_dot_files) return list;

	work = list;
	while (work)
		{
		gchar *name = work->data;
		const gchar *base;

		base = filename_from_path(name);

		if ((!show_dot_files && ishidden(base)) ||
		    (!is_dir_list && !filter_name_exists(base)) ||
		    (is_dir_list && base[0] == '.' && (strcmp(base, GQVIEW_CACHE_LOCAL_THUMB) == 0 ||
						       strcmp(base, GQVIEW_CACHE_LOCAL_METADATA) == 0)) )
			{
			GList *link = work;
			work = work->next;
			list = g_list_remove_link(list, link);
			g_free(name);
			g_list_free(link);
			}
		else
			{
			work = work->next;
			}
		}

	return list;
}

/*
 *-----------------------------------------------------------------------------
 * path list recursive
 *-----------------------------------------------------------------------------
 */

static gint path_list_sort_cb(gconstpointer a, gconstpointer b)
{
	return CASE_SORT((gchar *)a, (gchar *)b);
}

GList *path_list_sort(GList *list)
{
	return g_list_sort(list, path_list_sort_cb);
}

static void path_list_recursive_append(GList **list, GList *dirs)
{
	GList *work;

	work = dirs;
	while (work)
		{
		const gchar *path = work->data;
		GList *f = NULL;
		GList *d = NULL;

		if (path_list(path, &f, &d))
			{
			f = path_list_filter(f, FALSE);
			f = path_list_sort(f);
			*list = g_list_concat(*list, f);

			d = path_list_filter(d, TRUE);
			d = path_list_sort(d);
			path_list_recursive_append(list, d);
			g_list_free(d);
			}

		work = work->next;
		}
}

GList *path_list_recursive(const gchar *path)
{
	GList *list = NULL;
	GList *d = NULL;

	if (!path_list(path, &list, &d)) return NULL;
	list = path_list_filter(list, FALSE);
	list = path_list_sort(list);

	d = path_list_filter(d, TRUE);
	d = path_list_sort(d);
	path_list_recursive_append(&list, d);
	path_list_free(d);

	return list;
}

/*
 *-----------------------------------------------------------------------------
 * text conversion utils
 *-----------------------------------------------------------------------------
 */

gchar *text_from_size(gint64 size)
{
	gchar *a, *b;
	gchar *s, *d;
	gint l, n, i;

	/* what I would like to use is printf("%'d", size)
	 * BUT: not supported on every libc :(
	 */
	if (size > G_MAXUINT)
		{
		/* the %lld conversion is not valid in all libcs, so use a simple work-around */
		a = g_strdup_printf("%d%09d", (guint)(size / 1000000000), (guint)(size % 1000000000));
		}
	else
		{
		a = g_strdup_printf("%d", (guint)size);
		}
	l = strlen(a);
	n = (l - 1)/ 3;
	if (n < 1) return a;

	b = g_new(gchar, l + n + 1);

	s = a;
	d = b;
	i = l - n * 3;
	while (*s != '\0')
		{
		if (i < 1)
			{
			i = 3;
			*d = ',';
			d++;
			}

		*d = *s;
		s++;
		d++;
		i--;
		}
	*d = '\0';

	g_free(a);
	return b;
}

gchar *text_from_size_abrev(gint64 size)
{
	if (size < (gint64)1024)
		{
		return g_strdup_printf(_("%d bytes"), (gint)size);
		}
	if (size < (gint64)1048576)
		{
		return g_strdup_printf(_("%.1f K"), (gfloat)size / 1024.0);
		}
	if (size < (gint64)1073741824)
		{
		return g_strdup_printf(_("%.1f MB"), (gfloat)size / 1048576.0);
		}

	/* to avoid overflowing the float, do division in two steps */
	size /= 1048576.0;
	return g_strdup_printf(_("%.1f GB"), (gfloat)size / 1024.0);
}

/* note: returned string is valid until next call to text_from_time() */
const gchar *text_from_time(time_t t)
{
	static gchar *ret = NULL;
	gchar buf[128];
	gint buflen;
	struct tm *btime;
	GError *error = NULL;

	btime = localtime(&t);

	/* the %x warning about 2 digit years is not an error */
	buflen = strftime(buf, sizeof(buf), "%x %H:%M", btime);
	if (buflen < 1) return "";

	g_free(ret);
	ret = g_locale_to_utf8(buf, buflen, NULL, NULL, &error);
	if (error)
		{
		printf("Error converting locale strftime to UTF-8: %s\n", error->message);
		g_error_free(error);
		return "";
		}

	return ret;
}
#endif

/*
 *-----------------------------------------------------------------------------
 * file info struct
 *-----------------------------------------------------------------------------
 */

FileData *file_data_new(const gchar *path, struct stat *st)
{
	FileData *fd;

	fd = g_new0(FileData, 1);
	fd->path = path_to_utf8(path);
	fd->name = filename_from_path(fd->path);
	fd->size = st->st_size;
	fd->date = st->st_mtime;
	fd->pixbuf = NULL;

	return fd;
}

#if 0
FileData *file_data_new_simple(const gchar *path)
{
	FileData *fd;
	struct stat st;

	fd = g_new0(FileData, 1);
	fd->path = g_strdup(path);
	fd->name = filename_from_path(fd->path);

	if (stat_utf8(fd->path, &st))
		{
		fd->size = st.st_size;
		fd->date = st.st_mtime;
		}

	fd->pixbuf = NULL;

	return fd;
}
#endif

void file_data_free(FileData *fd)
{
	g_free(fd->path);
	if (fd->pixbuf) g_object_unref(fd->pixbuf);
	g_free(fd);
}

/*
 *-----------------------------------------------------------------------------
 * load file list
 *-----------------------------------------------------------------------------
 */

#if 0
static SortType filelist_sort_method = SORT_NONE;
static gint filelist_sort_ascend = TRUE;

static gint sort_file_cb(void *a, void *b)
{
	FileData *fa = a;
	FileData *fb = b;

	if (!filelist_sort_ascend)
		{
		fa = b;
		fb = a;
		}

	switch (filelist_sort_method)
		{
		case SORT_SIZE:
			if (fa->size < fb->size) return -1;
			if (fa->size > fb->size) return 1;
			return 0;
			break;
		case SORT_TIME:
			if (fa->date < fb->date) return -1;
			if (fa->date > fb->date) return 1;
			return 0;
			break;
#ifdef HAVE_STRVERSCMP
		case SORT_NUMBER:
			return strverscmp(fa->name, fb->name);
			break;
#endif
		case SORT_NAME:
		default:
			return CASE_SORT(fa->name, fb->name);
			break;
		}
}

GList *filelist_sort(GList *list, SortType method, gint ascend)
{
	filelist_sort_method = method;
	filelist_sort_ascend = ascend;
	return g_list_sort(list, (GCompareFunc) sort_file_cb);
}

GList *filelist_insert_sort(GList *list, FileData *fd, SortType method, gint ascend)
{
	filelist_sort_method = method;
	filelist_sort_ascend = ascend;
	return g_list_insert_sorted(list, fd, (GCompareFunc) sort_file_cb);
}
#endif

gint filelist_read(const gchar *path, GList **files, GList **dirs)
{
	DIR *dp;
	struct dirent *dir;
	struct stat ent_sbuf;
	gchar *pathl;
	GList *dlist;
	GList *flist;

	dlist = NULL;
	flist = NULL;

	pathl = path_from_utf8(path);
	if (!pathl || (dp = opendir(pathl)) == NULL)
		{
		g_free(pathl);
		if (files) *files = NULL;
		if (dirs) *dirs = NULL;
		return FALSE;
		}

	/* root dir fix */
	if (pathl[0] == '/' && pathl[1] == '\0')
		{
		g_free(pathl);
		pathl = g_strdup("");
		}

	while ((dir = readdir(dp)) != NULL)
		{
		gchar *name = dir->d_name;
		if (show_dot_files || !ishidden(name))
			{
			gchar *filepath = g_strconcat(pathl, "/", name, NULL);
			if (stat(filepath, &ent_sbuf) >= 0)
				{
				if (S_ISDIR(ent_sbuf.st_mode))
					{
					/* we ignore the .thumbnails dir for cleanliness */
					if ((dirs) &&
					    !(name[0] == '.' && (name[1] == '\0' || (name[1] == '.' && name[2] == '\0'))) /*&&
					    strcmp(name, GQVIEW_CACHE_LOCAL_THUMB) != 0 &&
					    strcmp(name, GQVIEW_CACHE_LOCAL_METADATA) != 0 */ )
						{
						dlist = g_list_prepend(dlist, file_data_new(filepath, &ent_sbuf));
						}
					}
				else
					{
					if ((files) && filter_name_exists(name))
						{
						flist = g_list_prepend(flist, file_data_new(filepath, &ent_sbuf));
						}
					}
				}
			g_free(filepath);
			}
		}

	closedir(dp);

	g_free(pathl);

	if (dirs) *dirs = dlist;
	if (files) *files = flist;

	return TRUE;
}

#if 0
void filelist_free(GList *list)
{
	GList *work;

	work = list;
	while (work)
		{
		file_data_free((FileData *)work->data);
		work = work->next;
		}

	g_list_free(list);
}

#endif

