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
#include "view_dir_tree.h"

#include "gqview.h"
#include "pixbuf_util.h"


#if 0

/*
 *-----------------------------------------------------------------------------
 * jpeg save
 *-----------------------------------------------------------------------------
 */

gboolean pixbuf_to_file_as_jpg(GdkPixbuf *pixbuf, const gchar *filename, gint quality)
{
	GError *error = NULL;
	gchar *qbuf;
	gboolean ret;

	if (!pixbuf || !filename) return FALSE;

	if (quality == -1) quality = 75;
	if (quality < 1 || quality > 100)
		{
		printf("Jpeg not saved, invalid quality %d\n", quality);
		return FALSE;
		}

	qbuf = g_strdup_printf("%d", quality);
	ret = gdk_pixbuf_save(pixbuf, filename, "jpeg", &error, "quality", qbuf, NULL);
	g_free(qbuf);

	if (error)
		{
		printf("Error saving jpeg to %s\n%s\n", filename, error->message);
		g_error_free(error);
		}

	return ret;
}
#endif

/*
 *-----------------------------------------------------------------------------
 * pixbuf from inline
 *-----------------------------------------------------------------------------
 */

typedef struct _PixbufInline PixbufInline;
struct _PixbufInline
{
	const gchar *key;
	const guint8 *data;
};

/*
static PixbufInline inline_pixbuf_data[] = {
	{ PIXBUF_INLINE_FOLDER_CLOSED,	folder_closed },
	{ PIXBUF_INLINE_FOLDER_LOCKED,	folder_locked },
	{ PIXBUF_INLINE_FOLDER_OPEN,	folder_open },
	{ PIXBUF_INLINE_FOLDER_UP,	folder_up },
	{ PIXBUF_INLINE_SCROLLER,	icon_scroller },
	{ PIXBUF_INLINE_BROKEN,		icon_broken },
	{ PIXBUF_INLINE_LOGO,		gqview_logo },
	{ NULL, NULL }
};

GdkPixbuf *pixbuf_inline(const gchar *key)
{
	gint i;

	if (!key) return NULL;

	i = 0;
	while (inline_pixbuf_data[i].key)
		{
		if (strcmp(inline_pixbuf_data[i].key, key) == 0)
			{
			return gdk_pixbuf_new_from_inline(-1, inline_pixbuf_data[i].data, FALSE, NULL);
			}
		i++;
		}

	printf("warning: inline pixbuf key \"%s\" not found.\n", key);

	return NULL;
}
*/

/*
 *-----------------------------------------------------------------------------
 * pixbuf rotation
 *-----------------------------------------------------------------------------
 */

#if 0

static void pixbuf_copy_block(guchar *src, gint src_row_stride, gint w, gint h,
			      guchar *dest, gint dest_row_stride, gint x, gint y, gint bytes_per_pixel)
{
	gint i;
	guchar *sp;
	guchar *dp;

	for (i = 0; i < h; i++)
		{
		sp = src + (i * src_row_stride);
		dp = dest + ((y + i) * dest_row_stride) + (x * bytes_per_pixel);
		memcpy(dp, sp, w * bytes_per_pixel);
		}
}

#define ROTATE_BUFFER_WIDTH 48
#define ROTATE_BUFFER_HEIGHT 48

/*
 * Returns a copy of pixbuf src rotated 90 degrees clockwise or 90 counterclockwise
 *
 */
