#include "config.h"
#include <stdlib.h>
#include <math.h>
#include "audio_decoder/ad.h"

double ad_maxsignal(const char *fn) {
	struct adinfo nfo;
	void * sf = ad_open(fn, &nfo);
	if (!sf) return 0.0;

	int     read_len = 1024 * nfo.channels;
	float* sf_data = malloc(sizeof(float) * read_len);

	int readcount;
	float max_val = 0.0;
	do {
		readcount = ad_read(sf, sf_data, read_len);
		int k;
		for (k = 0; k < readcount; k++){
			const float temp = fabs (sf_data [k]);
			max_val = temp > max_val ? temp : max_val;
		};
	} while (readcount > 0);

	ad_close(sf);
	free(sf_data);
	return max_val;
}
