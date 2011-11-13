#ifndef __SAMPLECAT_LADSPA_H_
#define __SAMPLECAT_LADSPA_H_
#include <stdlib.h> /* size_t */

typedef struct _ladspa LadSpa; 

LadSpa* ladspah_alloc       ();
void    ladspah_free        (LadSpa *);

int     ladspah_init        (LadSpa*, const char* dll, const int plugin, const int samplerate, const size_t max_nframes);
void    ladspah_deinit      (LadSpa*);
void    ladspah_process     (LadSpa*, float *d, const unsigned long s);
void    ladspah_process_nch (LadSpa**, int nchannels, float *d, const unsigned long s);

void    ladspah_set_param(LadSpa**, int nchannels, int p, float val);
#endif
