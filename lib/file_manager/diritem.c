/*
 * $Id: diritem.c,v 1.47 2005/07/24 10:19:30 tal197 Exp $
 *
 * Copyright (C) 2005, the ROX-Filer team.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
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

/* diritem.c - get details about files */

/* Don't load icons larger than 400K (this is rather excessive, basically
 * we just want to stop people crashing the filer with huge icons).
 */
#define MAX_ICON_SIZE (400 * 1024)

#include "config.h"
#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include "debug/debug.h"

#include "file_manager.h"

#include "diritem.h"
#include "pixmaps.h"

#define _g_object_unref0(var) ((var == NULL) ? NULL : (var = (g_object_unref (var), NULL)))

#define RECENT_DELAY (5 * 60)	/* Time in seconds to consider a file recent */
#define ABOUT_NOW(time) (diritem_recent_time - time < RECENT_DELAY)
/* If you want to make use of the RECENT flag, make sure this is set to
 * the current time before calling diritem_restat().
 */
time_t diritem_recent_time;

/* Static prototypes */
static void examine_dir(const guchar *path, DirItem *item, struct stat *link_target);

/****************************************************************
 *			EXTERNAL INTERFACE			*
 ****************************************************************/

void diritem_init(void)
{
}


/* Bring this item's structure uptodate.
 * 'parent' is optional; it saves one stat() for directories.
 */
void
diritem_restat (const guchar* path, DirItem* item, struct stat* parent)
{
	struct stat	info;

	_g_object_unref0(item->_image);
	item->flags = 0;
	item->mime_type = NULL;

	if (lstat((char*)path, &info) == -1)
	{
		item->lstat_errno = errno;
		item->base_type = TYPE_ERROR;
		item->size = 0;
		item->mode = 0;
		item->mtime = item->ctime = item->atime = 0;
		item->uid = (uid_t) -1;
		item->gid = (gid_t) -1;
	}
	else
	{
		guchar* target_path;

		item->lstat_errno = 0;
		item->size = info.st_size;
		item->mode = info.st_mode;
		item->atime = info.st_atime;
		item->ctime = info.st_ctime;
		item->mtime = info.st_mtime;
		item->uid = info.st_uid;
		item->gid = info.st_gid;
		if (ABOUT_NOW(item->mtime) || ABOUT_NOW(item->ctime))
			item->flags |= ITEM_FLAG_RECENT;

		//if (xtype_have_attr(path)) item->flags |= ITEM_FLAG_HAS_XATTR;

		if (S_ISLNK(info.st_mode))
		{
			if (stat((char*)path, &info))
				item->base_type = TYPE_ERROR;
			else
				item->base_type =
					mode_to_base_type(info.st_mode);

			item->flags |= ITEM_FLAG_SYMLINK;

			target_path = (guchar*)readlink_dup((char*)path);
		}
		else
		{
			item->base_type = mode_to_base_type(info.st_mode);
			target_path = (guchar *) path;
		}

		/*
		if (item->base_type == TYPE_DIRECTORY)
		{
			if (mount_is_mounted(target_path, &info,
					target_path == path ? parent : NULL))
				item->flags |= ITEM_FLAG_MOUNT_POINT
						| ITEM_FLAG_MOUNTED;
			else if (g_hash_table_lookup(fstab_mounts,
							target_path))
				item->flags |= ITEM_FLAG_MOUNT_POINT;
		}
		*/

		if (path != target_path)
			g_free(target_path);
	}

	if (item->base_type == TYPE_DIRECTORY)
	{
		/* KRJW: info.st_uid will be the uid of the dir, regardless
		 * of whether `path' is a dir or a symlink to one.  Note that
		 * if path is a symlink to a dir, item->uid will be the uid
		 * of the *symlink*, but we really want the uid of the dir
		 * to which the symlink points.
		 */
		examine_dir(path, item, &info);
	}
	else if (item->base_type == TYPE_FILE)
	{
		if (item->size == 0) item->mime_type = text_plain;
		/*
		else if (item->flags & ITEM_FLAG_SYMLINK)
		{
			guchar *link_path;
			link_path = pathdup(path);
			item->mime_type = type_from_path(link_path ? link_path : path);
			g_free(link_path);
		}
		*/
		else
			item->mime_type = type_from_path((char*)path);

		/* Note: for symlinks we need the mode of the target */
		if (info.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))
		{
			/* Note that the flag is set for ALL executable
			 * files, but the mime_type must also be executable
			 * for clicking on the file to run it.
			 */
			item->flags |= ITEM_FLAG_EXEC_FILE;

			if (item->mime_type == NULL ||
			    item->mime_type == application_octet_stream)
			{
				item->mime_type = application_executable;
			}
			else if (item->mime_type == text_plain &&
			         !strchr(item->leafname, '.'))
			{
				item->mime_type = application_x_shellscript;
			}
		}		
		else if (item->mime_type == application_x_desktop)
		{
			item->flags |= ITEM_FLAG_EXEC_FILE;
		}

		if (!item->mime_type)
			item->mime_type = text_plain;

		//check_globicon(path, item);

		if (item->mime_type == application_x_desktop && item->_image == NULL)
		{
			//item->_image = g_fscache_lookup(desktop_icon_cache, path);
			item->_image = NULL;
		}
	}
	//else check_globicon(path, item);

	if (!item->mime_type)
		//item->mime_type = mime_type_from_base_type(item->base_type);
		item->mime_type = text_plain;
}


