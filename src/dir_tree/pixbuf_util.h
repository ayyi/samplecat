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


#ifndef PIXBUF_UTIL_H
#define PIXBUF_UTIL_H


gboolean pixbuf_to_file_as_png (GdkPixbuf *pixbuf, const char *filename);
gboolean pixbuf_to_file_as_jpg(GdkPixbuf *pixbuf, const gchar *filename, gint quality);


GdkPixbuf *pixbuf_inline(const gchar *key);

#define PIXBUF_INLINE_FOLDER_CLOSED	"folder_closed"
#define PIXBUF_INLINE_FOLDER_LOCKED	"folder_locked"
#define PIXBUF_INLINE_FOLDER_OPEN	"folder_open"
#define PIXBUF_INLINE_FOLDER_UP		"folder_up"
#define PIXBUF_INLINE_SCROLLER		"scroller"
#define PIXBUF_INLINE_BROKEN		"broken"
#define PIXBUF_INLINE_LOGO		"logo"


GdkPixbuf *pixbuf_copy_rotate_90(GdkPixbuf *src, gint counter_clockwise);
GdkPixbuf *pixbuf_copy_mirror(GdkPixbuf *src, gint mirror, gint flip);


void pixbuf_draw_rect_fill(GdkPixbuf *pb,
			   gint x, gint y, gint w, gint h,
			   gint r, gint g, gint b, gint a);

void pixbuf_draw_rect(GdkPixbuf *pb,
		      gint x, gint y, gint w, gint h,
		      gint r, gint g, gint b, gint a,
		      gint left, gint right, gint top, gint bottom);

void pixbuf_set_rect_fill(GdkPixbuf *pb,
			  gint x, gint y, gint w, gint h,
			  gint r, gint g, gint b, gint a);

void pixbuf_set_rect(GdkPixbuf *pb,
		     gint x, gint y, gint w, gint h,
		     gint r, gint g, gint b, gint a,
		     gint left, gint right, gint top, gint bottom);

void pixbuf_pixel_set(GdkPixbuf *pb, gint x, gint y, gint r, gint g, gint b, gint a);


void pixbuf_draw_layout(GdkPixbuf *pixbuf, PangoLayout *layout, GtkWidget *widget,
			gint x, gint y,
			guint8 r, guint8 g, guint8 b, guint8 a);


#endif


