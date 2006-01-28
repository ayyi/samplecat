#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <gtk/gtk.h>
#include <libgnomecanvas/libgnomecanvas.h>

#include "../includes/dae_setup.h"
#include "main.h"
#include "window.h"
#include "interface.h"
#include "pool.h"
#include "peak.h"
#include "track.h"
#include "colour.h"
#include "song.h"
#include "part.h"
#include "arrange_utils.h"
#include "support.h"
#include "part_item.h"
#include "../includes/dae_malloc.h"

#include "../includes/dae_event_rec.h"
#include "../includes/dae_song_client.h"
#include "../includes/dae_app_types.h"

#define MIN_OVERVIEW_HEIGHT 2

#define PEAK_BYTE_DEPTH 2           //we are using 16bit peak files.
#define PEAK_VALUES_PER_SAMPLE 2    //one positive and one negative.
#define PEAK_BYTES_PER_SAMPLE 4     //16bits for +ve and 16bits for -ve.
#define BODGEEEE 2                  //theres a mistake somewhere, but it could be anywhere....

/*

ardour:
canvas item derived from GtkCanvasItemClass
*/
//http://ardour.org/cgi-bin/viewcvs.cgi/*checkout*/ardour/gtk_ardour/canvas-waveview.c?rev=1.47&content-type=text/plain
/*
ardour has the peakfiles in a separate dir to reduce fragmentation.

specimen: http://www.gazuga.net (src/gui/waveform.c) is reportedly worth a look.

hi zoom
-------
when the peakfile resolution is not high enough we currently just duplicate values.
There are 2 approaches to doing this properly:
1-access the audio file directly and show real data
2-interpolate the data from the peakfile. I suspect this is how some apps do it. Is it much faster?

interpolation
-------------
the peaks are not interpolated. Ie, the values displayed only represent the instantantaneous level,
and not that of the underlying signal. Eg if 2 adjacent samples have a value of 0.9, the signal
peak is likely to be higher than this. I dont consider this a problem.

*/

/*
//void*
short*
peak_load(struct _pool_item *pool_item)
{
  //loads the peak file for the given poolitem into a buffer.

  //printf("peak_load()...\n");
  //printf("peak_load(): filename=%s\n", filename);

  char fullpath[256];
  char *filename = pool_item->filename;
  if(filename[0]=='/'){
    //absolute path.
  }else{
    //relative paths are relative to the song default audio directory.
    snprintf(fullpath, 256, "%s/%s", song->audiodir, filename);
  }

  if(!g_file_test(fullpath, G_FILE_TEST_EXISTS)){ 
    printf("%s peak_load(): audio file doesnt exist! (%s)\n", smwarn, fullpath);
    pool_item->valid = FALSE;
  } else pool_item->valid = TRUE;

  //find the name of the peakfile:
  gboolean peak_valid = FALSE;
  char peakfile[256];
  peak_name(peakfile, fullpath);
  if(!g_file_test(peakfile, G_FILE_TEST_EXISTS)){ 
    printf("%s peak_load(): peakfile doesnt exist! (%s)\n", smwarn, peakfile);
    //pool_item->valid = FALSE;
  } else peak_valid = TRUE;

  //get file size:
  int len;
  struct stat statbuf;
  if(pool_item->valid){
    stat(peakfile, &statbuf);
    len = statbuf.st_size / PEAK_BYTE_DEPTH; //buffer len. =length of peakfile in samples x2.
    if(statbuf.st_size<1)
             {printf("%s peak_load(): bad file size: %i\n", smerr, (int)statbuf.st_size); return NULL;}
  }
  else len = 100; //FIXME use length of "missing" pixbuf.
  printf("peak_load(): %s: size=%i\n", peakfile, (int)statbuf.st_size);

  int read_tot = 0;
  short *buf = NULL;
  if(peak_valid){
    int fp = open(peakfile, O_RDONLY);
    if(!fp){printf("%s peak_load(): file open failure.\n", smwarn); return 0;}

    buf = dae_malloc(sizeof(*buf) * len);

    short c;
    int i=0;
    int count = 2;
    while(read(fp,&c,count) == count){
      read_tot += 1;
      buf[i] = c;
      if(++i >= len){ break;}
    }
  
    close(fp);
  }
  pool_item->buflen = len;
  //printf("peak_load(): done. %s: readcount=%i of %isamples (%li beats / %.3f secs).\n", 
  //                           filename, read_tot, len, samples2beats(len), samples2secs(len*PEAK_RATIO));
  
  return buf;
}
*/

