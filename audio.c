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



