#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sndfile.h>

#include <gtk/gtk.h>
#include "typedefs.h"
#include "support.h"
#include "audio_decoder/ad_plugin.h"

/* internal abstraction */

void *ad_open_sndfile(const char *fn, struct adinfo *nfo) {
	SF_INFO sfinfo;
	SNDFILE *sffile;
	sfinfo.format=0;
	if(!(sffile = sf_open(fn, SFM_READ, &sfinfo))){
		dbg(0, "unable to open file '%s'.", fn);
		puts(sf_strerror(NULL));
		int e = sf_error(NULL);
		dbg(0, "error=%i", e);
		return NULL;
	}
	if (nfo) {
		nfo->channels    = sfinfo.channels;
		nfo->frames      = sfinfo.frames;
		nfo->sample_rate = sfinfo.samplerate;
		nfo->length      = sfinfo.samplerate ? (sfinfo.frames * 1000) / sfinfo.samplerate : 0;
	}
	return (void*) sffile;
}

int ad_close_sndfile(void *sf) {
	SNDFILE *sffile = (SNDFILE*) sf;
	if(!sf || sf_close(sffile)) { 
		perr("bad file close.\n");
		return -1;
	}
	return 0;
}

ssize_t ad_read_sndfile(void *sf, double* d, size_t len) {
	SNDFILE *sffile = (SNDFILE*) sf;
	return sf_read_double (sffile, d, len);
}

const static ad_plugin ad_sndfile = {
#if 1
	&ad_open_sndfile,
	&ad_close_sndfile,
	&ad_read_sndfile
#else
	&ad_open_null,
	&ad_close_null,
	&ad_read_null
#endif
};

/* dlopen handler */
const ad_plugin * get_sndfile() {
	return &ad_sndfile;
}
