/*
 * gdl-dock-item-grip.c
 *
 * Author: Michael Meeks Copyright (C) 2002 Sun Microsystems, Inc.
 *
 * Based on BonoboDockItemGrip.  Original copyright notice follows.
 *
 * Copyright (C) 1998 Ettore Perazzoli
 * Copyright (C) 1998 Elliot Lee
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * All rights reserved.
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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include "il8n.h"
#include "debug.h"
#include "utils.h"
#include "gdl-dock-item.h"
#include "gdl-dock-item-grip.h"
#include "gdl-dock-item-button-image.h"
#include "gdl-switcher.h"

/**
 * SECTION:gdl-dock-item-grip
 * @title: GdlDockItemGrip
 * @short_description: A grip for dock widgets.
 * @see_also: GtlDockItem, GdlDockItemButtonImage
 * @stability: Private
 *
 * This widget contains an area where the user can click to drag the dock item
 * and two buttons. The first button allows to iconify the dock item.
 * The second one allows to close the dock item.
 */

#define ALIGN_BORDER 5
#define DRAG_HANDLE_SIZE 10

enum {
    PROP_0,
    PROP_ITEM
};

struct _GdlDockItemGripPrivate {
    GdlDockItem *item;

    GtkWidget   *label;

    GtkWidget   *close_button;
    GtkWidget   *iconify_button;

    gboolean    handle_shown;
};

struct _GdlDockItemGripClassPrivate {
    GtkCssProvider *css;
};

G_DEFINE_TYPE_WITH_CODE (GdlDockItemGrip, gdl_dock_item_grip, GTK_TYPE_WIDGET,
	G_ADD_PRIVATE (GdlDockItemGrip)
	g_type_add_class_private (g_define_type_id, sizeof (GdlDockItemGripClassPrivate));
)

/**
 * gdl_dock_item_create_label_widget:
 * @grip: The GdlDockItemGrip for which to create a label widget
 *
 * Creates a label for a given grip, containing an icon and title
 * text if applicable.  The icon is created using either the
 * #GdlDockObject:stock-id or #GdlDockObject:pixbuf-icon properties,
 * depending on how the grip was created.  If both properties are %NULL,
 * then the icon is omitted.  The title is taken from the
 * #GdlDockObject:long-name property.  Again, if the property is %NULL,
 * then the title is omitted.
 *
 * Returns: The newly-created label box for the grip.
 */
GtkWidget*
gdl_dock_item_create_label_widget (GdlDockItemGrip *grip)
{
    gchar *icon_name = NULL;
    gchar *title = NULL;
    GdkPixbuf *pixbuf;

    GtkBox* label_box = (GtkBox*)gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);

    g_object_get (G_OBJECT (grip->priv->item), "stock-id", &icon_name, "pixbuf-icon", &pixbuf, NULL);
    if (icon_name) {
        GtkImage* image = GTK_IMAGE(gtk_image_new_from_icon_name (icon_name));

        gtk_box_append (GTK_BOX(label_box), GTK_WIDGET(image));

        g_free (icon_name);

    } else if (pixbuf) {
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        GtkImage* image = GTK_IMAGE(gtk_image_new_from_pixbuf (pixbuf));
#pragma GCC diagnostic warning "-Wdeprecated-declarations"

        gtk_box_append (GTK_BOX(label_box), GTK_WIDGET(image));
    }

    g_object_get (G_OBJECT (grip->priv->item), "long-name", &title, NULL);
    if (title) {
        GtkLabel* label = GTK_LABEL(gtk_label_new(title));
        gtk_label_set_ellipsize(label, PANGO_ELLIPSIZE_END);
        gtk_label_set_justify(label, GTK_JUSTIFY_LEFT);
		gtk_widget_set_halign(GTK_WIDGET(label), GTK_ALIGN_START);

        if (gtk_widget_get_direction (GTK_WIDGET(grip)) == GTK_TEXT_DIR_RTL) {
            gtk_box_append (GTK_BOX(label_box), GTK_WIDGET(label));
        } else {
            gtk_box_append (GTK_BOX(label_box), GTK_WIDGET(label));
        }

        g_free(title);
    }

    return GTK_WIDGET(label_box);
}

