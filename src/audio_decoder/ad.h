#ifndef __AD_H__
#define __AD_H__
struct adinfo {
	unsigned int sample_rate;
	int64_t length; //milliseconds
	int64_t frames; //total number of frames (eg a frame for 16bit stereo is 4 bytes).
	unsigned int channels;
};

void ad_init();

/* low level API */
void *ad_open(const char *, struct adinfo *);
int ad_close(void *);
ssize_t ad_read(void *, double*, size_t);

/* high level API */
#include <gtk/gtk.h>
gboolean ad_info (const char *fn, struct adinfo *nfo);
double ad_maxsignal(const char *fn);
#endif
