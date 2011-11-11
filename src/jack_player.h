#include <jack/jack.h>
// TODO use weakjack.h 
#include <jack/ringbuffer.h>
#include "audio_decoder/ad.h"

int      playback_init(Sample*);
void     playback_stop();
