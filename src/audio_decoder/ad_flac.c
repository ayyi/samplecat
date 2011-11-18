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

void flac_error_cb(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *data) {
	dbg(0, "flac error: %s\n", FLAC__StreamDecoderStateString[FLAC__stream_decoder_get_state(decoder)]);
}

#include <jack/ringbuffer.h> 

FLAC__StreamDecoderWriteStatus 
flac_write_cb(const FLAC__StreamDecoder *decoder, const FLAC__Frame* frame, const FLAC__int32 * const buffer[], void* outbuf)
{
	const unsigned bps             = frame->header.bits_per_sample;
	const unsigned channels        = frame->header.channels;

#if 0
	FLAC__uint64 sample_num;
	FLAC__uint32 frame_num;
	if(frame->header.number_type == FLAC__FRAME_NUMBER_TYPE_FRAME_NUMBER){
		frame_num = frame->header.number.frame_number;
		sample_num = frame_num * channels;
		//printf("flac_write_cb(): FRAME %i.\n", frame_num);
	}else{
		sample_num = frame->header.number.sample_number;
		frame_num = sample_num / channels;
		//printf("flac_write_cb(): SAMPLE %i.\n", frame_num);
	}

	int i, ch;

	FLAC__bool is_big_endian       = TRUE;
	FLAC__bool is_unsigned_samples = FALSE;
	FLAC__bool is_big_endian_host_ = FALSE;

	unsigned wide_samples          = frame->header.blocksize;
	//printf("flac_write_cb(): blocksize=%u\n", wide_samples);
	unsigned buffer_total_frames   = frame->header.blocksize;// looks like we dont have to divide by channel count here.
	unsigned wide_sample, sample, channel, byte;
	//printf("flac_write_cb(): frame=%p blocksize=%u.\n", frame, wide_samples);

	static FLAC__int8 s8buffer[FLAC__MAX_BLOCK_SIZE * FLAC__MAX_CHANNELS * sizeof(FLAC__int32)]; /* WATCHOUT: can be up to 2 megs */
	FLAC__uint8  *u8buffer  = (FLAC__uint8  *)s8buffer;
	FLAC__int16  *s16buffer = (FLAC__int16  *)s8buffer;
	FLAC__uint16 *u16buffer = (FLAC__uint16 *)s8buffer;
	FLAC__int32  *s32buffer = (FLAC__int32  *)s8buffer;
	FLAC__uint32 *u32buffer = (FLAC__uint32 *)s8buffer;
	size_t bytes_to_write = 0;

	(void)decoder;

	if(wide_samples > 0) {
		if(TRUE) {
			if(bps == 8) {
				if(is_unsigned_samples) {
					for(sample = wide_sample = 0; wide_sample < wide_samples; wide_sample++)
						for(channel = 0; channel < channels; channel++, sample++)
							u8buffer[sample] = (FLAC__uint8)(buffer[channel][wide_sample] + 0x80);
				}
				else {
					for(sample = wide_sample = 0; wide_sample < wide_samples; wide_sample++)
						for(channel = 0; channel < channels; channel++, sample++)
							s8buffer[sample] = (FLAC__int8)(buffer[channel][wide_sample]);
				}
				bytes_to_write = sample;
			}
			else if(bps == 16) {
				if(is_unsigned_samples) {
					for(sample = wide_sample = 0; wide_sample < wide_samples; wide_sample++)
						for(channel = 0; channel < channels; channel++, sample++)
							u16buffer[sample] = (FLAC__uint16)(buffer[channel][wide_sample] + 0x8000);
				}
				else {
					for(sample = wide_sample = 0; wide_sample < wide_samples; wide_sample++)
						for(channel = 0; channel < channels; channel++, sample++)
							s16buffer[sample] = (FLAC__int16)(buffer[channel][wide_sample]); //this interleaves the data??
				}
				if(is_big_endian != is_big_endian_host_) {
					unsigned char tmp;
					const unsigned bytes = sample * 2;
					for(byte = 0; byte < bytes; byte += 2) {
						tmp = u8buffer[byte];
						u8buffer[byte] = u8buffer[byte+1];
						u8buffer[byte+1] = tmp;
					}
				}
				bytes_to_write = 2 * sample;
			}
			else if(bps == 24) {
				if(is_unsigned_samples) {
					for(sample = wide_sample = 0; wide_sample < wide_samples; wide_sample++)
						for(channel = 0; channel < channels; channel++, sample++)
							u32buffer[sample] = buffer[channel][wide_sample] + 0x800000;
				}
				else {
					for(sample = wide_sample = 0; wide_sample < wide_samples; wide_sample++)
						for(channel = 0; channel < channels; channel++, sample++)
							s32buffer[sample] = buffer[channel][wide_sample];
				}
				if(is_big_endian != is_big_endian_host_) {
					unsigned char tmp;
					const unsigned bytes = sample * 4;
					for(byte = 0; byte < bytes; byte += 4) {
						tmp = u8buffer[byte];
						u8buffer[byte] = u8buffer[byte+3];
						u8buffer[byte+3] = tmp;
						tmp = u8buffer[byte+1];
						u8buffer[byte+1] = u8buffer[byte+2];
						u8buffer[byte+2] = tmp;
					}
				}
				if(is_big_endian) {
					unsigned lbyte;
					const unsigned bytes = sample * 4;
					for(lbyte = byte = 0; byte < bytes; ) {
						byte++;
						u8buffer[lbyte++] = u8buffer[byte++];
						u8buffer[lbyte++] = u8buffer[byte++];
						u8buffer[lbyte++] = u8buffer[byte++];
					}
				}
				else {
					unsigned lbyte;
					const unsigned bytes = sample * 4;
					for(lbyte = byte = 0; byte < bytes; ) {
						u8buffer[lbyte++] = u8buffer[byte++];
						u8buffer[lbyte++] = u8buffer[byte++];
						u8buffer[lbyte++] = u8buffer[byte++];
						byte++;
					}
				}
				bytes_to_write = 3 * sample;
			}
			else {
				FLAC__ASSERT(0);
			}
		}
	}
#else
	if(channels > 2 || bps != 16) {
		dbg(0, "unsupported FLAC format (more than 2 channels or not 16bit)");
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT; 
	}
	unsigned int buffer_total_frames  = frame->header.blocksize;
	const int bytes_to_write = buffer_total_frames;
	int i, ch;
#endif
	if(bytes_to_write > 0) {
		for(i=0;i<buffer_total_frames;i++){
			for(ch=0;ch<channels;ch++){
				float float1;
				const FLAC__int16 src1 = (FLAC__int16)(buffer[ch][i]);
				if (src1 >= 0) float1 = src1 / 32767.0; else float1 = src1 / 32768.0;
				if (jack_ringbuffer_write_space((jack_ringbuffer_t*)outbuf) <  sizeof(float)) {
					dbg(0, "ringbuffer overflow");
				} else {
					jack_ringbuffer_write((jack_ringbuffer_t*)outbuf, (char*) &float1, sizeof(float));
				}
			}
		}
	}
	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

typedef struct {
	FLAC__StreamDecoder *flac;
  jack_ringbuffer_t *rb;
	FLAC__StreamMetadata streaminfo;
} flac_audio_decoder;

int ad_info_flac(void *sf, struct adinfo *nfo) {
	flac_audio_decoder *priv = (flac_audio_decoder*) sf;
	if (!priv) return -1;
	if (nfo) {
		int64_t total_samples = priv->streaminfo.data.stream_info.total_samples;
		nfo->sample_rate  = priv->streaminfo.data.stream_info.sample_rate;
		nfo->channels     = priv->streaminfo.data.stream_info.channels;
		nfo->frames       = total_samples;
		nfo->length       = total_samples * 1000 / nfo->sample_rate  / nfo->channels;
		nfo->bit_rate     = 0;
		nfo->bit_depth    = 0;
		nfo->meta_data    = NULL;
	}
	return 0;
}


void *ad_open_flac(const char *fn, struct adinfo *nfo) {
	flac_audio_decoder *priv = (flac_audio_decoder*) calloc(1, sizeof(flac_audio_decoder));

	if(!FLAC__metadata_get_streaminfo(fn, &priv->streaminfo)) {
		dbg(0, "can't read file to get info: '%s'.", fn);
		free(priv);
		return NULL;
	}

	//priv->nfo.bitrate      = streaminfo.data.stream_info.bits_per_sample;
	
  dbg(0, "flac - %s", fn);
	ad_info_flac((void*)priv, nfo);
	if (nfo) {
		dbg(0, "flac - sr:%i c:%i d:%"PRIi64" f:%"PRIi64, nfo->sample_rate, nfo->channels, nfo->length, nfo->frames);
	}

	//const size_t rbsize =FLAC__MAX_BLOCK_SIZE * FLAC__MAX_CHANNELS * sizeof(FLAC__int32);
	const size_t rbsize =FLAC__MAX_BLOCK_SIZE * 2 * sizeof(FLAC__int16);
	priv->rb = jack_ringbuffer_create(rbsize);
	memset(priv->rb->buf, 0, rbsize); 

	priv->flac = FLAC__stream_decoder_new();
	(void)FLAC__stream_decoder_set_md5_checking(priv->flac, true);

	FLAC__StreamDecoderInitStatus init_status = 
		FLAC__stream_decoder_init_file(priv->flac, fn, flac_write_cb, NULL, flac_error_cb, priv->rb);

	if(init_status != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
		FLAC__stream_decoder_delete(priv->flac);
		dbg(0, "unable to open input file '%s'.\n", fn);
		//fprintf(stderr, "ERROR: initializing decoder: %s\n", FLAC__StreamDecoderInitStatusString[init_status]);
		return NULL;
	}
	FLAC__stream_decoder_process_until_end_of_metadata(priv->flac);
	return (void*) priv;
}

int ad_close_flac(void *sf) {
	flac_audio_decoder *priv = (flac_audio_decoder*) sf;
	if (!priv) return -1;
	dbg(0, "...");
	FLAC__stream_decoder_finish(priv->flac);
	FLAC__stream_decoder_delete(priv->flac);
	jack_ringbuffer_free(priv->rb);
	free(priv);
	return 0;
}

ssize_t ad_read_flac(void *sf, float* d, size_t len) {
	flac_audio_decoder *priv = (flac_audio_decoder*) sf;
	if (!priv) return -1;
	int written=0;

#if 0
	if(FLAC__stream_decoder_get_state(priv->flac) != FLAC__STREAM_DECODER_READ_FRAME){
		errprintf("flac_next_block(): process error. state=%i %s\n", 
				FLAC__stream_decoder_get_state(priv->flac),
				FLAC__StreamDecoderStateString[FLAC__stream_decoder_get_state(priv->flac)]);
		return -1;
	}
#endif

	while (written < len) {
#if 1
		if(jack_ringbuffer_read_space(priv->rb) > sizeof(float)) {
			jack_ringbuffer_read(priv->rb, (char*) &d[written++], sizeof(float));
		} 
#else // XXX float vs double
		int nf = MIN(len-written , jack_ringbuffer_read_space(priv->rb)/sizeof(float));
		if (nf > 0) {
			jack_ringbuffer_read(priv->rb, (char*) &d[written], nf*sizeof(float));
			written+=nf;
		}
#endif
		else {
			if(FLAC__stream_decoder_get_state(priv->flac) == FLAC__STREAM_DECODER_END_OF_STREAM)
				break;
			if(!FLAC__stream_decoder_process_single(priv->flac)){
				errprintf("flac_next_block(): process error. state=%i %s\n", 
						FLAC__stream_decoder_get_state(priv->flac),
						FLAC__StreamDecoderStateString[FLAC__stream_decoder_get_state(priv->flac)]);
				break;
			}
		}
	}
	//dbg(0, "flac-decode - %d/%d", written, len);
	return written;
}

int64_t ad_seek_flac(void *sf, int64_t pos) {
	flac_audio_decoder *priv = (flac_audio_decoder*) sf;
	if (!priv) return -1;
	jack_ringbuffer_reset(priv->rb);
	// FLAC__stream_decoder_flush(priv->flac);
	return (FLAC__stream_decoder_seek_absolute(priv->flac, pos))?0:-1;
}

int ad_eval_flac(const char *f) { 
	char *ext = strrchr(f, '.');
	if (!ext) return 0;
	if (!strcasecmp(ext, ".flac")) return 100;
	if (!strcasecmp(ext, ".ogg")) return 50;
	return -1; 
}

#endif

const static ad_plugin ad_flac = {
#ifdef HAVE_FLAC 
  &ad_eval_flac,
	&ad_open_flac,
	&ad_close_flac,
	&ad_info_flac,
	&ad_seek_flac,
	&ad_read_flac
#else
  &ad_eval_null,
	&ad_open_null,
	&ad_close_null,
	&ad_info_null,
	&ad_seek_null,
	&ad_read_null
#endif
};

/* dlopen handler */
const ad_plugin * get_libflac() {
	return &ad_flac;
}

