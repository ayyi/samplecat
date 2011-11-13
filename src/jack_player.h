#ifndef __SAMPLEJACK_H_
#define __SAMPLEJACK_H_
#include "typedefs.h"
#include "sample.h"

void     jplay__connect    ();
void     jplay__play       (Sample*);
void     jplay__toggle     (Sample*);
int      jplay__play_path  (const char* path);
void     jplay__stop       ();
void     jplay__play_all   ();
#endif
