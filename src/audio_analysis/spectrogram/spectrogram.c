/*
** Copyright (C) 2007-2009 Erik de Castro Lopo <erikd@mega-nerd.com>
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 2 or version 3 of the
** License.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
**	Generate a spectrogram as a PNG file from a given sound file.
*/

/*
**	Todo:
**      - Decouple height of image from FFT length. FFT length should be
*         greater than height and then interpolated to height.
**      - Make magnitude to colour mapper allow abitrary scaling (ie cmdline
**        arg).
**      - Better cmdline arg parsing and flexibility.
**      - Add option to do log frequency scale.
*/

#define _ISOC99_SOURCE
#define _XOPEN_SOURCE

#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <unistd.h>
#include <sys/stat.h>

#include <sndfile.h>
#include <cairo.h>
#include <fftw3.h>
#include <gtk/gtk.h>
#include <glib/gstdio.h>

#include "typedefs.h"
#include "support.h"

#include "audio_decoder/ad.h"
#include "audio_analysis/spectrogram/sndfile_window.h"
#include "main.h"

extern unsigned debug;

typedef void (*SpectrogramReady)(const char* filename, GdkPixbuf*, gpointer);
typedef void (*SpectrogramReadyTarget)(const char* filename, GdkPixbuf*, gpointer, void* target);

#define MIN_WIDTH 640
#define MIN_HEIGHT 480
#define MAX_WIDTH 8192
#define MAX_HEIGHT 4096

#define TICK_LEN 6
#define BORDER_LINE_WIDTH 1.8

#define TITLE_FONT_SIZE 20.0
#define NORMAL_FONT_SIZE 12.0

#define	LEFT_BORDER			65.0
#define	TOP_BORDER			30.0
#define	RIGHT_BORDER		75.0
#define	BOTTOM_BORDER		40.0

//#define	SPEC_FLOOR_DB		-180.0
#define	SPEC_FLOOR_DB		-90.0


typedef struct
{
	const char*        sndfilepath;
	const char*        filename;
	int                width, height;
	bool               border, log_freq;
	double             spec_floor_db;
	GdkPixbuf*         pixbuf;
	SpectrogramReady   callback;
	gpointer           user_data;
} RENDER;

typedef struct 
{
	int left, top, width, height;
} RECT;

static const char font_family [] = "Terminus";

static void
get_colour_map_value (float value, double spec_floor_db, unsigned char colour [3])
{	static unsigned char map [][3] =
	{	/* These values were originally calculated for a dynamic range of 180dB. */
		{	255, 255, 255 },  /* -0dB */
		{	240, 254, 216 },  /* -10dB */
		{	242, 251, 185 },  /* -20dB */
		{	253, 245, 143 },  /* -30dB */
		{	253, 200, 102 },  /* -40dB */
		{	252, 144,  66 },  /* -50dB */
		{	252,  75,  32 },  /* -60dB */
		{	237,  28,  41 },  /* -70dB */
		{	214,   3,  64 },  /* -80dB */
		{	183,   3, 101 },  /* -90dB */
		{	157,   3, 122 },  /* -100dB */
		{	122,   3, 126 },  /* -110dB */
		{	 80,   2, 110 },  /* -120dB */
		{	 45,   2,  89 },  /* -130dB */
		{	 19,   2,  70 },  /* -140dB */
		{	  1,   3,  53 },  /* -150dB */
		{	  1,   3,  37 },  /* -160dB */
		{	  1,   2,  19 },  /* -170dB */
		{	  0,   0,   0 },  /* -180dB */
	} ;

	float rem ;
	int indx ;

	if (value >= 0.0)
	{	colour = map [0] ;
		return;
	};

	value = fabs (value * (-180.0 / spec_floor_db) * 0.1);

	indx = lrintf (floor (value)) ;

	if (indx < 0) {
		printf ("\nError : colour map array index is %d\n\n", indx);
		exit (1) ; // XXX we must not exit samplecat !
	};

	if (indx >= G_N_ELEMENTS (map) - 1) {
		colour = map [G_N_ELEMENTS (map) - 1] ;
		return ;
	};

	rem = fmod (value, 1.0) ;

	colour [0] = lrintf ((1.0 - rem) * map [indx][0] + rem * map [indx + 1][0]) ;
	colour [1] = lrintf ((1.0 - rem) * map [indx][1] + rem * map [indx + 1][1]) ;
	colour [2] = lrintf ((1.0 - rem) * map [indx][2] + rem * map [indx + 1][2]) ;

	return ;
} /* get_colour_map_value */

