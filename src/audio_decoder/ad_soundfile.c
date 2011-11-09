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

typedef struct {
	SF_INFO sfinfo;
	SNDFILE *sffile;
} sndfile_audio_decoder;

int ad_info_sndfile(void *sf, struct adinfo *nfo) {
	sndfile_audio_decoder *priv = (sndfile_audio_decoder*) sf;
	if (!priv) return -1;
	if (nfo) {
		nfo->channels    = priv->sfinfo.channels;
		nfo->frames      = priv->sfinfo.frames;
		nfo->sample_rate = priv->sfinfo.samplerate;
		nfo->length      = priv->sfinfo.samplerate ? (priv->sfinfo.frames * 1000) / priv->sfinfo.samplerate : 0;
	}
	return 0;
}

void *ad_open_sndfile(const char *fn, struct adinfo *nfo) {
	sndfile_audio_decoder *priv = (sndfile_audio_decoder*) calloc(1, sizeof(sndfile_audio_decoder));
	priv->sfinfo.format=0;
	if(!(priv->sffile = sf_open(fn, SFM_READ, &priv->sfinfo))){
		dbg(0, "unable to open file '%s'.", fn);
		puts(sf_strerror(NULL));
		int e = sf_error(NULL);
		dbg(0, "error=%i", e);
		return NULL;
	}
	ad_info_sndfile(priv, nfo);
	return (void*) priv;
}

int ad_close_sndfile(void *sf) {
	sndfile_audio_decoder *priv = (sndfile_audio_decoder*) sf;
	if (!priv) return -1;
	if(!sf || sf_close(priv->sffile)) { 
		perr("bad file close.\n");
		return -1;
	}
	free(priv);
	return 0;
}

int64_t ad_seek_sndfile(void *sf, int64_t pos) {
	sndfile_audio_decoder *priv = (sndfile_audio_decoder*) sf;
	if (!priv) return -1;
	return sf_seek(priv->sffile, pos, SEEK_SET);
}

ssize_t ad_read_sndfile(void *sf, double* d, size_t len) {
	sndfile_audio_decoder *priv = (sndfile_audio_decoder*) sf;
	if (!priv) return -1;
	return sf_read_double (priv->sffile, d, len);
}

const static ad_plugin ad_sndfile = {
#if 1
	&ad_open_sndfile,
	&ad_close_sndfile,
	&ad_info_sndfile,
	&ad_seek_sndfile,
	&ad_read_sndfile
#else
	&ad_open_null,
	&ad_close_null,
	&ad_info_null,
	&ad_seek_null,
	&ad_read_null
#endif
};

/* dlopen handler */
const ad_plugin * get_sndfile() {
	return &ad_sndfile;
}
