#ifndef __AD_H__
#define __AD_H__
#include <unistd.h>
#include <stdint.h>
#include <glib.h>

typedef struct adinfo {
   unsigned int sample_rate;
   unsigned int channels;
   int64_t      length;       // milliseconds
   int64_t      frames;       // total number of frames (eg a frame for 16bit stereo is 4 bytes).
   int          bit_rate;
   int          bit_depth;
   GPtrArray*   meta_data;
} AdInfo;

/* global init function - register codecs */
void ad_init();

/* low level API */
void *  ad_open           (const char*, AdInfo*);
int     ad_close          (void*);
int64_t ad_seek           (void*, int64_t);
ssize_t ad_read           (void*, float*, size_t);
int     ad_info           (void* sf, AdInfo*);

void    ad_clear_nfo      (AdInfo*);
void    ad_free_nfo       (AdInfo*);

/* high level API - wrappers around low-level functions */
gboolean ad_finfo         (const char*, AdInfo*);
ssize_t  ad_read_mono_dbl (void*, AdInfo*, double*, size_t);
void     dump_nfo         (int dbglvl, AdInfo*);
#endif
