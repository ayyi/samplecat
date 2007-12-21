
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <sndfile.h>
#include <jack/jack.h>
#include <gtk/gtk.h>

#include <gdk-pixbuf/gdk-pixdata.h>
#ifdef OLD
  #include <libart_lgpl/libart.h>
#else
  #include <cairo.h>
#endif
//#include <libgnomevfs/gnome-vfs.h>
#ifdef HAVE_FLAC_1_1_1
  #include <FLAC/all.h>
#endif
#include <jack/ringbuffer.h>

#include "mysql/mysql.h"
#include "dh-link.h"
#include "support.h"
#include "typedefs.h"
#include "main.h"
#include "audio.h"
#include "overview.h"
#include "cellrenderer_hypertext.h"

#include "rox/rox_global.h"
#include "mimetype.h"
#include "pixmaps.h"

extern struct _app app;
extern GAsyncQueue* msg_queue; //receive messages from main thread.
extern unsigned debug;

gpointer
overview_thread(gpointer data)
{
	if(debug) printf("%s(): new thread!\n", __func__);

	if(!msg_queue){ errprintf("%s(): no msg_queue!\n", __func__); return NULL; }

	g_async_queue_ref(msg_queue);

	GList* msg_list = NULL;
	gpointer message;
	while(1){

		//any new work ?
		//printf("overview_thread(): checking for work...\n");
		while(g_async_queue_length(msg_queue)){
			message = g_async_queue_pop(msg_queue);
			if(debug) printf("%s(): new message! %p\n", __func__, message);
			msg_list = g_list_append(msg_list, message);
		}

		while(msg_list!=NULL){
			//printf("overview_thread(): list length: %i.\n", g_list_length(msg_list));
			sample* sample = (struct _sample*)g_list_first(msg_list)->data;
			if(debug) printf("%s(): sample=%p filename=%s.\n", __func__, sample, sample->filename);
			make_overview(sample);
			g_idle_add(on_overview_done, sample); //notify();
			msg_list = g_list_remove(msg_list, sample);
			//printf("overview_thread(): row_ref=%p\n", sample->row_ref);
		}
		sleep(1); //FIXME
	}
}


GdkPixbuf*
make_overview(sample* sample)
{
  GdkPixbuf* pixbuf = NULL;
#ifdef HAVE_FLAC_1_1_1
  if (sample->filetype == TYPE_FLAC) pixbuf = make_overview_flac(sample);
#else
  if(0){}
#endif
  else pixbuf = make_overview_sndfile(sample);
  return pixbuf;
}


