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

int     ad_eval_null(const char *f) { return -1; }
void *  ad_open_null(const char *f, struct adinfo *n) { return NULL; }
int     ad_close_null(void *x) { return -1; }
int     ad_info_null(void *x, struct adinfo *n) { return -1; }
int64_t ad_seek_null(void *x, int64_t p) { return -1; }
ssize_t ad_read_null(void *x, float*d, size_t s) { return -1;}

typedef struct {
	ad_plugin const *b; ///< decoder back-end
	void *d; ///< backend data
} adecoder;

/* samplecat api */

void ad_init() { /* global init */ }

ad_plugin const * choose_backend(const char *fn) {
	int max, val;
	ad_plugin const *b=NULL;
	max=0;

	val=get_sndfile()->eval(fn);
	if (val>max) {max=val; b=get_sndfile();}

	val=get_ffmpeg()->eval(fn);
	if (val>max) {max=val; b=get_ffmpeg();}

	val=get_libflac()->eval(fn);
	if (val>max) {max=val; b=get_libflac();}

	return b;
}

void *ad_open(const char *fn, struct adinfo *nfo) {
	adecoder *d = (adecoder*) calloc(1, sizeof(adecoder));
	ad_clear_nfo(nfo);

	d->b = choose_backend(fn);
	if (!d->b) {
		dbg(0, "fatal: no decoder backend available");
		free(d);
		return NULL;
	}
	d->d = d->b->open(fn, nfo);
	if (!d->d) {
		free(d);
		return NULL;
	}
	return (void*)d;
}
int ad_info(void *sf, struct adinfo *nfo) {
	adecoder *d = (adecoder*) sf;
	if (!d) return -1;
	return d->b->info(d->d, nfo);
}
int ad_close(void *sf) {
	adecoder *d = (adecoder*) sf;
	if (!d) return -1;
	int rv = d->b->close(d->d);
	free(d);
	return rv;
}
int64_t ad_seek(void *sf, int64_t pos) {
	adecoder *d = (adecoder*) sf;
	if (!d) return -1;
	return d->b->seek(d->d, pos);
}
ssize_t ad_read(void *sf, float* out, size_t len){
	adecoder *d = (adecoder*) sf;
	if (!d) return -1;
	return d->b->read(d->d, out, len);
}

/*
 *  side-effects: allocates buffer
 */
ssize_t ad_read_mono_dbl(void *sf, struct adinfo *nfo, double* d, size_t len){
	int c,f;
	int chn = nfo->channels;
	if (len<1) return 0;

	static float *buf = NULL;
	static size_t bufsiz = 0;
	if (!buf || bufsiz != len*chn) {
		bufsiz=len*chn;
		buf = (float*) realloc((void*)buf, bufsiz * sizeof(float));
	}

	len = ad_read(sf, buf, bufsiz);

	for (f=0;f<len/chn;f++) {
		double val=0.0;
		for (c=0;c<chn;c++) {
			val+=buf[f*chn + c];
		}
		d[f]= val/chn;
	}
	return len/chn;
}


gboolean ad_finfo (const char *fn, struct adinfo *nfo) {
	ad_clear_nfo(nfo);
	void * sf = ad_open(fn, nfo);
	return ad_close(sf)?false:true;
}

void ad_clear_nfo(struct adinfo *nfo) {
	memset(nfo, 0, sizeof(struct adinfo));
}

void ad_free_nfo(struct adinfo *nfo) {
	if (nfo->meta_data) free(nfo->meta_data);
}

void dump_nfo(int dbglvl, struct adinfo *nfo) {
	dbg(dbglvl, "sample_rate: %u", nfo->sample_rate);
	dbg(dbglvl, "channels:    %u", nfo->channels);
	dbg(dbglvl, "length:      %"PRIi64" ms", nfo->length);
	dbg(dbglvl, "frames:      %"PRIi64, nfo->frames);
	dbg(dbglvl, "bit_rate:    %d", nfo->bit_rate);
	dbg(dbglvl, "bit_depth:   %d", nfo->bit_depth);
	dbg(dbglvl, "channels:    %u", nfo->channels);
	dbg(dbglvl, "meta-data:   %s", nfo->meta_data?nfo->meta_data:"-");
}
