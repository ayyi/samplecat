/** @file simple_client.c
 *
 * @brief This simple client demonstrates the basic features of JACK
 * as they would be used by many applications.
 */

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <libart_lgpl/libart.h>

#include <sndfile.h>
#include <jack/jack.h>

#include "mysql/mysql.h"
#include "dh-link.h"
#include <FLAC/all.h>
#include <jack/ringbuffer.h>
#include "main.h"
#include "support.h"
#include "audio.h"
extern struct _app app;
extern char err [32];
extern char warn[32];

_audition audition;
jack_port_t *output_port1, *output_port2;
jack_client_t *client = NULL;
#define MAX_JACK_BUFFER_SIZE 16384
float buffer[MAX_JACK_BUFFER_SIZE];



int
jack_process(jack_nframes_t nframes, void* arg)
{
	if(audition.type == TYPE_FLAC) return jack_process_flac(nframes, arg); //or maybe set this function directly as the jack callback?

	int readcount, src_offset;
	int channels = audition.sfinfo.channels;

	jack_default_audio_sample_t *out1 = (jack_default_audio_sample_t *)jack_port_get_buffer(output_port1, nframes);
	jack_default_audio_sample_t *out2 = (jack_default_audio_sample_t *)jack_port_get_buffer(output_port2, nframes);

	//while ((readcount = sf_read_float(audition.sffile, buffer, buffer_len))){
	if(audition.sffile){
		//printf("1 sff=%p n=%i\n", audition.sffile, nframes); fflush(stdout);
		readcount = sf_read_float(audition.sffile, buffer, channels * nframes);
		if(readcount < channels * nframes){
			printf("EOF %i<%i\n", readcount, channels * nframes);
			playback_stop();
		}
		else if(sf_error(audition.sffile)) printf("%s read error\n", err);
	}

	int frame_size = sizeof(jack_default_audio_sample_t);
	if(audition.sfinfo.channels == 1 || !app.playing_id){
		memcpy(out1, buffer, nframes * frame_size);
		memcpy(out2, buffer, nframes * frame_size);
	}else if(audition.sfinfo.channels == 2){
		//de-interleave:
		//printf("jack_process(): stereo: size=%i n=%i\n", frame_size, nframes);
		int frame;
		for(frame=0;frame<nframes;frame++){
			src_offset = frame * 2;
			memcpy(out1 + frame, buffer + src_offset,     frame_size);
			memcpy(out2 + frame, buffer + src_offset + 1, frame_size);
			//if(frame<8) printf("%i %i (%i) ", src_offset, src_offset + frame_size, dst_offset);
		}
		printf("\n");
	}else{
		printf("jack_process(): unsupported channel count (%i).\n", audition.sfinfo.channels);
	}

	return 0;      
}

int
jack_process_flac(jack_nframes_t nframes, void *arg)
{
	//flac decoding should really be in another thread....

	jack_default_audio_sample_t *out1 = (jack_default_audio_sample_t *)jack_port_get_buffer(output_port1, nframes);
	jack_default_audio_sample_t *out2 = (jack_default_audio_sample_t *)jack_port_get_buffer(output_port2, nframes);

	flac_fill_ringbuffer(audition.session);

	return TRUE;
}

/**
 * This is the shutdown callback for this JACK application.
 * It is called by JACK if the server ever shuts down or
 * decides to disconnect the client.
 */
void
jack_shutdown(void *arg)
{
}

