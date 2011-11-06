/*

Copyright (C) Tim Orford 2007-2008

This software is licensed under the GPL. See accompanying file COPYING.

*/

/*

This file contains functions from the Ayyi C client model lib.
The limited Ayyi functionality required by Samplecat doesnt warrant linking against the full lib.

*/
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include "ayyi.h"
#include "ayyi_model.h"


gboolean
pool__file_exists(char* fname)
{
	// looks through the core fil array looking for the given filename.
	// @param fname - must be either absolute path, or relative to song root.

	g_return_val_if_fail (ayyi.got_shm, FALSE);
	dbg (2, "looking for '%s'...", fname);

	char leafname[256];
	audio_path_get_leaf(fname, leafname); //this means we cant have different files with the same name. Fix later...

	AyyiFilesource* f = NULL;
	while((f = ayyi_song__filesource_next(f))){
		dbg (3, "%i: %s", f, f->name);
		if(!strcmp(f->name, leafname)) return TRUE;
	}
	dbg (0, "file not found in pool (%s).", fname);
	return FALSE;
}



