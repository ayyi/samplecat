/*
  JACK audio player
  This file is part of the Samplecat project.

  copyright (C) 2006-2015 Tim Orford <tim@orford.org>
  copyright (C) 2011 Robin Gareus <robin@gareus.org>

  written by Robin Gareus

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/
#ifndef __SAMPLEJACK_H_
#define __SAMPLEJACK_H_
#include "typedefs.h"

#define VARISPEED 1 ///< allow to change speed while playing
#define JACK_MIDI 1 ///< use JACK midi to trigger and - if LADSPA/rubberband is avail: pitch-shift

/*
 * the following functions are relevant for the internal
 * player and not present in the Samplecat auditioner API
 */

/** 
 * toggle, set or unset playback pause
 * @param on: 1: pause playback, 0: resume playback, -1: toggle,
 * -2: query current state
 *
 * @return -1 if player is not active, otherwise return (new) state: 
 * 0:playing, 1:paused
 */
int      jplay__pause      (int on);
/**
 * seek in file
 * @param pos: position [0.0 .. 1.0]
 */
void     jplay__seek       (double pos);
/**
 * get current playhead position and player status.
 * @return play position [0.0 .. 1.0],
 * -1 if not playing, -2 if a seek is currently in progress.
 */
guint    jplay__getposition();
#endif
