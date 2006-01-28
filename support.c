#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "mysql/mysql.h"
#include <gtk/gtk.h>
#include <libart_lgpl/libart.h>
#include <libgnomevfs/gnome-vfs.h>

#include "main.h"
#include "support.h"
//#include "gnome-vfs-uri.h"

extern struct _app app;


void
errprintf(char *format, ...)
{
  //fn prints an error string, then passes arguments on to vprintf.

  va_list argp; //points to each unnamed arg in turn

  printf("%s ", err);

  va_start(argp, format); //make ap (arg pointer) point to 1st unnamed arg

  vprintf(format, argp);

  va_end(argp); //clean up
}


void
warnprintf(char *format, ...)
{
  //fn prints a warning string, then passes arguments on to vprintf.

  printf("%s ", warn);

  va_list argp;           //points to each unnamed arg in turn
  va_start(argp, format); //make ap (arg pointer) point to 1st unnamed arg
  vprintf(format, argp);
  va_end(argp);           //clean up
}


#ifdef NEVER
//below is stuff from gnome-vfs-uri.c
//-i think eventually we might as well statically link the whole file.


/**
 * gnome_vfs_uri_list_parse:
 * @uri_list:
 *
 * Extracts a list of #GnomeVFSURI objects from a standard text/uri-list,
 * such as one you would get on a drop operation.  Use
 * #gnome_vfs_uri_list_free when you are done with the list.
 *
 * Return value: A GList of GnomeVFSURIs
 **/
GList*
gnome_vfs_uri_list_parse(const gchar* uri_list)
{
    /* Note that this is mostly very stolen from old libgnome/gnome-mime.c */

    const gchar *p, *q;
    gchar *retval;
    GnomeVFSURI/*gchar*/ *uri;
    GList *result = NULL;

    g_return_val_if_fail (uri_list != NULL, NULL);

    p = uri_list;

    /* We don't actually try to validate the URI according to RFC
     * 2396, or even check for allowed characters - we just ignore
     * comments and trim whitespace off the ends.  We also
     * allow LF delimination as well as the specified CRLF.
     */
    while (p != NULL) {
        if (*p != '#') {
            while (g_ascii_isspace (*p))
                p++;

            q = p;
            while ((*q != '\0')
                   && (*q != '\n')
                   && (*q != '\r'))
                q++;

            if (q > p) {
                q--;
                while (q > p
                       && g_ascii_isspace (*q))
                    q--;

                retval = g_malloc (q - p + 2);
                strncpy (retval, p, q - p + 1);
                retval[q - p + 1] = '\0';

                //uri = gnome_vfs_uri_new (retval);
                uri = malloc(128);
                snprintf(uri, 128, retval);

                g_free(retval);

                if(uri != NULL) result = g_list_prepend(result, uri);
            }
        }
        p = strchr (p, '\n');
        if (p != NULL)
            p++;
    }

    return g_list_reverse(result);
}


const gchar *
vfs_get_method_string(const gchar *substring, gchar **method_string)
{
	//get the first part of a uri, eg "file://"

    const gchar *p;
    char *method;

    for (p = substring;
         g_ascii_isalnum (*p) || *p == '+' || *p == '-' || *p == '.';
         p++)
        ;

    if (*p == ':') {
        /* Found toplevel method specification.  */
        method = g_strndup (substring, p - substring);
        *method_string = g_ascii_strdown (method, -1);
        g_free (method);
        p++;
    } else {
        *method_string = g_strdup ("file");
        p = substring;
    }
    return p;
}

/**
 * gnome_vfs_uri_list_free:
 * @list: list of GnomeVFSURI elements
 *
 * Decrements the reference count of each member of @list by one,
 * and frees the list itself.
 **/
void
gnome_vfs_uri_list_free (GList *list)
{
    g_list_free (gnome_vfs_uri_list_unref (list));
}

/**
 * gnome_vfs_uri_list_unref:
 * @list: list of GnomeVFSURI elements
 *
 * Decrements the reference count of the items in @list by one.
 * Note that the list is *not freed* even if each member of the list
 * is freed.
 *
 * Return value: @list
 **/
GList *
gnome_vfs_uri_list_unref (GList *list)
{
    g_list_foreach (list, (GFunc) gnome_vfs_uri_unref, NULL);
    return list;
}

/**
 * gnome_vfs_uri_unref:
 * @uri: A GnomeVFSURI.
 *
 * Decrement @uri's reference count.  If the reference count reaches zero,
 * @uri is destroyed.
 **/
void
gnome_vfs_uri_unref (GnomeVFSURI *uri)
{
    GnomeVFSURI *p, *parent;

    g_return_if_fail (uri != NULL);
    g_return_if_fail (uri->ref_count > 0);

    for (p = uri; p != NULL; p = parent) {
        parent = p->parent;
        g_assert (p->ref_count > 0);
        p->ref_count--;
        if (p->ref_count == 0)
            destroy_element (p);
    }
}

