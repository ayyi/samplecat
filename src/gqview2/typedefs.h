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


#ifndef __gqview2_typedefs_h__
#define __gqview2_typedefs_h__


/*
typedef enum {
	SORT_NONE,
	SORT_NAME,
	SORT_SIZE,
	SORT_TIME,
	SORT_PATH,
	SORT_NUMBER
} SortType;
*/

typedef enum {
	ALTER_NONE,		/* do nothing */
	ALTER_ROTATE_90,
	ALTER_ROTATE_90_CC,	/* counterclockwise */
	ALTER_ROTATE_180,
	ALTER_MIRROR,
	ALTER_FLIP
} AlterType;

typedef enum {
	LAYOUT_HIDE   = 0,
	LAYOUT_LEFT   = 1 << 0,
	LAYOUT_RIGHT  = 1 << 1,
	LAYOUT_TOP    = 1 << 2,
	LAYOUT_BOTTOM = 1 << 3
} LayoutLocation;


typedef struct _ImageLoader ImageLoader;
typedef struct _ThumbLoader ThumbLoader;

typedef struct _CollectInfo CollectInfo;
typedef struct _CollectionData CollectionData;
typedef struct _CollectTable CollectTable;
typedef struct _CollectWindow CollectWindow;

typedef struct _ImageWindow ImageWindow;

typedef struct _FileData FileData;

typedef struct _LayoutWindow LayoutWindow;
typedef struct _ViewDirList ViewDirList;
typedef struct _ViewDirTree ViewDirTree;
typedef struct _ViewFileList ViewFileList;
typedef struct _ViewFileIcon ViewFileIcon;

typedef struct _SlideShowData SlideShowData;
typedef struct _FullScreenData FullScreenData;

typedef struct _PixmapFolders PixmapFolders;


struct _ImageLoader
{
	GdkPixbuf *pixbuf;
	gchar *path;

	gint bytes_read;
	gint bytes_total;

	guint buffer_size;

	gint requested_width;
	gint requested_height;

	gint done;
	gint idle_id;
	gint idle_priority;

	GdkPixbufLoader *loader;
	gint load_fd;

	void (*func_area_ready)(ImageLoader *, guint x, guint y, guint w, guint h, gpointer);
	void (*func_error)(ImageLoader *, gpointer);
	void (*func_done)(ImageLoader *, gpointer);
	void (*func_percent)(ImageLoader *, gdouble, gpointer);

	gpointer data_area_ready;
	gpointer data_error;
	gpointer data_done;
	gpointer data_percent;

	gint idle_done_id;
};

typedef void (* ThumbLoaderFunc)(ThumbLoader *tl, gpointer data);

struct _ThumbLoader
{
	gint standard_loader;

	GdkPixbuf *pixbuf;	/* contains final (scaled) image when done */
	ImageLoader *il;
	gchar *path;

	gint cache_enable;
	gint cache_hit;
	gdouble percent_done;

	gint max_w;
	gint max_h;

	ThumbLoaderFunc func_done;
	ThumbLoaderFunc func_error;
	ThumbLoaderFunc func_progress;

	gpointer data;

	gint idle_done_id;
};

struct _CollectInfo
{
	gchar *path;
	gint64 size;
	time_t date;

	GdkPixbuf *pixbuf;
	gint flag_mask;
};

struct _CollectionData
{
	gchar *path;
	gchar *name;
	GList *list;
	SortType sort_method;

	ThumbLoader *thumb_loader;
	CollectInfo *thumb_info;

	void (*info_updated_func)(CollectionData *, CollectInfo *, gpointer);
	gpointer info_updated_data;

	gint ref;

	/* geometry */
	gint window_read;
	gint window_x;
	gint window_y;
	gint window_w;
	gint window_h;

	/* contents changed since save flag */
	gint changed;
};

struct _CollectTable
{
	GtkWidget *scrolled;
	GtkWidget *listview;
	gint columns;
	gint rows;

	CollectionData *cd;

	GList *selection;
	CollectInfo *prev_selection;

	CollectInfo *click_info;

	GtkWidget *tip_window;
	gint tip_delay_id;
	CollectInfo *tip_info;

	GdkWindow *marker_window;
	CollectInfo *marker_info;

	GtkWidget *status_label;
	GtkWidget *extra_label;

	gint focus_row;
	gint focus_column;
	CollectInfo *focus_info;

	GtkWidget *popup;
	CollectInfo *drop_info;
	GList *drop_list;

	gint sync_idle_id;
	gint drop_idle_id;

	gint show_text;
};

struct _CollectWindow
{
	GtkWidget *window;
	CollectTable *table;
	GtkWidget *status_box;
	GList *list;

	GtkWidget *close_dialog;

	CollectionData *cd;
};