static int
read_mono_audio (void * file, int64_t filelen, double * data, int datalen, int indx, int total)
{
	int rv = 0;
	int64_t start = (indx * filelen) / total - datalen / 2 ;
	memset (data, 0, datalen * sizeof (data [0])) ;

	if (start >= 0)
		ad_seek (file, start);
	else {
		start = -start;
		ad_seek (file, 0);
		data += start;
		datalen -= start;
	}

	int i;
	// TODO: statically allocate this buffer - realloc on demand
	float *tbuf = malloc(datalen*sizeof(float));
	if (ad_read_mono(file, tbuf, datalen) <1) rv=-1;
	for (i=0;i<datalen;i++) {
		const double val = tbuf[i];
		data[i] = val;
	}
	free(tbuf);
	return rv;
} /* read_mono_audio */

static void
apply_window (double * data, int datalen)
{
	static double window [10 * MAX_HEIGHT] ;
	static int window_len = 0 ;
	int k ;

	if (window_len != datalen)
	{
		window_len = datalen ;
		if (datalen > G_N_ELEMENTS (window))
		{
			printf ("%s : datalen >  MAX_HEIGHT\n", __func__) ;
			exit (1) ; // XXX we must not exit samplecat !
		} ;

		calc_kaiser_window (window, datalen, 20.0) ;
	} ;

	for (k = 0 ; k < datalen ; k++)
		data [k] *= window [k] ;

	return ;
} /* apply_window */

static double
calc_magnitude (const double * freq, int freqlen, double * magnitude)
{
	int k ;
	double max = 0.0 ;

	for (k = 1 ; k < freqlen / 2 ; k++)
	{	magnitude [k] = sqrt (freq [k] * freq [k] + freq [freqlen - k - 1] * freq [freqlen - k - 1]) ;
		max = MAX (max, magnitude [k]) ;
		} ;
	magnitude [0] = 0.0 ;

	return max ;
} /* calc_magnitude */


static void
_render_spectrogram_to_pixbuf (GdkPixbuf* pixbuf, double spec_floor_db, float mag2d [MAX_WIDTH][MAX_HEIGHT], double maxval, double left, double top, double width, double height)
{
	unsigned char colour [3] = { 0, 0, 0 } ;
	double linear_spec_floor ;

	int stride = gdk_pixbuf_get_rowstride (pixbuf);
	int n_channels = gdk_pixbuf_get_n_channels(pixbuf);

	unsigned char* data = gdk_pixbuf_get_pixels(pixbuf);

	linear_spec_floor = pow (10.0, spec_floor_db / 20.0) ;

	int w, h;
	for (w = 0 ; w < width ; w ++) {
		for (h = 0 ; h < height ; h++) {

			mag2d [w][h] = mag2d [w][h] / maxval ;
			mag2d [w][h] = (mag2d [w][h] < linear_spec_floor) ? spec_floor_db : 20.0 * log10 (mag2d [w][h]) ;

			get_colour_map_value (mag2d [w][h], spec_floor_db, colour);

			int y = height + top - 1 - h;
			int x = (w + left) * n_channels;
			data [y * stride + x + 0] = colour [2];
			data [y * stride + x + 1] = colour [1];
			data [y * stride + x + 2] = colour [0];
			//data [y * stride + x + 3] = 0;
		};
	}
}