/*
void
peak2pic(struct _gpart *part)
{
  //takes the loaded peakfile buffer, turns it into a pixel map, and displays it on the canvas for the 
  // given part.
  //-can be called for new parts and updates?

  //-the pic is at the part size so has to be remade if the arrange window scaling changes.

  //-a gdkpixmap is used to draw onto. This is copied to a gdk_pixbuf in order to put on the canvas.
  // It would be quicker to write directly to the (device independant) pixbuf.
  //    see: http://10.0.0.151/man/gdk-pixbuf/gdk-pixbuf-gdk-pixbuf.html
  //         this shows setting individual pixels. Dont know if you can use higher level drawing commands.
  // A pointer to the pixbuf can be obtained via the canvaspixbuf "pixbuf" property.
  // If we drew directly to the pixbuf, would that be equiv to drawing to the canvasbuf?

  //GnomeCanvasBuf
  //widget implementations can draw directly to this buffer!! http://developer.gnome.org/doc/GGAD/z186.html
  //is this faster than writing directly to a gnomecanvaspixbuf pixbuf?

  //nb: make sure we know which pics are on Xserver and which are on local(to the app) machine.
  //    -potentially, transferring lots of data between machines could be very slow.

  //FIXME if the pixbuf is remade, i think we lose the handler to it. Either we need to
  //      re-add it here, or notify the calling fn.

  //--------------------

  //caching of raw peakfile data:
  
  //-to some extent these files are cached by the kernel.

  //how much data would it be if we cached all the peakdata?
  //  for 1000 mono regions of length 5secs at 44kHz,
  //  size = 1000 * 5 * 44100 * 2 / 256 = 1.7 Mbytes
  //  (this is supposed to be an average case not a worst-case.)
  //
  //  another common case: 32 files of length 5 minutes
  //  size = (5 * 60) * 32 * 44100 * 2 / 256 = 3.3 MB 

  //We could use something like
  //gdk_pixbuf_copy_area (), or scale, to regenerate the pics?

  //currently we load the complete waveform, even if song regions are not using the full file.
  //-this is useful for display in pool window lists.

  //--------------------

  //caching of scaled lines:

  //-perhaps we need to differentiate somehow between permanent and transient sizes?
  //   1-common for user to mostly be working at one size, but temporarily change (eg zoom in).
  //     We should attempt to not throw away the 'main' size
  //   2-intermediate sizes are used during zooming that are short lived.

  //--------------------

  //siesmix peakfiles are 16bit int with no header.

  //what is the format of the ardour peak files?
  //-looks like they are 4 bytes per sample. float? how stored in file?


  if(!arrange->part_contents_show) return;

  if((int)part<1024){printf("%s peak2pic(): bad part ref: %p\n", smerr, part); return;}
  //int p = part->idx;
  //if(p>dsong->max_prt){printf("%s peak2pic(): bad part number: %i\n", smerr, p); return;}
  unsigned int p_obj_idx = part->objidx;
  if(p_obj_idx>dsong->max_slots){errprintf("%s peak2pic(): bad pobjidx: %i\n", p_obj_idx); return;}

  gboolean dont_draw = FALSE; //experimental. Perhaps this is more versatile than returning imediately?
                              //what to do if no pic is needed? answer depends on the caching solution.

  //int width = part->length * arrange->hzoom -2; //width is the part width in pixels (excluding borders)
  int width = part_end_px(part) - gpart_start_px(part) -2; //width is the part width in pixels (excluding borders)
  if(width<4){
    printf("peak2pic(): too small to draw peak: width=%i.\n", width); 
	if(part->peak_item) gnome_canvas_item_hide(part->peak_item);
	return;
  }

  int tracknum = part->track;
  int t        = part->track;
  if(tracknum >= MAX_GTRKS){printf("%s peak2pic(): track num out of range: %i\n", smerr, t); return;}
  int height = trkA[tracknum]->height * arrange->vzoom - 3 - 10;// -2*borderwidth -labelheight
  if(height < MIN_OVERVIEW_HEIGHT){ dont_draw = TRUE;}
  printf("peak2pic(): %i: track=%i height=%i NO LONGER BEING USED.\n", part->idx, t, height);

  //temporary pixmap to draw onto (is destroyed below):
  GdkPixmap *pixmap = gdk_pixmap_new(NULL,          //drawable
                                     width, height, //width, height
                                     display_depth);//number of bits per pixel

  //set background colour:
  GdkColor color;
  color.red   = song->palette.red[part->colour];
  color.green = song->palette.grn[part->colour];
  color.blue  = song->palette.blu[part->colour];

  GdkColormap *colormap = gdk_colormap_get_system();
  if(colormap==NULL){ errprintf("peak2pic(): colormap NULL.\n"); return;}
  //part->label_colormap = colormap;
  gdk_drawable_set_colormap(GDK_DRAWABLE(pixmap), colormap);

  GdkGC *gc = gdk_gc_new(GDK_DRAWABLE(pixmap)); //default settings.
  //gdk_gc_set_foreground(gc, &color);
  gdk_gc_set_rgb_fg_color(gc, &color);//this avoids having to 'allocate' the color.

  //clear the pixmap:
  gdk_draw_rectangle(GDK_DRAWABLE(pixmap), gc, TRUE, 0,0, width,height);
  
  //set foreground colour:
  //color.red   = 0x0000;
  color.red   = song->palette.red[56];
  color.green = song->palette.grn[56];
  color.blue  = song->palette.blu[56];
  //color = {0,0,0,0};
  gdk_gc_set_rgb_fg_color(gc, &color);

  //-------------------------------------------------------------------

  //draw the waveform onto the drawable:
 
  if(!dont_draw){
    float onepx = beats2samples(1) / PX_PER_BEAT; //units = samples.
    //printf("peak2pic(): 1px==%.0fsamples\n", onepx);

    double xmag = onepx / arrange->hzoom;   //constant multiplier setting horizontal magnification.

    peak_render_pixmap(pixmap, part->pool_item, part_get_region(part), gc, xmag * BODGEEEE);

    //put the pixmap onto a pixbuf:
    //printf("peak2pic(): pixmap2pixbuf...\n");
    if(part->peak_pixbuf != 0){
      //i dont think pixbufs can be resized, so we delete the old one and start again:
      //printf("text2pic(): deleting existing pixbuf...\n");
      gdk_pixbuf_unref(part->peak_pixbuf);
    }
    //else printf("pixbuf = 0!\n");
    //printf("text2pic(): width=%d\n", width);
    part->peak_pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB,
      									//FALSE,  //TRUE, //gboolean has_alpha
      									TRUE,   //gboolean has_alpha
										8,      //int bits_per_sample
	      								width,height);    //int width, int height)
    gdk_pixbuf_get_from_drawable(part->peak_pixbuf, GDK_DRAWABLE(pixmap), colormap, 
  								0,0,            //int src_x, int src_y
	    						0,0, 	        //int dest_x, int dest_y
								width, height); //int width, int height
 
    //test transparency (yes it works):
    //part->peak_pixbuf = gdk_pixbuf_add_alpha(part->peak_pixbuf, TRUE, 0,0,0); //makes a new pixbuf!

 
    //put the pixbuf onto the canvas:
    if(!part->peak_item){
      //this is a new part.
      //printf("peak2pic(): new canvas item. old=%d group=%d\n", GPOINTER_TO_INT(part->peak_item),
      //					GPOINTER_TO_INT(part->canvasgroup));
      part->peak_item = gnome_canvas_item_new(GNOME_CANVAS_GROUP(gnome_canvas_root(GNOME_CANVAS(arrange->canvas_widget))), 
                          gnome_canvas_pixbuf_get_type(),
  						  "x", 1.0,
						  "y", 1.0,//(double)height/2 + 2.0 - 1.0,
						  "width", (double)width,
						  //"width-set", TRUE, //this causes the pixbuf to be scaled.
						  "height", (double)height,
						  "pixbuf", part->peak_pixbuf,
						  "anchor", GTK_ANCHOR_NORTH_WEST,
  						  NULL);
    } else {
      //printf("peak2pic(): reusing old canvas item... width=%d\n", width);
	  gnome_canvas_item_set(part->peak_item, 
	  					  "pixbuf", part->peak_pixbuf, 
						  "width", (double)width, 
						  "y", 1.0, 
						  NULL);
    }
    gnome_canvas_item_show(part->peak_item);

  }
  else gnome_canvas_item_hide(part->peak_item);

  g_object_unref(pixmap);
  //printf("peak2pic(): done.\n");
}
    */


