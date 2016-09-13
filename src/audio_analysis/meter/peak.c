/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2007-2016 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#include "config.h"
#include <stdlib.h>
#include <math.h>
#include "decoder/ad.h"


double
ad_maxsignal(const char* filename)
{
	WfDecoder d = {{0,}};
	if(!ad_open(&d, filename)) return 0.0;

	size_t read_len = 1024 * d.info.channels;
	float* sf_data = malloc(sizeof(float) * read_len);

	int readcount;
	float max_val = 0.0;
	do {
		readcount = ad_read(&d, sf_data, read_len);
		int k;
		for (k = 0; k < readcount; k++){
			const float temp = fabs (sf_data [k]);
			max_val = temp > max_val ? temp : max_val;
		};
	} while (readcount > 0);

	ad_close(&d);
	free(sf_data);
	ad_free_nfo(&d.info);

	return max_val;
}
