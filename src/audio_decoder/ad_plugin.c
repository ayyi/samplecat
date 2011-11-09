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
ssize_t ad_read_null(void *x, double*d, size_t s) { return -1;}

/* samplecat api */

static ad_plugin const * backend =NULL;

void ad_init() {
	// TODO: dynamically use backends depending on file.
	backend = get_ffmpeg();
	//backend = get_sndfile();
}

void *ad_open(const char *fn, struct adinfo *nfo) {
	return backend->open(fn, nfo);
}
int ad_close(void *sf) {
	return backend->close(sf);
}
ssize_t ad_read(void *sf, double* d, size_t len){
	return backend->read_dbl(sf, d, len);
}

gboolean ad_info (const char *fn, struct adinfo *nfo) {
	void * sf = ad_open(fn, nfo);
	return ad_close(sf)?false:true;
}

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
