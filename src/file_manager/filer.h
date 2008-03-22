/*
 * ROX-Filer, filer for the ROX desktop project
 * By Thomas Leonard, <tal197@users.sourceforge.net>.
 */

#ifndef _FILER_H
#define _FILER_H

#include <gtk/gtk.h>

#if 0
enum {
	RESIZE_STYLE = 0,
	RESIZE_ALWAYS = 1,
	RESIZE_NEVER = 2,
};

typedef enum
{
	OPEN_SHIFT		= 0x01,	/* Do ShiftOpen */
	OPEN_SAME_WINDOW	= 0x02, /* Directories open in same window */
	OPEN_CLOSE_WINDOW	= 0x04, /* Opening files closes the window */
	OPEN_FROM_MINI		= 0x08,	/* Non-dir => close minibuffer */
} OpenFlags;
#endif

typedef enum
{
	FILER_NEEDS_RESCAN	= 0x01, /* Call may_rescan after scanning */
	FILER_UPDATING		= 0x02, /* (scanning) items may already exist */
	FILER_CREATE_THUMBS	= 0x04, /* Create thumbs when scan ends */
} FilerFlags;

/* Numbers used in options */
typedef enum
{
	VIEW_TYPE_COLLECTION = 0,	/* Icons view */
	VIEW_TYPE_DETAILS = 1		/* TreeView details list */
} ViewType;

/* Filter types */
typedef enum
{
	FILER_SHOW_ALL,           /* Show all files, modified by show_hidden */
	FILER_SHOW_GLOB,          /* Show files that match a glob pattern */
} FilterType;

/* iter's next method has just returned the clicked item... */
typedef void (*TargetFunc)(Filer* filer, ViewIter *iter, gpointer data);

struct _Filer
{
	GtkWidget	 *window;
	//GtkBox		 *toplevel_vbox, *view_hbox;
	gboolean	 scanning;      /* State of the 'scanning' indicator */
	gchar		 *sym_path;     /* Path the user sees */
	gchar		 *real_path;    /* realpath(sym_path) */
	ViewIface	 *view;
	ViewType	 view_type;
	gboolean	 temp_item_selected;
	gboolean	 show_hidden;
	gboolean	 filter_directories;
	FilerFlags	 flags;
	SortType	 sort_type;
	GtkSortType	 sort_order;

	DetailsType	 details_type;
	DisplayStyle display_style;
	DisplayStyle display_style_wanted;

	Directory	*directory;

	gboolean	 had_cursor;	/* (before changing directory) */
	char		*auto_select;	/* If it we find while scanning */

	//GtkWidget	*message;	/* The 'Running as ...' message */

	GtkWidget	*minibuffer_area;	/* The hbox to show/hide */
	GtkWidget	*minibuffer_label;	/* The operation name */
	GtkWidget	*minibuffer;		/* The text entry */
	int          mini_cursor_base;	/* XXX */
	MiniType     mini_type;

	FilterType   filter;
	gchar       *filter_string;  /* Glob or regexp pattern */
	gchar       *regexp;         /* Compiled regexp pattern */
	/* TRUE if hidden files are shown because the minibuffer leafname
	 * starts with a dot.
	 */
	gboolean 	temp_show_hidden;

	TargetFunc	target_cb;
	gpointer	target_data;

	GtkWidget	*toolbar;
	GtkWidget	*toolbar_text;
	GtkWidget	*scrollbar;

	gint		open_timeout;	/* Will resize and show window... */

	GtkStateType	selection_state;	/* for drawing selection */
	
	gboolean	show_thumbs;
	GList		*thumb_queue;		/* paths to thumbnail */
	//GtkWidget	*thumb_bar, *thumb_progress;
	int		    max_thumbs;		/* total for this batch */

	gint		auto_scroll;		/* Timer */

	char		*window_id;		/* For remote control */

	GtkWidget*  menu;
};

void update_display(Directory *dir, DirAction action, GPtrArray* items, Filer* filer_window);
#if 0
extern FilerWindow 	*window_with_focus;
extern GList		*all_filer_windows;
extern GHashTable	*child_to_filer;
extern Option		o_filer_auto_resize, o_unique_filer_windows;
extern Option		o_filer_size_limit;

/* Prototypes */
void filer_init(void);
FilerWindow *filer_opendir(const char *path, FilerWindow *src_win, const gchar *wm_class);
#endif //0
gboolean filer_update_dir(Filer *filer_window, gboolean warning);
#if 0
DirItem *filer_selected_item(FilerWindow *filer_window);
#endif
void change_to_parent(Filer *filer_window);
void full_refresh(void);
#if 0
void filer_openitem(FilerWindow *filer_window, ViewIter *iter, OpenFlags flags);
void filer_check_mounted(const char *real_path);
void filer_close_recursive(const char *path);
#endif
void filer_change_to(Filer *filer_window, const char *path, const char *from);
#if 0
gboolean filer_exists(FilerWindow *filer_window);
FilerWindow *filer_get_by_id(const char *id);
void filer_set_id(FilerWindow *, const char *id);
void filer_open_parent(FilerWindow *filer_window);
void filer_detach_rescan(FilerWindow *filer_window);
void filer_target_mode(FilerWindow	*filer_window,
			TargetFunc	fn,
			gpointer	data,
			const char	*reason);
#endif
GList *filer_selected_items(Filer *filer_window);
void filer_create_thumb(Filer* filer_window, const gchar *pathname);
void filer_cancel_thumbnails(Filer* filer_window);
#if 0
void filer_set_title(FilerWindow *filer_window);
#endif
void filer_create_thumbs(Filer* filer_window);
//void filer_add_tip_details(FilerWindow *filer_window, GString *tip, DirItem *item);
void filer_selection_changed(Filer *filer_window, gint time);
#if 0
void filer_lost_selection(FilerWindow *filer_window, guint time);
void filer_window_set_size(FilerWindow *filer_window, int w, int h);
gboolean filer_window_delete(GtkWidget *window, GdkEvent *unused, FilerWindow *filer_window);
void filer_set_view_type(FilerWindow *filer_window, ViewType type);
void filer_window_toggle_cursor_item_selected(FilerWindow *filer_window);
void filer_perform_action(FilerWindow *filer_window, GdkEventButton *event);
gint filer_motion_notify(FilerWindow *filer_window, GdkEventMotion *event);
gint filer_key_press_event(GtkWidget *widget, GdkEventKey *event, FilerWindow *filer_window);
void filer_set_autoscroll(FilerWindow *filer_window, gboolean auto_scroll);
#endif
void filer_refresh(Filer *filer_window);

gboolean filer_match_filter(Filer*, DirItem *item);
#if 0
gboolean filer_set_filter(FilerWindow *filer_window, FilterType type, const gchar *filter_string);
void filer_set_filter_directories(FilerWindow *fwin, gboolean filter_directories);
void filer_set_hidden(FilerWindow *fwin, gboolean hidden);
void filer_next_selected(FilerWindow *filer_window, int dir);
void filer_save_settings(FilerWindow *fwin);
#endif

/*static */void attach(Filer* filer_window);

#endif /* _FILER_H */
