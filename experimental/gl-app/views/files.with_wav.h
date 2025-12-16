/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
 | copyright (C) 2016-2026 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#pragma once

typedef struct _FilesView FilesView;

#include "waveform/actor.h"
#include "../directory.h"
#include "behaviours/dragzone.h"
#include "views/files.impl.h"

typedef struct _FilesWithWav {
   FilesView         files;
   WaveformContext*  wfc;
   AMObject          positions;
   DragZoneBehaviour dzb;
   DragZone          zones[5];
} FilesWithWav;

AGlActorClass* files_with_wav_get_class ();

AGlActor* files_with_wav              (gpointer);
void      files_with_wav_set_path     (FilesWithWav*, const char*);
int       files_with_wav_row_at_coord (FilesWithWav*, int x, int y);
void      files_with_wav_select       (FilesWithWav*, int);
