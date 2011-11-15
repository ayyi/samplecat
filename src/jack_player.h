/*
  JACK audio player
  This file is part of the Samplecat project.

  copyright (C) 2006-2007 Tim Orford <tim@orford.org>
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
#include "sample.h"

#define VARISPEED 1 ///< allow to change speed while playing

/** stop playback - if active */
void     jplay__stop       ();

void     jplay__connect    ();
void     jplay__disconnect ();
/**
 * look up path from sample and play it
 * it sets global app.playing_id and triggers
 * showing the player UI
 */
void     jplay__play       (Sample*);
/**
 * identical to \ref jplay__play
 */
void     jplay__toggle     (Sample*);
/**
 * play audio-file with given file-path
 * app.playing_id is set to (-1), player UI is triggered
 * @param path absolute path to the file.
 * @param reset_pitch 1: reset set midi pitch adj.
 * @return 0 on success, -1 on error.
 */
int      jplay__play_path  (const char* path, int reset_pitch);
/**
 * play all files currently in main Tree model
 */
void     jplay__play_all   ();

/*
 * the following functions are relevant for the internal
 * player and not present in the Samplecat auditioner API
 */

void     jplay__play_selected();
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
double   jplay__getposition();
#endif