GdkPixbuf *pixbuf_copy_rotate_90(GdkPixbuf *src, gint counter_clockwise)
{
	GdkPixbuf *dest;
	gint has_alpha;
	gint sw, sh, srs;
	gint dw, dh, drs;
	guchar *s_pix;
        guchar *d_pix;
#if 0
	guchar *sp;
        guchar *dp;
#endif
	gint i, j;
	gint a;
	GdkPixbuf *buffer;
        guchar *b_pix;
	gint brs;
	gint w, h;

	if (!src) return NULL;

	sw = gdk_pixbuf_get_width(src);
	sh = gdk_pixbuf_get_height(src);
	has_alpha = gdk_pixbuf_get_has_alpha(src);
	srs = gdk_pixbuf_get_rowstride(src);
	s_pix = gdk_pixbuf_get_pixels(src);

	dw = sh;
	dh = sw;
	dest = gdk_pixbuf_new(GDK_COLORSPACE_RGB, has_alpha, 8, dw, dh);
	drs = gdk_pixbuf_get_rowstride(dest);
	d_pix = gdk_pixbuf_get_pixels(dest);

	a = (has_alpha ? 4 : 3);

	buffer = gdk_pixbuf_new(GDK_COLORSPACE_RGB, has_alpha, 8,
				ROTATE_BUFFER_WIDTH, ROTATE_BUFFER_HEIGHT);
	b_pix = gdk_pixbuf_get_pixels(buffer);
	brs = gdk_pixbuf_get_rowstride(buffer);

	for (i = 0; i < sh; i+= ROTATE_BUFFER_WIDTH)
		{
		w = MIN(ROTATE_BUFFER_WIDTH, (sh - i));
		for (j = 0; j < sw; j += ROTATE_BUFFER_HEIGHT)
			{
			gint x, y;

			h = MIN(ROTATE_BUFFER_HEIGHT, (sw - j));
			pixbuf_copy_block_rotate(s_pix, srs, j, i,
						 b_pix, brs, h, w,
						 a, counter_clockwise);

			if (counter_clockwise)
				{
				x = i;
				y = sw - h - j;
				}
			else
				{
				x = sh - w - i;
				y = j;
				}
			pixbuf_copy_block(b_pix, brs, w, h,
					  d_pix, drs, x, y, a);
			}
		}

	gdk_pixbuf_unref(buffer);

#if 0
	/* this is the simple version of rotation (roughly 2-4x slower) */

	for (i = 0; i < sh; i++)
		{
		sp = s_pix + (i * srs);
		for (j = 0; j < sw; j++)
			{
			if (counter_clockwise)
				{
				dp = d_pix + ((dh - j - 1) * drs) + (i * a);
				}
			else
				{
				dp = d_pix + (j * drs) + ((dw - i - 1) * a);
				}

			*(dp++) = *(sp++);	/* r */
			*(dp++) = *(sp++);	/* g */
			*(dp++) = *(sp++);	/* b */
			if (has_alpha) *(dp) = *(sp++);	/* a */
			}
		}
#endif

	return dest;
}

/*
 * Returns a copy of pixbuf mirrored and or flipped.
 * TO do a 180 degree rotations set both mirror and flipped TRUE
 * if mirror and flip are FALSE, result is a simple copy.
 */
GdkPixbuf *pixbuf_copy_mirror(GdkPixbuf *src, gint mirror, gint flip)
{
	GdkPixbuf *dest;
	gint has_alpha;
	gint w, h, srs;
	gint drs;
	guchar *s_pix;
        guchar *d_pix;
	guchar *sp;
        guchar *dp;
	gint i, j;
	gint a;

	if (!src) return NULL;

	w = gdk_pixbuf_get_width(src);
	h = gdk_pixbuf_get_height(src);
	has_alpha = gdk_pixbuf_get_has_alpha(src);
	srs = gdk_pixbuf_get_rowstride(src);
	s_pix = gdk_pixbuf_get_pixels(src);

	dest = gdk_pixbuf_new(GDK_COLORSPACE_RGB, has_alpha, 8, w, h);
	drs = gdk_pixbuf_get_rowstride(dest);
	d_pix = gdk_pixbuf_get_pixels(dest);

	a = has_alpha ? 4 : 3;

	for (i = 0; i < h; i++)
		{
		sp = s_pix + (i * srs);
		if (flip)
			{
			dp = d_pix + ((h - i - 1) * drs);
			}
		else
			{
			dp = d_pix + (i * drs);
			}
		if (mirror)
			{
			dp += (w - 1) * a;
			for (j = 0; j < w; j++)
				{
				*(dp++) = *(sp++);	/* r */
				*(dp++) = *(sp++);	/* g */
				*(dp++) = *(sp++);	/* b */
				if (has_alpha) *(dp) = *(sp++);	/* a */
				dp -= (a + 3);
				}
			}
		else
			{
			for (j = 0; j < w; j++)
				{
				*(dp++) = *(sp++);	/* r */
				*(dp++) = *(sp++);	/* g */
				*(dp++) = *(sp++);	/* b */
				if (has_alpha) *(dp++) = *(sp++);	/* a */
				}
			}
		}

	return dest;
}


/*
 *-----------------------------------------------------------------------------
 * pixbuf drawing
 *-----------------------------------------------------------------------------
 */

/*
 * Fills region of pixbuf at x,y over w,h
 * with colors red (r), green (g), blue (b)
 * applying alpha (a), use a=255 for solid.
 */
