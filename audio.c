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
jack_process(jack_nframes_t nframes, void *arg)
{
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

	audition.nframes = 4096;

	//jack_client_close(client);
	printf("jack_init(): done ok.\n");
	return 1;
}


//gboolean
int
playback_init(char* filename, int id)
{
	//open a file ready for playback.

	if(app.playing_id) playback_stop(); //stop any previous playback.

	if(!id){errprintf("playback_init(): bad arg: id=0\n"); return 0; }

	SF_INFO *sfinfo = &audition.sfinfo;
	SNDFILE *sffile;
	sfinfo->format  = 0;
	//int            readcount;

	if(!(sffile = sf_open(filename, SFM_READ, sfinfo))){
		printf("playback_init(): not able to open input file %s.\n", filename);
		puts(sf_strerror(NULL));    // print the error message from libsndfile:
	}
	audition.sffile = sffile;

	char chanwidstr[64];
	if     (sfinfo->channels==1) snprintf(chanwidstr, 64, "mono");
	else if(sfinfo->channels==2) snprintf(chanwidstr, 64, "stereo");
	else                         snprintf(chanwidstr, 64, "channels=%i", sfinfo->channels);
	printf("playback_init(): %iHz %s frames=%i\n", sfinfo->samplerate, chanwidstr, (int)sfinfo->frames);
	

	if(!client) jack_init();

	app.playing_id = id;
	return 1;
}


void
playback_stop()
{
	printf("playback_stop()...\n");
	if(sf_close(audition.sffile)) printf("error! bad file close.\n");
	audition.sffile = NULL;
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
	char *filename = sample->filename;

	FLAC__StreamMetadata streaminfo;
	if(!FLAC__metadata_get_streaminfo(filename, &streaminfo)) {
		errprintf("get_file_info_flac(): can't open file or get STREAMINFO block\n", filename);
		return false;
	}

	sample->sample_rate  = streaminfo.data.stream_info.sample_rate;
	sample->channels     = streaminfo.data.stream_info.channels;
	sample->bitdepth     = streaminfo.data.stream_info.bits_per_sample;

	int total_samples    = streaminfo.data.stream_info.total_samples;
	sample->length       = (total_samples * 1000) / sample->sample_rate;

	int bits_per_sample = sample->bitdepth;
	if(bits_per_sample < FLAC__MIN_BITS_PER_SAMPLE || bits_per_sample > FLAC__MAX_BITS_PER_SAMPLE) warnprintf("get_file_info_flac() bitdepth=%i\n", bits_per_sample);

	if(sample->channels<1 || sample->channels>100){ printf("get_file_info_flac(): bad channel count: %i\n", sample->channels); return FALSE; }

	return TRUE;
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


FLAC__StreamDecoderWriteStatus
flac_write_cb(const FLAC__FileDecoder *dec, const FLAC__Frame *frame, const FLAC__int32 * const buf[], void *data)
{
	//FIXME do something with the data!

	return 0;
}


void
flac_error_cb(const FLAC__FileDecoder *dec, FLAC__StreamDecoderErrorStatus status, void *data)
{
    errprintf("flac_error_cb(): flac error handler called!\n");
}

//--------------------------------------------------------------------------------------------

//seekable flac - lets get to this later!

void
flac_decode_seekable(char* filename)
{
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


