/*
  This file is part of the Samplecat project.
  copyright (C) 2006-2007 Tim Orford <tim@orford.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/
#include "config.h"
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#ifdef OLD
  #include <libart_lgpl/libart.h>
#endif

#include <sndfile.h>

#include "dh-link.h"
#ifdef HAVE_FLAC_1_1_1
  #include <FLAC/all.h>
#endif
#include <jack/ringbuffer.h>
#include "typedefs.h"
#include "sample.h"
#include "support.h"
//#include <gqview2/typedefs.h>
#include "main.h"
#include "audio.h"
extern struct _app app;
extern char err [32];
extern char warn[32];
extern unsigned debug;

_audition audition;
jack_port_t *output_port1, *output_port2;
jack_client_t *jack_client = NULL;
#define MAX_JACK_BUFFER_SIZE 16384
float buffer[MAX_JACK_BUFFER_SIZE];
#define JACK_PORT_COUNT 2;

/*

the flac code here works with version 1.1.2 and earlier.
	-flac porting guide for 1.1.3: http://flac.sourceforge.net/api/group__porting__1__1__2__to__1__1__3.html

*/


static int
jack_process(jack_nframes_t nframes, void* arg)
{
	//return non-zero on error.

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
		int frame;
		for(frame=0;frame<nframes;frame++){
			src_offset = frame * 2;
			memcpy(out1 + frame, buffer + src_offset,     frame_size);
			memcpy(out2 + frame, buffer + src_offset + 1, frame_size);
			//if(frame<8) printf("%i %i (%i) ", src_offset, src_offset + frame_size, dst_offset);
		}
		//printf("\n");
	}else{
		dbg(3, "unsupported channel count (%i).\n", audition.sfinfo.channels);
		playback_stop();
		return 1;
	}

	return 0;      
}

int
jack_process_flac(jack_nframes_t nframes, void *arg)
{
	jack_default_audio_sample_t *out1 = (jack_default_audio_sample_t *)jack_port_get_buffer(output_port1, nframes);
	jack_default_audio_sample_t *out2 = (jack_default_audio_sample_t *)jack_port_get_buffer(output_port2, nframes);

	int bytes_to_copy = nframes * sizeof(jack_default_audio_sample_t);// * JACK_PORT_COUNT;

	jack_ringbuffer_t* rb1 = audition.rb1;
	jack_ringbuffer_t* rb2 = audition.rb2;
	if((unsigned)rb1 < 1024) perr("bad rb1.\n");
	if((unsigned)rb2 < 1024) perr("bad rb2.\n");

	g_mutex_lock(app.mutex);
	if(jack_ringbuffer_read_space(rb1) < bytes_to_copy){ //yes, its *bytes*.
		pwarn("not enough data ready. Aborting playback...\n");
		g_idle_add_full(G_PRIORITY_HIGH_IDLE, (GSourceFunc)jack_process_stop_playback, NULL, NULL); //FIXME this is not aggressive enough
		g_mutex_unlock(app.mutex);
		return FALSE;
	}
	//jack_ringbuffer_read_space(rb2);

	//copy nframes from the ringbuffer into the jack buffer:
	jack_ringbuffer_read(rb1, (char*)out1, bytes_to_copy); //the count is in *bytes*.
	jack_ringbuffer_read(rb2, (char*)out2, bytes_to_copy);

	//inform the main thread:
	g_idle_add_full(G_PRIORITY_HIGH_IDLE, (GSourceFunc)jack_process_finished, NULL, NULL); //FIXME this is not aggressive enough
	g_mutex_unlock(app.mutex);

	return TRUE;
}

gboolean
jack_process_finished(gpointer data)
{
	//runs in the main thread.

	gdk_threads_enter(); //this is probably a complete waste as we're not calling any gdk functions.
#ifdef HAVE_FLAC_1_1_1
	if(audition.session) flac_fill_ringbuffer(audition.session);
#endif
	gdk_threads_leave();
	return FALSE; //remove the source.
}


gboolean
jack_process_stop_playback(gpointer data)
{
	//runs in the main thread.
	gdk_threads_enter();
	printf("jack_process_stop_playback(). jack thread requests playback_stop()...\n");

	if(audition.session) playback_stop(); //this "if" could probably be improved.
	gdk_threads_leave();
	return FALSE; //remove the source.
}