struct _ImageWindow
{
	GtkWidget *widget;	/* use this to add it and show it */
	GtkWidget *image;

	gchar *image_path;
	const gchar *image_name;

	gint image_width;	/* image actual dimensions (pixels) */
	gint image_height;
	GdkPixbuf *pixbuf;
	gint64 size;		/* file size (bytes) */
	time_t mtime;		/* file modified time stamp */
	gint unknown;		/* failed to load image */

	gint window_width;	/* allocated size of window (drawing area) */
	gint window_height;

	gint x_offset;		/* offset of image start (non-zero when image < window) */
	gint y_offset;

	gint vis_width;		/* dimensions of visible part of image */
	gint vis_height;

	gint width;		/* size of scaled image (result) */
	gint height;

	gint x_scroll;		/* scroll offset of image (into width, height to start drawing) */
	gint y_scroll;

	gdouble zoom;		/* zoom we want (0 is auto) */
	gdouble scale;		/* zoom we got (should never be 0) */

	gint tile_width;
	gint tile_height;
	gint tile_cols;		/* count of tile columns */
	GList *tiles;		/* list of buffer tiles */
	GList *tile_cache;	/* list of pixmap/pixbuf tile allocations */
	gint tile_cache_size;	/* allocated size of pixmaps/pixbufs */
	GList *draw_queue;	/* list of areas to redraw */

	GList *draw_queue_2pass;/* list when 2 pass is enabled */

	ImageLoader *il;

	gint draw_idle_id;
	gint draw_idle_high;	/* current idle_id has high priority */

	gint in_drag;
	gint drag_last_x;
	gint drag_last_y;
	gint drag_moved;

	gint has_frame;
	gint top_window_sync;	/* resize top_window when image dimensions change */

	/* top level (not necessarily parent) window */
	GtkWidget *top_window;	/* window that gets title, and window to resize when 'fitting' */
	gchar *title;		/* window title to display left of file name */
	gchar *title_right;	/* window title to display right of file name */
	gint title_show_zoom;	/* option to include zoom in window title */

	gint completed;

	void (*func_update)(ImageWindow *, gpointer);
	void (*func_complete)(ImageWindow *, gint preload, gpointer);
	void (*func_new)(ImageWindow *, gpointer);

	gpointer data_update;
	gpointer data_complete;
	gpointer data_new;

	/* button, scroll functions */
	void (*func_button)(ImageWindow *, gint button,
			    guint32 time, gdouble x, gdouble y, guint state, gpointer);
	void (*func_scroll)(ImageWindow *, GdkScrollDirection direction,
			    guint32 time, gdouble x, gdouble y, guint state, gpointer);

	gpointer data_button;
	gpointer data_scroll;

	/* collection info */
	CollectionData *collection;
	CollectInfo *collection_info;

	AlterType delay_alter_type;

	ImageLoader *read_ahead_il;
	GdkPixbuf *read_ahead_pixbuf;
	gchar *read_ahead_path;

	GdkPixbuf *prev_pixbuf;
	gchar *prev_path;

	gint auto_refresh_id;
	gint auto_refresh_interval;

	gint delay_flip;

	gint scroller_id;
	gint scroller_overlay;
	gint scroller_x;
	gint scroller_y;
	gint scroller_xpos;
	gint scroller_ypos;
	gint scroller_xinc;
	gint scroller_yinc;

	GList *overlay_list;
};

struct _FileData {
	gchar *path;
	const gchar *name;
	gint64 size;
	time_t date;

	GdkPixbuf *pixbuf;
};

struct _LayoutWindow
{
	gchar *path;

	/* base */

	GtkWidget *window;

	GtkWidget *main_box;

	GtkWidget *group_box;
	GtkWidget *h_pane;
	GtkWidget *v_pane;

	GtkTooltips *tooltips;

	/* menus, path selector */

	GtkActionGroup *action_group;
	GtkUIManager *ui_manager;

	GtkWidget *path_entry;

	/* image */

	LayoutLocation image_location;

	ImageWindow *image;

	/* tools window (float) */

	GtkWidget *tools;
	GtkWidget *tools_pane;

	gint tools_float;
	gint tools_hidden;

	/* toolbar */

	GtkWidget *toolbar;
	gint toolbar_hidden;

	GtkWidget *thumb_button;
	gint thumbs_enabled;

	/* dir view */

	LayoutLocation dir_location;

	ViewDirList *vdl;
	ViewDirTree *vdt;
	GtkWidget *dir_view;

	gint tree_view;

	/* file view */

	LayoutLocation file_location;

	ViewFileList *vfl;
	ViewFileIcon *vfi;
	GtkWidget *file_view;

	gint icon_view;
	SortType sort_method;
	gint sort_ascend;

	/* status bar */