static inline void
x_line (cairo_t * cr, double x, double y, double len)
{	cairo_move_to (cr, x, y) ;
	cairo_rel_line_to (cr, len, 0.0) ;
	cairo_stroke (cr) ;
} /* x_line */

static inline void
y_line (cairo_t * cr, double x, double y, double len)
{	cairo_move_to (cr, x, y) ;
	cairo_rel_line_to (cr, 0.0, len) ;
	cairo_stroke (cr) ;
} /* y_line */

typedef struct
{	double value [15] ;
	double distance [15] ;
} TICKS ;

static inline int
calculate_ticks (double max, double distance, TICKS * ticks)
{	const int div_array [] =
	{	10, 10, 8, 6, 8, 10, 6, 7, 8, 9, 10, 11, 12, 12, 7, 14, 8, 8, 9
		} ;

	double scale = 1.0, scale_max ;
	int k, leading, divisions ;

	if (max < 0)
	{	printf ("\nError in %s : max < 0\n\n", __func__) ;
		exit (1) ; // XXX we must not exit samplecat !
		} ;

	while (scale * max >= G_N_ELEMENTS (div_array))
		scale *= 0.1 ;

	while (scale * max < 1.0)
		scale *= 10.0 ;

	leading = lround (scale * max) ;
	divisions = div_array [leading % G_N_ELEMENTS (div_array)] ;

	/* Scale max down. */
	scale_max = leading / scale ;
	scale = scale_max / divisions ;

	if (divisions > G_N_ELEMENTS (ticks->value) - 1)
	{	printf ("Error : divisions (%d) > G_N_ELEMENTS (ticks->value) (%lu)\n", divisions, (long unsigned) G_N_ELEMENTS (ticks->value)) ;
		exit (1) ; // XXX we must not exit samplecat !
		} ;

	for (k = 0 ; k <= divisions ; k++)
	{	ticks->value [k] = k * scale ;
		ticks->distance [k] = distance * ticks->value [k] / max ;
		} ;

	return divisions + 1 ;
} /* calculate_ticks */

static void
interp_spec (float * mag, int maglen, const double *spec, int speclen)
{
	int k, lastspec = 0 ;

	mag [0] = spec [0] ;

	for (k = 1 ; k < maglen ; k++)
	{	double sum = 0.0 ;
		int count = 0 ;

		do
		{	sum += spec [lastspec] ;
			lastspec ++ ;
			count ++ ;
			}
		while (lastspec <= ceil ((k * speclen) / maglen)) ;

		mag [k] = sum / count ;
		} ;

	return ;
} /* interp_spec */

