/**
* +----------------------------------------------------------------------+
* | This file is part of the Ayyi project. http://ayyi.org               |
* | copyright (C) 2013-2020 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#ifndef __file_manager_h__
#define __file_manager_h__

#define BITS_PER_CHAR_8 8

#ifndef true
  #define true TRUE
  #define false FALSE
#endif

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <gtk/gtk.h>
#pragma GCC diagnostic warning "-Wdeprecated-declarations"
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
void             file_manager__update_all     ();
void             file_manager__on_dir_changed ();
AyyiFilemanager* file_manager__get            ();

#endif
