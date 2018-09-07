/*
** Copyright (C) 2007-2009 Erik de Castro Lopo <erikd@mega-nerd.com>
**
** modified for samplecat by Tim Orford and Robin Gareus
** based on sndfile-tools-1.03
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
#include <stdint.h>

#include <sndfile.h>
#include <cairo.h>
#include <fftw3.h>
#include <gtk/gtk.h>
#include <glib/gstdio.h>

#include "debug/debug.h"
#include "decoder/ad.h"

#include "audio_analysis/spectrogram/spectrogram.h"
#include "audio_analysis/spectrogram/sndfile_window.h"

extern gchar* str_replace (const gchar* string, const gchar* search, const gchar* replace);

#define _g_object_unref0(var) ((var == NULL) ? NULL : (var = (g_object_unref (var), NULL)))
#define call(FN, A, ...) if(FN) (FN)(A, ##__VA_ARGS__)

typedef void (*SpectrogramReadyTarget)(const char* filename, GdkPixbuf*, gpointer, void* target);

#define SG_WIDTH  (1024)
#define SG_HEIGHT (364)
#define MAX_HEIGHT (2048) // due to speclen depending on sample-rate, max samplerate that can be analysed is MAX_HEIGHT*100 so this value allows 192k
#define SPEC_FLOOR_DB -120.0


typedef struct
{
	const char*        sndfilepath;
	GdkPixbuf*         pixbuf;
	SpectrogramReady   callback;
	gpointer           user_data;
} RENDER;


static int
get_colour_map_value (float value, double spec_floor_db, unsigned char colour [3])
{
	static unsigned char map [][3] = {
		/* These values were originally calculated for a dynamic range of 180dB. */
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
	};

	g_return_val_if_fail(!isnan(value), -1);

	if (!(value < 0.0)) { // this also checks for NaN
		colour = map [0];
		return 0;
	};

	value = fabs (value * (-180.0 / spec_floor_db) * 0.1);

	int indx = rintf (floorf (value));

	if (indx < 0) {
		printf ("\nError: colour map array index is %d\n\n", indx);
		return -1;
	};

	if (indx >= G_N_ELEMENTS (map) - 1) {
		colour = map [G_N_ELEMENTS (map) - 1];
		return 0;
	};

	float rem = fmod (value, 1.0);

	colour [0] = lrintf ((1.0 - rem) * map [indx][0] + rem * map [indx + 1][0]);
	colour [1] = lrintf ((1.0 - rem) * map [indx][1] + rem * map [indx + 1][1]);
	colour [2] = lrintf ((1.0 - rem) * map [indx][2] + rem * map [indx + 1][2]);

	return 0;
}


static int
read_mono_audio (WfDecoder* file, double* data, int datalen, int index, int total)
{
	int rv = 0;
	int64_t start = (index * file->info.frames) / total - datalen / 2;
	int i=0; for(i=0;i<datalen;i++) data[i]=0;

	if (start >= 0) {
		if(ad_seek (file, start) < 0) return rv;
	} else {
		start = -start;
		if(ad_seek (file, 0) < 0) return rv;
		data += start;
		datalen -= start;
	}
	ad_read_mono_dbl(file, data, datalen);
	return rv;
} /* read_mono_audio */


static int
apply_window (double * data, int datalen)
{
	static double window [10 * MAX_HEIGHT];
	static int window_len = 0;
	int k;

	if (window_len != datalen)
	{
		window_len = datalen;
		if (datalen > G_N_ELEMENTS (window))
		{
			printf ("%s : datalen >  MAX_HEIGHT\n", __func__);
			return -1;
		}

		calc_kaiser_window (window, datalen, 20.0);
	}

	for (k = 0; k < datalen; k++)
		data [k] *= window [k];

	return 0;
} /* apply_window */


static double
calc_magnitude (const double * freq, int freqlen, double * magnitude)
{
	int k;
	double max = 0.0;

	for (k = 1; k < freqlen / 2; k++)
	{	magnitude [k] = sqrt (freq [k] * freq [k] + freq [freqlen - k - 1] * freq [freqlen - k - 1]);
		max = MAX (max, magnitude [k]);
		};
	magnitude [0] = 0.0;

	return max;
} /* calc_magnitude */