static void
render_to_pixbuf (const RENDER * render, void *infile, int samplerate, int64_t filelen, GdkPixbuf* pixbuf)
{
	static double time_domain [10 * MAX_HEIGHT];
	static double freq_domain [10 * MAX_HEIGHT];
	static double single_mag_spec [5 * MAX_HEIGHT];
	static float mag_spec [MAX_WIDTH][MAX_HEIGHT];

	fftw_plan plan;
	double max_mag = 0.0;
	int width, height, w, speclen;

	if (render->border) {
		width = lrint (gdk_pixbuf_get_width (pixbuf) - LEFT_BORDER - RIGHT_BORDER) ;
		height = lrint (gdk_pixbuf_get_height (pixbuf) - TOP_BORDER - BOTTOM_BORDER) ;
	}
	else
	{	width = render->width;
		height = render->height;
	}

	/*
	**	Choose a speclen value that is long enough to represent frequencies down
	**	to 20Hz, and then increase it slightly so it is a multiple of 0x40 so that
	**	FFTW calculations will be quicker.
	*/
	speclen = height * (samplerate / 20 / height + 1) ;
	speclen += 0x40 - (speclen & 0x3f) ;

	if (2 * speclen > G_N_ELEMENTS (time_domain)) {
		printf ("%s : 2 * speclen > G_N_ELEMENTS (time_domain)\n", __func__);
		exit (1); // XXX we must not exit samplecat !
	};

	plan = fftw_plan_r2r_1d (2 * speclen, time_domain, freq_domain, FFTW_R2HC, FFTW_MEASURE | FFTW_PRESERVE_INPUT) ;
	if (plan == NULL) {
		printf ("%s : line %d : create plan failed.\n", __FILE__, __LINE__);
		exit (1) ; // XXX we must not exit samplecat !
	};

	for (w = 0 ; w < width ; w++) {
		double single_max;

		if (read_mono_audio(infile, filelen, time_domain, 2 * speclen, w, width)) {
			dbg(1, "read failed before EOF");
			break;
		}

		apply_window (time_domain, 2 * speclen) ;

		fftw_execute (plan) ;

		single_max = calc_magnitude (freq_domain, 2 * speclen, single_mag_spec) ;
		max_mag = MAX (max_mag, single_max) ;

		interp_spec (mag_spec [w], height, single_mag_spec, speclen) ;
	};

	/* FIXME: there's some worm in here - on OSX i386 max_mag can become NaN
	 * it's also been seen but more rarely on OSX x86_64.
	 * -> image creation fails w/ "colour map array index < 0"
	 */

	fftw_destroy_plan (plan);

	_render_spectrogram_to_pixbuf (pixbuf, render->spec_floor_db, mag_spec, max_mag, 0, 0, width, height);

	return;
}


static GdkPixbuf*
render_pixbuf (const RENDER * render, void *infile, int samplerate, int64_t filelen)
{
	PF;
	/*
	**	CAIRO_FORMAT_RGB24 	 each pixel is a 32-bit quantity, with the upper 8 bits
	**	unused. Red, Green, and Blue are stored in the remaining 24 bits in that order.
	*/
	GdkPixbuf* pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, /*alpha*/ FALSE, 8, render->width, render->height);
	render_to_pixbuf (render, infile, samplerate, filelen, pixbuf);
	return pixbuf;
}


static GdkPixbuf*
render_sndfile (const RENDER* render)
{
	struct adinfo nfo;

	void* infile = ad_open (render->sndfilepath, &nfo);
	if (!infile) {
		return NULL;
	};

	GdkPixbuf* pixbuf = render_pixbuf(render, infile, nfo.sample_rate, nfo.frames);
	ad_close(infile);
	return pixbuf;
}


static void
check_int_range (const char * name, int value, int lower, int upper)
{
	if (value < lower || value > upper)
	{	printf ("Error : '%s' parameter must be in range [%d, %d]\n", name, lower, upper) ;
		exit (1) ; // XXX we must not exit samplecat !
		} ;
} /* check_int_range */


/*************************
 * SoundCat render queue
 */

RENDER*
render_new()
{
	RENDER* r = g_new0(RENDER, 1);
	r->spec_floor_db = SPEC_FLOOR_DB;
	return r;
}


void
render_free(RENDER* render)
{
	if(render->sndfilepath) g_free((char*)render->sndfilepath);
	g_free(render);
}

typedef enum
{
	QUEUE = 1,
	CANCEL,
} MsgType;

typedef struct _msg
{
	MsgType type;
	RENDER* render;
} Message;

static GAsyncQueue* msg_queue = NULL;

static Message*
message_new(MsgType type)
{
	Message* m = g_new0(Message, 1);
	m->type = type;
	return m;
}


static void
message_free(Message* message)
{
	g_free(message);
}