GdkPixbuf*
make_overview_sndfile(sample* sample)
{
  /*
  we need to load a file onto a pixbuf.

  specimen: 
		-loads complete file into ram. (true)
		-makes a widget not a bitmap.
	probably not much we can use....

  */
  if(debug) printf("make_overview_sndfile()...\n");

  char *filename = sample->filename;

  SF_INFO        sfinfo;   //the libsndfile struct pointer
  SNDFILE        *sffile;
  sfinfo.format  = 0;
  int            readcount;

  if(debug) printf("make_overview(): row_ref=%p\n", sample->row_ref);

  if(!(sffile = sf_open(filename, SFM_READ, &sfinfo))){
    errprintf("make_overview(): not able to open input file (%p) %s.\n", filename, filename);
    puts(sf_strerror(NULL));    // print the error message from libsndfile
    return NULL;
  }

  GdkPixbuf* pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, 1, //HAS_ALPHA_FALSE,
                                        8,      //int bits_per_sample
                                        OVERVIEW_WIDTH, OVERVIEW_HEIGHT);
  if(debug) printf("make_overview(): pixbuf=%p\n", pixbuf);
  pixbuf_clear(pixbuf, &app.fg_colour);

  //FIXME cairo doesnt support any 24bpp formats.
  cairo_format_t format;
  if (gdk_pixbuf_get_n_channels(pixbuf) == 3) format = CAIRO_FORMAT_RGB24; else format = CAIRO_FORMAT_ARGB32;
  cairo_surface_t* surface = cairo_image_surface_create_for_data (gdk_pixbuf_get_pixels(pixbuf), format, OVERVIEW_WIDTH, OVERVIEW_HEIGHT, gdk_pixbuf_get_rowstride(pixbuf));
  cairo_t*         cr      = cairo_create(surface);
  float r, g, b;
  colour_get_float(&app.bg_colour, &r, &g, &b, 0xff);
  cairo_set_source_rgb (cr, r, g, b);
  cairo_set_line_width (cr, 1.0);
  

  //how many samples should we load at a time? Lets make it a multiple of the image width.
  //-this will use up a lot of ram for a large file, 600M file will use 4MB.
  int frames_per_buf = sfinfo.frames / OVERVIEW_WIDTH;
  int buffer_len = frames_per_buf * sfinfo.channels; //may be a small remainder?
  if(debug) printf("make_overview(): buffer_len=%i\n", buffer_len);
  short *data = malloc(sizeof(*data) * buffer_len);

  int x=0, frame, ch;
  int srcidx_start=0;       //index into the source buffer for each sample pt.
  int srcidx_stop =0;
  short min;                //negative peak value for each pixel.
  short max;                //positive peak value for each pixel.
  short sample_val;

  while ((readcount = sf_read_short(sffile, data, buffer_len))){
    //if(readcount < buffer_len) printf("EOF %i<%i\n", readcount, buffer_len);

    if(sf_error(sffile)) errprintf("make_overview(): read error.\n");

    srcidx_start = 0;
    srcidx_stop  = frames_per_buf;

    min = 0; max = 0;
    for(frame=srcidx_start;frame<srcidx_stop;frame++){ //iterate over all the source samples for this pixel.
      for(ch=0;ch<sfinfo.channels;ch++){
        
        if(frame * sfinfo.channels + ch > buffer_len){ printf("index error!\n"); break; }    
        sample_val = data[frame * sfinfo.channels + ch];
        max = MAX(max, sample_val);
        min = MIN(min, sample_val);
      }
    }

    //scale the values to the part height:
    min = (min * OVERVIEW_HEIGHT) / (256*128*2);
    max = (max * OVERVIEW_HEIGHT) / (256*128*2);

#ifdef OLD
    struct _ArtDRect pts = {x, OVERVIEW_HEIGHT/2 + min, x, OVERVIEW_HEIGHT/2 + max};
    pixbuf_draw_line(pixbuf, &pts, 1.0, &app.bg_colour);
#else
	//TODO libart overviews look better - why? antialiasing? colour is same?
	rect pts = {x, OVERVIEW_HEIGHT/2 + min, x, OVERVIEW_HEIGHT/2 + max};
    pixbuf_draw_line(cr, &pts, 1.0, &app.bg_colour);
#endif

    //printf(" %i max=%i\n", x,OVERVIEW_HEIGHT/2);
    x++;
  }  

  if(sf_close(sffile)) errprintf("make_overview(): bad file close.\n");
  //printf("end of loop...\n");
  free(data);
  cairo_destroy(cr);
  cairo_surface_destroy(surface);
  sample->pixbuf = pixbuf;
  if(!GDK_IS_PIXBUF(sample->pixbuf)) errprintf("make_overview(): pixbuf is not a pixbuf.\n");
  return pixbuf;
}


#ifdef HAVE_FLAC_1_1_1
GdkPixbuf*
make_overview_flac(sample* sample)
{
  /*
  decode a complete flac file generating an overview pixbuf as we go.

  */
  if(debug) printf("make_overview_flac()...\n");

  char *filename = sample->filename;

  _decoder_session session;
  session.output_peakfile = TRUE;
  if(!flac_decoder_sesssion_init(&session, sample)){
    errprintf("make_overview_flac(): unable to initialise flac decoder session. %s.\n", filename);
    return NULL;
  }

  FLAC__FileDecoder* flac = flac_open(&session);

  if(!(flac)){
    errprintf("make_overview_flac(): not able to open input file (%p) %s.\n", filename, filename);
    return NULL;
  }

  GdkPixbuf* pixbuf = overview_pixbuf_new();

  //int frames_per_buf = sfinfo.frames / OVERVIEW_WIDTH;
  //int buffer_len = frames_per_buf * sfinfo.channels; //may be a small remainder?
  //printf("make_overview_flac(): buffer_len=%i\n", buffer_len);

  flac_read(flac);

  //update the pixbuf:
  int x;
  for(x=0;x<OVERVIEW_WIDTH;x++){
    overview_draw_line(pixbuf, x, session.max[x], session.min[x]);
    //printf("make_overview_flac(): max=%i\n", session.max[x]);
  }

  sample->pixbuf = pixbuf;
  return pixbuf;
}


