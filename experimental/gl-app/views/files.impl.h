/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
 | copyright (C) 2007-2018 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 +----------------------------------------------------------------------+
 | files.impl implements the FileManager View interface.                |
 | It is a 'model view' object, representing a particular view          |
 | of a Directory object.                                               |
 | It should be the same object as the FileView actor, but is separate  |
 | for now because AGlActor is not a GObject so does not support        |
 | GInterface.                                                          |
 +----------------------------------------------------------------------+
 |
 */

#pragma once

#include <glib.h>
#include "../directory.h"
typedef struct _DirectoryView DirectoryView;
#include "views/files.h"

G_BEGIN_DECLS

typedef struct _DirectoryViewPrivate DirectoryViewPrivate;

struct _DirectoryView {
    GObject               parent_instance;
    FilesView*            view;
    VMDirectory*          model;
    GPtrArray*            items;            // array of WavViewItem*
    int                   selection;
    FmSortType            sort_type;
    GtkSortType           sort_order;
    DirectoryViewPrivate* priv;
};

typedef struct {
    DirItem*      item;
    MaskedPixmap* image;
    int           old_pos;     // Used while sorting
    gchar*        utf8_name;   // NULL => leafname is valid
    AGlActor*     wav;
} WavViewItem;

DirectoryView* directory_view_new       (VMDirectory*, FilesView*);
DirectoryView* directory_view_construct (GType);

void           directory_view_set_sort  (DirectoryView*, FmSortType, GtkSortType order);

G_END_DECLS
