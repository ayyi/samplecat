/*
 * GQview
 * (C) 2004 John Ellis
 *
 * Author: John Ellis
 *
 * This software is released under the GNU General Public License (GNU GPL).
 * Please read the included file COPYING for more information.
 * This software comes with no warranty of any kind, use at your own risk!
 */


#ifndef GQVIEW_H
#define GQVIEW_H

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#ifdef HAVE_STRVERSCMP
#  define _GNU_SOURCE
#endif

/*
 *-------------------------------------
 * Standard library includes
 *-------------------------------------
 */

#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>

/*
 *-------------------------------------
 * includes for glib / gtk / gdk-pixbuf
 *-------------------------------------
 */

#include <gdk/gdk.h>
#include <gtk/gtk.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk-pixbuf/gdk-pixbuf-loader.h>

#include "typedefs.h"

/*
 *----------------------------------------------------------------------------
 * defines
 *----------------------------------------------------------------------------
 */

#define GQVIEW_RC_DIR             ".gqview"
#define GQVIEW_RC_DIR_COLLECTIONS GQVIEW_RC_DIR"/collections"
#define GQVIEW_RC_DIR_TRASH       GQVIEW_RC_DIR"/trash"

#define RC_FILE_NAME "gqviewrc"

#define ZOOM_RESET_ORIGINAL 0
#define ZOOM_RESET_FIT_WINDOW 1
#define ZOOM_RESET_NONE 2

#define SCROLL_RESET_TOPLEFT 0
#define SCROLL_RESET_CENTER 1
#define SCROLL_RESET_NOCHANGE 2

#define MOUSEWHEEL_SCROLL_SIZE 20

#define GQVIEW_EDITOR_SLOTS 10

/*
 *----------------------------------------------------------------------------
 * globals
 *----------------------------------------------------------------------------
 */

/*
 * Since globals are used everywhere,
 * it is easier to define them here.
 */

extern GList *filename_filter;

/* -- options -- */
extern gint main_window_w;
extern gint main_window_h;
extern gint main_window_x;
extern gint main_window_y;
extern gint main_window_maximized;

extern gint float_window_w;
extern gint float_window_h;
extern gint float_window_x;
extern gint float_window_y;
extern gint float_window_divider;

extern gint window_hdivider_pos;
extern gint window_vdivider_pos;

extern gint save_window_positions;
extern gint tools_float;
extern gint tools_hidden;
extern gint toolbar_hidden;
extern gint progressive_key_scrolling;

extern gint startup_path_enable;
extern gchar *startup_path;
extern gint confirm_delete;
extern gint enable_delete_key;
extern gint safe_delete_enable;
extern gchar *safe_delete_path;
extern gint safe_delete_size;
extern gint restore_tool;
extern gint zoom_mode;
extern gint two_pass_zoom;
extern gint scroll_reset_method;
extern gint fit_window;
extern gint limit_window_size;
extern gint zoom_to_fit_expands;
extern gint max_window_size;
extern gint thumb_max_width;
extern gint thumb_max_height;
extern gint enable_thumb_caching;
extern gint enable_thumb_dirs;
extern gint use_xvpics_thumbnails;
extern gint thumbnail_spec_standard;
extern gint enable_metadata_dirs;
extern gint show_dot_files;
extern gint file_filter_disable;
extern gchar *editor_name[];
extern gchar *editor_command[];

extern gint thumbnails_enabled;
extern DtSortType file_sort_method;
extern gint file_sort_ascending;

extern gint slideshow_delay;	/* in tenths of a second */
extern gint slideshow_random;
extern gint slideshow_repeat;

extern gint mousewheel_scrolls;
extern gint enable_in_place_rename;

extern gint black_window_background;

extern gint fullscreen_screen;
extern gint fullscreen_clean_flip;
extern gint fullscreen_disable_saver;
extern gint fullscreen_above;

extern gint dupe_custom_threshold;

extern gint debug;

extern gint recent_list_max;

extern gint collection_rectangular_selection;

extern gint tile_cache_max;	/* in megabytes */
extern gint thumbnail_quality;
extern gint zoom_quality;
extern gint dither_quality;

extern gint zoom_increment;	/* 10 is 1.0, 5 is 0.05, 20 is 2.0, etc. */

extern gint enable_read_ahead;

extern gint place_dialogs_under_mouse;

/* layout */
extern gchar *layout_order;
extern gint layout_style;

extern gint layout_view_icons;
extern gint layout_view_tree;

extern gint show_icon_names;

extern gint tree_descend_subdirs;

extern gint lazy_image_sync;
extern gint update_on_time_change;
extern gint exif_rotate_enable;

/*
 *----------------------------------------------------------------------------
 * main.c
 *----------------------------------------------------------------------------
 */

/*
 * This also doubles as the main.c header.
 */

void help_window_show(const gchar *key);

#ifdef GTK4_TODO
void keyboard_scroll_calc(gint *x, gint *y, GdkEventKey *event);
gint key_press_cb(GtkWidget *widget, GdkEventKey *event, gpointer data);
#endif
void exit_gqview(void);

#endif