static gpointer
fft_thread(gpointer data)
{
	PF;
	g_async_queue_ref(msg_queue);

	GList* msg_list = NULL;
	while(1){
		//any new work ?
		void process_messages(gboolean* got_cancel)
		{
			*got_cancel = false;
			while(g_async_queue_length(msg_queue)){
				Message* message = g_async_queue_pop(msg_queue);
				dbg(1, "new message");
				switch(message->type){
					case QUEUE:
						msg_list = g_list_append(msg_list, message->render);
						break;
					case CANCEL:
						g_list_free(msg_list);
						msg_list = NULL;
						dbg(1, "queue cleared");
						*got_cancel = true;
						break;
					default:
						perr("bad message type");
						break;
				}
				message_free(message);
			}
		}
		gboolean got_cancel = false;
		process_messages(&got_cancel);

		while(msg_list){
			gboolean do_callback(gpointer _render)
			{
				RENDER* render = _render;
				if(debug && !render->pixbuf) gwarn("no pixbuf.");

				render->callback(render->sndfilepath, render->pixbuf, render->user_data);

				render_free(render);
				return TIMER_STOP;
			}

			RENDER* render = g_list_first(msg_list)->data;

			//check for cancels
			/*
			const char* path = render->sndfilepath;
			gboolean is_cancelled(const char* path)
			{
				GList* l = cancel_list;
				for(;l;l=l->next){
					if(!strcmp(path, l->data)){
						dbg(0, "is queued for cancel! %s", path);
						return true;
					}
				}
				return false;
			}
			if(is_cancelled(path)){
			}
			*/

			dbg(1, "filename=%s.", render->filename);
			render->pixbuf = render_sndfile(render);

			//check the completed render wasnt cancelled in the meantime
			gboolean got_cancel = false;
			process_messages(&got_cancel);

			if(!got_cancel){
				g_idle_add(do_callback, render);
			}else{
				render_free(render);
			}
			msg_list = g_list_remove(msg_list, render);
		}

		sleep(1); //FIXME this is a bit primitive - maybe the thread should have its own GMainLoop
		          //-even better is to use a blocking call on the async queue, waiting for messages.
	}
	return NULL;
}

static void
send_message(Message* msg)
{

	if(!msg_queue){
		dbg(2, "creating fft thread...");
		GError* error = NULL;
		msg_queue = g_async_queue_new();
		if(!g_thread_create(fft_thread, NULL, false, &error)){
			perr("error creating thread: %s\n", error->message);
			g_error_free(error);
		}
	}

	g_async_queue_push(msg_queue, msg);
}

void
render_spectrogram(const char* path, SpectrogramReady callback, gpointer user_data)
{
	g_return_if_fail(path);
	g_return_if_fail(callback);
	PF;

	#define no_border false

	RENDER* render = render_new();
	render->sndfilepath = g_strdup(path);
	render->callback = callback;
	render->user_data = user_data;

	//other rendering options:
	//render->spec_floor_db = -1.0 * fabs (fval); //dynamic range
	//render->log_freq = true;                    //log freq

	render->width = 640;
	render->height = 640;

	check_int_range ("width", render->width, MIN_WIDTH, MAX_WIDTH);
	check_int_range ("height", render->height, MIN_HEIGHT, MAX_HEIGHT);

	render->filename = strrchr (render->sndfilepath, '/'); 
	render->filename = (render->filename != NULL) ? render->filename + 1 : render->sndfilepath;

	Message* m = message_new(QUEUE);
	m->render = render;

	dbg(1, "%s", render->filename);
	send_message(m);
}

void
cancel_spectrogram(const char* path)
{
	// if path is NULL, cancel all pending spectrograms.
	// -actually we currently always cancel ALL preceding renders.

	Message* msg = message_new(CANCEL);
	send_message(msg);
}


