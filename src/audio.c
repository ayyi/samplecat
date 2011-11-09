/*
  This file is part of the Samplecat project.
  copyright (C) 2006-2007 Tim Orford <tim@orford.org>
  copyright (C) 2011 Robin Gareus <robin@gareus.org>

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
#include <math.h>
#include <pthread.h>
#include <jack/ringbuffer.h>

#include "typedefs.h"
#include "sample.h"
#include "support.h"
#include "main.h"
#include "audio.h"

extern struct _app app;

#if (defined HAVE_JACK && !(defined USE_DBUS || defined USE_GAUDITION))

_audition audition;

jack_client_t *j_client = NULL;
jack_port_t **j_output_port = NULL;
jack_default_audio_sample_t **j_out = NULL;
jack_ringbuffer_t *rb = NULL;
void *myplayer;
int m_channels = 2;
int thread_run = 0;
volatile int silent = 0;

pthread_t player_thread_id;
pthread_mutex_t player_thread_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  buffer_ready = PTHREAD_COND_INITIALIZER;

#ifdef ENABLE_RESAMPLING
#include <samplerate.h>
float m_fResampleRatio = 1.0;

#ifndef SRC_QUALITY // alternatives: SRC_SINC_MEDIUM_QUALITY, SRC_SINC_BEST_QUALITY, (SRC_ZERO_ORDER_HOLD, SRC_LINEAR)
#define SRC_QUALITY SRC_SINC_FASTEST
#endif

#endif

#define DEFAULT_RB_SIZE 16384

void JACKclose();

int jack_audio_callback(jack_nframes_t nframes, void *arg) {
	int c,s;

	for(c=0; c< m_channels; c++) {
		j_out[c] = (jack_default_audio_sample_t*) jack_port_get_buffer(j_output_port[c], nframes);
	}

	if(jack_ringbuffer_read_space(rb) < m_channels * nframes * sizeof(jack_default_audio_sample_t)) {
		silent=1;
		for(c=0; c< m_channels; c++) {
			memset(j_out[c], 0, nframes * sizeof(jack_default_audio_sample_t));
		}
	} else {
		silent=0;
		/* de-interleave */
		for(s=0; s<nframes; s++) {
			for(c=0; c< m_channels; c++) {
				jack_ringbuffer_read(rb, (char*) &j_out[c][s], sizeof(jack_default_audio_sample_t));
			}
		}
	}

	/* Tell the player thread there is work to do. */
	if(pthread_mutex_trylock(&player_thread_lock) == 0) {
		pthread_cond_signal(&buffer_ready);
		pthread_mutex_unlock(&player_thread_lock);
	}

	return(0);
};


void jack_shutdown_callback(void *arg){
	dbg(0, "jack server [unexpectedly] shut down.");
	j_client=NULL;
	JACKclose();
}

void *jack_player_thread(void *unused){
	const int nframes = 4096;
	float *tmpbuf = (float*) calloc(nframes * m_channels, sizeof(float));
	float *bufptr = tmpbuf;
#ifdef ENABLE_RESAMPLING
	SRC_STATE* src_state = src_new(SRC_QUALITY, m_channels, NULL);
	SRC_DATA src_data;
	int nframes_r = floorf((float) nframes*m_fResampleRatio); ///< # of frames after resampling
	float *smpbuf = (float*) calloc((1+nframes_r) * m_channels, sizeof(float));

	src_data.input_frames  = nframes;
	src_data.output_frames = nframes_r;
	src_data.end_of_input  = 0;
	src_data.src_ratio     = m_fResampleRatio;
	src_data.input_frames_used = 0;
	src_data.output_frames_gen = 0;
	src_data.data_in       = tmpbuf;
	src_data.data_out      = smpbuf;
#else
	int nframes_r = nframes;
#endif

	size_t rbchunk = nframes_r * m_channels * sizeof(float);

	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	pthread_mutex_lock(&player_thread_lock);

	while(thread_run) {
		int rv = ad_read_float(myplayer, tmpbuf, nframes * m_channels);
		if (rv != nframes * m_channels) {
			dbg(0, "end of file.");
			if (rv>0) {
#ifdef ENABLE_RESAMPLING
				if(m_fResampleRatio != 1.0) {
					src_data.input_frames = rv / m_channels;
					src_data.output_frames = floorf((float)(rv / m_channels)*m_fResampleRatio);
					src_data.end_of_input=1;
					src_process(src_state, &src_data);
					bufptr = smpbuf;
					rv = src_data.output_frames_gen * m_channels;
				}
#endif
				jack_ringbuffer_write(rb, (char *) bufptr, rv *  sizeof(float));
			}
			break;
		}

#ifdef ENABLE_RESAMPLING
		if(m_fResampleRatio != 1.0) {
			src_process(src_state, &src_data);
			bufptr = smpbuf;
			rbchunk = src_data.output_frames_gen * m_channels * sizeof(float);
		}
#endif
		jack_ringbuffer_write(rb, (char *) bufptr, rbchunk);

		while(thread_run && jack_ringbuffer_write_space(rb) <= rbchunk) {
			pthread_cond_wait(&buffer_ready, &player_thread_lock);
		}
	}

	pthread_mutex_unlock(&player_thread_lock);

	if (thread_run) {
		// wait for ringbuffer to empty.
		while (!silent) usleep(50);
		thread_run=0;
		JACKclose();
	}
	free(tmpbuf);
#ifdef ENABLE_RESAMPLING
	src_delete(src_state);
	free(smpbuf);
#endif
	return NULL;
}

