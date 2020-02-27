/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2016-2019 Tim Orford <tim@orford.org>                  |
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
#include "../directory.h"
typedef struct _FilesView FilesView;
#include "views/files.impl.h"
#include "agl/behaviours/selectable.h"
#include "behaviours/state.h"
#include "agl/observable.h"

#define FILES_STATE(A) ((StateBehaviour*)((A)->behaviours[2]))

typedef struct {
   AGlBehaviour   behaviour;
   AGlObservable* observable;
} ScrollBehaviour;

struct _FilesView {
   AGlActor       actor;
   VMDirectory*   viewmodel;
   DirectoryView* view;
   AGlActor*      scrollbar;
};

AGlActorClass* files_view_get_class ();

AGlActor*   files_view              (gpointer);
const char* files_view_get_path     (FilesView*);
void        files_view_set_path     (FilesView*, const char*);
int         files_view_row_at_coord (FilesView*, int x, int y);

#endif
