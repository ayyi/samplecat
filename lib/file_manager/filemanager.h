/*
 +----------------------------------------------------------------------+
 | This file is part of the Ayyi project. https://www.ayyi.org          |
 | copyright (C) 2011-2024 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#pragma once

#include <glib.h>
#include <glib-object.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include "file_manager/view_iface.h"
#include "file_manager/typedefs.h"
#include "file_manager/dir.h"

G_BEGIN_DECLS


#define AYYI_TYPE_FILEMANAGER            (ayyi_filemanager_get_type ())
#define AYYI_FILEMANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), AYYI_TYPE_FILEMANAGER, AyyiFilemanager))
#define AYYI_FILEMANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), AYYI_TYPE_FILEMANAGER, AyyiFilemanagerClass))
#define AYYI_IS_FILEMANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AYYI_TYPE_FILEMANAGER))
#define AYYI_IS_FILEMANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), AYYI_TYPE_FILEMANAGER))
#define AYYI_FILEMANAGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), AYYI_TYPE_FILEMANAGER, AyyiFilemanagerClass))

typedef struct _AyyiFilemanager        AyyiFilemanager;
typedef struct _AyyiFilemanagerClass   AyyiFilemanagerClass;
typedef struct _AyyiFilemanagerPrivate AyyiFilemanagerPrivate;

typedef enum
{
    OPEN_SHIFT        = 0x01,	// Do ShiftOpen
    OPEN_SAME_WINDOW  = 0x02,   // Directories open in same window
    OPEN_CLOSE_WINDOW = 0x04,   // Opening files closes the window
    OPEN_FROM_MINI    = 0x08,	// Non-dir => close minibuffer
} OpenFlags;

typedef enum
{
    FILER_NEEDS_RESCAN  = 0x01, // Call may_rescan after scanning
    FILER_UPDATING      = 0x02, // (scanning) items may already exist
    FILER_CREATE_THUMBS = 0x04, // Create thumbs when scan ends
} FilerFlags;

typedef enum
{
	VIEW_TYPE_COLLECTION = 0,	// Icons view
	VIEW_TYPE_DETAILS = 1		// TreeView details list
} ViewType;

typedef struct {
    GtkWidget* area;            // The hbox to show/hide
    GtkWidget* label;           // The operation name
    GtkWidget* entry;           // The text entry
    int        cursor_base;     // XXX
    MiniType   type;
} Mini;

typedef void (*TargetFunc)(AyyiFilemanager*, ViewIter*, gpointer); // iter's next method has just returned the clicked item

struct _AyyiFilemanager {
    GObject                    parent_instance;
    GtkWidget*                 window;          // TODO rename to 'widget'
    gboolean	               scanning;        // State of the 'scanning' indicator
    gchar*                     sym_path;        // Path the user sees
    gchar*                     real_path;       // realpath(sym_path)
    ViewIface*                 view;
    ViewType                   view_type;
    gboolean                   temp_item_selected;
    gboolean                   show_hidden;
    gboolean                   filter_directories;
    FilerFlags                 flags;
    FmSortType                 sort_type;
    GtkSortType                sort_order;

	Mini                       mini;

    DetailsType                details_type;
    DisplayStyle               display_style;
    DisplayStyle               display_style_wanted;

    Directory*                 directory;

    gboolean                   had_cursor;      // (before changing directory)
    char*                      auto_select;     // If it we find while scanning

    FilterType                 filter;
    gchar*                     filter_string;   // Glob or regexp pattern
    gchar*                     regexp;          // Compiled regexp pattern
    gboolean 	               temp_show_hidden;// TRUE if hidden files are shown because the minibuffer leafname starts with a dot.

    gint                       open_timeout;    // Will resize and show window...

#ifdef GTK4_TODO
    GtkStateType               selection_state; // for drawing selection
#endif

    gboolean                   show_thumbs;
    GList*                     thumb_queue;     // paths to thumbnail
    //GtkWidget	*thumb_bar, *thumb_progress;
    int                        max_thumbs;      // total for this batch

#if 0
    gint                       auto_scroll;     // Timer
#endif

    struct {
       GtkWidget*              widget;
       GMenuModel*             model;
    }                          menu;

    TargetFunc	               target_cb;
    gpointer	               target_data;

    GtkWidget*                 toolbar;
    GtkWidget*                 toolbar_text;
#if 0
    GtkWidget*                 scrollbar;

    char*                      window_id;       // For remote control
#endif

    AyyiFilemanagerPrivate* priv;
};

struct _AyyiFilemanagerClass {
    GObjectClass parent_class;
    GHashTable*  filetypes;
};


GType            ayyi_filemanager_get_type         (void) G_GNUC_CONST;
AyyiFilemanager* ayyi_filemanager_new              ();
AyyiFilemanager* ayyi_filemanager_construct        (GType);
GtkWidget*       ayyi_filemanager_new_window       (AyyiFilemanager*, const gchar* path);
void             fm__change_to                     (AyyiFilemanager*, const char* path, const char* from);
void             fm__change_to_parent              (AyyiFilemanager*);
GList*           fm__selected_items                (AyyiFilemanager*);
void             fm__selection_changed             (AyyiFilemanager*, gint time);
void             fm__next_selected                 (AyyiFilemanager*, int direction);
gboolean         fm__update_dir                    (AyyiFilemanager*, gboolean warning);
gboolean         fm__match_filter                  (AyyiFilemanager*, DirItem*);
gboolean         fm__set_filter                    (AyyiFilemanager*, FilterType, const gchar* filter_string);
void             fm__create_thumb                  (AyyiFilemanager*, const gchar* pathname);
void             fm__cancel_thumbnails             (AyyiFilemanager*);
void             fm__create_thumbs                 (AyyiFilemanager*);
void             fm__open_item                     (AyyiFilemanager*, ViewIter*, OpenFlags);
void             fm__target_mode                   (AyyiFilemanager*, TargetFunc, gpointer, const char* reason);
void             filer_detach_rescan               (AyyiFilemanager*);

/*static */void  attach                            (AyyiFilemanager*);

void             ayyi_filemanager_emit_dir_changed (AyyiFilemanager*);

gboolean         fm__exists                        (AyyiFilemanager*);

G_END_DECLS