/* JACK player control */
void JACKaudiooutputinit(void *sf, int channels, int samplerate){
	int i;

	if(j_client || thread_run || rb) {
			dbg(0, "already connected to jack.");
			return;
	}

	myplayer=sf;
	m_channels = channels;

	j_client = jack_client_open("samplecat", (jack_options_t) 0, NULL);

	if(!j_client) {
			dbg(0, "could not connect to JACK.");
			return;
	}        

	jack_on_shutdown(j_client, jack_shutdown_callback, NULL);
	jack_set_process_callback(j_client, jack_audio_callback, NULL);

	j_output_port=(jack_port_t**) calloc(m_channels, sizeof(jack_port_t*));

	for(i=0;i<m_channels;i++) {
		char channelid[16];
		snprintf(channelid, 16, "output_%i", i);
		j_output_port[i] = jack_port_register(j_client, channelid, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
		if(!j_output_port[i]) {
			dbg(0, "no more jack ports availabe.");
			JACKclose();
			return;
		}
	}

	j_out = (jack_default_audio_sample_t**) calloc(m_channels, sizeof(jack_default_audio_sample_t*));
	const size_t rbsize = DEFAULT_RB_SIZE * m_channels * sizeof(jack_default_audio_sample_t);
	rb = jack_ringbuffer_create(rbsize);
	memset(rb->buf, 0, rbsize);

	jack_nframes_t jsr = jack_get_sample_rate(j_client);
	if(jsr != samplerate) {
#ifdef ENABLE_RESAMPLING
		m_fResampleRatio = (float) jsr / (float) samplerate;
#else
		dbg(0, "audio samplerate does not match JACK's samplerate.");
#endif
	}

	thread_run = 1;
	pthread_create(&player_thread_id, NULL, jack_player_thread, NULL);
	sched_yield();

	jack_activate(j_client);

#if 1
	char *jack_autoconnect = getenv("JACK_AUTOCONNECT");
	if(!jack_autoconnect) {
		jack_autoconnect = (char*) "system:playback_";
	} else if(!strncmp(jack_autoconnect,"DISABLE", 7)) {
		jack_autoconnect = NULL;
	}
	if(jack_autoconnect) {
		int myc=0;
		const char **found_ports = jack_get_ports(j_client, jack_autoconnect, NULL, JackPortIsInput);
		for(i = 0; found_ports && found_ports[i]; i++) {
				if(jack_connect(j_client, jack_port_name(j_output_port[myc]), found_ports[i])) {
						dbg(0, "can not connect to jack output");
				}
				if(myc >= m_channels) break;
		}
	}
#endif
}

void JACKclose() {
	if(thread_run) {
		thread_run = 0;
		if(pthread_mutex_trylock(&player_thread_lock) == 0) {
			pthread_cond_signal(&buffer_ready);
			pthread_mutex_unlock(&player_thread_lock);
		}
		pthread_join(player_thread_id, NULL);
	}
	if(j_client){
		//jack_deactivate(j_client);
		jack_client_close(j_client);
	}
	if(j_output_port) {
		free(j_output_port);
		j_output_port=NULL;
	}
	if(j_out) {
		free(j_out);
		j_out = NULL;
	}
	if(rb) {
		jack_ringbuffer_free(rb);
		rb=NULL;
	}
	if (myplayer) ad_close(myplayer);
	myplayer=NULL;
	j_client=NULL;
	app.playing_id = 0;
};


/* SampleCat API */

int
playback_init(Sample* sample)
{
	if(app.playing_id) playback_stop(); //stop any previous playback.
	if(j_client) playback_stop();

	struct adinfo nfo;

	if(!sample->id){ perr("bad arg: id=0\n"); return 0; }
	void *sf = ad_open(sample->filename, &nfo);
	if (!sf) return false;
	JACKaudiooutputinit(sf, nfo.channels, nfo.sample_rate);
	app.playing_id = sample->id;
	return TRUE;
}


void
playback_stop()
{
	JACKclose();
}
#endif /* HAVE_JACK */
