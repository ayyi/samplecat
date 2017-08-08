/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2007-2017 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <gtk/gtk.h>
#include "debug/debug.h"
#include "samplecat.h"
#include "audio_analysis/waveform/waveform.h"
#include "audio_analysis/meter/peak.h"
#include "audio_analysis/ebumeter/ebur128.h"

typedef struct
{
   bool    success;
} WorkResult;


static bool
calc_ebur128(Sample* sample)
{
	struct ebur128 ebur;
	if (!ebur128analyse(sample->full_path, &ebur)){
		if (sample->ebur) free(sample->ebur);
		sample->ebur = g_strdup_printf(ebur.err
			?	"-"
			:	"intgd loudness:%.1lf LU%s\n"
				"loudness range:%.1lf LU\n"
				"intgd threshold:%.1lf LU%s\n"
				"range threshold:%.1lf LU%s\n"
				"range min:%.1lf LU%s\n"
				"range max:%.1lf LU%s\n"
				"momentary max:%.1lf LU%s\n"
				"short term max:%.1lf LU%s\n"
				, ebur.integrated , ebur.lufs?"FS":""
				, ebur.range
				, ebur.integ_thr  , ebur.lufs?"FS":""
				, ebur.range_thr  , ebur.lufs?"FS":""
				, ebur.range_min  , ebur.lufs?"FS":""
				, ebur.range_max  , ebur.lufs?"FS":""
				, ebur.maxloudn_M , ebur.lufs?"FS":""
				, ebur.maxloudn_S , ebur.lufs?"FS":""
		);
		return true;
	}
	return false;
}


/*
 *  Combine all analysis jobs in one for the purpose of having only a single model update
 */
void
request_analysis(Sample* sample)
{
	typedef struct {
		int changed;
	} C;
	C* c = g_new0(C, 1);

	void analysis_work(Sample* sample, gpointer _c)
	{
		C* c = _c;

		if(!sample->overview){
			if(make_overview(sample)) c->changed |= 1 << COL_OVERVIEW;
		}
		if(sample->peaklevel == 0){
			if((sample->peaklevel = ad_maxsignal(sample->full_path))) c->changed |= 1 << COL_PEAKLEVEL;
		}
		if(!sample->ebur || !strlen(sample->ebur)){
			if(calc_ebur128(sample)) c->changed |= 1 << COL_X_EBUR;
		}
	}

	void analysis_done(Sample* sample, gpointer _c)
	{
		C* c = _c;
		switch(c->changed){
			case 1 << COL_PEAKLEVEL:
				samplecat_model_update_sample (samplecat.model, sample, COL_PEAKLEVEL, NULL);
				break;
			case 1 << COL_OVERVIEW:
				samplecat_model_update_sample (samplecat.model, sample, COL_OVERVIEW, NULL);
				break;
			case 1 << COL_X_EBUR:
				samplecat_model_update_sample (samplecat.model, sample, COL_X_EBUR, NULL);
				break;
			case 0:
				break;
			default:
				samplecat_model_update_sample (samplecat.model, sample, COL_ALL, NULL);
		}
		g_free(c);
	}

	if(!sample->overview || sample->peaklevel == 0.0 || !sample->ebur || !strlen(sample->ebur))
		worker_add_job(sample, analysis_work, analysis_done, c);
}


void
request_overview(Sample* sample)
{
	void overview_work(Sample* sample, gpointer user_data)
	{
		make_overview(sample);
	}

	void overview_done(Sample* sample, gpointer user_data)
	{
		PF;
		g_return_if_fail(sample);
		if(sample->overview){
			samplecat_model_update_sample (samplecat.model, sample, COL_OVERVIEW, NULL);
		}else{
			dbg(1, "overview creation failed (no pixbuf).");
		}
	}

	dbg(2, "sample=%p filename=%s", sample, sample->full_path);
	worker_add_job(sample, overview_work, overview_done, NULL);
}


void
request_peaklevel(Sample* sample)
{
	void peaklevel_work(Sample* sample, gpointer user_data)
	{
		sample->peaklevel = ad_maxsignal(sample->full_path);
	}

	void peaklevel_done(Sample* sample, gpointer user_data)
	{
		dbg(1, "peaklevel=%.2f id=%i", sample->peaklevel, sample->id);
		// important not to do too many updates to the model as it makes the tree unresponsive.
		// this can happen when file is not available.
		if(sample->peaklevel > 0.0){
			samplecat_model_update_sample (samplecat.model, sample, COL_PEAKLEVEL, NULL);
		}
	}

	dbg(2, "sample=%p filename=%s", sample, sample->full_path);
	worker_add_job(sample, peaklevel_work, peaklevel_done, NULL);
}


void
request_ebur128(Sample* sample)
{
	void ebur128_work(Sample* sample, gpointer _result)
	{
		WorkResult* result = _result;
		result->success = calc_ebur128(sample);
	}

	void ebur128_done(Sample* sample, gpointer _result)
	{
		WorkResult* result = _result;
		if(result->success) samplecat_model_update_sample (samplecat.model, sample, COL_X_EBUR, NULL);
		g_free(result);
	}

	dbg(2, "sample=%p filename=%s", sample, sample->full_path);

	worker_add_job(sample, ebur128_work, ebur128_done, g_new0(WorkResult, 1));
}