void
peak_render_pixmap(GdkPixmap *pixmap, struct _pool_item *pool_item, unsigned int region_idx, GdkGC *gc, double samples_per_px)
{
  //renders a peakfile (loaded into the buffer given by pool_item->buf) onto the given pixmap.
  //-pixmap must already be cleared, with the correct background colour.

  //samples_per_px sets the magnification.
  //-pool entries:   we scale to the given width using peak_render_pixmap_at_size().
  //-part overviews: we need to use the scale from the arrangezoom. 



  //FIXME move this gc_grey to an init fn:
  //set background colour:
  GdkColor color;
  color.red   = song->palette.red[57];
  color.green = song->palette.grn[57];
  color.blue  = song->palette.blu[57];

  GdkColormap *colormap = gdk_colormap_get_system();
  if(colormap==NULL){ errprintf("peak2pic(): colormap NULL.\n"); return;}
  //gdk_drawable_set_colormap(GDK_DRAWABLE(pixmap), colormap);

  GdkGC *gc_grey = gdk_gc_new(GDK_DRAWABLE(pixmap)); //default settings.
  //gdk_gc_set_foreground(gc, &color);
  gdk_gc_set_rgb_fg_color(gc_grey, &color);//this avoids having to 'allocate' the color.





  GdkGC *gc_line = gc;

  //access the songcore event:
  struct _audi_evr *ev = (struct _audi_evr*)&dcore->obj[region_idx].evt.ev;
  unsigned long sample_end = ev->sample_end;
  printf("peak_render_pixmap(): samples_end=%lu.\n", sample_end);

  //printf("peak_render_pixmap()...\n");
  if(samples_per_px<0.001) printf("%s peak_render_pixmap(): samples_per_pix=%f\n", smerr, samples_per_px);
  short sample_positive;    //even numbered bytes in the src peakfile are positive peaks;
  short sample_negative;    //odd  numbered bytes in the src peakfile are negative peaks;
  short min;                //negative peak value for each pixel.
  short max;                //positive peak value for each pixel.
  //struct _pool_item *pool_item = g_list_nth_data(song->pool, pool_idx);
  //short *buf = g_list_nth_data(song->peaklist, pkbuf_idx); //source buffer (4096 bytes atm).
  if((int)pool_item<1024)
                   {printf("%s peak_render_pixmap(): bad pool item ref: %p.\n", smerr,pool_item); return;}
  if(!pool_item->valid) return;
  if(!pool_item->fidx)
             {errprintf("%s peak_render_pixmap(): bad core file idx: %i.\n", pool_item->fidx); return;}

  short *buf = pool_item->buf; //source buffer.
  if((int)buf<1024){printf("%s peak_render_pixmap(): bad peak buf ref: %p.\n", smerr, buf); return;}
  //printf("peak2pic(): idx=%i peakbuff=%p\n", buf_idx, buf);
  int buflen = pool_item->buflen;

  int width, height;
  gdk_drawable_get_size(pixmap, &width, &height);
  //printf("peak_render_pixmap(): w=%i h=%i.\n", width, height);

  int px=0;                       //pixel count starting at the lhs of the Part.
  int j;
  int srcidx_start=0;             //index into the source buffer for each sample pt.
  int srcidx_stop =0;   

  double xmag = samples_per_px / (PEAK_RATIO * PEAK_VALUES_PER_SAMPLE);
                   //making xmag smaller increases the visual magnification.
                   //-the bigger the mag, the less samples we need to skip.
                   //-as we're only dealing with smaller peak files, we need to adjust by the RATIO.

  //printf("peak_render_pixmap(): xmag=%.2f srcbuflen=%i\n", xmag, buflen);
  //if(tracknum==0) printf("height=%i size=%i\n", height, sizeof(*buf));

  //check we have enough peak data for this zoom level:
  //printf("peak2pic(): start=%i stop=%i\n", (int)(xmag * px), (int)(xmag * (px+1)));
#ifdef LATER
  if((int)(xmag * px) == (int)(xmag * (px+1))) 
                  printf("%s peak_render_pixmap(): zoom is too large for peakfile. mag=%.1f\n",smwarn,xmag); 
#endif
  //printf("peak_render_pixmap(): pool_item: %s\n", pool_item->leafname);
  //printf("peak_render_pixmap(): (width=%i) bufwid=%i maxidx=%i\n", width, buflen, (int)(xmag*width));

  int line_done_count = 0; //for debugging only.
  for(px=0;px<width;px++){
    srcidx_start = ((int)(xmag * px));
    srcidx_stop  = ((int)(xmag * (px+1)));

    if(srcidx_stop<buflen/2){

      if(FALSE) gc_line = gc_grey; //region has ended. switched to 'greyed out' colour.

      //if(!(srcidx % 2)) srcidx++; //not strictly correct!!
      min = 0; max = 0;
      for(j=srcidx_start;j<srcidx_stop;j++){ //iterate over all the source samples for this pixel.
        sample_positive = buf[2*j   ];
        sample_negative = buf[2*j +1];
        if(sample_positive > max) max = sample_positive;
	    if(sample_negative < min) min = sample_negative;
	  }
      //sample = buf[srcidx];
      //if(sample < -32768 || sample > 32768) printf("out of range!sample=%i\n", sample);
      //val = (height * song->peak[i]) / 256;
      //val = (height * buf[i]) / 256*256*256*256;
      //val = (height * sample) / (256*128*2);//(sizeof(sample) ^ 8);

      //scale the values to the part height:
      min = (height * min) / (256*128*2);
      max = (height * max) / (256*128*2);

      //if((tracknum==0) && (i % 5 == 0)) printf("buf=%i val=%i\n", sample, val);
      gdk_draw_line(GDK_DRAWABLE(pixmap), gc_line, px, height/2+min, px, height/2 + max);//x1, y1, x2, y2
      line_done_count++;
    
    }else{
      //no more source data available - as the pixmap is clear, we have nothing much to do.
      gdk_draw_line(GDK_DRAWABLE(pixmap), gc, px, 0, px, height);//x1, y1, x2, y2
      line_done_count++;
    }
	//next = srcidx + 1;
  }

  //printf("peak_render_pixmap(): done. drawn: %i of %i\n", line_done_count, width);
}


