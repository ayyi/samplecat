/*
  JACK audio player
  This file is part of the Samplecat project.

  copyright (C) 2006-2007 Tim Orford <tim@orford.org>
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

#include "jack_player.h"
#include "listview.h"
#include "support.h"
#include "main.h"
#include "inspector.h"
#include "sample.h"
#include "ladspa_proc.h"
#include "audio_decoder/ad.h"

extern struct _app app;
#define JACK_MIDI 1 // use JACK midi to trigger samples.


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

jack_ringbuffer_t *jack_ringbuffer_create(size_t sz) JACK_OPTIONAL_WEAK_EXPORT;
void jack_ringbuffer_free(jack_ringbuffer_t *rb) JACK_OPTIONAL_WEAK_EXPORT;
void jack_ringbuffer_read_advance(jack_ringbuffer_t *rb, size_t cnt) JACK_OPTIONAL_WEAK_EXPORT;
size_t jack_ringbuffer_read_space(const jack_ringbuffer_t *rb) JACK_OPTIONAL_WEAK_EXPORT;
int jack_ringbuffer_mlock(jack_ringbuffer_t *rb) JACK_OPTIONAL_WEAK_EXPORT;
size_t jack_ringbuffer_write(jack_ringbuffer_t *rb, const char *src, size_t cnt) JACK_OPTIONAL_WEAK_EXPORT;
size_t jack_ringbuffer_read(jack_ringbuffer_t *rb, char *dest, size_t cnt) JACK_OPTIONAL_WEAK_EXPORT;
size_t jack_ringbuffer_write_space(const jack_ringbuffer_t *rb) JACK_OPTIONAL_WEAK_EXPORT;
#endif


static jack_client_t *j_client = NULL;
static jack_port_t **j_output_port = NULL;
static jack_default_audio_sample_t **j_out = NULL;
static jack_ringbuffer_t *rb = NULL;


#ifdef JACK_MIDI
#warning COMPILING W/ JACK-MIDI PLAYER TRIGGER
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

static int      midi_thread_run = 0;
pthread_t midi_thread_id;
pthread_mutex_t midi_thread_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  midi_ready = PTHREAD_COND_INITIALIZER;
static int midi_note = 0;
static int midi_octave = 0;
#endif

static void *   myplayer;
static LadSpa** myplugin = NULL;
static int      m_channels   = 2;
static int      m_samplerate = 0;
static int64_t  m_frames = 0;
static int      thread_run   = 0;
static int      player_active = 0;
static volatile int silent = 0;
static volatile int playpause  = 0;
static volatile double seek_request = -1.0;
static int64_t  play_position =0;

static const char *  m_use_effect = "ladspa-rubberband.so";
static const int     m_effectno   = 0;

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

static GList* play_queue = NULL;
void jplay__play_next();
void JACKclose();
void JACKconnect();
void JACKdisconnect();


int jack_audio_callback(jack_nframes_t nframes, void *arg) {
	int c,s;

#ifdef JACK_MIDI
	int n;
	void *jack_buf = jack_port_get_buffer(jack_midi_port, nframes);
	int nevents = jack_midi_get_event_count(jack_buf);
	for (n=0; n<nevents; n++) {
		jack_midi_event_t ev;
		jack_midi_event_get(&ev, jack_buf, n);

		if (ev.size <3 || ev.size > 3) continue; // filter note on/off
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

	for(c=0; c< m_channels; c++) {
		j_out[c] = (jack_default_audio_sample_t*) jack_port_get_buffer(j_output_port[c], nframes);
	}

	if(playpause || jack_ringbuffer_read_space(rb) < m_channels * nframes * sizeof(jack_default_audio_sample_t)) {
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

#ifdef JACK_MIDI
gboolean idle_play(gpointer data) {
	jplay__play_selected();
	return false;
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
		 int velo = ev->buffer[2]&0x7f;
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
			 //jplay__play_selected();
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
	j_client=NULL;
	JACKdisconnect();
}

inline void
update_playposition (int64_t decoder_position, float varispeed) {
#ifdef ENABLE_RESAMPLING
	const int64_t latency = floor(jack_ringbuffer_read_space(rb)/m_channels/sizeof(float)/m_fResampleRatio/varispeed);
#else
	const int64_t latency = floor(jack_ringbuffer_read_space(rb)/m_channels);
#endif
	if (!app.loop_playback || decoder_position>latency)
		play_position = decoder_position - latency;
	else
		play_position = m_frames+decoder_position-latency;
}

void *jack_player_thread(void *unused){
	int err=0;
	const int nframes = 1024;
	float *tmpbuf = (float*) calloc(nframes * m_channels, sizeof(float));
	float *bufptr = tmpbuf;
	size_t maxbufsiz = nframes;
#ifdef ENABLE_RESAMPLING
	SRC_STATE* src_state = src_new(SRC_QUALITY, m_channels, NULL);
	SRC_DATA src_data;
	int nframes_r = floorf((float) nframes*m_fResampleRatio); ///< # of frames after resampling
#ifdef VARISPEED
	maxbufsiz = (2+nframes_r) * 2;
#else
	maxbufsiz = (1+nframes_r);
#endif
	float *smpbuf = (float*) calloc(maxbufsiz * m_channels, sizeof(float));

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

	int ladspaerr=0;
	if (m_use_effect && app.enable_effect) {
		int i;
		for(i=0;i<m_channels;i++) {
			ladspaerr|= ladspah_init(myplugin[i], m_use_effect, m_effectno, m_samplerate, maxbufsiz);
		}
		if (ladspaerr) {
			dbg(0, "error setting up LADSPA plugin - effect disabled.\n");
		}
	} else {
		ladspaerr=1;
	}
	app.effect_enabled = ladspaerr?false:true; // read-only for GUI

	size_t rbchunk = nframes_r * m_channels * sizeof(float);
	play_position =0;
	int64_t decoder_position = 0;

	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	pthread_mutex_lock(&player_thread_lock);

	/** ALL SYSTEMS GO **/
	player_active=1;
	while(thread_run) {
		int rv = ad_read(myplayer, tmpbuf, nframes * m_channels);
		decoder_position+=rv/m_channels;
#ifdef JACK_MIDI
		const float pp[3] = {app.effect_param[0], app.effect_param[1]+midi_note, app.effect_param[2]+midi_octave};
#else
		const float pp[3] = {app.effect_param[0], app.effect_param[1], app.effect_param[2]};
#endif
#if (defined ENABLE_RESAMPLING) && (defined VARISPEED)
		const float varispeed = app.playback_speed;
#if 0
		/* note: libsamplerate slowly approaches a new sample-rate slowly
		 * src_set_ratio() allow for immediate updates at loss of quality */
		static float oldspd = -1;
		if (oldspd != varispeed) {
			if ((err=src_set_ratio(src_state, varispeed))) dbg(0, "SRC ERROR: %s", src_strerror(err)); // instant change.
			oldspd = varispeed;
		}
#endif
		nframes_r = floorf((float) nframes*m_fResampleRatio*varispeed); ///< # of frames after resampling
		src_data.output_frames = nframes_r;
		src_data.src_ratio     = m_fResampleRatio * varispeed;
#endif

		if (rv != nframes * m_channels) {
			dbg(1, "end of file.");
			if (rv>0) {
#ifdef ENABLE_RESAMPLING
# ifdef VARISPEED
				if (1) 
# else
				if(m_fResampleRatio != 1.0)
# endif
				{
					src_data.input_frames = rv / m_channels;
#ifdef VARISPEED
					src_data.output_frames = floorf((float)(rv / m_channels)*m_fResampleRatio*varispeed);
#else
					src_data.output_frames = floorf((float)(rv / m_channels)*m_fResampleRatio);
#endif
					src_data.end_of_input=1;
					src_process(src_state, &src_data);
					bufptr = smpbuf;
					rv = src_data.output_frames_gen * m_channels;
				}
#endif
				ladspah_set_param(myplugin, m_channels, 0 /*cents */, pp[0]); /* -100 .. 100 */
				ladspah_set_param(myplugin, m_channels, 1 /*semitones */, pp[1]); /* -12 .. 12 */
				ladspah_set_param(myplugin, m_channels, 2 /*octave */, pp[2]);    /*  -3 ..  3 */
				if (!ladspaerr) ladspah_process_nch(myplugin, m_channels, bufptr, rv / m_channels);
				jack_ringbuffer_write(rb, (char *) bufptr, rv *  sizeof(float));
			}
			if (app.loop_playback) {
				ad_seek(myplayer, 0);
				decoder_position=0;
#ifdef ENABLE_RESAMPLING
				src_reset (src_state);
				if(m_fResampleRatio != 1.0){
					src_data.end_of_input=0;
					src_data.input_frames  = nframes;
					src_data.output_frames = nframes_r;
				}
#endif
#if (defined ENABLE_RESAMPLING) && (defined VARISPEED)
				while(thread_run && jack_ringbuffer_write_space(rb) <= maxbufsiz*m_channels*sizeof(float)) 
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
			rbchunk = src_data.output_frames_gen * m_channels * sizeof(float);
		}
#endif
		if (!ladspaerr) {
			ladspah_set_param(myplugin, m_channels, 0 /*cent */, pp[0]); /* -100 .. 100 */
			ladspah_set_param(myplugin, m_channels, 1 /*semitones */, pp[1]); /* -12 .. 12 */
			ladspah_set_param(myplugin, m_channels, 2 /*octave */, pp[2]);    /*  -3 ..  3 */
			ladspah_process_nch(myplugin, m_channels, bufptr, rbchunk/ m_channels / sizeof(float));
		}
		jack_ringbuffer_write(rb, (char *) bufptr, rbchunk);

#if (defined ENABLE_RESAMPLING) && (defined VARISPEED)
		while(thread_run && jack_ringbuffer_write_space(rb) <= maxbufsiz*m_channels*sizeof(float)) 
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
			decoder_position=floor(m_frames * seek_request);
			ad_seek(myplayer, decoder_position);
			if (csr == seek_request) {
				seek_request=-1;
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

#if (defined ENABLE_RESAMPLING) && (defined VARISPEED)
	const float varispeed = app.playback_speed;
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
		thread_run=0;
		player_active=0;
		app.effect_enabled = false;
		int i;
		for(i=0;i<m_channels;i++) {
			ladspah_deinit(myplugin[i]);
		}
		JACKclose(); 
	}
	free(tmpbuf);
#ifdef ENABLE_RESAMPLING
	src_delete(src_state);
	free(smpbuf);
#endif
	player_active=0;
	if(play_queue) jplay__play_next();
	return NULL;
}

/* JACK player control */
void JACKaudiooutputinit(void *sf, int channels, int samplerate, int64_t frames){
	int i;

	if(!j_client) JACKconnect();
	if(!j_client) return;

	if(thread_run || rb) {
			dbg(0, "already playing.");
			return;
	}

	myplayer=sf;
	m_channels = channels;
	m_frames = frames;
	m_samplerate = samplerate;
	playpause = silent = 0;
	play_position =0;

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

	myplugin = malloc(m_channels*sizeof(LadSpa*));
	for(i=0;i<m_channels;i++) {
		myplugin[i] = ladspah_alloc();
	}

	j_out = (jack_default_audio_sample_t**) calloc(m_channels, sizeof(jack_default_audio_sample_t*));
	const size_t rbsize = DEFAULT_RB_SIZE * m_channels * sizeof(jack_default_audio_sample_t);
	rb = jack_ringbuffer_create(rbsize);
	jack_ringbuffer_mlock(rb);
	memset(rb->buf, 0, rbsize);

	jack_nframes_t jsr = jack_get_sample_rate(j_client);
	if(jsr != samplerate) {
#ifdef ENABLE_RESAMPLING
		m_fResampleRatio = (float) jsr / (float) samplerate;
#else
		dbg(0, "audio samplerate does not match JACK's samplerate.");
#endif
	}

#if (defined ENABLE_RESAMPLING) && !(defined VARISPEED)
	m_fResampleRatio *= app.playback_speed; ///< fixed speed change. <1.0: speed-up, > 1.0: slow-down
#endif

	thread_run = 1;
	pthread_create(&player_thread_id, NULL, jack_player_thread, NULL);
	sched_yield();

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
	player_active=0;

	if(j_output_port) {
		int i;
		for(i=0;i<m_channels;i++) {
			jack_port_unregister(j_client, j_output_port[i]);
		}
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
	if (myplugin) {
		int i;
		for(i=0;i<m_channels;i++) ladspah_free(myplugin[i]);
		free(myplugin);
	}
	if (myplayer) ad_close(myplayer);
	myplayer=NULL;
	myplugin=NULL;
	app.playing_id = 0;
};

void JACKconnect() {
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
}

void JACKdisconnect() {
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
	j_client=NULL;
}


/* SampleCat API */

void jplay__connect() {
	JACKconnect();
}

void jplay__disconnect() {
	JACKdisconnect();
}

int
jplay__play_pathX(const char* path, int reset_pitch)
{
	if(app.playing_id) jplay__stop(); //stop any previous playback.
	if(myplayer) jplay__stop();

#ifdef JACK_MIDI
	if (reset_pitch) {
		midi_note =0;
		midi_octave =0;
	}
#endif

	struct adinfo nfo;
	void *sf = ad_open(path, &nfo);
	if (!sf) return -1;
	seek_request = -1.0;
	JACKaudiooutputinit(sf, nfo.channels, nfo.sample_rate, nfo.frames);
	app.playing_id = -1; // ID of filer-inspector (non imported sample)
	show_player();
	return 0;
}

void
jplay__play_path(const char* path)
{
	jplay__play_pathX(path, 1);
}

void
jplay__play(Sample* sample)
{
	if(!sample->id){ perr("bad arg: id=0\n"); return; }
	if (jplay__play_pathX(sample->full_path, 1)) return;
	app.playing_id = sample->id;
	show_player();
}

void
jplay__toggle(Sample* sample)
{
	if(app.playing_id || myplayer) jplay__stop();
	else jplay__play(sample);
}

void
jplay__stop()
{
	if (play_queue) {
		g_list_foreach(play_queue,(GFunc)sample_unref, NULL);
		g_list_free(play_queue);
		play_queue=NULL;
	}
	JACKclose();
}

gboolean idle_highlight(gpointer data) {
	Sample* s = (Sample*) data;
	highlight_playing_by_ref(s->row_ref); 
	jplay__play(s);
	sample_unref(s);
	return false;
}

void
jplay__play_next() {
	if(play_queue){
		Sample* result = play_queue->data;
		play_queue = g_list_remove(play_queue, result);
		dbg(1, "%s", result->full_path);
		g_idle_add(idle_highlight, result);
	}else{
		dbg(1, "play_all finished. disconnecting...");
	}
}

void
jplay__play_selected() {
	dbg(0, "...");
	Sample *sample = sample_get_by_row_ref(app.inspector->row_ref);
	if (!sample) {
		dbg(0, "no sample selected. not starting playback");
		return;
	}
	dbg(0, "MIDI PLAY %s", sample->full_path);

	/* jplay__play(s); - no reset midi pitch */
	if(!sample->id){ perr("bad arg: id=0\n"); return; }
	if (jplay__play_pathX(sample->full_path, 0)) return;
	app.playing_id = sample->id;
	show_player();
	/* jplay__play(s); */

	sample_unref(sample);
}

void
jplay__play_all() {
	dbg(1, "...");
	if(play_queue){
		pwarn("already playing");
		return;
	}

	gboolean foreach_func(GtkTreeModel* model, GtkTreePath* path, GtkTreeIter* iter, gpointer user_data)
	{
		Sample* result = sample_get_from_model(path);
		play_queue = g_list_append(play_queue, result);
		dbg(2, "%s", result->sample_name);
		return FALSE; //continue
	}
	gtk_tree_model_foreach(GTK_TREE_MODEL(app.store), foreach_func, NULL);
	if(play_queue) jplay__play_next();
}

void jplay__seek (double pos) {
	if(!app.playing_id) return;
	if(!myplayer) return;
	seek_request = pos;
}


int jplay__pause (int on) {
	if(!myplayer) return -1;
	if (on==1) playpause=1;
	else if (on==0) playpause=0;
	else if (on==-1) playpause = !playpause;
	return playpause;
}

double jplay__getposition() {
	if(!app.playing_id) return -1;
	if(seek_request != -1.0) return -2;
	if(!myplayer) return -1;
	if(m_frames<1) return -1;
	return (double)play_position / m_frames;
}

int jplay__check() {
#ifdef __APPLE__
	/* weak linking on OSX test */
	if (!jack_client_open) return -1; /* FAIL */
#endif
	// TODO: check for getenv("JACK_NO_START_SERVER")
	// try connect to jackd,..
	return 0; /*OK*/
}
#endif /* HAVE_JACK */
