/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
 | copyright (C) 2017-2022 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |                                                                      |
 | Directory implements the GtkTreeModel interface                      |
 |                                                                      |
 +----------------------------------------------------------------------+
 |
*/

#pragma once

#include "file_manager/dir.h"

#include <glib.h>

G_BEGIN_DECLS

typedef struct _VMDirectory VMDirectory;
typedef struct _VMDirectoryClass VMDirectoryClass;
typedef struct _VMDirectoryPrivate VMDirectoryPrivate;

struct _VMDirectory {
    GObject             parent_instance;
	char*               path;
    ViewIface*          view;
    VMDirectoryPrivate* priv;
};

VMDirectory* vm_directory_new          ();
VMDirectory* vm_directory_construct    (GType);
void         vm_directory_set_path     (VMDirectory*, const char*);
gboolean     vm_directory_match_filter (VMDirectory*, DirItem*);


G_END_DECLS