void
peak_render_pixmap_at_size(GdkPixmap *pixmap, struct _pool_item *pool_item, unsigned int region_idx, GdkGC *gc, int width)
{
  //calculates the magnification required to call peak_render_pixmap() for a fixed size pixmap.

  if(!pool_item->buflen) printf("%s peak_render_pixmap_at_size(): buflen not set.\n", smerr);
  double sample_count = pool_item->buflen;
  double samples_per_px = (sample_count / width) * ((PEAK_RATIO * PEAK_VALUES_PER_SAMPLE)) / 2;

  //printf("peak_render_pixmap_at_size(): samples_per_px=%.1f wid=%i len=%i\n", 
  //                                                     samples_per_px, width, pool_item->buflen);

  peak_render_pixmap(pixmap, pool_item, region_idx, gc, samples_per_px);
}


void
peak_render_to_pixbuf(GdkPixbuf *pixbuf, struct _pool_item *pool_item, unsigned int region_idx, double samples_per_px)
{
  //renders a peakfile (loaded into the buffer given by pool_item->buf) onto the given pixbuf.
  //-the pixbuf must already be cleared, with the correct background colour.

  //samples_per_px sets the magnification.
  //-pool entries:   we scale to the given width using peak_render_pixmap_at_size().
  //-part overviews: we need to use the scale from the arrangezoom. 



  //FIXME move this gc_grey to an init fn:
  //set background colour:
  GdkColor colour;
  colour.red   = song->palette.red[57];
  colour.green = song->palette.grn[57];
  colour.blue  = song->palette.blu[57];

  GdkColormap *colormap = gdk_colormap_get_system();
  if(colormap==NULL){ errprintf("peak2pic(): colormap NULL.\n"); return;}
  //gdk_drawable_set_colormap(GDK_DRAWABLE(pixmap), colormap);

  //GdkGC *gc_grey = gdk_gc_new(GDK_DRAWABLE(pixmap)); //default settings.
  //gdk_gc_set_foreground(gc, &color);
  //gdk_gc_set_rgb_fg_color(gc_grey, &color);//this avoids having to 'allocate' the color.




  //GdkGC *gc_line = gc;

  //access the songcore event:
  struct _audi_evr *ev = (struct _audi_evr*)&dcore->obj[region_idx].evt.ev;
  unsigned long sample_end = ev->sample_end;
  printf("peak_render_to_pixbuf(): samples_end=%lu.\n", sample_end);

  //printf("peak_render_to_pixbuf()...\n");
  if(samples_per_px<0.001) errprintf("peak_render_to_pixbuf(): samples_per_pix=%f\n", samples_per_px);
  short sample_positive;    //even numbered bytes in the src peakfile are positive peaks;
  short sample_negative;    //odd  numbered bytes in the src peakfile are negative peaks;
  short min;                //negative peak value for each pixel.
  short max;                //positive peak value for each pixel.
  //struct _pool_item *pool_item = g_list_nth_data(song->pool, pool_idx);
  //short *buf = g_list_nth_data(song->peaklist, pkbuf_idx); //source buffer (4096 bytes atm).
  if((int)pool_item<1024)
                   {errprintf("peak_render_to_pixbuf(): bad pool item ref: %p.\n", pool_item); return;}
  if(!pool_item->valid) return;
  if(!pool_item->fidx)
             {errprintf("peak_render_to_pixbuf(): bad core file idx: %i.\n", pool_item->fidx); return;}

  short *buf = pool_item->buf; //source buffer.
  if((int)buf<1024){ errprintf("peak_render_to_pixbuf(): bad peak buf ref: %p.\n", buf); return; }
  //printf("peak2pic(): idx=%i peakbuff=%p\n", buf_idx, buf);
  int buflen = pool_item->buflen;

  int width = gdk_pixbuf_get_width(pixbuf);
  int height= gdk_pixbuf_get_height(pixbuf);
  int px=0;                       //pixel count starting at the lhs of the Part.
  int j;
  int srcidx_start=0;             //index into the source buffer for each sample pt.
  int srcidx_stop =0;   

  double xmag = samples_per_px / (PEAK_RATIO * PEAK_VALUES_PER_SAMPLE);
                   //making xmag smaller increases the visual magnification.
                   //-the bigger the mag, the less samples we need to skip.
                   //-as we're only dealing with smaller peak files, we need to adjust by the RATIO.

  //printf("peak_render_to_pixbuf(): xmag=%.2f srcbuflen=%i\n", xmag, buflen);
  //if(tracknum==0) printf("height=%i size=%i\n", height, sizeof(*buf));

  //check we have enough peak data for this zoom level:
  //printf("peak2pic(): start=%i stop=%i\n", (int)(xmag * px), (int)(xmag * (px+1)));
#ifdef LATER
  if((int)(xmag * px) == (int)(xmag * (px+1))) 
                  printf("%s peak_render_to_pixbuf(): zoom is too large for peakfile. mag=%.1f\n",smwarn,xmag); 
#endif
  //printf("peak_render_to_pixbuf(): pool_item: %s\n", pool_item->leafname);
  //printf("peak_render_to_pixbuf(): (width=%i) bufwid=%i maxidx=%i\n", width, buflen, (int)(xmag*width));

  int line_done_count = 0; //for debugging only.
  for(px=0;px<width;px++){
    srcidx_start = ((int)(xmag * px));
    srcidx_stop  = ((int)(xmag * (px+1)));

    if(srcidx_stop<buflen/2){

      //if(FALSE) gc_line = gc_grey; //region has ended. switched to 'greyed out' colour.

      //if(!(srcidx % 2)) srcidx++; //not strictly correct!!
      min = 0; max = 0;
      for(j=srcidx_start;j<srcidx_stop;j++){ //iterate over all the source samples for this pixel.
        sample_positive = buf[2*j   ];
        sample_negative = buf[2*j +1];
        if(sample_positive > max) max = sample_positive;
	    if(sample_negative < min) min = sample_negative;
	  }
      //sample = buf[srcidx];
      //if(sample < -32768 || sample > 32768) printf("out of range!sample=%i\n", sample);
      //val = (height * song->peak[i]) / 256;
      //val = (height * buf[i]) / 256*256*256*256;
      //val = (height * sample) / (256*128*2);//(sizeof(sample) ^ 8);

      //scale the values to the part height:
      min = (height * min) / (256*128*2);
      max = (height * max) / (256*128*2);

      //if((tracknum==0) && (i % 5 == 0)) printf("buf=%i val=%i\n", sample, val);
      //gdk_draw_line(GDK_DRAWABLE(pixmap), gc_line, px, height/2+min, px, height/2 + max);//x1, y1, x2, y2
      struct _ArtDRect pts = {px, height/2+min, px, height/2 + max};
      pixbuf_draw_line(pixbuf, &pts, 1.0, &colour);

      line_done_count++;
    
    }else{
      //no more source data available - as the pixmap is clear, we have nothing much to do.
      //gdk_draw_line(GDK_DRAWABLE(pixmap), gc, px, 0, px, height);//x1, y1, x2, y2
      line_done_count++;
    }
	//next = srcidx + 1;
  }

  //printf("peak_render_pixmap(): done. drawn: %i of %i\n", line_done_count, width);
}


