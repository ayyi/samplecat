/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. https://ayyi.github.io/samplecat/    |
 | copyright (C) 2007-2023 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#pragma once

#include "samplecat/list_store.h"

struct _libraryview {
   GtkWidget*         widget;         // treeview
   GtkWidget*         scroll;

   struct {
      GtkCellRenderer* name;
      GtkCellRenderer* tags;
   }                   cells;
   GtkTreeViewColumn* col_name;
   GtkTreeViewColumn* col_path;
   GtkTreeViewColumn* col_pixbuf;
   GtkTreeViewColumn* col_tags;

   int                  selected;
   GtkTreeRowReference* mouseover_row_ref;
};

GtkWidget*  listview__new                  ();
GList*      listview__get_selection        ();

void        listview__block_motion_handler ();
gint        listview__get_mouseover_row    ();
