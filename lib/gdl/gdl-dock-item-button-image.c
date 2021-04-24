/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*- 
 *
 * gdl-dock-item-button-image.c
 *
 * Author: Joel Holdsworth <joel@airwebreathe.org.uk>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h" 
#include "gdl-dock-item-button-image.h"

#include <math.h>
#include "gdl-tools.h"

#define ICON_SIZE 12

GDL_CLASS_BOILERPLATE (GdlDockItemButtonImage, gdl_dock_item_button_image, GtkWidget, GTK_TYPE_WIDGET);
                       
static gint
gdl_dock_item_button_image_expose (GtkWidget* widget, GdkEventExpose* event)
{
	g_return_val_if_fail (widget, 0);
	GdlDockItemButtonImage* button_image = GDL_DOCK_ITEM_BUTTON_IMAGE (widget);

	cairo_t* cr = gdk_cairo_create (event->window);
	cairo_translate (cr, event->area.x, event->area.y);

	/* Set up the pen */
	cairo_set_line_width (cr, 1.0);

	GtkStyle* style = gtk_widget_get_style (widget);
	g_return_val_if_fail (style, 0);

	GdkColor* color = &style->fg[GTK_STATE_NORMAL];
	cairo_set_source_rgba (cr, color->red / 65535.0, color->green / 65535.0, color->blue / 65535.0, 0.55);

	cairo_new_path (cr);

	switch(button_image->image_type) {
		case GDL_DOCK_ITEM_BUTTON_IMAGE_ICONIFY:
			cairo_arc (cr, 8.5, 3.5, 1.5, 0, 2.0 * M_PI);
			cairo_arc (cr, 8.5, 8.5, 1.5, 0, 2.0 * M_PI);
			cairo_arc (cr, 8.5, 13.5, 1.5, 0, 2.0 * M_PI);
			break;
		default:
			break;
	}

	cairo_fill (cr);

	cairo_destroy (cr);

	return 0;
}

static void
gdl_dock_item_button_image_instance_init (
    GdlDockItemButtonImage *button_image)
{
    GTK_WIDGET_SET_FLAGS (button_image, GTK_NO_WINDOW);
}

static void
gdl_dock_item_button_image_size_request (GtkWidget* widget, GtkRequisition* requisition)
{
	g_return_if_fail (GDL_IS_DOCK_ITEM_BUTTON_IMAGE (widget));
	g_return_if_fail (requisition);

	requisition->width = ICON_SIZE;
	requisition->height = ICON_SIZE;
}

static void
gdl_dock_item_button_image_class_init (GdlDockItemButtonImageClass* klass)
{
	GtkWidgetClass* widget_class = GTK_WIDGET_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	widget_class->expose_event = gdl_dock_item_button_image_expose;
	widget_class->size_request = gdl_dock_item_button_image_size_request;
}

/* ----- Public interface ----- */

/**
 * gdl_dock_item_button_image_new:
 * @param image_type: Specifies what type of image the widget should
 * display
 * 
 * Creates a new GDL dock button image object.
 * Returns: The newly created dock item button image widget.
 **/
GtkWidget*
gdl_dock_item_button_image_new (GdlDockItemButtonImageType image_type)
{
	GdlDockItemButtonImage* button_image = g_object_new (GDL_TYPE_DOCK_ITEM_BUTTON_IMAGE, NULL);
	button_image->image_type = image_type;

	return GTK_WIDGET (button_image);
}