void
part_set_peak_slow(struct _gpart *gpart)
{
  //delete.

  //a replacement for peak2pic - renders indirectly onto the part pixbuf.
  printf("part_set_peak_slow()...\n");
  if(!arrange->part_contents_show) return;

  if((int)gpart<1024){ errprintf("part_set_peak(): bad part ref: %p\n", gpart); return;}
  unsigned int p_obj_idx = gpart->objidx;
  if(p_obj_idx>dsong->max_slots){errprintf("part_set_peak(): bad pobjidx: %i\n", p_obj_idx); return;}

  gboolean dont_draw = FALSE; //experimental. Perhaps this is more versatile than returning imediately?
                              //eg for freeing memory.
                              //what to do if no pic is needed? answer depends on the caching solution.

  GnomeCanvasPart *canvas_item = GNOME_CANVAS_PART(gpart->box);
  GdkPixbuf *pixbuf = canvas_item->pixbuf;
  //int width = part_end_px(part) - gpart_start_px(part) -2; //width is the part width in pixels (excluding borders)
  int width = gdk_pixbuf_get_width(pixbuf);
  if(width<4){
    printf("part_set_peak(): too small to draw peak: width=%i.\n", width); 
	//if(part->peak_item) gnome_canvas_item_hide(part->peak_item);
    dont_draw = TRUE;
	return;
  }

  int tracknum = gpart->track;
  int t        = gpart->track;
  if(tracknum >= MAX_GTRKS){ errprintf("part_set_peak(): track num out of range: %i\n", t); return;}
  int height = trkA[tracknum]->height * arrange->vzoom - 3 - 10;// -2*borderwidth -labelheight
  if(height < MIN_OVERVIEW_HEIGHT){ dont_draw = TRUE;}
  printf("part_set_peak(): %i: track=%i height=%i colour=%i\n", gpart->objidx, t, height, gpart->colour);

  //temporary pixmap to draw onto (is destroyed below):
  GdkPixmap *pixmap = gdk_pixmap_new(NULL,          //drawable
                                     width, height, //width, height
                                     display_depth);//number of bits per pixel

  //set background colour:
  GdkColor color;
  color.red   = song->palette.red[gpart->colour];
  color.green = song->palette.grn[gpart->colour];
  color.blue  = song->palette.blu[gpart->colour];

  GdkColormap *colormap = gdk_colormap_get_system();
  if(colormap==NULL){ errprintf("part_set_peak(): colormap NULL.\n"); return; }
  //part->label_colormap = colormap;
  gdk_drawable_set_colormap(GDK_DRAWABLE(pixmap), colormap);

  GdkGC *gc = gdk_gc_new(GDK_DRAWABLE(pixmap)); //default settings.
  //gdk_gc_set_foreground(gc, &color);
  gdk_gc_set_rgb_fg_color(gc, &color);//this avoids having to 'allocate' the color.

  //clear the pixmap:
  gdk_draw_rectangle(GDK_DRAWABLE(pixmap), gc, TRUE, 0,0, width,height);
  
  //set foreground colour:
  //color.red   = 0x0000;
  color.red   = song->palette.red[56];
  color.green = song->palette.grn[56];
  color.blue  = song->palette.blu[56];
  //color = {0,0,0,0};
  gdk_gc_set_rgb_fg_color(gc, &color);

  //-------------------------------------------------------------------

  //draw the waveform onto the drawable:
 
  if(!dont_draw){
    /*
    so lets get some proper x scaling happening.....

    -assume zoom = 1.
    -xmag=1 means 1 sample  per pixel.
    -xmag=2 means 2 samples per pixel.
    -mapping is defined by: PX_PER_BEAT.


    */
    float onepx = beats2samples(1) / PX_PER_BEAT; //units = samples.
    //printf("part_set_peak(): 1px==%.0fsamples\n", onepx);

    double xmag = onepx / arrange->hzoom;   //constant multiplier setting horizontal magnification.

	//FIXME we're sposed to be drawing onto the pixbuf directly!
    peak_render_pixmap(pixmap, gpart->pool_item, part_get_region(gpart), gc, xmag * BODGEEEE);

    //put the pixmap onto a pixbuf:
    //printf("part_set_peak(): pixmap2pixbuf...\n");
    /*
    if(part->peak_pixbuf != 0){
      //i dont think pixbufs can be resized, so we delete the old one and start again:
      //printf("text2pic(): deleting existing pixbuf...\n");
      gdk_pixbuf_unref(part->peak_pixbuf);
    }
    */
    //else printf("pixbuf = 0!\n");
    //printf("text2pic(): width=%d\n", width);

    /*
    part->peak_pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB,
      									//FALSE,  //TRUE, //gboolean has_alpha
      									TRUE,   //gboolean has_alpha
										8,      //int bits_per_sample
	      								width,height);    //int width, int height)
    */

    //copy the pixmap onto our canvas pixbuf....

	//note: this is s *copy* operation.
    GdkPixbuf* src_pixbuf = NULL;
    src_pixbuf = gdk_pixbuf_get_from_drawable(src_pixbuf, GDK_DRAWABLE(pixmap), colormap, 
  								0,0,            //int src_x, int src_y
	    						0,0, 	        //int dest_x, int dest_y
								-1, -1); //int width, int height
								//width, height); //int width, int height
    if(!src_pixbuf){ errprintf("part_set_peak(): pixbuf creation failed.\n"); }
    else{
	  //-note: gdk_pixbuf_new_from_data() does not copy any data.
	  /*
	  GdkPixbuf* dest_pixbuf = gdk_pixbuf_new_from_data(pixbuf,
							  GDK_COLORSPACE_RGB,
							  FALSE,
							  8,
							  buf->rect.x1 - buf->rect.x0,
							  buf->rect.y1 - buf->rect.y0,
							  buf->buf_rowstride,
							  NULL, NULL);
	  */
	  gdk_pixbuf_composite(src_pixbuf,
					 pixbuf,
					 0, 0,                   //int dest x,y
					 width, height,          //dest w,h
					 0, 0,                   //double offset (translation)
					 1.0, 1.0,               //double scale
					 GDK_INTERP_NEAREST,
					 255);
   
	  //test transparency (yes it works):
	  //part->peak_pixbuf = gdk_pixbuf_add_alpha(part->peak_pixbuf, TRUE, 0,0,0); //makes a new pixbuf!
    }
  }

  g_object_unref(pixmap);
  //printf("peak2pic(): done.\n");
}


