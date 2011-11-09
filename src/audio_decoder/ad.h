#ifndef __AD_H__
#define __AD_H__
struct adinfo {
	unsigned int sample_rate;
	int64_t length; //milliseconds
	int64_t frames; //total number of frames (eg a frame for 16bit stereo is 4 bytes).
	unsigned int channels;
#if 0 // TODO
	unsigned int bitrate;
	unsigned int bitdepth;
	void *metadata;
#endif
};

void ad_init();

/* low level API */
void *  ad_open(const char *, struct adinfo *);
int     ad_close(void *);
int64_t ad_seek(void *, int64_t);
ssize_t ad_read(void *, double*, size_t);
int     ad_info (void *sf, struct adinfo *nfo);

/* high level API */
#include <gtk/gtk.h>
gboolean ad_finfo (const char *, struct adinfo *);
double ad_maxsignal(const char *);
ssize_t ad_read_mono(void *, double*, size_t);
#endif
