/*
  simple LADSPA Host
  This file is part of the Samplecat project.

  Copyright (C) Robert Ham 2002, 2003 (node@users.sourceforge.net)
  Copyright (C) 2009-2011 Robin Gareus <robin@gareus.org>
  Copyright (C) 2011 Tim Orford <tim@orford.org>

  written by Robin Gareus

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2, or (at your option)
  any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software Foundation,
  Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. 
*/

#include "config.h"
#include "ladspa_proc.h"

#ifndef ENABLE_LADSPA
LadSpa* ladspah_alloc() {return NULL;}
void    ladspah_free(LadSpa* x) { ; }
int     ladspah_init(LadSpa* x, const char* dll, const int plugin, const int s, const size_t n) {return -1;}
void    ladspah_deinit(LadSpa* x) { ; }
void    ladspah_process(LadSpa* x, float *d, const unsigned long s) { ; }
void    ladspah_process_nch(LadSpa** x, const int n, float *d, const unsigned long s) { ; }
void    ladspah_set_param(LadSpa**px, int nch, int p, float val) { ; }
#else

/* simple LADSPA HOST */

#include <stdio.h>
#include <string.h>
#include <ladspa.h>
#include <dlfcn.h>

#define MAX_CONTROL_VALUES (255)

static int verbose = 0; ///< 1: print info and debug messages.
static int quiet = 0;   ///< 1: don't print any errors

struct _ladspa {
	void *                 dl_handle;
	LADSPA_Descriptor *    l_desc;
	LADSPA_Handle          l_handle;
	LADSPA_Data *          inbuf;
	LADSPA_Data *          outbuf;
	LADSPA_Data            dummy_control_output;
	LADSPA_Data            control_values[MAX_CONTROL_VALUES];
	int ctrls;
};

static LADSPA_Data plugin_desc_get_default_control_value(const LADSPA_PortRangeHint* , unsigned long, int, LADSPA_Data*, LADSPA_Data*);


#ifdef DARWIN
extern char * program_name;
#include <libgen.h> /* dirname */
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h> /* stat */
#endif

LadSpa* ladspah_alloc() {
	return calloc(1,sizeof(LadSpa));
};

void ladspah_free(LadSpa* x) {
	if (x) free(x);
}

