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

static gboolean audio_path_get_leaf(const char* path, char* leaf);


gboolean
pool__file_exists(char* fname)
{
	// looks through the core fil array looking for the given filename.
	// @param fname - must be either absolute path, or relative to song root.

	dbg (0, "looking for '%s'...", fname);

	char leafname[256];
	audio_path_get_leaf(fname, leafname); //this means we cant have different files with the same name. Fix later...

	filesource_shared* f = NULL;
	while((f = ayyi_song__filesource_next(f))){
		dbg (3, "%i: %s", f, f->name);
		if(!strcmp(f->name, leafname)) return TRUE;
	}
	dbg (0, "file not found in pool (%s).", fname);
	return FALSE;
}


static gboolean
audio_path_get_leaf(const char* path, char* leaf)
{
  //gives the leafname contained in the given path.
  //-if the path is a directory, leaf should be empty, but without checking we have no way of knowing...
  //-relative paths are assumed to be relative to song->audiodir.

  //TODO just use basename() instead?

  g_return_val_if_fail(strlen(path), FALSE);

  //look for slashes and chop off anything before:
  char* pos;
  if((pos = g_strrstr(path, "/"))){
    pos += 1; //move to the rhs of the slash.
  }else pos = (char*)path;

  //make leaf contain the last segment:
  g_strlcpy(leaf, pos, 128);
  dbg (3, "pos=%s leaf=%s", pos, leaf);

  //these tests dont seem to belong here. Use pool_item_file_exists() instead, or do them somewhere else.
#if 0
  //path now contains no slashes. Check to see if this is a file/dir/link:
  if(g_file_test(path, G_FILE_TEST_IS_DIR)){
    //this is a directory. So we cannot find a leaf.
    dbg (0, "is dir (%s).", path);
    return FALSE;
  }
  if(g_file_test(path, G_FILE_TEST_IS_SYMLINK)){
    P_WARN ("symlink: not supported.\n", "");
    return FALSE;
  }
  if(!g_file_test(path, G_FILE_TEST_IS_REGULAR)){
    //this could be because the file doesnt exist.
    P_WARN ("file is not regular: %s\n", path);
    return FALSE;
  }
#endif

  dbg (2, "path=%s --> %s", path, leaf);
  return TRUE;
}


