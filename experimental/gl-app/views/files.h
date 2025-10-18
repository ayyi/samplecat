/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
 | copyright (C) 2016-2025 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#pragma once

#include "waveform/actor.h"
#include "../directory.h"
typedef struct _FilesView FilesView;
#include "views/files.impl.h"
#include "ayyi-utils/observable.h"
#include "agl/behaviours/selectable.h"
#include "behaviours/state.h"

#define FILES_STATE(A) ((StateBehaviour*)((A)->behaviours[2]))

struct _FilesView {
   AGlActor       actor;
   VMDirectory*   viewmodel;
   DirectoryView* view;
   AGlActor*      filelist;
   AGlActor*      scrollbar;
   AGlObservable* scroll;
   int            row_height;
};

AGlActorClass* files_view_get_class ();

AGlActor*   files_view              (gpointer);
const char* files_view_get_path     (FilesView*);
void        files_view_set_path     (FilesView*, const char*);
int         files_view_row_at_coord (FilesView*, int x, int y);