int ladspah_init(LadSpa* p, const char* dll, const int plugin, int samplerate, size_t max_nframes) {
	const char * dlerr;
	char *object_file = NULL;

  p->inbuf = p->outbuf = NULL;
	p->dl_handle = NULL;
	p->l_desc = NULL;
	p->l_handle = 0;
	p->ctrls = 0;

#ifdef DARWIN /* OSX APPLICATION BUNDLE */
	char * bundledir = dirname(program_name); // XXX may modify program_name (!)
	object_file = malloc(strlen(bundledir) + strlen(dll) + 20);
	sprintf(object_file, "%s/../Frameworks/%s", bundledir, dll);
#else
	struct stat info;
	char* ladspaPath;
	if (getenv("LADSPA_PATH")){
		ladspaPath=strdup(getenv("LADSPA_PATH"));
	} else {
		const char *home = getenv("HOME");
		if (home) {
			ladspaPath = malloc(strlen(home) + 60);
			sprintf(ladspaPath, "/usr/local/lib/ladspa:/usr/lib/ladspa:%s/.ladspa", home);
		} else {
			ladspaPath = strdup("/usr/local/lib/ladspa:/usr/lib/ladspa");
		}
	}
	if (verbose) fprintf (stderr, "LADSPA Host: '%s'\n", ladspaPath);
	char* path = ladspaPath;
	char* element;
	while ((element = strtok(path, ":")) != 0) {
		path = 0;
		if (element[0] != '/') continue;
		char* filePath = (char *)malloc(strlen(element) + strlen(dll) + 2);
		sprintf(filePath, "%s/%s", element, dll);
		if (!stat(filePath,&info)) {
			object_file = filePath;
			break;
		}
		free(filePath);
	}
	free(ladspaPath);
	if (!object_file) {
		if (!quiet) fprintf (stderr, "LADSPA Host: file-not-found: '%s'\n", dll);
		return -1;
	}
#endif

	LADSPA_Descriptor_Function get_descriptor;
	p->dl_handle = dlopen (object_file, RTLD_NOW|RTLD_GLOBAL);
	if (!p->dl_handle) {
		if (!quiet)
			fprintf (stderr, "%s: error opening shared object file '%s': %s\n", __FUNCTION__, object_file, dlerror());
		free(object_file);
		return -1;
	}

	dlerror (); /* clear the error report */
	get_descriptor = (LADSPA_Descriptor_Function) dlsym (p->dl_handle, "ladspa_descriptor");
	dlerr = dlerror();
	if (dlerr || !get_descriptor) {
		if (!quiet)
			fprintf (stderr, "%s: error finding descriptor symbol in object file '%s': %s\n", __FUNCTION__, object_file, dlerr);
		dlclose (p->dl_handle);
		p->dl_handle=NULL;
		ladspah_deinit(p);
		free(object_file);
		return -1;
	}

	if (verbose) printf("LADSPA: plugin '%s' loaded.\n", object_file);
	free(object_file);

#if 0
	int i=0;
	while (1) {
		p->l_desc = (LADSPA_Descriptor*) get_descriptor(i);
		if (!p->l_desc) break;
		printf("LADSPA: %i - '%s' - '%s'\n", i, p->l_desc->Label, p->l_desc->Name);
		int pp = p->l_desc->Properties;
		if (LADSPA_IS_INPLACE_BROKEN(pp)) {
			printf("LADSPA:  (no inplace)\n");
		}
		i++;
	}
#endif


	p->l_desc = (LADSPA_Descriptor*) get_descriptor(plugin);
	if (!p->l_desc) {
		if (!quiet)
			fprintf(stderr, "Error: Invalid LADSPA Plugin descriptor.\n");
		ladspah_deinit(p);
		return -1;
	}

	int pp = p->l_desc->Properties;
	if (LADSPA_IS_INPLACE_BROKEN(pp)) {
		if (!quiet)
			fprintf(stderr, "Error: LADSPA Plugin does not support in-place processing.\n");
		ladspah_deinit(p);
		return -1;
	}

	printf("LADSPA: %s - '%s' by '%s'\n", p->l_desc->Label, p->l_desc->Name, p->l_desc->Maker);

	// prepare buffers
	p->inbuf  = calloc(max_nframes, sizeof(LADSPA_Data));
	p->outbuf = calloc(max_nframes, sizeof(LADSPA_Data));

	// NOW init plugin
	p->l_handle = p->l_desc->instantiate(p->l_desc, samplerate);
	if (!p->l_handle) {
		if (!quiet)
			fprintf(stderr, "Error: Failed to instantiate LADSPA plugin.\n");
		ladspah_deinit(p);
		return -1;
	}

	int connected = 0;
	unsigned long port;
	int control_index = 0;

	for (port=0; port < p->l_desc->PortCount && port < MAX_CONTROL_VALUES; port++) {
		LADSPA_PortDescriptor pd = p->l_desc->PortDescriptors[port];

		if (LADSPA_IS_PORT_AUDIO(pd)) {
			if (verbose) printf(" audio%2lu %s - '%s'\n", port, (LADSPA_IS_PORT_INPUT(pd))?"input ":"output", p->l_desc->PortNames[port]);
			if (LADSPA_IS_PORT_INPUT(pd)) {
				if ((connected&1) == 0) {
					connected|=1;
					p->l_desc->connect_port(p->l_handle, port, p->inbuf);
				} else {
					if (!quiet)
						fprintf(stderr, "ERROR: Dangling input port - we only support 1 in - 1 out mono plugins.\n");
					ladspah_deinit(p);
					return -1;
				}
			}
			if (LADSPA_IS_PORT_OUTPUT(pd)) {
				if ((connected&2) == 0) {
					connected|=2;
					p->l_desc->connect_port(p->l_handle, port, p->outbuf);
				} else {
					if (!quiet)
						fprintf(stderr, "ERROR: Dangling output port - we only support 1 in - 1 out mono plugins.\n");
					ladspah_deinit(p);
					return -1;
				}
			}
		} else {
			if (verbose) printf("  ctrl%2lu %s - idx:%2d '%s'\n",
					port,
					LADSPA_IS_PORT_INPUT(pd)?"in ":"out",
					LADSPA_IS_PORT_INPUT(pd)?control_index:-1,
					p->l_desc->PortNames[port]
					);

			if (LADSPA_IS_PORT_INPUT(pd)) {
				p->l_desc->connect_port(p->l_handle, port, &p->control_values[control_index]);
				LADSPA_Data lower, upper;
				p->control_values[control_index] = plugin_desc_get_default_control_value(p->l_desc->PortRangeHints, port, samplerate, &lower, &upper);
				if (verbose) {
					printf("  \\ range %.5f .. %.5f\n", lower, upper);
					printf("   `-> %f\n", p->control_values[control_index]);
				}
				control_index++;
			} else if (LADSPA_IS_PORT_OUTPUT(pd)) {
#if 0
				p->l_desc->connect_port(p->l_handle, port, &p->dummy_control_output);
#else
				p->l_desc->connect_port(p->l_handle, port, NULL);
#endif
			}
		}
	}
	if (connected != 3) {
		if (!quiet)
			fprintf(stderr, "Error: Dangling port(s): input:%s, output:%s.\n",
					(connected&1)?"connected":"NOT connected",
					(connected&2)?"connected":"NOT connected");
		ladspah_deinit(p);
		return -1;
	}

	if (verbose) printf("LADSPA Host: connected mono audio-port | control ports: %i\n", control_index);
	p->ctrls=control_index;

#if 0
	p->control_values[0] = 0; // 'Cents'
	p->control_values[1] = 0; // 'Semitones'
	p->control_values[2] = 0; // 'Octaves'
	p->control_values[3] = 0; // 'Crispness'
	p->control_values[4] = 0; // 'Formant Preserving'
	p->control_values[5] = 0; // 'Faster'
#endif

	p->l_desc->activate(p->l_handle);
	return 0;
}

