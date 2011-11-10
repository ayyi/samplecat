#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <gtk/gtk.h>

#include "typedefs.h"
#include "support.h"
#include "audio_decoder/ad_plugin.h"

void *ad_open_null(const char *f, struct adinfo *n) { return NULL; }
int ad_close_null(void *x) { return -1; }
int ad_info_null(void *x, struct adinfo *n) { return -1; }
int64_t ad_seek_null(void *x, int64_t p) { return -1; }
ssize_t ad_read_null(void *x, double*d, size_t s) { return -1;}

typedef struct {
	ad_plugin const *b; ///< decoder back-end
	void *d; ///< backend data
} adecoder;

/* samplecat api */

void ad_init() { /* global init */ }

void *ad_open(const char *fn, struct adinfo *nfo) {
	adecoder *d = (adecoder*) calloc(1, sizeof(adecoder));

	/* TODO find /best/ decoder backend for given file */
#ifdef HAVE_FFMPEG
	d->b = get_ffmpeg();
#elif 0 // (defined HAVE_FLAC)
	d->b = get_libflac();
#else
	d->b = get_sndfile();
#endif

	d->d = d->b->open(fn, nfo);
	if (!d->d) {
		free(d);
		return NULL;
	}
	return (void*)d;
}
int ad_info(void *sf, struct adinfo *nfo) {
	adecoder *d = (adecoder*) sf;
	return d->b->info(d->d, nfo)?true:false;
}
int ad_close(void *sf) {
	adecoder *d = (adecoder*) sf;
	int rv = d->b->close(d->d);
	free(d);
	return rv;
}
int64_t ad_seek(void *sf, int64_t pos) {
	adecoder *d = (adecoder*) sf;
	return d->b->seek(d->d, pos);
}
ssize_t ad_read(void *sf, double* out, size_t len){
	adecoder *d = (adecoder*) sf;
	return d->b->read_dbl(d->d, out, len);
}

/* why default double* anyway -- legagy reasons are not good enough! 
 * TODO change API: ad_read() uses float
 */
ssize_t ad_read_float(void *sf, float* d, size_t len){
	int f;
	static double *buf = NULL;
	static size_t bufsiz = 0;
	if (!buf || bufsiz != len) {
		bufsiz=len;
		buf = (double*) realloc((void*)buf, bufsiz * sizeof(double));
	}
	len = ad_read(sf, buf, bufsiz);
	for (f=0;f<len;f++) {
		const float val = (float) buf[f];
		d[f] = val;
	}
	return len;
}

ssize_t ad_read_mono(void *sf, double* d, size_t len){
	struct adinfo nfo;
	ad_info(sf, &nfo);
	int chn = nfo.channels;
	if(chn == 1)
		return ad_read(sf, d, len);

	int c,f;

	static double *buf = NULL;
	static size_t bufsiz = 0;
	if (!buf || bufsiz != len*chn) {
		bufsiz=len*chn;
		buf = (double*) realloc((void*)buf, bufsiz * sizeof(double));
	}

	len = ad_read(sf, buf, bufsiz);

	for (f=0;f<len;f++) {
		double val=0.0;
		for (c=0;c<chn;c++) {
			val+=buf[f*chn + c];
		}
		d[f] = val/chn;
	}
	return len;
}

gboolean ad_finfo (const char *fn, struct adinfo *nfo) {
	void * sf = ad_open(fn, nfo);
	return ad_close(sf)?false:true;
}
