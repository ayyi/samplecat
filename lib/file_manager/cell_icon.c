/*
 * ROX-Filer, filer for the ROX desktop project
 * Copyright (C) 2006, Thomas Leonard and others (see changelog for details).
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

/* cell_icon.c - a GtkCellRenderer used for the icons in details mode
 *
 * Based on gtkcellrendererpixbuf.c.
 */

#include "config.h"

#include <gtk/gtk.h>
#include <string.h>
#include <sys/stat.h>

#include "debug/debug.h"

#include "file_manager.h"

#include "file_view.h"
#include "cell_icon.h"
#include "dir.h"
#include "display.h"
#include "diritem.h"
#include "pixmaps.h"
#include "fscache.h"

typedef struct _CellIcon CellIcon;
typedef struct _CellIconClass CellIconClass;

struct _CellIcon {
	GtkCellRenderer parent;

	ViewDetails *view_details;
	ViewItem *item;
#ifdef GTK4_TODO
	GdkColor background;
#endif
};

struct _CellIconClass {
	GtkCellRendererClass parent_class;
};


static void  cell_icon_set_property         (GObject*, guint param_id, const GValue *value, GParamSpec *pspec);
static void  cell_icon_init                 (CellIcon*);
static void  cell_icon_class_init           (CellIconClass *class);
static void  cell_icon_get_preferred_width  (GtkCellRenderer*, GtkWidget *widget, int *minimal_size, int *natural_size);
static void  cell_icon_get_preferred_height (GtkCellRenderer*, GtkWidget *widget, int *minimal_size, int *natural_size);
static void  cell_icon_snapshot             (GtkCellRenderer*, GtkSnapshot*, GtkWidget *widget, const GdkRectangle *background_area, const GdkRectangle *cell_area, GtkCellRendererState flags);
static GType cell_icon_get_type             (void);

enum {
	PROP_ZERO,
	PROP_ITEM,
	PROP_BACKGROUND_GDK,
};

/****************************************************************
 *			EXTERNAL INTERFACE			*
 ****************************************************************/

GtkCellRenderer*
cell_icon_new (ViewDetails *view_details)
{
	GtkCellRenderer *cell = GTK_CELL_RENDERER(g_object_new(cell_icon_get_type(), NULL));
	((CellIcon *) cell)->view_details = view_details;

	return cell;
}

/****************************************************************
 *			INTERNAL FUNCTIONS			*
 ****************************************************************/


