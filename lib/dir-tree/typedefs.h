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

#pragma once

#include <gtk/gtk.h>

typedef enum {
	DT_SORT_NONE,
	DT_SORT_NAME,
	DT_SORT_SIZE,
	DT_SORT_TIME,
	DT_SORT_PATH,
	DT_SORT_NUMBER
} DtSortType;

typedef struct _FileData FileData;

//typedef struct _LayoutWindow LayoutWindow;
typedef struct _ViewDirList ViewDirList;
typedef struct _ViewDirTree ViewDirTree;

typedef struct _PixmapFolders PixmapFolders;


struct _FileData {
	gchar *path;
	const gchar *name;
	gint64 size;
	time_t date;

	GdkPixbuf *pixbuf;
};

struct _ViewDirList
{
	GtkWidget *widget;
	GtkWidget *listview;

	gchar *path;
	GList *list;

	FileData *click_fd;

	FileData *drop_fd;
	GList *drop_list;

	gint drop_scroll_id;

	/* func list */
	void (*select_func)(ViewDirList *vdl, const gchar *path, gpointer data);
	gpointer select_data;

	GtkWidget *popup;

	PixmapFolders *pf;
};

struct _ViewDirTree
{
	GtkWidget *widget;
	GtkWidget *treeview;

	gchar *path;
	gchar *root_path;

	FileData *click_fd;

	FileData *drop_fd;
	GList *drop_list;

	gint drop_scroll_id;
	gint drop_expand_id;

	/* func list */
	void (*select_func)(ViewDirTree *vdt, const gchar *path, gpointer data);
	gpointer select_data;

	GtkWidget *popup;

	PixmapFolders *pf;

	gint busy_ref;
};

struct _PixmapFolders
{
	GdkPixbuf *close;
	GdkPixbuf *open;
	GdkPixbuf *deny;
	GdkPixbuf *parent;
};
