/**
* +----------------------------------------------------------------------+
* | This file is part of the Ayyi project. http://ayyi.org               |
* | copyright (C) 2011-2017 Tim Orford <tim@orford.org>                  |
* | copyright (C) 2006, Thomas Leonard and others                        |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#ifndef __fm_dir_h__
#define __fm_dir_h__

#include <sys/types.h>
#include <dirent.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>

// Check for [id]notify support
#if defined(HAVE_SYS_INOTIFY_H)
# define USE_INOTIFY
#elif defined(DN_MULTISHOT) && defined(SIGRTMIN)
# define USE_DNOTIFY
#endif
#if defined(USE_INOTIFY) || defined(USE_DNOTIFY)
#define USE_NOTIFY
extern gboolean dnotify_wakeup_flag;
#endif
#if defined(USE_INOTIFY)
#undef USE_DNOTIFY //TODO this conflicts with configure.ac
#endif

typedef enum {
    DIR_START_SCAN,         // Set 'scanning' indicator
    DIR_END_SCAN,           // Clear 'scanning' indicator
    DIR_ADD,                // Add the listed items to the display
    DIR_REMOVE,             // Remove listed items from display
    DIR_UPDATE,             // Redraw these items
    DIR_ERROR_CHANGED,      // Check dir->error
    DIR_QUEUE_INTERESTING,  // Call dir_queue_recheck
} DirAction;

typedef struct _DirUser DirUser;
typedef void (*DirCallback)(Directory *dir, DirAction action, GPtrArray *items, gpointer data);

extern GFSCache *dir_cache;

struct _DirUser
{
	DirCallback	callback;
	gpointer	data;
};

typedef struct _DirectoryClass DirectoryClass;

struct _DirectoryClass {
	GObjectClass parent;
};

struct _Directory
{
    GObject object;

    char*       pathname;        // Internal use only
    GList*      users;           // Functions to call on update
    char*       error;           // NULL => no error

    struct stat stat_info;       // Internal use

    gboolean	notify_active;   // Notify timeout is running
    gint		idle_callback;   // Idle callback ID

    GHashTable* known_items;     // What our users know about
    GPtrArray*  new_items;       // New items to add in
    GPtrArray*  up_items;        // Items to redraw
    GPtrArray*  gone_items;      // Items removed

    GList*      recheck_list;    // Items to check on callback

    gboolean    have_scanned;    // TRUE after first complete scan
    gboolean    scanning;        // TRUE if we sent DIR_START_SCAN

    /* Indicates that the directory needs to be rescanned.
     * This is cleared when scanning starts, and set when the fscache
     * detects that the directory needs to be rescanned and is already
     * scanning.
     *
     * If scanning finishes when this is set, or if someone attaches
     * and scanning is not in progress, a rescan is triggered.
     */
    gboolean    needs_update;

    gint        rescan_timeout;	 // See dir_rescan_soon()

#ifdef USE_NOTIFY
    int         notify_fd;       // -1 if not watching
#endif
#ifdef USE_INOTIFY
    guint       inotify_source;
#endif
};

/*
 *  Note that Directory objects are not created directly,
 *  a GFSCache object is created, and then g_fscache_lookup() is called.
 */

void     dir_init               ();
void     dir_attach             (Directory*, DirCallback, gpointer);
void     dir_detach             (Directory*, DirCallback, gpointer);
void     dir_update             (Directory*, gchar* pathname);
void     refresh_dirs           (const char* path);
void     dir_check_this         (const guchar* path);
DirItem* dir_update_item        (Directory*, const gchar* leafname);
void     dir_merge_new          (Directory*);
void     dir_force_update_path  (const gchar* path);
void     dnotify_wakeup         ();
void     dir_drop_all_dnotifies ();
void     dir_queue_recheck      (Directory*, DirItem*);

#endif