gboolean
make_overview_flac_process(FLAC__FileDecoder* flac, sample* sample, jack_ringbuffer_t* rb)
{
  //we now have enough data in the ringbuffer for 1 pixels worth. required size is (frames / OVERVIEW_WIDTH).
  //-copy this data onto the pixbuf.

  //hmmmm - i think a better way of doing this is to have an array of min/max and fill it straight out of flac_decode()... Make the pixbuf at the end.

  static int x=0;
  int ch, frame;
  short sample_val;
  const unsigned channels        = sample->channels;//frame->header.channels;
  const unsigned bps             = sample->bitdepth;//frame->header.bits_per_sample;
  int bytes_per_sample;
  if(bps == 16) bytes_per_sample = 2; else errprintf("make_overview_flac_process(): unsupported bitdepth\n");
  int bytes_per_frame = channels * bytes_per_sample;
  char buf[bytes_per_frame];
  short* samples = (short*)buf;
  short min;                //negative peak value for each pixel.
  short max;                //positive peak value for each pixel.

  //srcidx_start = 0;
  //srcidx_stop  = frames_per_buf;

  //FLAC__StreamMetadata streaminfo;
  //FLAC__metadata_get_streaminfo(filename, &streaminfo);
  int total_samples = sample->frames;//streaminfo.data.stream_info.total_samples;
  int frames_per_pix = total_samples / OVERVIEW_WIDTH;
  int buffer_size = (total_samples * channels ) / OVERVIEW_WIDTH; //FIXME remainder.

  if(jack_ringbuffer_read_space(rb) < buffer_size) { errprintf("make_overview_flac_process(): ringbuffer not full.\n"); return 0; }

  min = 0; max = 0;
  for(frame=0;frame<frames_per_pix;frame++){ //iterate over all the source samples for this pixel.
    jack_ringbuffer_read(rb, buf, bytes_per_frame);

    for(ch=0;ch<channels;ch++){
        
      sample_val = samples[ch];
      max = MAX(max, sample_val);
      min = MIN(min, sample_val);
    }
  }

  overview_draw_line(sample->pixbuf, x, max, min);

  //printf(" %i max=%i\n", x,OVERVIEW_HEIGHT/2);
  x++;
  return TRUE;
}


void
overview_draw_line(GdkPixbuf* pixbuf, int x, short max, short min)
{
  //scale the values to the part height:
  min = (min * OVERVIEW_HEIGHT) / (256*128*2);
  max = (max * OVERVIEW_HEIGHT) / (256*128*2);

  struct _ArtDRect pts = {x, OVERVIEW_HEIGHT/2 + min, x, OVERVIEW_HEIGHT/2 + max};
  pixbuf_draw_line(pixbuf, &pts, 1.0, &app.bg_colour);
}


gboolean
make_overview_flac_finish(FLAC__FileDecoder* flac, sample* sample)
{
  if(flac_close(flac)) errprintf("make_overview_flac(): bad file close.\n");
  //printf("end of loop...\n");
  if(!GDK_IS_PIXBUF(sample->pixbuf)){ errprintf("make_overview(): pixbuf is not a pixbuf.\n"); return FALSE; }
  return TRUE;
}
#endif


GdkPixbuf*
overview_pixbuf_new()
{
  GdkPixbuf* pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB,
                                        FALSE,  //gboolean has_alpha
                                        8,      //int bits_per_sample
                                        OVERVIEW_WIDTH, OVERVIEW_HEIGHT);  //int width, int height)
  printf("overview_pixbuf_new(): pixbuf=%p\n", pixbuf);
  pixbuf_clear(pixbuf, &app.fg_colour);
  return pixbuf;
}

