/*
 * Copyright (C) 2007-2011, Tim Orford
 * Copyright (C) 2006, Thomas Leonard and others (see changelog for details).
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

//mount using gvfs ?
//	-appear in ~/.gvfs

#include "../file_manager.h"

static void
sounfont_init (void)
{
}

static void soundfont_deinit (void) { }

static struct filetypePlugin soundfontInfo = {
	.api_version   = FILETYPE_PLUGIN_API_VERSION,
	.name          = "Soundfont",
	.plugin_init   = sounfont_init,
	.plugin_deinit = soundfont_deinit,
	//.create		= webkit_new,
	//.write		= webkit_write_html,
};

/*
static struct htmlplugin pi = {
	FILETYPE_PLUGIN_API_VERSION,
	"Soundfont Plugin",
	1, //type
	&soundfontInfo
};
*/

DECLARE_FILETYPE_PLUGIN(soundfontInfo);