static int
_render_spectrogram_to_pixbuf (GdkPixbuf* pixbuf, double spec_floor_db, float mag2d [SG_WIDTH][MAX_HEIGHT], double maxval)
//_render_spectrogram_to_pixbuf (GdkPixbuf* pixbuf, double spec_floor_db, float** mag2d, double maxval)
{
	unsigned char colour [3] = { 0, 0, 0 };

	int stride = gdk_pixbuf_get_rowstride (pixbuf);
	int n_channels = gdk_pixbuf_get_n_channels(pixbuf);

	unsigned char* data = gdk_pixbuf_get_pixels(pixbuf);

	double linear_spec_floor = pow (10.0, spec_floor_db / 20.0);

	int w, h;
	for (w = 0; w < SG_WIDTH; w ++) {
		for (h = 0; h < SG_HEIGHT; h++) {
			mag2d [w][h] = mag2d [w][h] / maxval;
			mag2d [w][h] = (mag2d [w][h] < linear_spec_floor) ? spec_floor_db : 20.0 * log10 (mag2d [w][h]);

			if (get_colour_map_value (mag2d [w][h], spec_floor_db, colour))
				return -1;

			int y = SG_HEIGHT - 1 - h;
			int x = w * n_channels;

			data[y * stride + x + 0] = colour[0];
			data[y * stride + x + 1] = colour[1];
			data[y * stride + x + 2] = colour[2];

			if (n_channels == 4) 
				data [y * stride + x + 3] = 0;
		}
	}
	return 0;
}


static void
interp_spec (float * mag, int maglen, const double *spec, int speclen)
{
	int k, lastspec = 0;

	mag [0] = spec [0];

	for (k = 1; k < maglen; k++)
	{	double sum = 0.0;
		int count = 0;

		do
		{	sum += spec [lastspec];
			lastspec ++;
			count ++;
			}
		while (lastspec <= ceil ((k * speclen) / maglen));

		mag [k] = sum / count;
		};

	return;
} /* interp_spec */


static int
spectrogram_render_to_pixbuf (WfDecoder* infile, GdkPixbuf* pixbuf)
{
	int rv = 0; /* OK */

	static double time_domain [10 * MAX_HEIGHT];
	static double freq_domain [10 * MAX_HEIGHT];
	static double single_mag_spec [5 * MAX_HEIGHT];

	static float mag_spec [SG_WIDTH][MAX_HEIGHT];
	memset(mag_spec, 0, SG_WIDTH * MAX_HEIGHT * sizeof(float));

	double max_mag = 0;

	/*
	**	Choose a speclen value that is long enough to represent frequencies down
	**	to 20Hz, and then increase it slightly so it is a multiple of 0x40 so that
	**	FFTW calculations will be quicker.
	*/
	int speclen = SG_HEIGHT * (infile->info.sample_rate / 20 / SG_HEIGHT + 1);
	speclen += 0x40 - (speclen & 0x3f);

	if (2 * speclen > G_N_ELEMENTS (time_domain)) {
		printf ("%s : 2 * speclen > G_N_ELEMENTS (time_domain) (%d > %li)\n", __func__, 2 * speclen, G_N_ELEMENTS (time_domain));
		return -1;
	}

	fftw_plan plan = fftw_plan_r2r_1d (2 * speclen, time_domain, freq_domain, FFTW_R2HC, FFTW_MEASURE | FFTW_PRESERVE_INPUT);
	if (!plan) {
		printf ("%s : line %d : create plan failed.\n", __FILE__, __LINE__);
		return -1;
	}

	int w; for (w = 0; w < SG_WIDTH; w++) {
		if (read_mono_audio(infile, time_domain, 2 * speclen, w, SG_WIDTH) < 0) {
			dbg(1, "read failed before EOF");
			break;
		}

		if (apply_window (time_domain, 2 * speclen)) {
			rv = -1;
			break;
		}

		fftw_execute (plan);

		double single_max = calc_magnitude (freq_domain, 2 * speclen, single_mag_spec);
		max_mag = MAX (max_mag, single_max);

		interp_spec (mag_spec[w], SG_HEIGHT, single_mag_spec, speclen);
	}

	/* FIXME: there's some worm in here - on OSX i386 max_mag can become NaN
	 * it's also been seen but more rarely on OSX x86_64.
	 * -> image creation fails w/ "colour map array index < 0"
	 */

	fftw_destroy_plan (plan);

	if (!rv){
		if(!(max_mag > 0.0)){
			dbg(1, "no signal");
			return -1;
		}

		rv = _render_spectrogram_to_pixbuf (pixbuf, SPEC_FLOOR_DB, mag_spec, max_mag);
	}

	return rv;
}

