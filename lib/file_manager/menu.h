/*
 +----------------------------------------------------------------------+
 | This file is part of the Ayyi project. https://www.ayyi.org          |
 | copyright (C) 2011-2025 Tim Orford <tim@orford.org>                  |
 | copyright (C) 2006, Thomas Leonard and others                        |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#pragma once

#include "file_manager/filemanager.h"

GtkWidget*      fm__make_context_menu   (AyyiFilemanager*);
void            fm__add_menu_item       (GMenuModel*, GAction*, char*);

// private
void            fm__menu_on_view_change (AyyiFilemanager*);