void ladspah_deinit(LadSpa* p) {
	if (p->l_handle) {
		if (p->l_desc->deactivate) p->l_desc->deactivate(p->l_handle);
		p->l_desc->cleanup(p->l_handle);
	}
	if (p->dl_handle) {
		dlclose(p->dl_handle);
	}
	if (p->inbuf)  free(p->inbuf);
	if (p->outbuf) free(p->outbuf);

	p->inbuf = p->outbuf = NULL;
	p->l_handle=0;
	p->l_desc=NULL;
	p->dl_handle= NULL;
}

void ladspah_process(LadSpa* p, float *d, const unsigned long s) {
	if (!p || !p->dl_handle || !p->l_desc) return;
	memcpy(p->inbuf, d, s * sizeof(float));
	p->l_desc->run(p->l_handle, s);
	int i;
	for (i=0;i<s;i++) {
		d[i] = p->outbuf[i];
	}
}

void ladspah_process_nch(LadSpa** px, const int nch, float *d, const unsigned long nframes) {
	if (nch==1) return ladspah_process(px[0], d, nframes);

	int s,c;
	float *p=d;
	// deinterleave
	for(s=0; s<nframes; s++) {
		for(c=0; c<nch; c++) {
			px[c]->inbuf[s] = *p++;
		}
	}
	for(c=0; c<nch; c++) {
		px[c]->l_desc->run(px[c]->l_handle, nframes);
	}
	p=d;
	for(s=0; s<nframes; s++) {
		for(c=0; c<nch; c++) {
			*p++ = px[c]->outbuf[s];
		}
	}
}

void ladspah_set_param(LadSpa**px, int nch, int p, float val) {
	int i;
	if (p<0 || p>= px[0]->ctrls) return;
	for(i=0; i<nch; i++) {
		px[i]->control_values[p] = val;
	}
}

