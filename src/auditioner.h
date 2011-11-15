#ifndef __AUDITIONER_H_
#define __AUDITIONER_H_

#include "typedefs.h"

void auditioner_connect();
void auditioner_disconnect();
void auditioner_play(Sample*);
void auditioner_play_path(const char*);
void auditioner_stop();
void auditioner_toggle(Sample*);
void auditioner_play_all();

#endif
