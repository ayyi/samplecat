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

/* samplecat api */

static ad_plugin const * backend =NULL;

void ad_init() {
	// TODO: dynamically select backends depending on file on ad_open!
	// store backend in opaque structure per decoder -> re-entrant code
#ifdef HAVE_FFMPEG
	backend = get_ffmpeg();
#else
	backend = get_sndfile();
#endif
}

void *ad_open(const char *fn, struct adinfo *nfo) {
	return backend->open(fn, nfo);
}
int ad_info(void *sf, struct adinfo *nfo) {
	return backend->info(sf, nfo)?true:false;
}
int ad_close(void *sf) {
	return backend->close(sf);
}
int64_t ad_seek(void *sf, int64_t pos) {
	return backend->seek(sf, pos);
}
ssize_t ad_read(void *sf, double* d, size_t len){
	return backend->read_dbl(sf, d, len);
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
		return backend->read_dbl(sf, d, len);

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

/* XXX this should be moved to audio_analyzers/peak/peak.c  */
double ad_maxsignal(const char *fn) {
	struct adinfo nfo;
	void * sf = ad_open(fn, &nfo);
	if (!sf) return 0.0;

	int     read_len = 1024 * nfo.channels;
	double* sf_data = g_malloc(sizeof(*sf_data) * read_len);

	int readcount;
	double max_val = 0.0;
	do {
		readcount = ad_read(sf, sf_data, read_len);
		int k;
		for (k = 0; k < readcount; k++){
			const double temp = fabs (sf_data [k]);
			max_val = temp > max_val ? temp : max_val;
		};
	} while (readcount > 0);

	ad_close(sf);
	g_free(sf_data);
	return max_val;
}
