#ifndef __GPLAYER_H_
#define __GPLAYER_H_

#include "typedefs.h"

void gplayer_connect();
void gplayer_disconnect();
void gplayer_play(Sample*);
void gplayer_play_path(const char*);
void gplayer_stop();
void gplayer_toggle(Sample*);
void gplayer_play_all();
#endif
