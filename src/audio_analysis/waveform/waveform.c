#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixdata.h>
#include <cairo.h>

#include "typedefs.h"
#include "support.h"
#include "main.h"
#include "sample.h"

#include "audio_decoder/ad.h"

void
draw_cairo_line(cairo_t* cr, drect *pts, double line_width, GdkColor *colour)
{
	if(pts->y1 == pts->y2) return;

	//cairo_set_line_width(cr, 1);
	float r, g, b;
	colour_get_float(colour, &r, &g, &b, 0xff);
	cairo_set_source_rgba (cr, b, g, r, 0.5);

	cairo_move_to (cr, pts->x1 - 0.5, pts->y1 + 1);
	cairo_line_to (cr, pts->x2 - 0.5, pts->y2 - 1);
	cairo_stroke (cr);
	cairo_move_to (cr, pts->x1 + 0.5, pts->y1);
	cairo_line_to (cr, pts->x2 + 0.5, pts->y2);
	cairo_stroke (cr);
	cairo_move_to (cr, pts->x1 + 1.5, pts->y1 + 1);
	cairo_line_to (cr, pts->x2 + 1.5, pts->y2 - 1);
	cairo_stroke (cr);

	//main line:
	cairo_set_source_rgba (cr, b, g, r, 1.0);
	cairo_move_to (cr, pts->x1 + 0.5, pts->y1 + 1);
	cairo_line_to (cr, pts->x2 + 0.5, pts->y2 - 1);
	cairo_stroke (cr);

}

void
pixbuf_clear(GdkPixbuf *pixbuf, GdkColor *colour)
{
	guint32 colour_rgba = ((colour->red/256)<< 24) | ((colour->green/256)<<16) | ((colour->blue/256)<<8) | (0x60); //
	gdk_pixbuf_fill(pixbuf, colour_rgba);
}

GdkPixbuf* make_overview(Sample* sample) {
	struct adinfo nfo;
	void *sf = ad_open(sample->full_path, &nfo);
	if (!sf) return NULL;
  dbg(0, "NEW OVERVIEW");

	GdkPixbuf* pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, HAS_ALPHA_TRUE, BITS_PER_CHAR_8, OVERVIEW_WIDTH, OVERVIEW_HEIGHT);
	pixbuf_clear(pixbuf, &app.fg_colour);

  cairo_format_t format;
  if (gdk_pixbuf_get_n_channels(pixbuf) == 3) format = CAIRO_FORMAT_RGB24; else format = CAIRO_FORMAT_ARGB32;
  cairo_surface_t* surface = cairo_image_surface_create_for_data (gdk_pixbuf_get_pixels(pixbuf), format, OVERVIEW_WIDTH, OVERVIEW_HEIGHT, gdk_pixbuf_get_rowstride(pixbuf));
  cairo_t* cr = cairo_create(surface);
  cairo_set_line_width (cr, 1.0);

  if (1) {
    drect pts = {0, OVERVIEW_HEIGHT/2, OVERVIEW_WIDTH, OVERVIEW_HEIGHT/2 +1};
    GdkColor color; color.red=0x7fff; color.green=0x7fff; color.blue=0x7fff;
    draw_cairo_line(cr, &pts, 1.0, &color);
  }

  int frames_per_buf = nfo.frames / OVERVIEW_WIDTH;
  int buffer_len = frames_per_buf * nfo.channels;
  float* data = malloc(sizeof(float) * buffer_len);

  int x=0;
  float min;                //negative peak value for each pixel.
  float max;                //positive peak value for each pixel.
	int readcount;
  while ((readcount = ad_read(sf, data, buffer_len))>0){
		int frame;
    const int srcidx_start = 0;
    const int srcidx_stop  = frames_per_buf;

    min = 0; max = 0;
    for(frame=srcidx_start;frame<srcidx_stop;frame++){ 
			int ch;
      for(ch=0;ch<nfo.channels;ch++){
        if(frame * nfo.channels + ch > buffer_len){ perr("index error!\n"); break; }    
        const float sample_val = data[frame * nfo.channels + ch];
        max = MAX(max, sample_val);
        min = MIN(min, sample_val);
      }
    }

    //scale the values to the part height:
    min = rint(min * OVERVIEW_HEIGHT/2.0);
    max = rint(max * OVERVIEW_HEIGHT/2.0);

    drect pts = {x, OVERVIEW_HEIGHT/2 + min, x, OVERVIEW_HEIGHT/2 + max};
    draw_cairo_line(cr, &pts, 1.0, &app.bg_colour);

    x++;
  }  

  if(ad_close(sf)) perr("bad file close.\n");
  free(data);
  cairo_destroy(cr);
  cairo_surface_destroy(surface);
  sample->overview = pixbuf;
  if(!GDK_IS_PIXBUF(sample->overview)) perr("pixbuf is not a pixbuf.\n");
  return pixbuf;
}
