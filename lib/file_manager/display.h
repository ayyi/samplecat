/**
* +----------------------------------------------------------------------+
* | This file is part of the Ayyi project. http://ayyi.org               |
* | copyright (C) 2011-2018 Tim Orford <tim@orford.org>                  |
* | copyright (C) 2006, Thomas Leonard and others                        |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#ifndef _DISPLAY_H
#define _DISPLAY_H

#include "file_manager.h"

#if 0
#define ROW_HEIGHT_LARGE 64

#include <gtk/gtk.h>
#include <sys/types.h>
#include <dirent.h>

typedef struct _ViewData ViewData;

struct _ViewData
{
	PangoLayout *layout;
	PangoLayout *details;

	int	name_width;
	int	name_height;
	int	details_width;
	int	details_height;

	MaskedPixmap *image;		/* Image; possibly thumbnail */
};

extern Option o_display_inherit_options, o_display_sort_by;
extern Option o_display_size, o_display_details, o_display_show_hidden;
extern Option o_display_show_headers, o_display_show_full_type;
extern Option o_display_show_thumbs;
extern Option o_small_width;
extern Option o_vertical_order_small, o_vertical_order_large;

/* Prototypes */
void display_init(void);
void display_set_layout(FilerWindow *filer_window, DisplayStyle style, DetailsType details, gboolean force_resize);
void display_set_hidden(FilerWindow *filer_window, gboolean hidden);
void display_set_filter_directories(FilerWindow *filer_window, gboolean filter_directories);
#endif
void display_update_hidden (AyyiFilemanager*);
void display_set_filter    (AyyiFilemanager*, FilterType, const gchar* filter_string);
#if 0
void display_set_thumbs(FilerWindow *filer_window, gboolean thumbs);
#endif
void display_set_sort_type(AyyiFilemanager*, FmSortType, GtkSortType order);
#if 0
void display_set_autoselect(FilerWindow *filer_window, const gchar *leaf);

void draw_large_icon(GdkWindow *window,
		     GdkRectangle *area,
		     DirItem  *item,
		     MaskedPixmap *image,
		     gboolean selected,
		     GdkColor *color);
gboolean display_is_truncated(FilerWindow *filer_window, int i);
void display_change_size(FilerWindow *filer_window, gboolean bigger);

ViewData *display_create_viewdata(FilerWindow *filer_window, DirItem *item);
void display_update_view(FilerWindow *filer_window,
			 DirItem *item,
			 ViewData *view,
			 gboolean update_name_layout);
void display_update_views(FilerWindow *filer_window);
#endif
void draw_small_icon(GdkWindow *window, GdkRectangle *area,
		     DirItem  *item, MaskedPixmap *image, gboolean selected,
		     GdkColor *color);
#if 0
void draw_huge_icon(GdkWindow *window, GdkRectangle *area, DirItem *item,
			   MaskedPixmap *image, gboolean selected,
			   GdkColor *color);
#endif
void display_set_actual_size(AyyiFilemanager*, gboolean force_resize);

#endif /* _DISPLAY_H */