static void
gdl_dock_item_grip_draw (GtkWidget *widget, cairo_t *cr)
{
    GdlDockItemGrip* grip = GDL_DOCK_ITEM_GRIP (widget);

    if (grip->priv->handle_shown) {
    	cairo_rectangle_int_t handle_area;

        GtkAllocation allocation;
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        gtk_widget_get_allocation (widget, &allocation);
#pragma GCC diagnostic warning "-Wdeprecated-declarations"
        if (gtk_widget_get_direction (widget) != GTK_TEXT_DIR_RTL) {
            handle_area.x = allocation.x;
            handle_area.y = allocation.y;
            handle_area.width = DRAG_HANDLE_SIZE;
            handle_area.height = allocation.height;
        } else {
            handle_area.x = allocation.x + allocation.width - DRAG_HANDLE_SIZE;
            handle_area.y = allocation.y;
            handle_area.width = DRAG_HANDLE_SIZE;
            handle_area.height = allocation.height;
        }

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        gtk_render_handle (gtk_widget_get_style_context (widget), cr, handle_area.x, handle_area.y, handle_area.width, handle_area.height);
#pragma GCC diagnostic warning "-Wdeprecated-declarations"
    }

    gint width = gtk_widget_get_width (widget);
    gint height = gtk_widget_get_height (widget);

    gtk_render_background (gtk_widget_get_style_context (widget), cr, 0, 0, width, height);
}

static void
gdl_dock_item_grip_snapshot (GtkWidget *widget, GtkSnapshot *snapshot)
{
	int width = gtk_widget_get_width (widget);
	int height = gtk_widget_get_height (widget);

	cairo_t* cr = gtk_snapshot_append_cairo (snapshot, &GRAPHENE_RECT_INIT ( 0, 0, width, height));

	gdl_dock_item_grip_draw (widget, cr);

	cairo_destroy (cr);

	for (GtkWidget* c = gtk_widget_get_first_child(widget); c; (c = gtk_widget_get_next_sibling (c))) {
		gtk_widget_snapshot_child (widget, c, snapshot);
	}
}

static void
gdl_dock_item_grip_item_notify (GObject *master, GParamSpec *pspec, gpointer data)
{
	GdlDockItemGrip *grip = GDL_DOCK_ITEM_GRIP (data);

	if ((!strcmp (pspec->name, "stock-id")) || (!strcmp (pspec->name, "long-name"))) {
		gdl_dock_item_grip_set_label (grip, gdl_dock_item_create_label_widget(grip));

	} else if (!strcmp (pspec->name, "behavior")) {
		if (grip->priv->close_button) {
			gtk_widget_set_visible(GTK_WIDGET (grip->priv->close_button), !GDL_DOCK_ITEM_CANT_CLOSE (grip->priv->item));
		}
		if (grip->priv->iconify_button) {
			gtk_widget_set_visible(GTK_WIDGET (grip->priv->iconify_button), !GDL_DOCK_ITEM_CANT_ICONIFY (grip->priv->item));
		}
	}
}

static void
gdl_dock_item_grip_dispose (GObject *object)
{
    GdlDockItemGrip *grip = GDL_DOCK_ITEM_GRIP (object);
    GdlDockItemGripPrivate *priv = grip->priv;

    if (priv->label) {
        gtk_widget_unparent(priv->label);
        priv->label = NULL;
    }

    if (grip->priv->item) {
        g_signal_handlers_disconnect_by_func (grip->priv->item, gdl_dock_item_grip_item_notify, grip);
        grip->priv->item = NULL;
    }

    G_OBJECT_CLASS (gdl_dock_item_grip_parent_class)->dispose (object);
}

