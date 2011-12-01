/*
 * ROX-Filer, filer for the ROX desktop project
 * By Thomas Leonard, <tal197@users.sourceforge.net>.
 */

#ifndef __VIEW_DETAILS_H__
#define __VIEW_DETAILS_H__

#include <gtk/gtk.h>

typedef struct _ViewDetailsClass ViewDetailsClass;

typedef struct _ViewItem ViewItem;

struct _ViewItem {
	DirItem*      item;
	MaskedPixmap* image;
	int	          old_pos;	    /* Used while sorting */
	gchar*        utf8_name;	/* NULL => leafname is valid */
};

typedef struct _ViewDetails ViewDetails;

struct _ViewDetails {
	GtkTreeView       treeview;
	GtkTreeSelection* selection;
	GtkWidget*        scroll_win;

	Filer*            filer_window; /* Used for styles, etc */

	GPtrArray*        items;		/* ViewItem */

	GtkCellRenderer*  cell_leaf;
	
	int               (*sort_fn)(const void *, const void *);

	int               cursor_base;	/* Cursor when minibuffer opened */

	int               wink_item;		/* -1 => not winking */
	gint              wink_timeout;
	int               wink_step;

	int	              can_change_selection;

	GtkRequisition    desired_size;

	gboolean	      lasso_box;
	int		          lasso_start_index;
	int		          drag_box_x[2];	/* Index 0 is the fixed corner */
	int		          drag_box_y[2];

	gboolean          use_alt_colours;
	GdkColor          alt_bg;
	GdkColor          alt_fg;
};


#define VIEW_DETAILS(obj) (GTK_CHECK_CAST((obj), view_details_get_type(), ViewDetails))

GtkWidget* view_details_new(Filer* filer_window);
GType      view_details_get_type();

void       view_details_dnd_get(GtkWidget*, GdkDragContext*, GtkSelectionData*, guint info, guint time, gpointer data);

void       view_details_set_alt_colours(ViewDetails* view, GdkColor* bg, GdkColor* fg);

#endif /* __VIEW_DETAILS_H__ */
