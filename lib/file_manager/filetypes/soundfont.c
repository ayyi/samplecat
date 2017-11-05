/**
* +----------------------------------------------------------------------+
* | This file is part of the Ayyi project. http://www.ayyi.org           |
* | copyright (C) 2007-2017 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/

// Mount the soundfound using gvfs ?
//  -appear in ~/.gvfs

#include "../file_manager.h"

static void sounfont_init    ();
static void soundfont_deinit ();

static struct filetypePlugin soundfont_info = {
	.api_version = FILETYPE_PLUGIN_API_VERSION,
	.name        = "Soundfont",
	.init        = sounfont_init,
	.deinit      = soundfont_deinit,
};


static void
sounfont_init ()
{
	g_hash_table_insert(AYYI_LIBFILEMANAGER_GET_CLASS(file_manager__get())->filetypes, "sf2", &soundfont_info);
}


static void
soundfont_deinit ()
{
}

DECLARE_FILETYPE_PLUGIN(soundfont_info);

