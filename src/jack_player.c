/*
  JACK audio player
  This file is part of the Samplecat project.

  copyright (C) 2006-2016 Tim Orford <tim@orford.org>
  copyright (C) 2011 Robin Gareus <robin@gareus.org>

  written by Robin Gareus

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

#include "debug/debug.h"
#include "decoder/ad.h"
#include "support.h"
#include "application.h"
#include "model.h"
#include "sample.h"
#include "ladspa_proc.h"
#include "jack_player.h"

#if __GNUC__ > 6
#pragma GCC diagnostic ignored "-Wformat-truncation"
#endif

#ifdef HAVE_JACK

#ifdef __APPLE__  /* weak linking on OSX */
#include <jack/weakjack.h>
#endif
#include <jack/jack.h>

#ifndef __APPLE__ 
#include <jack/ringbuffer.h>
#else /* weak linking on OSX - ringbuffer is not exported as weak */
typedef struct {
    char *buf;
    size_t len;
}
jack_ringbuffer_data_t ;

typedef struct {
    char	*buf;
    volatile size_t write_ptr;
    volatile size_t read_ptr;
    size_t	size;
    size_t	size_mask;
    int	mlocked;
}
jack_ringbuffer_t ;

jack_ringbuffer_t* jack_ringbuffer_create       (size_t sz)                               JACK_OPTIONAL_WEAK_EXPORT;
void               jack_ringbuffer_free         (jack_ringbuffer_t*)                      JACK_OPTIONAL_WEAK_EXPORT;
void               jack_ringbuffer_read_advance (jack_ringbuffer_t*, size_t cnt)          JACK_OPTIONAL_WEAK_EXPORT;
size_t             jack_ringbuffer_read_space   (const jack_ringbuffer_t*)                JACK_OPTIONAL_WEAK_EXPORT;
int                jack_ringbuffer_mlock        (jack_ringbuffer_t*)                      JACK_OPTIONAL_WEAK_EXPORT;
size_t             jack_ringbuffer_write        (jack_ringbuffer_t*, const char*, size_t) JACK_OPTIONAL_WEAK_EXPORT;
size_t             jack_ringbuffer_read         (jack_ringbuffer_t*, char*, size_t)       JACK_OPTIONAL_WEAK_EXPORT;
size_t             jack_ringbuffer_write_space  (const jack_ringbuffer_t*)                JACK_OPTIONAL_WEAK_EXPORT;
#endif


static jack_client_t *j_client = NULL;
static jack_port_t **j_output_port = NULL;
static jack_default_audio_sample_t **j_out = NULL;
static jack_ringbuffer_t *rb = NULL;


#ifdef JACK_MIDI
//#warning COMPILING W/ JACK-MIDI PLAYER TRIGGER
#include <jack/midiport.h>
static jack_port_t *jack_midi_port = NULL;

#define JACK_MIDI_QUEUE_SIZE (1024)
typedef struct my_midi_event {
	jack_nframes_t time;
	size_t size;
	jack_midi_data_t buffer[16];
} my_midi_event_t;

my_midi_event_t event_queue[JACK_MIDI_QUEUE_SIZE];
int queued_events_start = 0;
int queued_events_end = 0;

static int midi_thread_run = 0;
pthread_t midi_thread_id;
pthread_mutex_t midi_thread_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  midi_ready = PTHREAD_COND_INITIALIZER;
static int midi_note = 0;
static int midi_octave = 0;
#endif

static int   jplay__check      ();
static void  jplay__stop       ();
static void  jplay__connect    (ErrorCallback, gpointer);
static void  jplay__disconnect ();
static bool  jplay__play       (Sample*);

const Auditioner a_jack = {
   &jplay__check,
   &jplay__connect,
   &jplay__disconnect,
   &jplay__play,
   NULL,
   &jplay__stop,
   &jplay__pause,
   &jplay__seek,
   &jplay__getposition
};

static int      jplay__play_pathX (const char* path, int reset_pitch);

static WfDecoder* myplayer;
static LadSpa** myplugin = NULL;
static int      m_samplerate = 0;
static int64_t  m_frames = 0;
static int      thread_run = 0;
static int      player_active = 0;
static volatile int silent = 0;
static volatile int playpause = 0;             // internal flag checked periodically in the jack thread.
static volatile double seek_request = -1.0;
static int64_t  play_position = 0;