#if 0
static void
check_int_range (const char * name, int value, int lower, int upper)
{
	if (value < lower || value > upper)
	{	printf ("Error : '%s' parameter must be in range [%d, %d]\n", name, lower, upper);
		exit (1); // XXX we must not exit samplecat !
		};
} /* check_int_range */
#endif


/* END COPY OF sndfile-spectrogram.c */

static GdkPixbuf*
spectrogram_render_sndfile (const char* file)
{
	WfDecoder decoder = {{0,}};

	if(!ad_open(&decoder, file)){
		dbg(1, "cannot open file: %s", file);
		return NULL;
	};

	GdkPixbuf* pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, /*alpha*/ FALSE, 8, SG_WIDTH, SG_HEIGHT);
	if (spectrogram_render_to_pixbuf(&decoder, pixbuf)) {
		dbg(0, "failed to create pixbuf for: %s", file);
		_g_object_unref0(pixbuf);
	}

	ad_close(&decoder);
	ad_free_nfo(&decoder.info);
	return pixbuf;
}

/*************************
 * SampleCat render queue
 */

/* TODO: merge all msg queue code with src/overview.c 
 * but make sure to put it on top of the queue.
 */

RENDER*
render_new()
{
	RENDER* r = g_new0(RENDER, 1);
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
				if(_debug_ && !render->pixbuf) gwarn("no pixbuf.");

				render->callback(render->sndfilepath, render->pixbuf, render->user_data);

				render_free(render);
				return G_SOURCE_REMOVE;
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

			dbg(1, "filename=%s.", render->sndfilepath);
			render->pixbuf = spectrogram_render_sndfile(render->sndfilepath); /* < this does the actual work */

			//check the completed render wasnt cancelled in the meantime
			gboolean got_cancel = false;
			process_messages(&got_cancel);

			if(!got_cancel && render->pixbuf){
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
		msg_queue = g_async_queue_new();
		if(!g_thread_new("fft", fft_thread, NULL)){
			perr("failed to create fft thread\n");
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

	//render->filename = strrchr (render->sndfilepath, '/'); 
	//render->filename = (render->filename != NULL) ? render->filename + 1 : render->sndfilepath;

	Message* m = message_new(QUEUE);
	m->render = render;

	dbg(1, "%s", render->sndfilepath);
	send_message(m);
}

/* SampleCat API */

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
	typedef struct
	{
		SpectrogramReady callback;
		void*            user_data;
	} Closure;

	gchar* cache_dir = g_build_filename(g_get_home_dir(), ".config", PACKAGE, "cache", "spectrogram", NULL);
	g_mkdir_with_parents(cache_dir, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP);

	gchar* make_cache_name(const char* path)
	{
		gchar* name = str_replace(path, "/", "___");
		gchar* name2 = g_strdup_printf("%s.png", name);
		gchar* cache_path = g_build_filename(g_get_home_dir(), ".config", PACKAGE, "cache", "spectrogram", name2, NULL);
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
		Closure* closure = g_new0(Closure, 1);
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
				#define MAX_CACHE_ITEMS 100 // files are ~ 700kBytes
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
				gchar* cache_dir = g_build_filename(g_get_home_dir(), ".config", PACKAGE, "cache", "spectrogram", NULL);

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

			Closure* closure = user_data;

			if(closure) call(closure->callback, filename, pixbuf, closure->user_data);

			g_free(closure);
		}

		render_spectrogram(path, spectrogram_ready, closure);
	}
	g_free(cache_dir);
	g_free(cache_path);
}


/*
 *  get_spectrogram_with_target provides an extra argument for api compatibility with vala code.
 */
#if 0
void
get_spectrogram_with_target(const char* path, SpectrogramReady callback, void* target, gpointer user_data)
{
	typedef struct
	{
		SpectrogramReadyTarget callback;
		gpointer               user_data;
		void*                  target;
	} TargetClosure;

	TargetClosure* closure = g_new0(TargetClosure, 1);
	*closure = (TargetClosure){
		.callback = (SpectrogramReadyTarget)callback,
		.user_data = user_data,
		.target = target
	};

	void ready(const char* filename, GdkPixbuf* pixbuf, gpointer __closure)
	{
		TargetClosure* closure = (TargetClosure*)__closure;
		closure->callback(filename, pixbuf, closure->user_data, closure->target);
		g_free(closure);
	}

	get_spectrogram(path, ready, closure);
}
#endif