DirItem*
diritem_new (const guchar* leafname)
{
	DirItem* item = g_new(DirItem, 1);
	*item = (DirItem){
		.leafname = g_strdup((gchar*)leafname),
	};
	item->may_delete = FALSE;
	item->_image = NULL;
	item->base_type = TYPE_UNKNOWN;
	item->flags = ITEM_FLAG_NEED_RESCAN_QUEUE;
	item->mime_type = NULL;
	item->leafname_collate = collate_key_new(leafname);

	return item;
}


void
diritem_free (DirItem* item)
{
	g_return_if_fail(item);

	_g_object_unref0(item->_image);
	collate_key_free(item->leafname_collate);
	g_free(item->leafname);
	g_free(item);
}


/* For use by di_image() only. Sets item->_image. */
void
_diritem_get_image (DirItem* item)
{
	g_return_if_fail(item->_image == NULL);

	if (item->base_type == TYPE_ERROR)
	{
		item->_image = im_error;
		g_object_ref(im_error);
	}
	else
		item->_image = type_to_icon(item->mime_type);
}


/****************************************************************
 *			INTERNAL FUNCTIONS			*
 ****************************************************************/

/* Fill in more details of the DirItem for a directory item.
 * - Looks for an image (but maybe still NULL on error)
 * - Updates ITEM_FLAG_APPDIR
 *
 * link_target contains stat info for the link target for symlinks (or for the
 * item itself if not a link).
 */
static void
examine_dir (const guchar* path, DirItem* item, struct stat* link_target)
{
	struct stat info;
	static GString* tmp = NULL;
	uid_t uid = link_target->st_uid;

	if (!tmp)
		tmp = g_string_new(NULL);

	//check_globicon(path, item);

	/*
	if (item->flags & ITEM_FLAG_MOUNT_POINT)
	{
		item->mime_type = inode_mountpoint;
		return;		// Try to avoid automounter problems
	}
	*/

	//if (link_target->st_mode & S_IWOTH)	return;		// Don't trust world-writable dirs

	/* Finding the icon:
	 *
	 * - If it contains a .DirIcon then that's the icon
	 * - If it contains an AppRun then it's an application
	 * - If it contains an AppRun but no .DirIcon then try to
	 *   use AppIcon.xpm as the icon.
	 *
	 * .DirIcon and AppRun must have the same owner as the
	 * directory itself, to prevent abuse of /tmp, etc.
	 * For symlinks, we want the symlink's owner.
	 */

	g_string_printf(tmp, "%s/.DirIcon", path);

	if (item->_image)
		goto no_diricon;	/* Already got an icon */

	if (lstat(tmp->str, &info) != 0 || info.st_uid != uid)
		goto no_diricon;	/* Missing, or wrong owner */

	if (S_ISLNK(info.st_mode) && stat(tmp->str, &info) != 0)
		goto no_diricon;	/* Bad symlink */

	if (info.st_size > MAX_ICON_SIZE || !S_ISREG(info.st_mode))
		goto no_diricon;	/* Too big, or non-regular file */

	/* Try to load image; may still get NULL... */
	//item->_image = g_fscache_lookup(pixmap_cache, tmp->str);
	item->_image = NULL;

no_diricon:

	if (stat(tmp->str, &info) != 0)
		goto out;	/* Missing, or broken symlink */

	if (info.st_size > MAX_ICON_SIZE || !S_ISREG(info.st_mode))
		goto out;	/* Too big, or non-regular file */

	/* Try to load image; may still get NULL... */
	//item->_image = g_fscache_lookup(pixmap_cache, tmp->str);
	item->_image = NULL;

out:
	;

#if 0
	if ((item->flags & ITEM_FLAG_APPDIR) && !item->_image)
	{
		/* This is an application without an icon */
		item->_image = im_appdir;
		g_object_ref(item->_image);
	}
#endif
}
