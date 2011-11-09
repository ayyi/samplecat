#include <jack/jack.h>
#include <jack/ringbuffer.h>
#include "audio_decoder/ad.h"

typedef struct __decoder_session _decoder_session;

typedef struct __audition
{
	char           type;     //either TYPE_SNDFILE or TYPE_FLAC

	//audio decoder stuff:
	void *sf; 
	struct adinfo nfo;
	int   nframes;  

	//flac stuff:
	jack_ringbuffer_t* rb1;  //probably move this to __decoder_session ...?
	jack_ringbuffer_t* rb2;
	_decoder_session*  session;
	
} _audition;

int      jack_init();
void     jack_close();
void     jack_shutdown(void *arg);
int      jack_process_flac(jack_nframes_t nframes, void *arg);

int      playback_init(Sample*);
void     playback_stop();

void     audition_init();
void     audition_reset();

gboolean jack_process_finished(gpointer data);
gboolean jack_process_stop_playback(gpointer data);