int
jack_init()
{
	const char **ports;

	// try to become a client of the JACK server
	if ((client = jack_client_new("samplecat")) == 0) {
		fprintf (stderr, "cannot connect to jack server.\n");
		return 1;
	}

	jack_set_process_callback(client, jack_process, 0);
	jack_on_shutdown(client, jack_shutdown, 0);
	//jack_set_buffer_size_callback(_jack, jack_bufsize_cb, this);

	// display the current sample rate. 
	printf("jack_init(): engine sample rate: %" PRIu32 "\n", jack_get_sample_rate(client));

	// create two ports
	output_port1 = jack_port_register(client, "output1", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
	output_port2 = jack_port_register(client, "output2", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

	// tell the JACK server that we are ready to roll
	if (jack_activate(client)){ fprintf (stderr, "cannot activate client"); return 1; }

	// connect the ports:

	//if ((ports = jack_get_ports(client, NULL, NULL, JackPortIsPhysical|JackPortIsOutput)) == NULL) {
	//	fprintf(stderr, "Cannot find any physical capture ports\n");
	//	exit(1);
	//}

	//if (jack_connect (client, ports[0], jack_port_name (input_port))) {
	//	fprintf (stderr, "cannot connect input ports\n");
	//}
	
	if((ports = jack_get_ports(client, NULL, NULL, JackPortIsPhysical|JackPortIsInput)) == NULL) {
		fprintf(stderr, "Cannot find any physical playback ports\n");
		return 1;
	}

	if(jack_connect(client, jack_port_name(output_port1), ports[0])) {
		fprintf (stderr, "cannot connect output ports\n");
	}
	if(jack_connect(client, jack_port_name(output_port2), ports[1])) {
		fprintf (stderr, "cannot connect output ports\n");
	}

	free (ports);

	audition.nframes = 4096; //FIXME!!??

	//jack_client_close(client);
	printf("jack_init(): done ok.\n");
	return 1;
}


//gboolean
int
//playback_init(char* filename, int id, char* mimetype)
playback_init(sample* sample)
{
	//open a file ready for playback.

	if(app.playing_id) playback_stop(); //stop any previous playback.

	if(!sample->id){ errprintf("playback_init(): bad arg: id=0\n"); return 0; }

	//switch(sw){
	//case TYPE_SNDFILE:
	//case 1:
	//if(!strcmp(mimetype, "audio/x-flac")){
	if(sample->filetype == TYPE_FLAC){
		audition.session = malloc(sizeof(_decoder_session*));
		flac_sesssion_init(audition.session, sample);
		FLAC__FileDecoder* decoder = flac_open(audition.session);
	}else{
	//if(sw==TYPE_SNDFILE){
		SF_INFO *sfinfo = &audition.sfinfo;
		SNDFILE *sffile;
		sfinfo->format  = 0;
		//int            readcount;

		if(!(sffile = sf_open(sample->filename, SFM_READ, sfinfo))){
			printf("playback_init(): not able to open input file %s.\n", sample->filename);
			puts(sf_strerror(NULL));    // print the error message from libsndfile:
		}
		audition.sffile = sffile;

		char chanwidstr[64];
		if     (sfinfo->channels==1) snprintf(chanwidstr, 64, "mono");
		else if(sfinfo->channels==2) snprintf(chanwidstr, 64, "stereo");
		else                         snprintf(chanwidstr, 64, "channels=%i", sfinfo->channels);
		printf("playback_init(): %iHz %s frames=%i\n", sfinfo->samplerate, chanwidstr, (int)sfinfo->frames);
		//break;

	}

	if(!client) jack_init();

	app.playing_id = sample->id;
	return 1;
}


void
playback_stop()
{
	printf("playback_stop()...\n");

	if(sf_close(audition.sffile)) printf("error! bad file close.\n");
	audition.sffile = NULL;

	if(audition.session){ free(audition.session); audition.session = NULL; }

	app.playing_id = 0;
	memset(buffer, 0, MAX_JACK_BUFFER_SIZE * sizeof(jack_default_audio_sample_t));
}


void
playback_free()
{
	free(buffer);
}


//--------------------------------------------------------------------------------------------


gboolean
get_file_info_flac(sample* sample)
{
	printf("get_file_info_flac()...\n");
	char *filename = sample->filename;

	FLAC__StreamMetadata streaminfo;
	if(!FLAC__metadata_get_streaminfo(filename, &streaminfo)) {
		errprintf("get_file_info_flac(): can't open file or get STREAMINFO block\n", filename);
		return false;
	}

	sample->sample_rate  = streaminfo.data.stream_info.sample_rate;
	sample->channels     = streaminfo.data.stream_info.channels;
	sample->bitdepth     = streaminfo.data.stream_info.bits_per_sample;

	unsigned int total_samples = streaminfo.data.stream_info.total_samples;
	sample->length       = (total_samples * 1000) / sample->sample_rate;
	printf("get_file_info_flac(): tot_samples=%u\n", total_samples);

	int bits_per_sample = sample->bitdepth;
	if(bits_per_sample < FLAC__MIN_BITS_PER_SAMPLE || bits_per_sample > FLAC__MAX_BITS_PER_SAMPLE) warnprintf("get_file_info_flac() bitdepth=%i\n", bits_per_sample);

	if(sample->channels<1 || sample->channels>100){ printf("get_file_info_flac(): bad channel count: %i\n", sample->channels); return FALSE; }

	return TRUE;
}


void
flac_sesssion_init(_decoder_session* session, sample* sample)
{
	if(!session) errprintf("flac_sesssion_init(): bad arg.\n");

	session->sample = sample;
	session->sample_num = 0;
	session->total_samples = sample->frames;
}


FLAC__FileDecoder*
flac_open(_decoder_session* session)
{
	FLAC__FileDecoder* flacstream = FLAC__file_decoder_new();
	FLAC__file_decoder_set_filename(flacstream, session->sample->filename);
	FLAC__file_decoder_set_md5_checking(flacstream, true);

	FLAC__file_decoder_set_error_callback(flacstream, flac_error_cb);
	FLAC__file_decoder_set_write_callback(flacstream, flac_write_cb);
	FLAC__file_decoder_set_client_data(flacstream, (void*)&session);
	FLAC__file_decoder_set_metadata_callback(flacstream, flac_metadata_cb);

	if(FLAC__file_decoder_init(flacstream) == FLAC__FILE_DECODER_OK){
		return flacstream;
	}

	errprintf("flac_open(): failed to initialise flac decoder (%s) flac=%p.\n", session->sample->filename, flacstream);
	FLAC__file_decoder_delete(flacstream);
	return NULL;
}


gboolean
flac_close(FLAC__FileDecoder* flacstream)
{
	printf("flac_close()...\n");
	FLAC__file_decoder_finish(flacstream);
	FLAC__file_decoder_delete(flacstream);
	return TRUE;
}


gboolean
flac_read(FLAC__FileDecoder* flacstream)
{
	printf("flac_read(): flacstream=%p\n", flacstream);

	FLAC__file_decoder_seek_absolute(flacstream, 0);

	if(!FLAC__file_decoder_process_until_end_of_file(flacstream)){
		errprintf("flac_read(): process error. state=%i: %s\n", FLAC__file_decoder_get_state(flacstream), FLAC__FileDecoderStateString[FLAC__file_decoder_get_state(flacstream)]);
		//print_error_with_state(flacstream, "ERROR while decoding frames"); //copy this from flac src?
		return false;
	}
	printf("flac_read(): finished.\n");
	flac_close(flacstream);
	return true;
}


void
flac_decode(char* filename)
{
	FLAC__FileDecoder *flacstream = FLAC__file_decoder_new();
	if(FLAC__file_decoder_init(flacstream) == FLAC__FILE_DECODER_OK){

		FLAC__file_decoder_set_filename(flacstream, filename);
		FLAC__file_decoder_set_error_callback(flacstream, flac_error_cb);
		FLAC__file_decoder_set_write_callback(flacstream, flac_write_cb);

		if(!FLAC__file_decoder_process_until_end_of_file(flacstream)){
			errprintf("flac_decode(): process error. state=\n", FLAC__file_decoder_get_state(flacstream));
		}

		FLAC__file_decoder_finish(flacstream);
		FLAC__file_decoder_delete(flacstream);
	}
	else errprintf("flac_decode(): failed to open stream.\n");
}


void
flac_next_block(_decoder_session* session)
{
	//if(!FLAC__file_decoder_process_single(flacstream)){
	//	errprintf("flac_next_block(): process error. state=\n", FLAC__file_decoder_get_state(flacstream));
	//}
}


void
flac_fill_ringbuffer(_decoder_session* session)
{
	jack_ringbuffer_t* rb = session->audition->rb;

		//size_t jack_ringbuffer_write_space (const jack_ringbuffer_t * rb)
		//jack_ringbuffer_write (jack_ringbuffer_t * rb, const char *src, size_t cnt)
}

/*
FLAC__StreamDecoderWriteStatus
flac_write_cb(const FLAC__FileDecoder *dec, const FLAC__Frame *frame, const FLAC__int32 * const buf[], void *data)
{
	//FIXME do something with the data!

	const unsigned bps      = frame->header.bits_per_sample;
	const unsigned channels = frame->header.channels;
	unsigned wide_samples   = frame->header.blocksize;
	unsigned wide_sample, sample, channel, byte;

	return 0;
}
*/

FLAC__StreamDecoderWriteStatus 
//write_callback(const void *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
flac_write_cb(const FLAC__FileDecoder *decoder, const FLAC__Frame* frame, const FLAC__int32 * const buffer[], void* data)
{
	/*
	-decoder 	  The decoder instance calling the callback.
	-frame 	      The description of the decoded frame. (?????)
	-buffer 	  An array of pointers to decoded channels of data (deinterleaved).
	-client_data  The callee's client data set through FLAC__file_decoder_set_client_data().

	note: buffer is not interleaved: buffer[channel][wide_sample]
	*/

	/*
	jack_ringbuffer_t* rb = (jack_ringbuffer_t*)data;
	if(!rb){ errprintf("flac_write_cb(): bad ringbuffer arg.\n"); return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;}
	*/
	_decoder_session* session = (_decoder_session*)data;
	unsigned sample_num = session->sample_num;

	//DecoderSession *decoder_session = (DecoderSession*)client_data;
	//FILE *fout = decoder_session->fout;
	const unsigned bps             = frame->header.bits_per_sample;
	const unsigned channels        = frame->header.channels;

	//FLAC__bool is_big_endian       = (decoder_session->is_aiff_out? true : (decoder_session->is_wave_out? false : decoder_session->is_big_endian));
	//FLAC__bool is_unsigned_samples = (decoder_session->is_aiff_out? false : (decoder_session->is_wave_out? bps<=8 : decoder_session->is_unsigned_samples));
	FLAC__bool is_big_endian       = TRUE;
	FLAC__bool is_unsigned_samples = FALSE;
	FLAC__bool is_big_endian_host_ = FALSE;

	unsigned wide_samples          = frame->header.blocksize;
	unsigned buffer_total_frames   = frame->header.blocksize / channels;
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

	//if(decoder_session->abort_flag)	return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;

	/*
	if(bps != decoder_session->bps) {
		flac__utils_printf(stderr, 1, "%s: ERROR, bits-per-sample is %u in frame but %u in STREAMINFO\n", decoder_session->inbasefilename, bps, decoder_session->bps);
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}
	if(channels != decoder_session->channels) {
		flac__utils_printf(stderr, 1, "%s: ERROR, channels is %u in frame but %u in STREAMINFO\n", decoder_session->inbasefilename, channels, decoder_session->channels);
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}
	if(frame->header.sample_rate != decoder_session->sample_rate) {
		flac__utils_printf(stderr, 1, "%s: ERROR, sample rate is %u in frame but %u in STREAMINFO\n", decoder_session->inbasefilename, frame->header.sample_rate, decoder_session->sample_rate);
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}
	*/

	/*
	 * limit the number of samples to accept based on --until
	 */
	/*
	FLAC__ASSERT(!decoder_session->skip_specification->is_relative);
	FLAC__ASSERT(decoder_session->skip_specification->value.samples >= 0);
	FLAC__ASSERT(!decoder_session->until_specification->is_relative);
	FLAC__ASSERT(decoder_session->until_specification->value.samples >= 0);
	if(decoder_session->until_specification->value.samples > 0) {
		const FLAC__uint64 skip = (FLAC__uint64)decoder_session->skip_specification->value.samples;
		const FLAC__uint64 until = (FLAC__uint64)decoder_session->until_specification->value.samples;
		const FLAC__uint64 input_samples_passed = skip + decoder_session->samples_processed;
		FLAC__ASSERT(until >= input_samples_passed);
		if(input_samples_passed + wide_samples > until)
			wide_samples = (unsigned)(until - input_samples_passed);
		if (wide_samples == 0) {
			decoder_session->abort_flag = true;
			decoder_session->aborting_due_to_until = true;
			return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
		}
	}
	*/

	if(wide_samples > 0) {
		/*
		decoder_session->samples_processed += wide_samples;
		decoder_session->frame_counter++;

		if(!(decoder_session->frame_counter & 0x3f)) print_stats(decoder_session);

		if(decoder_session->analysis_mode) {
			flac__analyze_frame(frame, decoder_session->frame_counter-1, decoder_session->aopts, fout);
		}
		*/
		//else if(!decoder_session->test_only) {
		if(TRUE) {
			/*if (decoder_session->replaygain.apply) {
				bytes_to_write = FLAC__replaygain_synthesis__apply_gain(
					u8buffer,
					!is_big_endian,
					is_unsigned_samples,
					buffer,
					wide_samples,
					channels,
					bps, // source_bps
					bps, // target_bps
					decoder_session->replaygain.scale,
					decoder_session->replaygain.spec.limiter == RGSS_LIMIT__HARD, // hard_limit
					decoder_session->replaygain.spec.noise_shaping != NOISE_SHAPING_NONE, // do_dithering
					&decoder_session->replaygain.dither_context
				);
			}
			else */if(bps == 8) {
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
	if(bytes_to_write > 0) {
		/*
		//size_t jack_ringbuffer_write_space (const jack_ringbuffer_t * rb)
		//jack_ringbuffer_write (jack_ringbuffer_t * rb, const char *src, size_t cnt)

		if(jack_ringbuffer_write(rb, (void *)buffer, bytes_to_write) < bytes_to_write){
			return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
		}
		*/

	}

	//calculate peak data:
	int i, ch, pixel_x;
	short sample_val;
	short* max = session->max;
	short* min = session->min;
	if(bps == 16){
		for(i=0;i<buffer_total_frames;i++){
			pixel_x = (sample_num * OVERVIEW_WIDTH) / session->total_samples;
			if(pixel_x >= OVERVIEW_WIDTH){errprintf("flac_write_cb(): pixbuf index error.\n"); break;}
			for(ch=0;ch<channels;ch++){
				sample_val = s16buffer[i + ch];
				max[pixel_x] = MAX(max[pixel_x], sample_val);
				min[pixel_x] = MIN(min[pixel_x], sample_val);
				sample_num++;         //probably should use frame_num instead.
			}
		}
	} else printf("flac_write_cb(): FIXME bitdepth not supported.\n");

	session->sample_num = sample_num;

	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void
flac_error_cb(const FLAC__FileDecoder *dec, FLAC__StreamDecoderErrorStatus status, void *data)
{
    errprintf("flac_error_cb(): flac error handler called!\n");
}

void
flac_metadata_cb(const FLAC__FileDecoder *dec, const FLAC__StreamMetadata *meta, void *data)
{
	printf("flac_metadata_cb()...\n");
}

//--------------------------------------------------------------------------------------------

//seekable flac - lets get to this later!

void
flac_decode_seekable(char* filename)
{
	/*
	FLAC__SeekableStreamDecoder* flacstream = FLAC__seekable_stream_decoder_new();
	if(FLAC__seekable_stream_decoder_init(flacstream) == FLAC__SEEKABLE_STREAM_DECODER_OK){

		//FLAC__file_decoder_set_filename(flacstream, filename);
		FLAC__file_decoder_set_error_callback(flacstream, flac_error_cb);

		FLAC__seekable_stream_decoder_set_read_callback(flacstream,	(FLAC__SeekableStreamDecoderReadCallback)flac_read_cb); //can we use the default?
		FLAC__file_decoder_set_write_callback(flacstream, flac_write_cb);

		if(!FLAC__stream_decoder_process_until_end_of_stream(flacstream)){
			errprintf("flac_decode(): process error. state=\n", FLAC__stream_decoder_get_state(flacstream));
		}

		FLAC__seekable_stream_decoder_finish(flacstream);
		FLAC__seekable_stream_decoder_delete(flacstream);
	}
	else errprintf("flac_decode(): failed to open stream.\n");
	*/
}


FLAC__StreamDecoderReadStatus
flac_read_cb(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data)
{
	//The address of the buffer to be filled is supplied, along with the number of bytes the buffer can hold
	/*
	-decoder      The decoder instance calling the callback.
	-buffer       A pointer to a location for the callee to store data to be decoded.
	-bytes        A pointer to the size of the buffer. On entry to the callback, it contains the maximum number of bytes that may be stored in buffer. The callee must set it to the actual number of bytes stored (0 in case of error or end-of-stream) before returning.
	-client_data  The callee's client data set through FLAC__stream_decoder_set_client_data().

	return value should be one of:
		-FLAC__STREAM_DECODER_READ_STATUS_CONTINUE      The read was OK and decoding can continue.
		-FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM The read was attempted at the end of the stream.
		-FLAC__STREAM_DECODER_READ_STATUS_ABORT         An unrecoverable error occurred. The decoder will return from the process call.

	*/
  return 0;
}


FLAC__StreamDecoderWriteStatus
flac_write_cb_seekable(const FLAC__FileDecoder *dec, const FLAC__Frame *frame, const FLAC__int32 * const buf[], void *data)
{
  return 0;
}