/* Destroy an URI element, but not its parent.  */
static void
destroy_element (GnomeVFSURI *uri)
{
    g_free (uri->text);
    g_free (uri->fragment_id);
    g_free (uri->method_string);

    if (uri->parent == NULL) {
        GnomeVFSToplevelURI *toplevel;

        toplevel = (GnomeVFSToplevelURI *) uri;
        g_free (toplevel->host_name);
        g_free (toplevel->user_name);
        g_free (toplevel->password);
    }

    g_free (uri);
}

/**
 * gnome_vfs_uri_get_toplevel:
 * @uri: A GnomeVFSURI.
 *
 * Retrieve the toplevel URI in @uri.
 *
 * Return value: A pointer to the toplevel URI object.
 **/
GnomeVFSToplevelURI *
gnome_vfs_uri_get_toplevel (const GnomeVFSURI *uri)
{
    const GnomeVFSURI *p;

    g_return_val_if_fail (uri != NULL, NULL);

    for (p = uri; p->parent != NULL; p = p->parent)
        ;

    return (GnomeVFSToplevelURI *) p;
}

#endif


void
pixbuf_clear(GdkPixbuf *pixbuf, GdkColor *colour)
{
	guint32 colour_rgba = ((colour->red/256)<< 24) | ((colour->green/256)<<16) | ((colour->blue/256)<<8) | (0xff); //
	gdk_pixbuf_fill(pixbuf, colour_rgba);
}


void
pixbuf_draw_line(GdkPixbuf *pixbuf, struct _ArtDRect *pts, double line_width, GdkColor *colour)
{
  //printf("pixbuf_draw_line(): pixbuf=%p\n", pixbuf);

  art_u8 *buffer = (art_u8*)gdk_pixbuf_get_pixels(pixbuf);
  int bufwidth  = gdk_pixbuf_get_width    (pixbuf);
  int bufheight = gdk_pixbuf_get_height   (pixbuf);
  int rowstride = gdk_pixbuf_get_rowstride(pixbuf);
  //printf("pixbuf_draw_line(): pixbuf width=%i height=%i\n", bufwidth, bufheight);
  art_u32 color_fg = ((colour->red/256)<< 24) | ((colour->green/256)<<16) | ((colour->blue/256)<<8) | (0xff); //

  //define the line as a libart vector:
  ArtVpath *vec = art_new(ArtVpath, 10);
  vec[0].code = ART_MOVETO;
  vec[0].x = pts->x0;
  vec[0].y = pts->y0;
  vec[1].code = ART_LINETO;
  vec[1].x = pts->x1;
  vec[1].y = pts->y1;
  vec[2].code = ART_END;

  ArtSVP *svp = art_svp_vpath_stroke(vec,
                             ART_PATH_STROKE_JOIN_ROUND,//ArtPathStrokeJoinType join,
                             ART_PATH_STROKE_CAP_BUTT,  //ArtPathStrokeCapType cap,
                             line_width,                //double line_width,
                             1.0,                       //??????? double miter_limit,
                             1.0);                      //double flatness
  //render to buffer:
  art_rgb_svp_alpha(svp, 0, 0,
                    bufwidth, bufheight,                //width, height,
                    color_fg, buffer,
                    rowstride,                          //number of bytes in each row.
                    NULL);
  free(vec);
  free(svp);
}


void
colour_get_style_fg(GdkColor *color, int state)
{
  //gives the default style foreground colour for the given widget state.

  //GtkStyle *style = NULL;
  //style = gtk_widget_get_default_style();
  GtkStyle *style = gtk_style_copy(gtk_widget_get_style(app.window));
  color->red   = style->fg[state].red;
  color->green = style->fg[state].green;
  color->blue  = style->fg[state].blue;
  g_free(style);
}

void
colour_get_style_bg(GdkColor *color, int state)
{
  //gives the default style foreground colour for the given widget state.

  GtkWidget *widget = app.window;

  GtkStyle *style = gtk_style_copy(gtk_widget_get_style(widget));
  //GtkStyle *style = NULL;
  //style = gtk_widget_get_default_style();
  color->red   = style->bg[state].red;
  color->green = style->bg[state].green;
  color->blue  = style->bg[state].blue;

  g_free(style);
}


void
format_time(char* length, char* milliseconds)
{
	if(!length){ errprintf("format_time()!\n"); return; }
	if(!milliseconds){ snprintf(length, 64, " "); return; }

	int t = atoi(milliseconds);
	snprintf(length, 64, "%i.%03i", t / 1000, t % 1000);
	//printf("format_time(): %s\n", length);
}


void
format_time_int(char* length, int milliseconds)
{
	if(!length){ errprintf("format_time()!\n"); return; }
	//if(!milliseconds){ snprintf(length, 64, " "); return; }

	snprintf(length, 64, "%i.%03i", milliseconds / 1000, milliseconds % 1000);
	//printf("format_time(): %s\n", length);
}