#if (defined ENABLE_LADSPA)
static const char* m_use_effect = "ladspa-rubberband.so";
#endif
static const int   m_effectno   = 0;

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

#define DEFAULT_RB_SIZE 24575

static void JACKclose();
static void JACKconnect();
static void JACKdisconnect();


int jack_audio_callback(jack_nframes_t nframes, void *arg) {
	int c,s;
	WfAudioInfo* nfo = &myplayer->info;

#ifdef JACK_MIDI
	int n;
	void *jack_buf = jack_port_get_buffer(jack_midi_port, nframes);
	int nevents = jack_midi_get_event_count(jack_buf);
	for (n=0; n<nevents; n++) {
		jack_midi_event_t ev;
		jack_midi_event_get(&ev, jack_buf, n);

		if (ev.size < 3 || ev.size > 3) continue; // filter note on/off
		else {
			event_queue[queued_events_end].time = ev.time;
			event_queue[queued_events_end].size = ev.size;
			memcpy (event_queue[queued_events_end].buffer, ev.buffer, ev.size);
			queued_events_end = (queued_events_end +1 ) % JACK_MIDI_QUEUE_SIZE;
		}
	}
	if (queued_events_start != queued_events_end) {
		/* Tell the midi thread there is work to do. */
		if(pthread_mutex_trylock(&midi_thread_lock) == 0) {
			pthread_cond_signal(&midi_ready);
			pthread_mutex_unlock(&midi_thread_lock);
		}
	}
#endif

	if (!player_active) return (0);

	for(c=0; c<nfo->channels; c++) {
		j_out[c] = (jack_default_audio_sample_t*) jack_port_get_buffer(j_output_port[c], nframes);
	}

	if(playpause || jack_ringbuffer_read_space(rb) < nfo->channels * nframes * sizeof(jack_default_audio_sample_t)) {
		silent = 1;
		for(c=0; c< nfo->channels; c++) {
			memset(j_out[c], 0, nframes * sizeof(jack_default_audio_sample_t));
		}
	} else {
		silent=0;
		/* de-interleave */
		for(s=0; s<nframes; s++) {
			for(c=0; c< nfo->channels; c++) {
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

#ifdef JACK_MIDI
gboolean idle_play(gpointer data) {
	application_play_selected();
	return G_SOURCE_REMOVE;
}

void *jack_midi_thread(void *unused){

	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	pthread_mutex_lock(&midi_thread_lock);
	while(midi_thread_run) {
		while (queued_events_start != queued_events_end) {
			my_midi_event_t *ev = &event_queue[queued_events_start];
			if (ev->size != 3) continue;
			if ((ev->buffer[0]&0x80) != 0x80) continue;
			int note = ev->buffer[1]&0x7f;
			//int velo = ev->buffer[2]&0x7f;
			if ((ev->buffer[0]&0x10)==0 || (ev->buffer[2]==0)) {
				dbg (0, "NOTE OFF: %d", note);
			} else {
#define BASE_NOTE (69) ///< untransposed sample on this key.
				midi_note= (132+note-(BASE_NOTE))%12;
				midi_octave= floor((note-BASE_NOTE)/12.0);
				if (midi_octave<-3) { midi_octave=-3; midi_note=0;}
				if (midi_octave> 3) { midi_octave=3;  midi_note=11;}
				dbg (0, "NOTE On : %d -- %d %d", note, midi_octave, midi_note);
				g_idle_add(idle_play, NULL);
			}
			queued_events_start = (queued_events_start +1 ) % JACK_MIDI_QUEUE_SIZE;
		}
		pthread_cond_wait(&midi_ready, &midi_thread_lock);
	}
	pthread_mutex_unlock(&midi_thread_lock);
	return NULL;
}
#endif

void jack_shutdown_callback(void *arg){
	dbg(0, "jack server [unexpectedly] shut down.");
	j_client = NULL;
	JACKdisconnect();
}

static inline void
update_playposition (int64_t decoder_position, float varispeed)
{
#ifdef ENABLE_RESAMPLING
	const int64_t latency = floor(jack_ringbuffer_read_space(rb) / myplayer->info.channels / sizeof(float) / m_fResampleRatio / varispeed);
#else
	const int64_t latency = floor(jack_ringbuffer_read_space(rb) / myplayer->info.channels);
#endif
	if (!app->config.loop_playback || decoder_position>latency)
		play_position = decoder_position - latency;
	else
		play_position = m_frames + decoder_position - latency;
}

void *jack_player_thread(void *unused){
	WfAudioInfo* nfo = &myplayer->info;
	const int nframes = 1024;
	float *tmpbuf = (float*) calloc(nframes * nfo->channels, sizeof(float));
	float *bufptr = tmpbuf;
#ifdef ENABLE_RESAMPLING
	size_t maxbufsiz = nframes;
	int err = 0;
	SRC_STATE* src_state = src_new(SRC_QUALITY, nfo->channels, NULL);
	SRC_DATA src_data;
	int nframes_r = floorf((float) nframes*m_fResampleRatio); ///< # of frames after resampling
#ifdef VARISPEED
	maxbufsiz = (2+nframes_r) * 2;
#else
	maxbufsiz = (1+nframes_r);
#endif
	float *smpbuf = (float*) calloc(maxbufsiz * nfo->channels, sizeof(float));

	src_data.input_frames  = nframes;
	src_data.output_frames = nframes_r;
	src_data.end_of_input  = 0;
	src_data.src_ratio     = m_fResampleRatio;
	src_data.input_frames_used = 0;
	src_data.output_frames_gen = 0;
	src_data.data_in       = tmpbuf;
	src_data.data_out      = smpbuf;
#else
	int nframes_r = nframes; // no resampling
#endif

	int ladspaerr = 0;
#if (defined ENABLE_LADSPA)
	if (m_use_effect && app->enable_effect) {
		int i;
		for(i=0;i<nfo->channels;i++) {
			ladspaerr |= ladspah_init(myplugin[i], m_use_effect, m_effectno, 
					jack_get_sample_rate(j_client) /* m_samplerate */, maxbufsiz);
		}
		if (ladspaerr) {
			dbg(0, "error setting up LADSPA plugin - effect disabled.\n");
		}
	} else {
		ladspaerr = 1;
	}
	app->effect_enabled = ladspaerr ? false : true; // read-only for GUI
#endif

	size_t rbchunk = nframes_r * nfo->channels * sizeof(float);
	play_position = 0;
	int64_t decoder_position = 0;

	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	pthread_mutex_lock(&player_thread_lock);

	/** ALL SYSTEMS GO **/
	player_active = 1;
	while(thread_run) {
		int rv = ad_read(myplayer, tmpbuf, nframes * nfo->channels);

		if (rv > 0) decoder_position += rv / nfo->channels;

#ifdef JACK_MIDI
		const float pp[3] = {app->effect_param[0], app->effect_param[1] + midi_note, app->effect_param[2] + midi_octave};
#else
		const float pp[3] = {app->effect_param[0], app->effect_param[1], app->effect_param[2]};
#endif
#if (defined ENABLE_RESAMPLING) && (defined VARISPEED)
		const float varispeed = app->playback_speed;
#if 0
		/* note: libsamplerate slowly approaches a new sample-rate slowly
		 * src_set_ratio() allow for immediate updates at loss of quality */
		static float oldspd = -1;
		if (oldspd != varispeed) {
			if ((err = src_set_ratio(src_state, m_fResampleRatio * varispeed))) dbg(0, "SRC ERROR: %s", src_strerror(err)); // instant change.
			oldspd = varispeed;
		}
#endif
		nframes_r = floorf((float) nframes * m_fResampleRatio * varispeed); ///< # of frames after resampling
		src_data.input_frames  = nframes;
		src_data.output_frames = nframes_r;
		src_data.src_ratio     = m_fResampleRatio * varispeed;
		src_data.end_of_input  = 0;
#endif

		if (rv != nframes * nfo->channels) {
			dbg(1, "end of file.");
			if (rv > 0) {
#ifdef ENABLE_RESAMPLING
# ifdef VARISPEED
				if (1) 
# else
				if(m_fResampleRatio != 1.0)
# endif
				{
					src_data.input_frames = rv / nfo->channels;
#ifdef VARISPEED
					src_data.output_frames = floorf((float)(rv / nfo->channels) * m_fResampleRatio * varispeed);
#else
					src_data.output_frames = floorf((float)(rv / nfo->channels) * m_fResampleRatio);
#endif
					src_data.end_of_input = app->config.loop_playback ? 0 : 1;
					src_process(src_state, &src_data);
					bufptr = smpbuf;
					rv = src_data.output_frames_gen * nfo->channels;
				}
#endif
				if (!ladspaerr) {
					ladspah_set_param(myplugin, nfo->channels, 0 /*cents */, pp[0]); /* -100 .. 100 */
					ladspah_set_param(myplugin, nfo->channels, 1 /*semitones */, pp[1]); /* -12 .. 12 */
					ladspah_set_param(myplugin, nfo->channels, 2 /*octave */, pp[2]);    /*  -3 ..  3 */
					ladspah_process_nch(myplugin, nfo->channels, bufptr, rv / nfo->channels);
				}
				jack_ringbuffer_write(rb, (char *) bufptr, rv *  sizeof(float));
			}
			if (app->config.loop_playback) {
				ad_seek(myplayer, 0);
				decoder_position = 0;
#ifdef ENABLE_RESAMPLING
				//src_reset (src_state); // XXX causes click
# ifdef VARISPEED
				if (1) 
# else
				if(m_fResampleRatio != 1.0)
# endif
				{
					src_data.end_of_input  = 0;
					src_data.input_frames  = nframes;
					src_data.output_frames = nframes_r;
				}
#endif
#if (defined ENABLE_RESAMPLING) && (defined VARISPEED)
				while(thread_run && jack_ringbuffer_write_space(rb) <= maxbufsiz*nfo->channels*sizeof(float))
#else
				while(thread_run && jack_ringbuffer_write_space(rb) <= rbchunk)
#endif
				{
					pthread_cond_wait(&buffer_ready, &player_thread_lock);
				}
				continue;
			}
			else
				break;
		} /* end EOF handling */

#ifdef ENABLE_RESAMPLING
#ifdef VARISPEED
		if(1) // m_fResampleRatio*varispeed != 1.0)
#else
		if(m_fResampleRatio != 1.0)
#endif
		{
			if ((err=src_process(src_state, &src_data))) {
				dbg(0, "SRC PROCESS ERROR: %s", src_strerror(err));
			}
			bufptr = smpbuf;
			rbchunk = src_data.output_frames_gen * nfo->channels * sizeof(float);
		}
#endif
		if (!ladspaerr) {
			ladspah_set_param(myplugin, nfo->channels, 0 /*cent */, pp[0]); /* -100 .. 100 */
			ladspah_set_param(myplugin, nfo->channels, 1 /*semitones */, pp[1]); /* -12 .. 12 */
			ladspah_set_param(myplugin, nfo->channels, 2 /*octave */, pp[2]);    /*  -3 ..  3 */
			ladspah_process_nch(myplugin, nfo->channels, bufptr, rbchunk/ nfo->channels / sizeof(float));
		}
		jack_ringbuffer_write(rb, (char *) bufptr, rbchunk);

#if (defined ENABLE_RESAMPLING) && (defined VARISPEED)
		while(thread_run && jack_ringbuffer_write_space(rb) <= maxbufsiz*nfo->channels*sizeof(float))
#else
		while(thread_run && jack_ringbuffer_write_space(rb) <= rbchunk) 
#endif
		{
#if 1 // flush_ringbuffer on seek
			if (playpause && seek_request != -1) break; 
#endif
			pthread_cond_wait(&buffer_ready, &player_thread_lock);
		}

		/* Handle SEEKs */
		while (seek_request>=0 && seek_request<=1) {
			double csr = seek_request;
			decoder_position = floor(m_frames * seek_request);
			ad_seek(myplayer, decoder_position);
			if (csr == seek_request) {
				seek_request = -1;
#if 1 // flush_ringbuffer !
				size_t rs=jack_ringbuffer_read_space(rb);
				jack_ringbuffer_read_advance(rb,rs);
#endif
				break;
			}
		}
#if (defined ENABLE_RESAMPLING) && (defined VARISPEED)
		update_playposition (decoder_position, varispeed);
#else
		update_playposition (decoder_position, 1.0);
#endif
	}

	/** END OF PLAYBACK **/
#if (defined ENABLE_RESAMPLING) && (defined VARISPEED)
	const float varispeed = app->playback_speed;
#else
	const float varispeed = 1.0;
#endif
	pthread_mutex_unlock(&player_thread_lock);

	if (thread_run) {
		// wait for ringbuffer to empty.
		while (!silent && !playpause) {
			usleep(20);
			update_playposition (decoder_position, varispeed);
		}
		thread_run = 0;
		player_active = 0;
		app->effect_enabled = false;
		int i;
		for(i=0;i<nfo->channels;i++) {
			ladspah_deinit(myplugin[i]);
		}
		JACKclose(); 
	}
	free(tmpbuf);
#ifdef ENABLE_RESAMPLING
	src_delete(src_state);
	free(smpbuf);
#endif
	player_active = 0;
  	if(app->play.status != PLAY_PLAY_PENDING){
		application_play_next();
	}
	return NULL;
}

/* JACK player control */
void
JACKaudiooutputinit(WfDecoder* d)
{
	int i;

	if(!j_client) JACKconnect();
	if(!j_client) return;

	if(thread_run || rb) {
		dbg(0, "already playing.");
		return;
	}

	myplayer = d;
	int channels = d->info.channels;
	int samplerate = d->info.sample_rate;
	m_frames = d->info.frames;
	m_samplerate = samplerate;
	playpause = silent = 0;
	play_position = 0;

	j_output_port = (jack_port_t**) calloc(channels, sizeof(jack_port_t*));

	for(i=0;i<channels;i++) {
		char channelid[16];
		snprintf(channelid, 16, "output_%i", i);
		j_output_port[i] = jack_port_register(j_client, channelid, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
		if(!j_output_port[i]) {
			dbg(0, "no more jack ports availabe.");
			JACKclose();
			return;
		}
	}

	myplugin = malloc(channels*sizeof(LadSpa*));
	for(i=0;i<channels;i++) {
		myplugin[i] = ladspah_alloc();
	}

	j_out = (jack_default_audio_sample_t**) calloc(channels, sizeof(jack_default_audio_sample_t*));
	const size_t rbsize = DEFAULT_RB_SIZE * channels * sizeof(jack_default_audio_sample_t);
	rb = jack_ringbuffer_create(rbsize);
	jack_ringbuffer_mlock(rb);
	memset(rb->buf, 0, rbsize);

	jack_nframes_t jsr = jack_get_sample_rate(j_client);

#ifdef ENABLE_RESAMPLING
	m_fResampleRatio = 1.0;
#endif
	if(jsr != samplerate) {
#ifdef ENABLE_RESAMPLING
		m_fResampleRatio = (float) jsr / (float) samplerate;
		dbg(2, "resampling %d -> %d (f:%.2f).", samplerate, jsr, m_fResampleRatio);
#else
		dbg(0, "audio samplerate does not match JACK's samplerate.");
#endif
	}

#if (defined ENABLE_RESAMPLING) && !(defined VARISPEED)
	m_fResampleRatio *= app->playback_speed; ///< fixed speed change. <1.0: speed-up, > 1.0: slow-down
#endif

	thread_run = 1;
	pthread_create(&player_thread_id, NULL, jack_player_thread, NULL);
	sched_yield();

#if 1
	char *jack_autoconnect = app->config.jack_autoconnect;
	if(!jack_autoconnect || strlen(jack_autoconnect)<1) {
		jack_autoconnect = (char*) "system:playback_";
	} else if(!strncmp(jack_autoconnect,"DISABLE", 7)) {
		jack_autoconnect = NULL;
	}
	if(jack_autoconnect) {
		int myc=0;
		dbg(1, "JACK connect to '%s'", jack_autoconnect);
		const char **found_ports = jack_get_ports(j_client, jack_autoconnect, NULL, JackPortIsInput);
		for(i = 0; found_ports && found_ports[i]; i++) {
			if(jack_connect(j_client, jack_port_name(j_output_port[myc]), found_ports[i])) {
				dbg(0, "cannot connect to jack output");
			}
			if(myc >= channels) break;
		}
	}
#endif
}

static void JACKclose() {
	if(thread_run) {
		thread_run = 0;
		if(pthread_mutex_trylock(&player_thread_lock) == 0) {
			pthread_cond_signal(&buffer_ready);
			pthread_mutex_unlock(&player_thread_lock);
		}
		pthread_join(player_thread_id, NULL);
	}
	player_active = 0;

	if(j_output_port) {
		int i;
		for(i=0;i<myplayer->info.channels;i++) {
			jack_port_unregister(j_client, j_output_port[i]);
		}
		free(j_output_port);
		j_output_port = NULL;
	}
	if(j_out) {
		free(j_out);
		j_out = NULL;
	}
	if(rb) {
		jack_ringbuffer_free(rb);
		rb = NULL;
	}
	if (myplugin) {
		int i;
		for(i=0;i<myplayer->info.channels;i++) ladspah_free(myplugin[i]);
		free(myplugin);
	}
	if (myplayer){
		ad_close(myplayer);
		ad_free_nfo(&myplayer->info);
		g_free0(myplayer);
	}
	myplugin = NULL;
};

static void JACKconnect() {
	dbg(1, "...");

	j_client = jack_client_open("samplecat", (jack_options_t) 0, NULL);

	if(!j_client) {
		dbg(0, "could not connect to JACK.");
		return;
	}

	jack_on_shutdown(j_client, jack_shutdown_callback, NULL);
	jack_set_process_callback(j_client, jack_audio_callback, NULL);

#ifdef JACK_MIDI
	jack_midi_port = jack_port_register(j_client, "Midi in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput , 0);
	if (jack_midi_port == NULL) {
		 dbg(0, "can't register jack-midi-port\n");
	}

	midi_thread_run = 1;
	pthread_create(&midi_thread_id, NULL, jack_midi_thread, NULL);
	sched_yield();
#endif

	jack_activate(j_client);
	if(_debug_) printf("jack activated\n");

#ifdef JACK_MIDI
	char *jack_midiconnect = app->config.jack_midiconnect;
	if(!jack_midiconnect || strlen(jack_midiconnect)<1) {
		jack_midiconnect = NULL;
	} else if(!strncmp(jack_midiconnect,"DISABLE", 7)) {
		jack_midiconnect = NULL;
	}
	if(jack_midiconnect) {
		dbg(1, "MIDI autoconnect '%s' -> '%s'", jack_midiconnect, jack_port_name(jack_midi_port));
		if(jack_connect(j_client, jack_midiconnect, jack_port_name(jack_midi_port))) {
			dbg(0, "Auto-connect jack midi port failed.");
		}
	}
#endif
}

static void JACKdisconnect() {
	JACKclose();
#ifdef JACK_MIDI
	if(midi_thread_run) {
		midi_thread_run = 0;
		if(pthread_mutex_trylock(&midi_thread_lock) == 0) {
			pthread_cond_signal(&midi_ready);
			pthread_mutex_unlock(&midi_thread_lock);
		}
		pthread_join(midi_thread_id, NULL);
	}
#endif

	if(j_client){
		//jack_deactivate(j_client);
		jack_client_close(j_client);
	}
	j_client = NULL;
}

/**
 * play audio-file with given file-path.
 * @param path absolute path to the file.
 * @param reset_pitch 1: reset set midi pitch adj.
 * @return 0 on success, -1 on error.
 */
static int
jplay__play_pathX(const char* path, int reset_pitch)
{
#ifdef JACK_MIDI
	if (reset_pitch) {
		midi_note = 0;
		midi_octave = 0;
	}
#endif

	WfDecoder* d = g_new0(WfDecoder, 1);
	if(!ad_open(d, path)) return -1;

	seek_request = -1.0;
	if(thread_run || rb){
		jplay__stop();
	}

	JACKaudiooutputinit(d);
	return 0;
}


/* SampleCat API */

static void jplay__connect(ErrorCallback callback, gpointer user_data) {
	JACKconnect();
	callback(NULL, user_data);
}

static void jplay__disconnect() {
	JACKdisconnect();
}

static bool
jplay__play(Sample* sample)
{
	return !jplay__play_pathX(sample->full_path, 1);
}

static void
jplay__stop()
{
	JACKclose();
}

void jplay__seek (double pos) {
	if(!app->play.sample) return;
	if(!myplayer) return;
	seek_request = pos * app->play.sample->sample_rate / (1000 * app->play.sample->frames);
}


int jplay__pause (int on) {
	if(!myplayer) return -1;

	// set the playpause flag for the jack thread
	return playpause = (on == 1)
		? 1
		: (on == 0)
			? 0
			: !playpause;
}

guint jplay__getposition() {
	if(!app->play.sample) return -1;
	if(seek_request != -1.0) return UINT_MAX;
	if(!myplayer) return -1;
	if(m_frames < 1) return -1;

	if(!app->play.sample->sample_rate) return UINT_MAX;

	return MAX(0, (play_position * 1000 / app->play.sample->sample_rate));
}

static int jplay__check() {
#ifdef __APPLE__
	/* weak linking on OSX test */
	if (!jack_client_open) return -1; /* FAIL */
#endif
	// TODO: check for getenv("JACK_NO_START_SERVER")
	// try connect to jackd,..
	return 0; /*OK*/
}
#endif /* HAVE_JACK */
