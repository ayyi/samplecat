/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2016-2017 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#ifndef __views_files_h__
#define __views_files_h__
#include "waveform/actor.h"
#include "gl-app/directory.h"
typedef struct _FilesView FilesView;
#include "views/files.impl.h"

struct _FilesView {
   AGlActor       actor;
   VMDirectory*   viewmodel;
   DirectoryView* view;
   int            scroll_offset;
};

AGlActor* files_view              (WaveformActor*);
int       files_view_row_at_coord (FilesView*, int x, int y);

#endif
