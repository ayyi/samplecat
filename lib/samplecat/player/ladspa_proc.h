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

/** 
 * @file ladspa_proc.h
 * @brief ladspah: simple LADSPA Host.
 * @author Robin Gareus <robin@gareus.org>
 *
 * A simple API to mono audio plugins (1 audio-in - 1 audio-out).
 * that allow for in-place processing.
 *
 * It includes convenience function to process multi-channel
 * audio by replicating the plugin N times for each channel
 */

#ifndef __SAMPLECAT_LADSPA_H_
#define __SAMPLECAT_LADSPA_H_
#include <stdlib.h> /* size_t */

/** internal private data storage */
typedef struct _ladspa LadSpa; 

/**
 * allocate an anonymous pointer to a LADSPA host instance object
 * for later use with all ladspah_* functions.
 *
 * @return an allocated object or NULL if out-of-memory
 */
LadSpa* ladspah_alloc       ();
/**
 * frees the allocated object - call after \ref ladspah_deinit
 * or re-use it after calling \ref ladspah_deinit.
 */
void    ladspah_free        (LadSpa *);

/**
 * load a plugin and prepare it for audio processing.
 * 
 * @param dll the name of the plugin library (e.g. "gverb_1216.so")
 * the lib is searched in LADSPA_PATH environment variable - or
 * if that variable is not set: in the usual places 
 * (/usr/lib/ladspa:/usr/local/lib/ladspa:$HOME/.ladspa).
 *
 * @param plugin: the plugin descriptor ID inside the DLL.
 * @param samplerate: the sample-rate at which to process audio
 * @param max_nframes: the maxium number of samples that will ever
 * be passed to \ref ladspah_process()
 *
 * @return: 0 on success, -1 on error. Note: \ref ladspah_deinit() does
 * not not be called called if an error occurred during initialization.
 */
int     ladspah_init        (LadSpa*, const char* dll, const int plugin, const int samplerate, const size_t max_nframes);
/**
 * close the plugin handler and release internal buffers.
 * The object can be re-used with \ref ladspah_init or freed
 * with \ref ladspah_free.
 */
void    ladspah_deinit      (LadSpa*);
/**
 * process N audio frames in-place. The given audio-data will 
 * be overwritten with the plugin output (!)
 * @param d pointer to the audio-data
 * @param s number of samples to process
 */
void    ladspah_process     (LadSpa*, float *d, const unsigned long s);
/**
 * de-interleave audio-data, call \ref ladspah_process for each channel and
 * merge back the audio-data to interleaved format.
 *
 * @param nchannels number of channels to process
 * @param d pointer to the interleaved audio-data. (nchannels * s samples)
 * @param s number of samples to process for each channel.
 */
void    ladspah_process_nch (LadSpa**, int nchannels, float *d, const unsigned long s);
/**
 * set plugin control parameters - 
 * @param nchannels number of objects to update
 * @param p parameter key (depends on plugin) - only control-inputs are enumerated.
 * load a plugin in verbose mode to discover the parameters and their range.
 * @param val parameter value.
 */
void    ladspah_set_param(LadSpa**, int nchannels, int p, float val);
#endif