/*
 * function borrowed from jack-rack
 * Copyright (C) Robert Ham 2002, 2003 (node@users.sourceforge.net)
 * GPLv2 or later
 */
#include <math.h>
#include <float.h>
LADSPA_Data
plugin_desc_get_default_control_value (const LADSPA_PortRangeHint* prh,
		unsigned long port_index, int sample_rate,
		LADSPA_Data* rl, LADSPA_Data* ru)
{
	LADSPA_Data upper, lower;
	LADSPA_PortRangeHintDescriptor hint_descriptor;
	
	hint_descriptor = prh[port_index].HintDescriptor;
	
	/* set upper and lower, possibly adjusted to the sample rate */
	if (LADSPA_IS_HINT_SAMPLE_RATE(hint_descriptor)) {
		upper = prh[port_index].UpperBound * (LADSPA_Data) sample_rate;
		lower = prh[port_index].LowerBound * (LADSPA_Data) sample_rate;
	} else {
		upper = prh[port_index].UpperBound;
		lower = prh[port_index].LowerBound;
	}
	
	if (LADSPA_IS_HINT_LOGARITHMIC(hint_descriptor)) {
		if (lower < FLT_EPSILON) lower = FLT_EPSILON;
	}

	if (rl) *rl=lower;
	if (ru) *ru=upper;

	if (LADSPA_IS_HINT_HAS_DEFAULT(hint_descriptor)) {
		if (LADSPA_IS_HINT_DEFAULT_MINIMUM(hint_descriptor)) {

			return lower;

		} else if (LADSPA_IS_HINT_DEFAULT_LOW(hint_descriptor)) {

			if (LADSPA_IS_HINT_LOGARITHMIC(hint_descriptor)) {
				return exp(log(lower) * 0.75 + log(upper) * 0.25);
			} else {
				return lower * 0.75 + upper * 0.25;
			}

		} else if (LADSPA_IS_HINT_DEFAULT_MIDDLE(hint_descriptor)) {

			if (LADSPA_IS_HINT_LOGARITHMIC(hint_descriptor)) {
				return exp(log(lower) * 0.5 + log(upper) * 0.5);
			} else {
				return lower * 0.5 + upper * 0.5;
			}

		} else if (LADSPA_IS_HINT_DEFAULT_HIGH(hint_descriptor)) {
			
			if (LADSPA_IS_HINT_LOGARITHMIC(hint_descriptor)) {
				return exp(log(lower) * 0.25 + log(upper) * 0.75);
			} else {
				return lower * 0.25 + upper * 0.75;
			}

		} else if (LADSPA_IS_HINT_DEFAULT_MAXIMUM(hint_descriptor)) {
			
			return upper;
		
		} else if (LADSPA_IS_HINT_DEFAULT_0(hint_descriptor)) {
			
			return 0.0;
			
		} else if (LADSPA_IS_HINT_DEFAULT_1(hint_descriptor)) {

			if (LADSPA_IS_HINT_SAMPLE_RATE(hint_descriptor)) {
				return (LADSPA_Data) sample_rate;
			} else {
				return 1.0;
			}

		} else if (LADSPA_IS_HINT_DEFAULT_100(hint_descriptor)) {

			if (LADSPA_IS_HINT_SAMPLE_RATE(hint_descriptor)) {
				return 100.0 * (LADSPA_Data) sample_rate;
			} else {
				return 100.0;
			}

		} else if (LADSPA_IS_HINT_DEFAULT_440(hint_descriptor)) {
			
			if (LADSPA_IS_HINT_SAMPLE_RATE(hint_descriptor)) {
				return 440.0 * (LADSPA_Data) sample_rate;
			} else {
				return 440.0;
			}

		}

	} else { /* try and find a reasonable default */

		if        (LADSPA_IS_HINT_BOUNDED_BELOW(hint_descriptor)) {
			return lower;
		} else if (LADSPA_IS_HINT_BOUNDED_ABOVE(hint_descriptor)) {
			return upper;
		}
	}

	return 0.0;
}

#endif