static GType
cell_icon_get_type (void)
{
	static GType cell_icon_type = 0;

	if (!cell_icon_type) {
		static const GTypeInfo cell_icon_info = {
			sizeof (CellIconClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			(GClassInitFunc) cell_icon_class_init,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (CellIcon),
			0,              /* n_preallocs */
			(GInstanceInitFunc) cell_icon_init,
		};

		cell_icon_type = g_type_register_static(GTK_TYPE_CELL_RENDERER, "CellIcon", &cell_icon_info, 0);
	}

	return cell_icon_type;
}

static void
cell_icon_init (CellIcon *icon)
{
	icon->view_details = NULL;
}

static void
cell_icon_class_init (CellIconClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(class);
	GtkCellRendererClass *cell_class = GTK_CELL_RENDERER_CLASS(class);

	object_class->set_property = cell_icon_set_property;

	cell_class->get_preferred_width = cell_icon_get_preferred_width;
	cell_class->get_preferred_height = cell_icon_get_preferred_height;
	cell_class->snapshot = cell_icon_snapshot;

	g_object_class_install_property(object_class,
			PROP_ITEM,
			g_param_spec_pointer("item",
				"DirItem",
				"The item to render.",
				G_PARAM_WRITABLE));

#ifdef GTK4_TODO
	g_object_class_install_property(object_class,
			PROP_BACKGROUND_GDK,
			g_param_spec_boxed("background_gdk",
				"Background color",
				"Background color as a GdkColor",
				GDK_TYPE_COLOR,
				G_PARAM_WRITABLE));
#endif
}

static void
cell_icon_set_property (GObject *object, guint param_id, const GValue *value, GParamSpec *pspec)
{
	CellIcon *icon = (CellIcon *) object;

	switch (param_id)
	{
		case PROP_ITEM:
			icon->item = (ViewItem *) g_value_get_pointer(value);
			break;
		case PROP_BACKGROUND_GDK:
		{
#ifdef GTK4_TODO
			GdkColor *bg = g_value_get_boxed(value);
			if (bg) {
				icon->background.red = bg->red;
				icon->background.green = bg->green;
				icon->background.blue = bg->blue;
			}
#endif
			break;
		}
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
			break;
	}
}

/* Return the size to display this icon at. If huge, ensures that the image
 * exists in that size, if present at all.
 */
static DisplayStyle
get_style (GtkCellRenderer *cell)
{
	CellIcon *icon = (CellIcon *) cell;
	ViewItem *view_item = icon->item;
	DirItem *item = view_item->item;

	if (!view_item->image)
	{
		AyyiFilemanager* fm = icon->view_details->filer_window;

		if (fm->show_thumbs && item->base_type == TYPE_FILE)
		{
			const guchar* path = make_path(fm->real_path, item->leafname);

			view_item->image = g_fscache_lookup_full(pixmap_cache, (char*)path, FSCACHE_LOOKUP_ONLY_NEW, NULL);
		}
		if (!view_item->image)
		{
			view_item->image = di_image(item);
			if (view_item->image) g_object_ref(view_item->image);
		}
	}
	
	DisplayStyle size = icon->view_details->filer_window->display_style_wanted;

	if (size == AUTO_SIZE_ICONS)
	{
		if (!view_item->image || view_item->image == di_image(item))
			size = SMALL_ICONS;
		else
			size = HUGE_ICONS;
	}
	if (size == HUGE_ICONS && view_item->image &&
	    !view_item->image->huge_pixbuf)
		pixmap_make_huge(view_item->image);

	return size;
}

static void
cell_icon_get_preferred_width  (GtkCellRenderer* cell, GtkWidget *widget, int *min_size, int *natural_size)
{
	int w;

	DisplayStyle size = get_style(cell);
	MaskedPixmap* image = ((CellIcon *) cell)->item->image;

	switch (size) {
		case SMALL_ICONS:
			w = SMALL_WIDTH;
			break;
		case LARGE_ICONS:
			w = image ? image->width : ICON_WIDTH;
			break;
		case HUGE_ICONS:
			w = image ? image->huge_width : HUGE_WIDTH;
			break;
		default:
			w = 2;
			break;
	}

	if (natural_size) *natural_size = w;
	if (min_size) *min_size = w;
}

static void
cell_icon_get_preferred_height (GtkCellRenderer* cell, GtkWidget *widget, int *minimal_size, int *natural_size)
{
	int h;

	DisplayStyle size = get_style(cell);
	MaskedPixmap* image = ((CellIcon *) cell)->item->image;

	switch (size) {
		case SMALL_ICONS:
			h = SMALL_HEIGHT;
			break;
		case LARGE_ICONS:
			if (image) {
				h = image->height;
			} else {
				h = ICON_HEIGHT;
			}
			break;
		case HUGE_ICONS:
			if (image) {
				h = image->huge_height;
			} else {
				h = HUGE_HEIGHT;
			}
			break;
		default:
			h = 2;
			break;
	}

	*natural_size = h;
}

static void
gtk_cell_renderer_pixbuf_get_size (GtkCellRenderer *cell, GtkWidget *widget, const GdkRectangle *cell_area, int *x_offset, int *y_offset, int *width, int *height)
{
  int pixbuf_width;
  int pixbuf_height;
  int calc_width;
  int calc_height;
  int xpad, ypad;

	//GtkStyleContext *context = gtk_widget_get_style_context (widget);
	//gtk_style_context_save (context);
	//gtk_style_context_add_class (context, "image");
	//gtk_icon_size_set_style_classes (gtk_style_context_get_node (context), priv->icon_size);

	pixbuf_width = pixbuf_height = 16;

	//gtk_style_context_restore (context);

	gtk_cell_renderer_get_padding (cell, &xpad, &ypad);
	calc_width  = (int) xpad * 2 + pixbuf_width;
	calc_height = (int) ypad * 2 + pixbuf_height;
  
	if (cell_area && pixbuf_width > 0 && pixbuf_height > 0) {

		float xalign, yalign;
		gtk_cell_renderer_get_alignment (cell, &xalign, &yalign);
		if (x_offset) {
			*x_offset = (((gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL) ?  (1.0 - xalign) : xalign) * (cell_area->width - calc_width));
			*x_offset = MAX (*x_offset, 0);
		}
		if (y_offset) {
			*y_offset = (yalign * (cell_area->height - calc_height));
			*y_offset = MAX (*y_offset, 0);
		}
	} else {
		if (x_offset) *x_offset = 0;
		if (y_offset) *y_offset = 0;
	}

	if (width)
		*width = calc_width;
  
	if (height)
		*height = calc_height;
}

static void
cell_icon_snapshot (GtkCellRenderer* cell, GtkSnapshot *snapshot, GtkWidget *widget, const GdkRectangle *background_area, const GdkRectangle *cell_area, GtkCellRendererState flags)
{
	CellIcon *icon = (CellIcon *) cell;
	ViewItem *view_item = icon->item;
#ifdef GTK4_TODO
	gboolean selected = (flags & GTK_CELL_RENDERER_SELECTED) != 0;
#endif
	
	g_return_if_fail(view_item);

#ifdef GTK4_TODO
	DirItem* item = view_item->item;
#endif
	DisplayStyle size = get_style(cell);
#ifdef GTK4_TODO
	GdkColor* color = &widget->style->base[icon->view_details->filer_window->selection_state];
#endif

	/* Draw the icon */

	if (!view_item->image) return;

	switch (size) {
		case SMALL_ICONS:
		{
  			GdkRectangle rect;
  			gtk_cell_renderer_pixbuf_get_size (cell, widget, cell_area, &rect.x, &rect.y, &rect.width, &rect.height);

			int xpad, ypad;
			gtk_cell_renderer_get_padding (cell, &xpad, &ypad);
			rect.x += cell_area->x + xpad;
			rect.y += cell_area->y + ypad;
			rect.width -= xpad * 2;
			rect.height -= ypad * 2;

			gtk_snapshot_save (snapshot);
			gtk_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (rect.x, rect.y));
			gdk_paintable_snapshot (GDK_PAINTABLE (view_item->image->paintable), snapshot, 16, 16);
			gtk_snapshot_restore (snapshot);

			break;
		}
#if 0
		case LARGE_ICONS:
			draw_large_icon(window, cell_area, item, view_item->image, selected, color);
			break;
		case HUGE_ICONS:
			if (!di_image(item)->huge_pixbuf)
				pixmap_make_huge(di_image(item));
			draw_huge_icon(window, cell_area, item,
					view_item->image, selected, color);
			break;
#endif
		default:
			g_warning("Unknown size %d\n", size);
			break;
	}
}
