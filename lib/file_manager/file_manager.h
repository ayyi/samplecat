/*
 +----------------------------------------------------------------------+
 | This file is part of the Ayyi project. https://www.ayyi.org          |
 | copyright (C) 2013-2025 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#pragma once

#define BITS_PER_CHAR_8 8

#include <gtk/gtk.h>
#include "file_manager/typedefs.h"
#include "file_manager/mimetype.h"
#include "file_manager/dir.h"
#include "file_manager/diritem.h"
#include "file_manager/support.h"
#include "file_manager/filemanager.h"
#include "file_manager/filetypes/filetype_plugin.h"
#include "menu.h"
#include "file_view.h"
#include "view_iface.h"

GtkWidget*       file_manager__new_window     (const char* path);
void             file_manager__on_dir_changed ();
AyyiFilemanager* file_manager__get            ();
