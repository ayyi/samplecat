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

#ifdef HAVE_FLAC
/* NOTE: This is incomplete - ad_ffmpeg decodes .flac files fine */
#include <FLAC/all.h>

void flac_error_cb(const FLAC__FileDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *data) {
	dbg(0, "flac error: %s\n", FLAC__FileDecoderStateString[FLAC__file_decoder_get_state(decoder)]);
}

FLAC__StreamDecoderWriteStatus 
flac_write_cb(const FLAC__FileDecoder *decoder, const FLAC__Frame* frame, const FLAC__int32 * const buffer[], _decoder_session* session) {
	//return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void *ad_open_flac(const char *fn, struct adinfo *nfo) {
	FLAC__StreamMetadata streaminfo;
	if(!FLAC__metadata_get_streaminfo(fn, &streaminfo)) {
		dbg(0, "can't read file to get info: '%s'.", filename);
		return NULL;
	}

	if (nfo) {
		unsigned int total_samples = streaminfo.data.stream_info.total_samples;
		nfo->sample_rate  = streaminfo.data.stream_info.sample_rate;
		nfo->channels     = streaminfo.data.stream_info.channels;
		nfo->frames       = total_samples / nfo->channels;
		nfo->length       = total_samples * 1000 / nfo->sample_rate;
	}

	FLAC__FileDecoder* flac = FLAC__file_decoder_new();
	FLAC__file_decoder_set_filename(flac, fn);

#if 0
	FLAC__file_decoder_set_write_callback   (flac, (FLAC__FileDecoderWriteCallback)flac_write_cb);
	FLAC__file_decoder_set_error_callback   (flac, flac_error_cb);
	FLAC__file_decoder_set_client_data      (flac, (void*)NULL);
#endif

	if(FLAC__file_decoder_init(flac) == FLAC__FILE_DECODER_OK){
		FLAC__file_decoder_seek_absolute(flac, 0);
		return (void*) flac;
	}

	FLAC__file_decoder_delete(flac);
	dbg(0, "unable to open input file '%s'.\n", fn);
	return NULL;
}

int ad_close_flac(void *sf) {
	FLAC__FileDecoder* flac = (FLAC__FileDecoder*)sf;
	if (!flac) return -1;
	FLAC__file_decoder_finish(flac);
	FLAC__file_decoder_delete(flac);
	return 0;
}

ssize_t ad_read_flac(void *sf, double* d, size_t len) {
	FLAC__FileDecoder* flac = (FLAC__FileDecoder*)sf;

	if(FLAC__file_decoder_get_state(flac) != FLAC__FILE_DECODER_OK){
		errprintf("flac_fill_ringbuffer(): session error! why are we being called?\n");
		return -1;
	}
	if(!FLAC__file_decoder_process_single(flac)){
		errprintf("flac_next_block(): process error. state=%i %s\n", 
				FLAC__file_decoder_get_state(flac),
				FLAC__FileDecoderStateString[FLAC__file_decoder_get_state(flac)]);
		return -1;
	}
	// TODO: copy data from callback buffer.
	return -1;
}
#endif

const static ad_plugin ad_flac = {
#ifdef HAVE_FLAC 
	&ad_open_flac,
	&ad_close_flac,
	&ad_read_flac
#else
	&ad_open_null,
	&ad_close_null,
	&ad_read_null
#endif
};

/* dlopen handler */
const ad_plugin * get_flac() {
	return &ad_flac;
}