/**
 * This is the shutdown callback for this JACK application.
 * It is called by JACK if the server ever shuts down or
 * decides to disconnect the client.
 */
void
jack_shutdown(void *arg)
{
	dbg(0, "FIXME");
}

int
jack_init()
{
	const char **ports;

	// try to become a client of the JACK server
	if ((jack_client = jack_client_new("samplecat")) == 0) {
		fprintf (stderr, "cannot connect to jack server.\n");
		return 1;
	}

	jack_set_process_callback(jack_client, jack_process, 0);
	jack_on_shutdown(jack_client, jack_shutdown, 0);
	//jack_set_buffer_size_callback(_jack, jack_bufsize_cb, this);

	// create two ports
	output_port1 = jack_port_register(jack_client, "output1", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
	output_port2 = jack_port_register(jack_client, "output2", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

	audition.nframes = jack_get_buffer_size(jack_client);

	dbg(1, "samplerate=%" PRIu32 " buffersize=%i", jack_get_sample_rate(jack_client), audition.nframes);

	// tell the JACK server that we are ready to roll
	if (jack_activate(jack_client)){ fprintf (stderr, "cannot activate client"); return 1; }

	// connect the ports:

	//if ((ports = jack_get_ports(client, NULL, NULL, JackPortIsPhysical|JackPortIsOutput)) == NULL) {
	//	fprintf(stderr, "Cannot find any physical capture ports\n");
	//	exit(1);
	//}

	//if (jack_connect (client, ports[0], jack_port_name (input_port))) {
	//	fprintf (stderr, "cannot connect input ports\n");
	//}
	
	if((ports = jack_get_ports(jack_client, NULL, NULL, JackPortIsPhysical|JackPortIsInput)) == NULL) {
		fprintf(stderr, "Cannot find any physical playback ports\n");
		return 1;
	}

	if(jack_connect(jack_client, jack_port_name(output_port1), ports[0])) {
		fprintf (stderr, "cannot connect output ports\n");
	}
	if(jack_connect(jack_client, jack_port_name(output_port2), ports[1])) {
		fprintf (stderr, "cannot connect output ports\n");
	}

	free (ports);

	dbg(1, "done ok.");
	return 1;
}


void
jack_close()
{
	if(jack_client){
		jack_deactivate(jack_client);
		jack_client_close(jack_client);
	}
}


void
audition_init()
{
	audition.session = NULL;
	//audition.sfinfo = 0; 
	audition.sffile = NULL;
	audition.rb1 = NULL;
	audition.rb2 = NULL;
	audition.type = 0;
}


void
audition_reset()
{
	dbg(1, "...");
	if(audition.rb1){ jack_ringbuffer_free(audition.rb1); jack_ringbuffer_free(audition.rb2); }
#ifdef HAVE_FLAC_1_1_1
	if(audition.session) decoder_session_free(audition.session);
#endif
	audition.session = NULL;
	printf("audition_reset(): session nulled.\n");
	audition.sffile = NULL;
	audition.rb1 = NULL;
	audition.rb2 = NULL;
	audition.type = 0;
}


#ifdef HAVE_FLAC_1_1_1
_decoder_session*
flac_decoder_session_new()
{
	_decoder_session* session = malloc(sizeof(*session));
	dbg(1, "size=%i", sizeof(*session));
	session->output_peakfile = FALSE;
	session->flacstream = NULL;
	session->sample = NULL;
	session->sample_num = 0;
	session->frame_num = 0;
	session->total_samples = 0;
	session->total_frames = 0;
	dbg(1, "session=%p", session);
	return session;
}


gboolean
flac_decoder_sesssion_init(_decoder_session* session, sample* sample)
{
	printf("flac_decoder_sesssion_init()...\n");
	if(!session) errprintf("flac_decoder_sesssion_init(): bad arg.\n");

	session->sample = sample;
	session->sample_num = 0;
	session->frame_num = 0;
	session->total_frames = sample->frames;
	session->total_samples = sample->frames * sample->channels;

	if(!session->total_samples) return FALSE;

	int x;
	for(x=0;x<OVERVIEW_WIDTH;x++){
		session->max[x] = 0;
		session->min[x] = 0;
	}
	return true;
}


void
decoder_session_free(_decoder_session* session)
{
	if((unsigned)session<1024){ warnprintf("decoder_session_free(): bad session arg (%p).\n", session); return; }
	printf("decoder_session_free()...\n");

	if(session->flacstream){
		flac_close(session->flacstream); //just in case. This should already be done?
		session->flacstream = NULL;
	}

	printf("decoder_session_free(): freeing session (%p)... FIXME!!!\n", session);
	//free(session);
	printf("decoder_session_free(): done.\n");

}
#endif


//gboolean
int
playback_init(Sample* sample)
{
	//open a file ready for playback.

	PF;
	int ok = TRUE;

	if(app.playing_id) playback_stop(); //stop any previous playback.

	if(!sample->id){ perr("bad arg: id=0\n"); return 0; }

	//switch(sw){
	//case TYPE_SNDFILE:
	//case 1:
	//if(!strcmp(mimetype, "audio/x-flac")){
#ifdef HAVE_FLAC_1_1_1
	if(sample->filetype == TYPE_FLAC){

		int rb_size = 128 * 1024;
		jack_ringbuffer_t *rb1, *rb2;
		if((rb1 = jack_ringbuffer_create(rb_size)) && (rb2 = jack_ringbuffer_create(rb_size))){

			audition.session = flac_decoder_session_new();
			audition.session->audition = &audition; //used as a flag.
			audition.type = TYPE_FLAC;
			audition.rb1 = rb1;
			audition.rb2 = rb2;
			if(flac_decoder_sesssion_init(audition.session, sample)){
				if(flac_open(audition.session)){
					dbg(0, "filling ringbuffer... %p %p", rb1, rb2);
					if(flac_fill_ringbuffer(audition.session)) ok = false;
					if(FLAC__file_decoder_get_state(audition.session->flacstream) != FLAC__FILE_DECODER_OK) ok = false;

				}else{ perr("failed to initialise flac decoder.\n"); ok = false; }
			}else{ perr("failed to initialise flac session.\n"); ok = false; }
		}else{ perr("failed to create ringbuffer.\n"); ok = false; }
#else
	if(0){
#endif
	}else{
	//if(sw==TYPE_SNDFILE){
		SF_INFO *sfinfo = &audition.sfinfo;
		SNDFILE *sffile;
		sfinfo->format  = 0;

		if(!(sffile = sf_open(sample->filename, SFM_READ, sfinfo))){
			dbg(0, "not able to open input file %s.", sample->filename);
			puts(sf_strerror(NULL));    // print the error message from libsndfile:
			return ok = false;
		}
		audition.sffile = sffile;
		audition.type = TYPE_SNDFILE;

		char chanwidstr[64];
		if     (sfinfo->channels==1) snprintf(chanwidstr, 64, "mono");
		else if(sfinfo->channels==2) snprintf(chanwidstr, 64, "stereo");
		else                         snprintf(chanwidstr, 64, "channels=%i", sfinfo->channels);
		dbg(0, "%iHz %s frames=%i", sfinfo->samplerate, chanwidstr, (int)sfinfo->frames);
	}

	if(ok){
		if(!jack_client) jack_init();
		app.playing_id = sample->id;
	}
	PF_DONE;
	return ok;
}


void
playback_stop()
{
	PF;

	if(audition.type == TYPE_SNDFILE){
		printf("playback_stop(): closing sndfile...\n");
		if(sf_close(audition.sffile)) printf("error! bad file close.\n");
#ifdef HAVE_FLAC_1_1_1
	}else{
		printf("playback_stop(): closing flac...\n");
		flac_close(audition.session->flacstream);
		audition.session->flacstream = NULL;
#endif
	}

	dbg(0, "reseting audition...");
	audition_reset();

	app.playing_id = 0;
	memset(buffer, 0, MAX_JACK_BUFFER_SIZE * sizeof(jack_default_audio_sample_t));
	PF_DONE;
}


void
playback_free()
{
	free(buffer);
}


//--------------------------------------------------------------------------------------------


#ifdef HAVE_FLAC_1_1_1
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
	//sample->total_samples = total_samples;
	sample->frames = total_samples / sample->channels;
	sample->length       = (total_samples * 1000) / sample->sample_rate;
	printf("get_file_info_flac(): tot_samples=%u\n", total_samples);

	int bits_per_sample = sample->bitdepth;
	if(bits_per_sample < FLAC__MIN_BITS_PER_SAMPLE || bits_per_sample > FLAC__MAX_BITS_PER_SAMPLE) warnprintf("get_file_info_flac() bitdepth=%i\n", bits_per_sample);

	if(sample->channels<1 || sample->channels>100){ printf("get_file_info_flac(): bad channel count: %i\n", sample->channels); return FALSE; }

	return TRUE;
}


FLAC__FileDecoder*
flac_open(_decoder_session* session)
{
	//opens a flac file ready for decoding block by block.

	if(debug) printf("flac_open()...\n");
	FLAC__FileDecoder* flacstream = FLAC__file_decoder_new();

	FLAC__file_decoder_set_filename(flacstream, session->sample->filename);
	//FLAC__file_decoder_set_md5_checking(flacstream, true);

	//must be set _before_ initialisation:
	FLAC__file_decoder_set_write_callback   (flacstream, (FLAC__FileDecoderWriteCallback)flac_write_cb);
	FLAC__file_decoder_set_error_callback   (flacstream, flac_error_cb);
	FLAC__file_decoder_set_client_data      (flacstream, (void*)session);
	FLAC__file_decoder_set_metadata_callback(flacstream, flac_metadata_cb);

	if(FLAC__file_decoder_init(flacstream) == FLAC__FILE_DECODER_OK){
		FLAC__file_decoder_seek_absolute(flacstream, 0);
		if(debug) printf("flac_open(): done ok.\n");
		return session->flacstream = flacstream;
	}

	errprintf("flac_open(): failed to initialise flac decoder (%s) flac=%p %s.\n", session->sample->filename, flacstream, FLAC__FileDecoderStateString[FLAC__file_decoder_get_state(flacstream)]);
	FLAC__file_decoder_delete(flacstream);
	return NULL;
}


gboolean
flac_close(FLAC__FileDecoder* flacstream)
{
	if((unsigned)flacstream<1024){ warnprintf("flac_close(): bad arg.\n"); return FALSE; }
	if(debug) printf("flac_close()...\n");
	FLAC__file_decoder_finish(flacstream);
	FLAC__file_decoder_delete(flacstream);
	return TRUE;
}


gboolean
flac_read(FLAC__FileDecoder* flacstream)
{
	//Decodes a complete flac file. Used for overview generation.

	printf("flac_read(): flacstream=%p\n", flacstream);

	FLAC__file_decoder_seek_absolute(flacstream, 0);

	if(!FLAC__file_decoder_process_until_end_of_file(flacstream)){
		errprintf("flac_read(): process error. state=%i: %s\n", FLAC__file_decoder_get_state(flacstream), FLAC__FileDecoderStateString[FLAC__file_decoder_get_state(flacstream)]);
		//print_error_with_state(flacstream, "ERROR while decoding frames"); //copy this from flac src?
		return false;
	}
	printf("flac_read(): finished.\n");
	flac_close(flacstream); //should we be closing here? how to NULL flacstream?
	return true;
}


void
flac_decode_file(char* filename)
{
	//decode a whole flac file.

	FLAC__FileDecoder *flacstream = FLAC__file_decoder_new();
	if(FLAC__file_decoder_init(flacstream) == FLAC__FILE_DECODER_OK){

		FLAC__file_decoder_set_filename(flacstream, filename);
		FLAC__file_decoder_set_error_callback(flacstream, flac_error_cb);
		FLAC__file_decoder_set_write_callback(flacstream, (FLAC__FileDecoderWriteCallback)flac_write_cb);

		if(!FLAC__file_decoder_process_until_end_of_file(flacstream)){
			errprintf("flac_decode(): process error. state=\n", FLAC__file_decoder_get_state(flacstream));
		}

		FLAC__file_decoder_finish(flacstream);
		FLAC__file_decoder_delete(flacstream);
	}
	else errprintf("flac_decode(): failed to open stream.\n");
}


FLAC__bool
flac_next_block(_decoder_session* session)
{
	//returning false indicates an error.

	printf("flac_next_block()...\n");
	if(!FLAC__file_decoder_process_single(session->flacstream)){
		errprintf("flac_next_block(): process error. state=%i %s\n", FLAC__file_decoder_get_state(session->flacstream), FLAC__FileDecoderStateString[FLAC__file_decoder_get_state(session->flacstream)]);
		playback_stop();
		return FALSE;
	}
	return TRUE;
}

#define RB_SPARE 4096

gboolean
flac_fill_ringbuffer(_decoder_session* session)
{
	//returning non-zero indicates an error.

	if((unsigned)session<1024){ errprintf("flac_fill_ringbuffer(): bad arg: session."); return TRUE; }
	if((unsigned)session->audition<1024){ errprintf("flac_fill_ringbuffer(): bad audition pointer (%p)\n", session->audition); return TRUE; }

	jack_ringbuffer_t* rb1 = session->audition->rb1;
	if((unsigned)rb1<1024){ errprintf("flac_fill_ringbuffer(): bad ringbuffer pointer (%p)\n", rb1); return TRUE; }

	if(FLAC__file_decoder_get_state(session->flacstream) != FLAC__FILE_DECODER_OK){ errprintf("flac_fill_ringbuffer(): session error! why are we being called?\n"); return TRUE; }

	int i = 0;
	//printf("flac_fill_ringbuffer(): checking ringbuffer space...\n");
	//FLAC__MAX_BLOCK_SIZE
	//g_mutex_lock(app.mutex);
	while(jack_ringbuffer_write_space(rb1) > 48 * 1024 ){ //FIXME we dont know yet the flac block size, but can we make a better guess? blocksize appears to be about 4000 => 4000*4*2 bytes.
		if(!flac_next_block(session)){
			printf("flac_fill_ringbuffer() cannot decode another block\n");
			break;
		}
		i++;
	}
	//g_mutex_unlock(app.mutex);

	printf("flac_fill_ringbuffer() finished. read %i blocks\n", i);

	return FALSE;
}


FLAC__StreamDecoderWriteStatus 
flac_write_cb(const FLAC__FileDecoder *decoder, const FLAC__Frame* frame, const FLAC__int32 * const buffer[], _decoder_session* session)
{
	/*
	a block has been decoded. We copy the data from *frame either to the peakdata array, or to ....?

	-decoder 	  The decoder instance calling the callback.
	-frame 	      Information about the decoded frame, and file metadata.
	-buffer 	  An array of pointers to decoded channels of data (deinterleaved).
	-client_data  The callee's client data set through FLAC__file_decoder_set_client_data().

	frame->header:
		unsigned                blocksize
		unsigned                sample_rate
		unsigned                channels
		FLAC__ChannelAssignment channel_assignment
		unsigned 	            bits_per_sample
		FLAC__FrameNumberType   number_type //tells us which type the union below contains.
		union {
		   FLAC__uint32         frame_number
		   FLAC__uint64         sample_number
		}                       number
		FLAC__uint8             crc

	note: buffer is not interleaved: buffer[channel][wide_sample]
	*/
	if((unsigned)session<1024){ errprintf("flac_write_cb(): bad session arg.\n"); return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT; }
	gboolean debug = FALSE;

	const unsigned bps             = frame->header.bits_per_sample;
	const unsigned channels        = frame->header.channels;

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

	int i, f, ch;

	//FLAC__bool is_big_endian       = (decoder_session->is_aiff_out? true : (decoder_session->is_wave_out? false : decoder_session->is_big_endian));
	//FLAC__bool is_unsigned_samples = (decoder_session->is_aiff_out? false : (decoder_session->is_wave_out? bps<=8 : decoder_session->is_unsigned_samples));
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
	if(debug){ printf("2"); fflush(stdout); }

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

		if(session->output_peakfile){
			//calculate peak data:
			//FIXME sometimes get errors at end of file. Doesnt seem to matter too much.
			int pixel_x;
			short sample_val;
			short* max = session->max;
			short* min = session->min;
			if(bps == 16){
				for(f=0;f<buffer_total_frames;f++){
					//pixel_x = (sample_num * OVERVIEW_WIDTH) / session->total_samples;
					pixel_x = (frame_num * OVERVIEW_WIDTH) / session->total_frames;
					if(pixel_x >= OVERVIEW_WIDTH){errprintf("flac_write_cb(): pixbuf index error. frame=%i sample=%i, total=%Li %li\n", 
					                                              frame_num, sample_num, session->total_frames, session->total_samples); break;}
					for(ch=0;ch<channels;ch++){
						//sample_val = s16buffer[i + ch];
						sample_val = (FLAC__int16)(buffer[ch][f]);
						max[pixel_x] = MAX(max[pixel_x], sample_val);
						min[pixel_x] = MIN(min[pixel_x], sample_val);
						sample_num++;
					}
					frame_num++;
				}
			} else printf("flac_write_cb(): FIXME bitdepth not supported.\n");
			//printf("flac_write_cb(): pixel_x=%i frame_num=%i of %Li (%li)\n", pixel_x, frame_num, session->total_frames, frame->header.number);

		}else{
			//output to a ringbuffer for jack (non-interleaved -> non-interleaved):

			//FIXME this doesnt use the s16 etc buffer filled above, so dont fill it.
			//FIXME for mono files, copy left ch to rhs.

			jack_ringbuffer_t* rb[channels];
			rb[0] = audition.rb1;
			rb[1] = audition.rb2;
			if((unsigned)rb[0]<1024){ errprintf("flac_write_cb(): bad ringbuffer arg (%p).\n", rb[0]); return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;}

			g_mutex_lock(app.mutex);
			if(jack_ringbuffer_write_space(audition.rb1) > bytes_to_write){

				//jack_ringbuffer_write (jack_ringbuffer_t * rb, const char *src, size_t cnt)

				//converting 16bit to float, we write twice as many bytes as there are from the decoder

				FLAC__int16 src1;
				FLAC__int16 src2;
				float float1;
				//float float2;
				for(i=0;i<buffer_total_frames;i++){
					if(debug){ printf("5"); fflush(stdout); }
					//printf("  buffer=%p\n", buffer);
					for(ch=0;ch<channels;ch++){
						//printf("  buffer0=%i\n", buffer[ch][i]);
						src1 = (FLAC__int16)(buffer[ch][i]); //FIXME this is an unneccesary copy.
						src2 = (FLAC__int16)(buffer[ch][i]);

						//convert from int to float:
						//printf("  src1=%i\n", src1);
						if (src1 >= 0) float1 = src1 / 32767.0; else float1 = src1 / 32768.0;
						//if (src2 >= 0) float2 = src2 / 32767.0; else float2 = src2 / 32768.0;

						if(debug){ printf("6"); fflush(stdout); }
						if(jack_ringbuffer_write(rb[ch], (void*)(&float1), 4) < 4){
							errprintf("flac_write_cb(): problem writing to ringbuffer: flac buffer size: %i\n", bytes_to_write);
							return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
						}
						//jack_ringbuffer_write(rb2, (void*)(&float2), 4);
						if(debug){ printf("7"); fflush(stdout); }
					}
				}

			} else { errprintf("flac_write_cb(): no space in ringbuffer. available=%i needed=%i\n", jack_ringbuffer_write_space(audition.rb1), bytes_to_write*2); return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT; }
			g_mutex_unlock(app.mutex);

		}
	} else errprintf("flac_write_cb(): bytes_to_write < 1.\n");

	//session->sample_num = sample_num;
	//session->frame_num  = frame_num;

	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void
flac_error_cb(const FLAC__FileDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *data)
{
    errprintf("flac_error_cb(): flac error handler called! %s\n", FLAC__FileDecoderStateString[FLAC__file_decoder_get_state(decoder)]);

	if(data){
		//_decoder_session* session = (_decoder_session*)data;
		playback_stop();
	}
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

#ifdef NEVER
FLAC__StreamDecoderReadStatus
flac_read_cb(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data)
{
	/*
	The address of the buffer to be filled is supplied, along with the number of bytes the buffer can hold

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
#endif


FLAC__StreamDecoderWriteStatus
flac_write_cb_seekable(const FLAC__FileDecoder *dec, const FLAC__Frame *frame, const FLAC__int32 * const buf[], void *data)
{
  return 0;
}

#endif //flac