void
part_set_peak(struct _gpart *gpart)
{
  //a replacement for peak2pic - renders straight onto the part pixbuf.
  printf("part_set_peak()...\n");
  if(!arrange->part_contents_show) return;

  if((int)gpart<1024){ errprintf("part_set_peak(): bad part ref: %p\n", gpart); return;}
  unsigned int p_obj_idx = gpart->objidx;
  if(p_obj_idx>dsong->max_slots){errprintf("part_set_peak(): bad pobjidx: %i\n", p_obj_idx); return;}

  gboolean draw = TRUE; //experimental. Perhaps this is more versatile than returning imediately?
                        //eg for freeing memory.
                        //what to do if no pic is needed? answer depends on the caching solution.

  GnomeCanvasPart *canvas_item = GNOME_CANVAS_PART(gpart->box);
  if((int)canvas_item<1024){ errprintf("part_set_peak(): bad canvas item ref: %p\n", canvas_item); return;}
  GdkPixbuf *pixbuf = canvas_item->pixbuf;
  //int width = part_end_px(part) - gpart_start_px(part) -2; //width is the part width in pixels (excluding borders)
  int width = gdk_pixbuf_get_width(pixbuf);
  if(width<4){
    printf("part_set_peak(): too small to draw peak: width=%i.\n", width); 
	//if(part->peak_item) gnome_canvas_item_hide(part->peak_item);
    draw = FALSE;
	return;
  }

  int tracknum = gpart->track;
  int t        = gpart->track;
  if(tracknum >= MAX_GTRKS){ errprintf("part_set_peak(): track num out of range: %i\n", t); return;}
  int height = trkA[tracknum]->height * arrange->vzoom - 3 - 10;// -2*borderwidth -labelheight
  if(height < MIN_OVERVIEW_HEIGHT){ draw = FALSE;}
  //printf("part_set_peak(): %i: track=%i height=%i colour=%i\n", gpart->objidx, t, height, gpart->colour);

  //set background colour:
  GdkColor colour;
  colour.red   = song->palette.red[gpart->colour];
  colour.green = song->palette.grn[gpart->colour];
  colour.blue  = song->palette.blu[gpart->colour];

  //FIXME remove gc stuff:
  GdkColormap *colormap = gdk_colormap_get_system();
  if(colormap==NULL){ errprintf("part_set_peak(): colormap NULL.\n"); return; }
  //part->label_colormap = colormap;
  //gdk_drawable_set_colormap(GDK_DRAWABLE(pixmap), colormap);

  //GdkGC *gc = gdk_gc_new(GDK_DRAWABLE(pixmap)); //default settings.
  //gdk_gc_set_foreground(gc, &color);
  //gdk_gc_set_rgb_fg_color(gc, &color);//this avoids having to 'allocate' the color.

  //clear the pixmap:
  struct _ArtDRect pts = {0.0,0.0,0.0,0.0};
  pixbuf_draw_rect(pixbuf, &pts, &colour);
  
  //set foreground colour:
  //color.red   = 0x0000;
  colour.red   = song->palette.red[56];
  colour.green = song->palette.grn[56];
  colour.blue  = song->palette.blu[56];
  //color = {0,0,0,0};
  //gdk_gc_set_rgb_fg_color(gc, &colour);

  //-------------------------------------------------------------------

  //draw the waveform directly onto the pixbuf:
 
  if(draw){
    /*
    so lets get some proper x scaling happening.....

    -assume zoom = 1.
    -xmag=1 means 1 sample  per pixel.
    -xmag=2 means 2 samples per pixel.
    -mapping is defined by: PX_PER_BEAT.


    */
    float onepx = beats2samples(1) / PX_PER_BEAT; //units = samples.
    //printf("part_set_peak(): 1px==%.0fsamples\n", onepx);

    double xmag = onepx / arrange->hzoom;   //constant multiplier setting horizontal magnification.

    peak_render_to_pixbuf(pixbuf, gpart->pool_item, part_get_region(gpart), xmag * BODGEEEE);
  }

  //printf("part_set_peak(): done.\n");
}


void
peak_view_toggle(GtkMenuItem *menuitem, gpointer user_data)
{
  printf("peak_view_toggle()...\n");

  if(arrange->part_contents_show){
    arrange->part_contents_show = FALSE;
	//hide all the peak canvas items:
	int p;
	struct _gpart *part;
	for(p=0;p<g_list_length(partlist);p++){
	  part = g_list_nth_data(partlist, p);
	  gnome_canvas_item_hide(part->peak_item);
	}
  } else {
    arrange->part_contents_show = TRUE;
	//the canvas items seem to be displayed without us doing anything here.
  }
  arr_canvas_redraw();
}


void
peak_name(char *peakname, char *filename)
{
  //currently we assume that the peakfile is in the same directory as the audio file,
  //and that it has the name extension .peak.

  //more versatile options will be added later.

  snprintf(peakname, 256, "%s.peak", filename);
}