	GtkWidget *info_box;
	GtkWidget *info_progress_bar;
	GtkWidget *info_sort;
	GtkWidget *info_status;
	GtkWidget *info_details;
	GtkWidget *info_zoom;

	/* slide show */

	SlideShowData *slideshow;

	/* full screen */

	FullScreenData *full_screen;
	gint full_screen_overlay_id;
	gint full_screen_overlay_on;

	/* dividers */

	gint div_h;
	gint div_v;
	gint div_float;

	/* directory update check */

	gint last_time_id;
	time_t last_time;

	/* misc */

	GtkWidget *utility_box;
	GtkWidget *bar_sort;
	GtkWidget *bar_exif;
	GtkWidget *bar_info;

	gint bar_sort_enabled;
	gint bar_exif_enabled;
	gint bar_info_enabled;

	gint bar_exif_size;
	gint bar_exif_advanced;
};

struct _ViewDirList
{
	GtkWidget *widget;
	GtkWidget *listview;

	gchar *path;
	GList *list;

	FileData *click_fd;

	FileData *drop_fd;
	GList *drop_list;

	gint drop_scroll_id;

	/* func list */
	void (*select_func)(ViewDirList *vdl, const gchar *path, gpointer data);
	gpointer select_data;

	LayoutWindow *layout;

	GtkWidget *popup;

	PixmapFolders *pf;
};

struct _ViewDirTree
{
	GtkWidget *widget;
	GtkWidget *treeview;

	gchar *path;
	gchar *root_path;

	FileData *click_fd;

	FileData *drop_fd;
	GList *drop_list;

	gint drop_scroll_id;
	gint drop_expand_id;

	/* func list */
	void (*select_func)(ViewDirTree *vdt, const gchar *path, gpointer data);
	gpointer select_data;

	LayoutWindow *layout;

	GtkWidget *popup;

	PixmapFolders *pf;

	gint busy_ref;
};

struct _ViewFileList
{
	GtkWidget *widget;
	GtkWidget *listview;

	gchar *path;
	GList *list;

	SortType sort_method;
	gint sort_ascend;

	FileData *click_fd;
	FileData *select_fd;

	gint thumbs_enabled;

	/* thumb updates */
	gint thumbs_running;
	gint thumbs_count;
	ThumbLoader *thumbs_loader;
	FileData *thumbs_filedata;

	/* func list */
	void (*func_thumb_status)(ViewFileList *vfl, gdouble val, const gchar *text, gpointer data);
	gpointer data_thumb_status;

	void (*func_status)(ViewFileList *vfl, gpointer data);
	gpointer data_status;

	gint select_idle_id;
	LayoutWindow *layout;

	GtkWidget *popup;
};

struct _ViewFileIcon
{
	GtkWidget *widget;
	GtkWidget *listview;

	gchar *path;
	GList *list;

	/* table stuff */

	gint columns;
	gint rows;

	GList *selection;
	FileData *prev_selection;

	GtkWidget *tip_window;
	gint tip_delay_id;
	FileData *tip_fd;

	FileData *click_fd;

	FileData *focus_fd;
	gint focus_row;
	gint focus_column;

	SortType sort_method;
	gint sort_ascend;

	gint show_text;

	gint sync_idle_id;

	/* thumbs */
	
	gint thumbs_running;
	GList *thumbs_list;
	gint thumbs_count;
	ThumbLoader *thumbs_loader;
	FileData *thumbs_fd;

	/* func list */
	void (*func_thumb_status)(ViewFileIcon *vfi, gdouble val, const gchar *text, gpointer data);
	gpointer data_thumb_status;

	void (*func_status)(ViewFileIcon *vfi, gpointer data);
	gpointer data_status;

	LayoutWindow *layout;

	GtkWidget *popup;
};

struct _SlideShowData
{
	ImageWindow *imd;

	GList *path_list;
	CollectionData *cd;
	gchar *layout_path;
	LayoutWindow *layout;

	GList *list;
	GList *list_done;

	gchar *slide_path;

	gint slide_count;
	gint timeout_id;

	gint from_selection;

	void (*stop_func)(SlideShowData *, gpointer);
	gpointer stop_data;

	gint paused;
};

struct _FullScreenData
{
	GtkWidget *window;
	ImageWindow *imd;

	GtkWidget *normal_window;
	ImageWindow *normal_imd;

	gint hide_mouse_id;
	gint busy_mouse_id;
	gint cursor_state;

	gint saver_block_id;

	void (*stop_func)(FullScreenData *, gpointer);
	gpointer stop_data;
};

struct _PixmapFolders
{
	GdkPixbuf *close;
	GdkPixbuf *open;
	GdkPixbuf *deny;
	GdkPixbuf *parent;
};


#endif