void
get_spectrogram(const char* path, SpectrogramReady callback, gpointer user_data)
{
	gchar* cache_dir = g_build_filename(app.cache_dir, "spectrogram", NULL);
	g_mkdir_with_parents(cache_dir, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP);

	gchar* make_cache_name(const char* path)
	{
		gchar* name = str_replace(path, "/", "___");
		gchar* name2 = g_strdup_printf("%s.png", name);
		gchar* cache_path = g_build_filename(app.cache_dir, "spectrogram", name2, NULL);
		g_free(name);
		g_free(name2);
		return cache_path;
	}

	gchar* cache_path = make_cache_name(path);
	if(g_file_test(cache_path, G_FILE_TEST_IS_REGULAR)){
		dbg(1, "cache: hit");
		GError** error = NULL;
		GdkPixbuf* pixbuf = gdk_pixbuf_new_from_file(cache_path, error);
		//TODO unref?
		if(pixbuf){
			dbg(1, "cache: pixbuf loaded");
			callback(path, pixbuf, user_data);
		}
	}else{
		dbg(1, "cache: not found: %s", cache_path);
		typedef struct __closure
		{
			SpectrogramReady callback;
			void*            user_data;
		} _closure;
		_closure* closure = g_new0(_closure, 1);
		closure->callback = callback;
		closure->user_data = user_data;

		void spectrogram_ready(const char* filename, GdkPixbuf* pixbuf, gpointer user_data)
		{
			void maintain_cache(const char* cache_dir)
			{
				void delete_oldest_file(const char* cache_dir)
				{
					GError** error = NULL;
					GDir* dir = g_dir_open(cache_dir, 0, error);
					struct stat s;
					const char* f;
					struct _o {time_t time; char* name;} oldest = {0, NULL};
					while((f = g_dir_read_name(dir))){
						char* p = g_build_filename(cache_dir, f, NULL);
						if(g_stat(p, &s) == -1){
							dbg(0, "stat error! %s", f);
						}else{
							dbg(2, "time=%lu %s", s.st_mtime, f);
							if(!oldest.time || s.st_mtime < oldest.time){
								oldest.time = s.st_mtime;
								oldest.name = g_strdup(p);
							}
						}
						g_free(p);
					}
					g_dir_close(dir);

					if(oldest.name){
						dbg(2, "deleting: %s", oldest.name);
						g_unlink(oldest.name);
						g_free(oldest.name);
					}
				}

				dbg(2, "...");
				GError** error = NULL;
				GDir* dir = g_dir_open(cache_dir, 0, error);
				#define MAX_CACHE_ITEMS 100
				int n = 0;
				while(g_dir_read_name(dir)) n++;
				g_dir_close(dir);

				if(n >= MAX_CACHE_ITEMS){
					dbg(2, "cache full. removing oldest cache item...");
					delete_oldest_file(cache_dir);
				}
				else dbg(2, "cache_size=%i", n);
			}

			if(pixbuf){
				gchar* cache_dir = g_build_filename(app.cache_dir, "spectrogram", NULL);

				maintain_cache(cache_dir);

				gchar* filepath = make_cache_name(filename);
				dbg(2, "saving: %s", filepath);
				GError* error = NULL;
				if(!gdk_pixbuf_save(pixbuf, filepath, "png", &error, NULL)){
					P_GERR;
				}
				g_free(filepath);

				g_free(cache_dir);
			}

			_closure* closure = user_data;

			if(closure) call(closure->callback, filename, pixbuf, closure->user_data);

			g_free(closure);
		}

		render_spectrogram(path, spectrogram_ready, closure);
	}
	g_free(cache_dir);
	g_free(cache_path);
}

/* SampleCat API */

void
get_spectrogram_with_target(const char* path, SpectrogramReady callback, void* target, gpointer user_data)
{
	typedef struct _closure
	{
		SpectrogramReadyTarget callback;
		gpointer               user_data;
		void*                  target;
	} TargetClosure;
	struct _closure* closure = g_new0(struct _closure, 1);
	closure->callback = (SpectrogramReadyTarget)callback;
	closure->user_data = user_data;
	closure->target = target;

	void
	ready(const char* filename, GdkPixbuf* pixbuf, gpointer __closure)
	{
		struct _closure* closure = (struct _closure*)__closure;
		closure->callback(filename, pixbuf, closure->user_data, closure->target);
		g_free(closure);
	}

	get_spectrogram(path, ready, closure);
}