void pixbuf_draw_rect_fill(GdkPixbuf *pb,
			   gint x, gint y, gint w, gint h,
			   gint r, gint g, gint b, gint a)
{
	gint p_alpha;
	gint pw, ph, prs;
	guchar *p_pix;
	guchar *pp;
	gint i, j;

	if (!pb) return;

	pw = gdk_pixbuf_get_width(pb);
	ph = gdk_pixbuf_get_height(pb);

	if (x < 0 || x + w > pw) return;
	if (y < 0 || y + h > ph) return;

	p_alpha = gdk_pixbuf_get_has_alpha(pb);
	prs = gdk_pixbuf_get_rowstride(pb);
	p_pix = gdk_pixbuf_get_pixels(pb);

        for (i = 0; i < h; i++)
		{
		pp = p_pix + (y + i) * prs + (x * (p_alpha ? 4 : 3));
		for (j = 0; j < w; j++)
			{
			*pp = (r * a + *pp * (256-a)) >> 8;
			pp++;
			*pp = (g * a + *pp * (256-a)) >> 8;
			pp++;
			*pp = (b * a + *pp * (256-a)) >> 8;
			pp++;
			if (p_alpha) pp++;
			}
		}
}

void pixbuf_draw_rect(GdkPixbuf *pb,
		      gint x, gint y, gint w, gint h,
		      gint r, gint g, gint b, gint a,
		      gint left, gint right, gint top, gint bottom)
{
	pixbuf_draw_rect_fill(pb, x + left, y, w - left - right, top,
			      r, g, b ,a);
	pixbuf_draw_rect_fill(pb, x + w - right, y, right, h,
			      r, g, b ,a);
	pixbuf_draw_rect_fill(pb, x + left, y + h - bottom, w - left - right, bottom,
			      r, g, b ,a);
	pixbuf_draw_rect_fill(pb, x, y, left, h,
			      r, g, b ,a);
}

void pixbuf_set_rect_fill(GdkPixbuf *pb,
			  gint x, gint y, gint w, gint h,
			  gint r, gint g, gint b, gint a)
{
	gint p_alpha;
	gint pw, ph, prs;
	guchar *p_pix;
	guchar *pp;
	gint i, j;

	if (!pb) return;

	pw = gdk_pixbuf_get_width(pb);
	ph = gdk_pixbuf_get_height(pb);

	if (x < 0 || x + w > pw) return;
	if (y < 0 || y + h > ph) return;

	p_alpha = gdk_pixbuf_get_has_alpha(pb);
	prs = gdk_pixbuf_get_rowstride(pb);
	p_pix = gdk_pixbuf_get_pixels(pb);

        for (i = 0; i < h; i++)
		{
		pp = p_pix + (y + i) * prs + (x * (p_alpha ? 4 : 3));
		for (j = 0; j < w; j++)
			{
			*pp = r; pp++;
			*pp = g; pp++;
			*pp = b; pp++;
			if (p_alpha) { *pp = a; pp++; }
			}
		}
}

void pixbuf_set_rect(GdkPixbuf *pb,
		     gint x, gint y, gint w, gint h,
		     gint r, gint g, gint b, gint a,
		     gint left, gint right, gint top, gint bottom)
{
	pixbuf_set_rect_fill(pb, x + left, y, w - left - right, top,
			     r, g, b ,a);
	pixbuf_set_rect_fill(pb, x + w - right, y, right, h,
			     r, g, b ,a);
	pixbuf_set_rect_fill(pb, x + left, y + h - bottom, w - left - right, bottom,
			     r, g, b ,a);
	pixbuf_set_rect_fill(pb, x, y, left, h,
			     r, g, b ,a);
}

void pixbuf_pixel_set(GdkPixbuf *pb, gint x, gint y, gint r, gint g, gint b, gint a)
{
	guchar *buf;
	gint has_alpha;
	gint rowstride;
	guchar *p;

	if (x < 0 || x >= gdk_pixbuf_get_width(pb) ||
            y < 0 || y >= gdk_pixbuf_get_height(pb)) return;

	buf = gdk_pixbuf_get_pixels(pb);
	has_alpha = gdk_pixbuf_get_has_alpha(pb);
	rowstride = gdk_pixbuf_get_rowstride(pb);

        p = buf + (y * rowstride) + (x * (has_alpha ? 4 : 3));
	*p = r; p++;
	*p = g; p++;
	*p = b; p++;
	if (has_alpha) *p = a;
}