static void
gdl_dock_item_grip_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    g_return_if_fail (GDL_IS_DOCK_ITEM_GRIP (object));

    GdlDockItemGrip *grip = GDL_DOCK_ITEM_GRIP (object);

    switch (prop_id) {
        case PROP_ITEM:
            grip->priv->item = g_value_get_object (value);
            if (grip->priv->item) {
                behaviour_subject_connect ((GObject*)grip->priv->item, "long-name", (void*)gdl_dock_item_grip_item_notify, grip);
                behaviour_subject_connect ((GObject*)grip->priv->item, "stock-id", (void*)gdl_dock_item_grip_item_notify, grip);
                g_signal_connect (grip->priv->item, "notify::behavior", G_CALLBACK (gdl_dock_item_grip_item_notify), grip);

                if (!GDL_DOCK_ITEM_CANT_CLOSE (grip->priv->item) && grip->priv->close_button)
                    gtk_widget_set_visible (grip->priv->close_button, true);
                if (!GDL_DOCK_ITEM_CANT_ICONIFY (grip->priv->item) && grip->priv->iconify_button)
                    gtk_widget_set_visible (grip->priv->iconify_button, true);
            }
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
gdl_dock_item_grip_close_clicked (GtkWidget *widget, GdlDockItemGrip *grip)
{
    g_return_if_fail (grip->priv->item != NULL);

    gdl_dock_item_hide_item (grip->priv->item);
}

static void
gdl_dock_item_grip_iconify_clicked (GtkWidget *widget, GdlDockItemGrip *grip)
{
    g_return_if_fail (grip->priv->item != NULL);

    GtkWidget* parent = gtk_widget_get_parent (GTK_WIDGET (grip->priv->item));
    if (GDL_IS_SWITCHER (parent)) {
        /* Note: We can not use gtk_container_foreach (parent) here because
         * during iconificatoin, the internal children changes in parent.
         * Instead we keep a list of items to iconify and iconify them
         * one by one.
         */
        GList *items = gtk_widget_get_children (parent);
        for (GList* node = items; node != NULL; node = node->next) {
            GdlDockItem *item = GDL_DOCK_ITEM (node->data);
            if (!GDL_DOCK_ITEM_CANT_ICONIFY (item) && !gdl_dock_item_is_closed (item))
                gdl_dock_item_iconify_item (item);
        }
        g_list_free (items);
    } else {
        gdl_dock_item_iconify_item (grip->priv->item);
    }

    /* Workaround to unhighlight the iconify button. See bug #676890
     * Set button insensitive in order to set priv->in_button = FALSE
     */
    gtk_widget_set_state_flags (grip->priv->iconify_button, GTK_STATE_FLAG_INSENSITIVE, TRUE);
    gtk_widget_set_state_flags (grip->priv->iconify_button, GTK_STATE_FLAG_NORMAL, TRUE);
}

static void
gdl_dock_item_grip_init (GdlDockItemGrip *grip)
{
	GdlDockItemGripPrivate* priv = grip->priv = gdl_dock_item_grip_get_instance_private (grip);

	grip->priv->label = NULL;
	grip->priv->handle_shown = FALSE;

	/* create the close button */
	grip->priv->close_button = gtk_button_new ();
	gtk_button_set_has_frame (GTK_BUTTON (priv->close_button), false);
	gtk_widget_set_can_focus (grip->priv->close_button, FALSE);
	gtk_widget_set_parent (grip->priv->close_button, GTK_WIDGET (grip));
	{
		GdkCursor* cursor = gdk_cursor_new_from_name ("default", NULL);
		gtk_widget_set_cursor(GTK_WIDGET(grip->priv->close_button), cursor);
		g_object_unref(cursor);
	}

	GtkWidget* image = gdl_dock_item_button_image_new(GDL_DOCK_ITEM_BUTTON_IMAGE_CLOSE);
	gtk_button_set_child (GTK_BUTTON (grip->priv->close_button), image);

	g_signal_connect (G_OBJECT (grip->priv->close_button), "clicked", G_CALLBACK (gdl_dock_item_grip_close_clicked), grip);

	/* create the iconify button */
	priv->iconify_button = gtk_button_new ();
	gtk_button_set_has_frame (GTK_BUTTON (priv->iconify_button), false);
	gtk_widget_set_can_focus (priv->iconify_button, FALSE);
	gtk_widget_set_parent (priv->iconify_button, GTK_WIDGET (grip));
	{
		GdkCursor* cursor = gdk_cursor_new_from_name ("default", NULL);
		gtk_widget_set_cursor(GTK_WIDGET(grip->priv->iconify_button), cursor);
		g_object_unref(cursor);
	}

	image = gdl_dock_item_button_image_new(GDL_DOCK_ITEM_BUTTON_IMAGE_ICONIFY);
	gtk_button_set_child (GTK_BUTTON (grip->priv->iconify_button), image);

	g_signal_connect (G_OBJECT (grip->priv->iconify_button), "clicked", G_CALLBACK (gdl_dock_item_grip_iconify_clicked), grip);

	gtk_widget_set_tooltip_text (grip->priv->iconify_button, _("Iconify this dock"));
	gtk_widget_set_tooltip_text (grip->priv->close_button, _("Close this dock"));

	GtkStyleContext* style = gtk_widget_get_style_context (GTK_WIDGET (priv->close_button));
	gtk_style_context_add_provider (style, GTK_STYLE_PROVIDER(GDL_DOCK_ITEM_GRIP_GET_CLASS(grip)->priv->css), GTK_STYLE_PROVIDER_PRIORITY_USER);
	style = gtk_widget_get_style_context (GTK_WIDGET (priv->iconify_button));
	gtk_style_context_add_provider (style, GTK_STYLE_PROVIDER(GDL_DOCK_ITEM_GRIP_GET_CLASS(grip)->priv->css), GTK_STYLE_PROVIDER_PRIORITY_USER);

    GdkCursor* cursor = gdk_cursor_new_from_name ("grab", NULL);
	gtk_widget_set_cursor(GTK_WIDGET(grip), cursor);
	g_object_unref(cursor);
}

static void
gdl_dock_item_grip_realize (GtkWidget *widget)
{
    GTK_WIDGET_CLASS (gdl_dock_item_grip_parent_class)->realize (widget);
}

static void
gdl_dock_item_grip_unrealize (GtkWidget *widget)
{
    GTK_WIDGET_CLASS (gdl_dock_item_grip_parent_class)->unrealize (widget);
}

static void
gdl_dock_item_grip_map (GtkWidget *widget)
{
    GTK_WIDGET_CLASS (gdl_dock_item_grip_parent_class)->map (widget);
}

static void
gdl_dock_item_grip_unmap (GtkWidget *widget)
{
    GTK_WIDGET_CLASS (gdl_dock_item_grip_parent_class)->unmap (widget);
}

static void
gdl_dock_item_grip_get_preferred_width (GtkWidget *widget, gint *minimum, gint *natural)
{
    gint child_min, child_nat;

    g_return_if_fail (GDL_IS_DOCK_ITEM_GRIP (widget));

    GdlDockItemGrip* grip = GDL_DOCK_ITEM_GRIP (widget);

    *minimum = *natural = 0;

    if (grip->priv->handle_shown) {
        *minimum += DRAG_HANDLE_SIZE;
        *natural += DRAG_HANDLE_SIZE;
    }

    if (gtk_widget_get_visible (grip->priv->close_button)) {
		gtk_widget_measure (grip->priv->close_button, GTK_ORIENTATION_HORIZONTAL, -1, &child_min, &child_nat, NULL, NULL);
		*minimum += child_min;
		*natural += child_nat;
	}

	if (gtk_widget_get_visible (grip->priv->iconify_button)) {
		gtk_widget_measure (grip->priv->iconify_button, GTK_ORIENTATION_HORIZONTAL, -1, &child_min, &child_nat, NULL, NULL);
		*minimum += child_min;
		*natural += child_nat;
	}

								if (grip->priv->label) {
	gtk_widget_measure (grip->priv->label, GTK_ORIENTATION_HORIZONTAL, -1, &child_min, &child_nat, NULL, NULL);
	*minimum += child_min;
	*natural += child_nat;
								}
}

static void
gdl_dock_item_grip_get_preferred_height (GtkWidget *widget, gint *minimum, gint *natural)
{
    GtkRequisition min = {0,};
    GtkRequisition nat = {0,};

    g_return_if_fail (GDL_IS_DOCK_ITEM_GRIP (widget));

    GdlDockItemGrip* grip = GDL_DOCK_ITEM_GRIP (widget);

    *minimum = *natural = 0;

    gtk_widget_get_preferred_size (grip->priv->close_button, &min, &nat);
    *minimum = MAX (*minimum, min.height);
    *natural = MAX (*natural, nat.height);

    gtk_widget_get_preferred_size (grip->priv->iconify_button, &min, &nat);
    *minimum = MAX (*minimum, min.height);
    *natural = MAX (*natural, nat.height);
    								*minimum = 20;
									*natural = 20;

								if (grip->priv->label) {
    gtk_widget_get_preferred_size (grip->priv->label, &min, &nat);
    *minimum = MAX (*minimum, min.height);
    *natural = MAX (*natural, nat.height);
								}
}

static void
gdl_dock_item_grip_measure (GtkWidget* widget, GtkOrientation orientation, int for_size, int* min, int* natural, int* min_baseline, int* natural_baseline)
{
	if (orientation == GTK_ORIENTATION_HORIZONTAL) {
		gdl_dock_item_grip_get_preferred_width (widget, min, natural);
	} else {
		gdl_dock_item_grip_get_preferred_height (widget, min, natural);
	}
}

static void
gdl_dock_item_grip_size_allocate (GtkWidget *widget, int width, int height, int baseline)
{
    GtkRequisition iconify_requisition = { 0, };
    GtkAllocation child_allocation = {0, };

    g_return_if_fail (GDL_IS_DOCK_ITEM_GRIP (widget));

    GdlDockItemGrip* grip = GDL_DOCK_ITEM_GRIP (widget);

    GTK_WIDGET_CLASS (gdl_dock_item_grip_parent_class)->size_allocate (widget, width, height, baseline);

    GtkRequisition close_requisition = { 0, };
	gtk_widget_measure (grip->priv->close_button, GTK_ORIENTATION_HORIZONTAL, -1, NULL, &close_requisition.width, NULL, NULL);
	gtk_widget_measure (grip->priv->close_button, GTK_ORIENTATION_VERTICAL, -1, NULL, &close_requisition.height, NULL, NULL);
    gtk_widget_get_preferred_size (grip->priv->iconify_button, &iconify_requisition, NULL);

	/* Calculate the Minimum Width where buttons will fit */
	int min_width = close_requisition.width + iconify_requisition.width;
	if (grip->priv->handle_shown)
		min_width += DRAG_HANDLE_SIZE;
	const gboolean space_for_buttons = (width >= min_width);

    /* Set up the rolling child_allocation rectangle */
    if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
        child_allocation.x = 0;
    else
        child_allocation.x = width;

    /* Layout Close Button */
    if (gtk_widget_get_visible (grip->priv->close_button)) {

        if (space_for_buttons) {
            if (gtk_widget_get_direction (widget) != GTK_TEXT_DIR_RTL)
                child_allocation.x -= close_requisition.width;

            child_allocation.width = close_requisition.width;
        } else {
            child_allocation.width = 0;
        }
        child_allocation.height = close_requisition.height;

        gtk_widget_size_allocate (grip->priv->close_button, &child_allocation, -1);

        if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
            child_allocation.x += close_requisition.width;
    }

    /* Layout Iconify Button */
    if (gtk_widget_get_visible (grip->priv->iconify_button)) {

        if (space_for_buttons) {
            if (gtk_widget_get_direction (widget) != GTK_TEXT_DIR_RTL)
                child_allocation.x -= iconify_requisition.width;

            child_allocation.width = iconify_requisition.width;
        } else {
            child_allocation.width = 0;
        }
        child_allocation.height = iconify_requisition.height;

        gtk_widget_size_allocate (grip->priv->iconify_button, &child_allocation, -1);

        if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
            child_allocation.x += iconify_requisition.width;
    }

    /* Layout the Grip Handle*/
    if (gtk_widget_get_direction (widget) != GTK_TEXT_DIR_RTL) {
        child_allocation.width = child_allocation.x;
        child_allocation.x = 0;

        if (grip->priv->handle_shown) {
            child_allocation.x += DRAG_HANDLE_SIZE;
            child_allocation.width -= DRAG_HANDLE_SIZE;
        }

    } else {
        child_allocation.width = width - (child_allocation.x)/* - ALIGN_BORDER*/;

        if (grip->priv->handle_shown)
            child_allocation.width -= DRAG_HANDLE_SIZE;
    }

    if (child_allocation.width < 0)
      child_allocation.width = 0;

    child_allocation.y = 0;
    child_allocation.height = height;
    if (grip->priv->label) {
      gtk_widget_size_allocate (grip->priv->label, &child_allocation, 0);
    }
}

void
gdl_dock_item_grip_remove (GtkWidget *container, GtkWidget *widget)
{
    gdl_dock_item_grip_set_label (GDL_DOCK_ITEM_GRIP (container), NULL);
}

static void
gdl_dock_item_grip_class_init (GdlDockItemGripClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass* widget_class = GTK_WIDGET_CLASS (klass);

	object_class->set_property = gdl_dock_item_grip_set_property;
	object_class->dispose = gdl_dock_item_grip_dispose;

	widget_class->snapshot = gdl_dock_item_grip_snapshot;

	widget_class->realize = gdl_dock_item_grip_realize;
	widget_class->unrealize = gdl_dock_item_grip_unrealize;
	widget_class->map = gdl_dock_item_grip_map;
	widget_class->unmap = gdl_dock_item_grip_unmap;
	widget_class->measure = gdl_dock_item_grip_measure;
	widget_class->size_allocate = gdl_dock_item_grip_size_allocate;

	g_object_class_install_property (
		object_class,
		PROP_ITEM,
		g_param_spec_object ("item", _("Controlling dock item"), _("Dockitem which 'owns' this grip"), GDL_TYPE_DOCK_ITEM, G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY)
	);

	klass->priv = G_TYPE_CLASS_GET_PRIVATE (klass, GDL_TYPE_DOCK_ITEM_GRIP, GdlDockItemGripClassPrivate);

	klass->priv->css = gtk_css_provider_new ();
	static const gchar grip_style[] =
		"* {\n"
			"padding: 0px;\n"
			"border-radius: 0;\n"
			"min-height: 14px;\n"
		"}";
	gtk_css_provider_load_from_data (klass->priv->css, grip_style, -1);

	gtk_widget_class_set_css_name (GTK_WIDGET_CLASS(klass), "dock-grip");
}

/* ----- Public interface ----- */

/**
 * gdl_dock_item_grip_new:
 * @item: The dock item that will "own" this grip widget.
 *
 * Creates a new GDL dock item grip object.
 * Returns: The newly created dock item grip widget.
 **/
GtkWidget *
gdl_dock_item_grip_new (GdlDockItem *item)
{
    GdlDockItemGrip *grip = g_object_new (GDL_TYPE_DOCK_ITEM_GRIP, "item", item, NULL);

    return GTK_WIDGET (grip);
}

/**
 * gdl_dock_item_grip_set_label:
 * @grip: The grip that will get it's label widget set.
 * @label: The widget that will become the label.
 *
 * Replaces the current label widget with another widget.
 **/
void
gdl_dock_item_grip_set_label (GdlDockItemGrip *grip, GtkWidget *label)
{
    g_return_if_fail (grip);

    if (grip->priv->label) {
        gtk_widget_unparent(grip->priv->label);
        g_clear_pointer(&grip->priv->label, g_object_unref);
    }

    if (label) {
        g_object_ref (label);
        gtk_widget_set_parent (label, GTK_WIDGET (grip));
        gtk_widget_set_visible (label, true);
        grip->priv->label = label;
    }
}
/**
 * gdl_dock_item_grip_hide_handle:
 * @grip: The dock item grip to hide the handle of.
 *
 * This function hides the dock item's grip widget handle hatching.
 **/
void
gdl_dock_item_grip_hide_handle (GdlDockItemGrip *grip)
{
    g_return_if_fail (grip);

    grip->priv->handle_shown = FALSE;
    gtk_widget_set_visible (GTK_WIDGET (grip), false);
}

/**
 * gdl_dock_item_grip_show_handle:
 * @grip: The dock item grip to show the handle of.
 *
 * This function shows the dock item's grip widget handle hatching.
 **/
void
gdl_dock_item_grip_show_handle (GdlDockItemGrip *grip)
{
    g_return_if_fail (grip != NULL);
    if (!grip->priv->handle_shown) {
        grip->priv->handle_shown = TRUE;
        gtk_widget_set_visible (GTK_WIDGET (grip), true);
    }
}

/**
 * gdl_dock_item_grip_set_cursor:
 * @grip: The dock item grip
 * @in_drag: %TRUE if a drag operation is started
 *
 * Change the cursor when a drag operation is started.
 *
 * Since: 3.6
 **/
void
gdl_dock_item_grip_set_cursor (GdlDockItemGrip *grip, gboolean in_drag)
{
    /* We check the window since if the item could have been redocked and have
     * been unrealized, maybe it's not realized again yet */
}

/**
 * gdl_dock_item_grip_has_event:
 * @grip: A #GdlDockItemGrip widget
 * @event: A #GdkEvent
 *
 * Return %TRUE if the grip window has reveived the event.
 *
 * Returns: %TRUE if the grip has received the event
 */
gboolean
gdl_dock_item_grip_has_event (GdlDockItemGrip *grip, GdkEvent *event)
{
	return FALSE;
}