/*
 *-----------------------------------------------------------------------------
 * pixbuf text rendering
 *-----------------------------------------------------------------------------
 */

static void pixbuf_copy_font(GdkPixbuf *src, gint sx, gint sy,
			     GdkPixbuf *dest, gint dx, gint dy,
			     gint w, gint h,
			     guint8 r, guint8 g, guint8 b, guint8 a)
{
	gint sw, sh, srs;
	gint s_alpha;
	gint s_step;
	guchar *s_pix;
	gint dw, dh, drs;
	gint d_alpha;
	gint d_step;
	guchar *d_pix;

	guchar *sp;
	guchar *dp;
	gint i, j;

	if (!src || !dest) return;

	sw = gdk_pixbuf_get_width(src);
	sh = gdk_pixbuf_get_height(src);

	if (sx < 0 || sx + w > sw) return;
	if (sy < 0 || sy + h > sh) return;

	dw = gdk_pixbuf_get_width(dest);
	dh = gdk_pixbuf_get_height(dest);

	if (dx < 0 || dx + w > dw) return;
	if (dy < 0 || dy + h > dh) return;

	s_alpha = gdk_pixbuf_get_has_alpha(src);
	d_alpha = gdk_pixbuf_get_has_alpha(dest);
	srs = gdk_pixbuf_get_rowstride(src);
	drs = gdk_pixbuf_get_rowstride(dest);
	s_pix = gdk_pixbuf_get_pixels(src);
	d_pix = gdk_pixbuf_get_pixels(dest);

	s_step = (s_alpha) ? 4 : 3;
	d_step = (d_alpha) ? 4 : 3;

	for (i = 0; i < h; i++)
		{
		sp = s_pix + (sy + i) * srs + sx * s_step;
		dp = d_pix + (dy + i) * drs + dx * d_step;
		for (j = 0; j < w; j++)
			{
			if (*sp)
				{
				guint8 asub;

				asub = a * sp[0] / 255;
				*dp = (r * asub + *dp * (256-asub)) >> 8;
				dp++;
				asub = a * sp[1] / 255;
				*dp = (g * asub + *dp * (256-asub)) >> 8;
				dp++;
				asub = a * sp[2] / 255;
				*dp = (b * asub + *dp * (256-asub)) >> 8;
				dp++;

				if (d_alpha)
					{
					*dp = MAX(*dp, a * ((sp[0] + sp[1] + sp[2]) / 3) / 255);
					dp++;
					}
				}
			else
				{
				dp += d_step;
				}

			sp += s_step;
			}
		}
}

void pixbuf_draw_layout(GdkPixbuf *pixbuf, PangoLayout *layout, GtkWidget *widget,
			gint x, gint y,
			guint8 r, guint8 g, guint8 b, guint8 a)
{
	GdkPixmap *pixmap;
	GdkPixbuf *buffer;
	gint w, h;
	GdkGC *gc;
	gint sx, sy;
	gint dw, dh;

	if (!widget || !widget->window) return;

	pango_layout_get_pixel_size(layout, &w, &h);
	pixmap = gdk_pixmap_new(widget->window, w, h, -1);

	gc = gdk_gc_new(widget->window);
	gdk_gc_copy(gc, widget->style->black_gc);
	gdk_draw_rectangle(pixmap, gc, TRUE, 0, 0, w, h);
	gdk_gc_copy(gc, widget->style->white_gc);
	gdk_draw_layout(pixmap, gc, 0, 0, layout);
	g_object_unref(gc);

	buffer = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, w, h);
	gdk_pixbuf_get_from_drawable(buffer, pixmap,
				     gdk_drawable_get_colormap(widget->window),
				     0, 0, 0, 0, w, h);
	g_object_unref(pixmap);

	sx = 0;
	sy = 0;
	dw = gdk_pixbuf_get_width(pixbuf);
	dh = gdk_pixbuf_get_height(pixbuf);

	if (x < 0)
		{
		w += x;
		sx = -x;
		x = 0;
		}

	if (y < 0)
		{
		h += y;
		sy = -y;
		y = 0;
		}

	if (x + w > dw)	w = dw - x;
	if (y + h > dh) h = dh - y;

	pixbuf_copy_font(buffer, 0, 0,
			 pixbuf, x, y, w, h,
			 r, g, b, a);

	g_object_unref(buffer);
}

#endif
