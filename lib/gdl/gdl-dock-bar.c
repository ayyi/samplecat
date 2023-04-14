/*
 * This file is part of the GNOME Devtools Libraries.
 *
 * Copyright (C) 2003 Jeroen Zwartepoorte <jeroen@xs4all.nl>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <string.h>

#include "il8n.h"
#include "gdl-dock.h"
#include "gdl-dock-master.h"
#include "gdl-dock-bar.h"
#include "libgdltypebuiltins.h"

/**
 * SECTION:gdl-dock-bar
 * @title: GdlDockBar
 * @short_description: A docking bar
 * @stability: Unstable
 * @see_also: #GdlDockMaster
 *
 * This docking bar is a widget containing a button for each iconified
 * #GdlDockItem widget. The widget can be re-opened by clicking on it.
 *
 * A dock bar is associated with one #GdlDockMaster and will get all iconified
 * widgets of this master. This can includes widgets from several #GdlDock
 * objects.
 */


enum {
    PROP_0,
    PROP_MASTER,
    PROP_DOCKBAR_STYLE
};

/* ----- Private prototypes ----- */

static void  gdl_dock_bar_class_init      (GdlDockBarClass *klass);

static void  gdl_dock_bar_get_property    (GObject         *object,
                                           guint            prop_id,
                                           GValue          *value,
                                           GParamSpec      *pspec);
static void  gdl_dock_bar_set_property    (GObject         *object,
                                           guint            prop_id,
                                           const GValue    *value,
                                           GParamSpec      *pspec);

static void  gdl_dock_bar_dispose         (GObject         *object);

static void  gdl_dock_bar_set_master      (GdlDockBar      *dockbar,
                                           GObject         *master);
static void gdl_dock_bar_remove_item      (GdlDockBar      *dockbar,
                                           GdlDockItem     *item);

/* ----- Class variables and definitions ----- */

struct _GdlDockBarPrivate {
    GdlDockMaster   *master;
    GSList          *items;
    GdlDockBarStyle  dockbar_style;

    glong layout_changed_id;
};

/* ----- Private functions ----- */

G_DEFINE_TYPE_WITH_CODE (GdlDockBar, gdl_dock_bar, GTK_TYPE_BOX, G_ADD_PRIVATE (GdlDockBar))

static void update_dock_items (GdlDockBar *dockbar, gboolean full_update);

void
gdl_dock_bar_class_init (GdlDockBarClass *klass)
{
    GObjectClass* object_class = G_OBJECT_CLASS (klass);

    object_class->get_property = gdl_dock_bar_get_property;
    object_class->set_property = gdl_dock_bar_set_property;
    object_class->dispose = gdl_dock_bar_dispose;

    /**
     * GdlDockBar:master:
     *
     * The #GdlDockMaster object attached to the dockbar.
     */
    g_object_class_install_property (
        object_class, PROP_MASTER,
        g_param_spec_object ("master", _("Master"),
                             _("GdlDockMaster object which the dockbar widget "
                               "is attached to"),
                             G_TYPE_OBJECT,
                             G_PARAM_READWRITE));

    /**
     * GdlDockBar:style:
     *
     * The style of the buttons in the dockbar.
     */
    g_object_class_install_property (
        object_class, PROP_DOCKBAR_STYLE,
        g_param_spec_enum ("dockbar-style", _("Dockbar style"),
                           _("Dockbar style to show items on it"),
                           GDL_TYPE_DOCK_BAR_STYLE,
                           GDL_DOCK_BAR_BOTH,
                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
}

static void
gdl_dock_bar_init (GdlDockBar* dockbar)
{
	dockbar->priv = gdl_dock_bar_get_instance_private (dockbar);

    dockbar->priv->master = NULL;
    dockbar->priv->items = NULL;
    dockbar->priv->dockbar_style = GDL_DOCK_BAR_BOTH;
    gtk_orientable_set_orientation (GTK_ORIENTABLE (dockbar), GTK_ORIENTATION_VERTICAL);
}

static void
gdl_dock_bar_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    GdlDockBar *dockbar = GDL_DOCK_BAR (object);

    switch (prop_id) {
        case PROP_MASTER:
            g_value_set_object (value, dockbar->priv->master);
            break;
        case PROP_DOCKBAR_STYLE:
            g_value_set_enum (value, dockbar->priv->dockbar_style);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    };
}

static void
gdl_dock_bar_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    GdlDockBar *dockbar = GDL_DOCK_BAR (object);

    switch (prop_id) {
        case PROP_MASTER:
            gdl_dock_bar_set_master (dockbar, g_value_get_object (value));
            break;
        case PROP_DOCKBAR_STYLE:
            dockbar->priv->dockbar_style = g_value_get_enum (value);
            update_dock_items (dockbar, TRUE);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    };
}

static void
on_dock_item_foreach_disconnect (GdlDockItem *item, GdlDockBar *dock_bar)
{
    g_signal_handlers_disconnect_by_func (item, gdl_dock_bar_remove_item, dock_bar);
}

static void
gdl_dock_bar_dispose (GObject *object)
{
    GdlDockBar *dockbar = GDL_DOCK_BAR (object);
    GdlDockBarPrivate *priv = dockbar->priv;

    if (priv->items) {
        g_slist_foreach (priv->items, (GFunc) on_dock_item_foreach_disconnect, object);
        g_slist_free (priv->items);
        priv->items = NULL;
    }

    if (priv->master) {
        gdl_dock_bar_set_master (dockbar, NULL);
    }

   G_OBJECT_CLASS (gdl_dock_bar_parent_class)->dispose (object);
}

static void
gdl_dock_bar_remove_item (GdlDockBar *dockbar, GdlDockItem *item)
{
    g_return_if_fail (GDL_IS_DOCK_BAR (dockbar));
    g_return_if_fail (GDL_IS_DOCK_ITEM (item));

    GdlDockBarPrivate* priv = dockbar->priv;

    if (g_slist_index (priv->items, item) == -1) {
        g_warning ("Item has not been added to the dockbar");
        return;
    }

    priv->items = g_slist_remove (priv->items, item);

    GtkWidget* button = g_object_get_data (G_OBJECT (item), "GdlDockBarButton");
    g_assert (button != NULL);
    gtk_box_remove (GTK_BOX (dockbar), button);
    g_object_set_data (G_OBJECT (item), "GdlDockBarButton", NULL);
    g_signal_handlers_disconnect_by_func (item, G_CALLBACK (gdl_dock_bar_remove_item), dockbar);
}

static void
gdl_dock_bar_item_clicked (GtkWidget *button, GdlDockItem *item)
{
    g_return_if_fail (item != NULL);

    GdlDockBar* dockbar = g_object_get_data (G_OBJECT (item), "GdlDockBar");
    g_assert (dockbar != NULL);
    g_object_set_data (G_OBJECT (item), "GdlDockBar", NULL);

    gdl_dock_item_show_item (item);
}

static void
gdl_dock_bar_add_item (GdlDockBar *dockbar, GdlDockItem *item)
{
    gchar *stock_id;
    gchar *name;
    GdkPixbuf *pixbuf_icon;
    GtkWidget *image, *label;

    g_return_if_fail (GDL_IS_DOCK_BAR (dockbar));
    g_return_if_fail (GDL_IS_DOCK_ITEM (item));

    GdlDockBarPrivate* priv = dockbar->priv;

    if (g_slist_index (priv->items, item) != -1) {
        g_warning ("Item has already been added to the dockbar");
        return;
    }

    priv->items = g_slist_append (priv->items, item);

    /* Create a button for the item. */
    GtkWidget* button = gtk_button_new ();
#ifdef GTK4_TODO
    gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
#endif

    GtkWidget* box = gtk_box_new (gtk_orientable_get_orientation (GTK_ORIENTABLE (dockbar)), 0);

    g_object_get (item, "stock-id", &stock_id, "pixbuf-icon", &pixbuf_icon, "long-name", &name, NULL);

    if (dockbar->priv->dockbar_style == GDL_DOCK_BAR_TEXT || dockbar->priv->dockbar_style == GDL_DOCK_BAR_BOTH) {
        label = gtk_label_new (name);
#ifdef GTK4_TODO
        if (gtk_orientable_get_orientation (GTK_ORIENTABLE (dockbar)) == GTK_ORIENTATION_VERTICAL)
            gtk_label_set_angle (GTK_LABEL (label), 90);
#endif
        gtk_box_append (GTK_BOX (box), label);
    }

    /* FIXME: For now AUTO behaves same as BOTH */

    if (dockbar->priv->dockbar_style == GDL_DOCK_BAR_ICONS ||
        dockbar->priv->dockbar_style == GDL_DOCK_BAR_BOTH ||
        dockbar->priv->dockbar_style == GDL_DOCK_BAR_AUTO
	){
        if (stock_id) {
#ifdef GTK4_TODO
            image = gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_SMALL_TOOLBAR);
#else
			image = NULL;
#endif
            g_free (stock_id);
        } else if (pixbuf_icon) {
            image = gtk_image_new_from_pixbuf (pixbuf_icon);
        } else {
#ifdef GTK4_TODO
            image = gtk_image_new_from_stock (GTK_STOCK_NEW, GTK_ICON_SIZE_SMALL_TOOLBAR);
#else
			image = NULL;
#endif
        }
        gtk_box_append (GTK_BOX (box), image);
    }

    gtk_button_set_child (GTK_BUTTON (button), box);
    gtk_box_append (GTK_BOX (dockbar), button);

    gtk_widget_set_tooltip_text (button, name);
    g_free (name);

    g_object_set_data (G_OBJECT (item), "GdlDockBar", dockbar);
    g_object_set_data (G_OBJECT (item), "GdlDockBarButton", button);
    g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (gdl_dock_bar_item_clicked), item);

    /* Set up destroy notify */
    g_signal_connect_swapped (item, "destroy", G_CALLBACK (gdl_dock_bar_remove_item), dockbar);
}

static void
build_list (GdlDockObject *object, GList **list)
{
    /* add only items, not toplevels */
    if (GDL_IS_DOCK_ITEM (object))
        *list = g_list_prepend (*list, object);
}

static void
update_dock_items (GdlDockBar *dockbar, gboolean full_update)
{
    g_return_if_fail (dockbar != NULL);

    if (!dockbar->priv->master)
        return;

    GdlDockMaster* master = dockbar->priv->master;

    /* build items list */
    GList* items = NULL;
    gdl_dock_master_foreach (master, (GFunc) build_list, &items);

    if (!full_update) {
        for (GList* l = items; l != NULL; l = l->next) {
            GdlDockItem *item = GDL_DOCK_ITEM (l->data);

            if (g_slist_index (dockbar->priv->items, item) != -1 && !gdl_dock_item_is_iconified (item))
                gdl_dock_bar_remove_item (dockbar, item);
            else if (g_slist_index (dockbar->priv->items, item) == -1 && gdl_dock_item_is_iconified (item) && !gdl_dock_item_is_placeholder (item))
                gdl_dock_bar_add_item (dockbar, item);
        }
    } else {
        for (GList* l = items; l != NULL; l = l->next) {
            GdlDockItem *item = GDL_DOCK_ITEM (l->data);

            if (g_slist_index (dockbar->priv->items, item) != -1)
                gdl_dock_bar_remove_item (dockbar, item);
            if (gdl_dock_item_is_iconified (item) && !gdl_dock_item_is_placeholder (item))
                gdl_dock_bar_add_item (dockbar, item);
        }
    }
    g_list_free (items);
}

static void
gdl_dock_bar_layout_changed_cb (GdlDockMaster *master, GdlDockBar *dockbar)
{
    update_dock_items (dockbar, FALSE);
}

static void
gdl_dock_bar_set_master (GdlDockBar *dockbar, GObject *master)
{
    g_return_if_fail (dockbar != NULL);
    g_return_if_fail (master == NULL || GDL_IS_DOCK_MASTER (master) || GDL_IS_DOCK_OBJECT (master));

    if (dockbar->priv->master) {
        g_signal_handler_disconnect (dockbar->priv->master, dockbar->priv->layout_changed_id);
        g_object_unref (dockbar->priv->master);
    }

    if (master != NULL) {
        /* Accept a GdlDockObject instead of a GdlDockMaster */
        if (GDL_IS_DOCK_OBJECT (master)) {
            master = gdl_dock_object_get_master (GDL_DOCK_OBJECT (master));
        }
        dockbar->priv->master = (GdlDockMaster*) g_object_ref (master);
        dockbar->priv->layout_changed_id = g_signal_connect (dockbar->priv->master, "layout-changed", (GCallback) gdl_dock_bar_layout_changed_cb, dockbar);

    } else {
        dockbar->priv->master = NULL;
    }

    update_dock_items (dockbar, FALSE);
}


/**
 * gdl_dock_bar_new:
 * @master: (allow-none): The associated #GdlDockMaster or #GdlDockObject object
 *
 * Creates a new GDL dock bar. If a #GdlDockObject is used, the dock bar will
 * be associated with the master of this object.
 *
 * Returns: The newly created dock bar.
 */
GtkWidget *
gdl_dock_bar_new (GObject* master)
{
    g_return_val_if_fail (master == NULL || GDL_IS_DOCK_MASTER (master) || GDL_IS_DOCK_OBJECT (master), NULL);

    return g_object_new (GDL_TYPE_DOCK_BAR, "master", master, NULL);
}

/**
 * gdl_dock_bar_set_style:
 * @dockbar: a #GdlDockBar
 * @style: the new style
 *
 * Set the style of the @dockbar.
 */
void
gdl_dock_bar_set_style (GdlDockBar* dockbar, GdlDockBarStyle style)
{
    g_return_if_fail (GDL_IS_DOCK_BAR (dockbar));

    g_object_set(G_OBJECT(dockbar), "dockbar-style", style, NULL);
}

/**
 * gdl_dock_bar_get_style:
 * @dockbar: a #GdlDockBar
 *
 * Retrieves the style of the @dockbar.
 *
 * Returns: the style of the @docbar
 */
GdlDockBarStyle
gdl_dock_bar_get_style (GdlDockBar* dockbar)
{
    GdlDockBarStyle style;

    g_return_val_if_fail (GDL_IS_DOCK_BAR (dockbar), 0);

    g_object_get(G_OBJECT(dockbar), "dockbar-style", &style, NULL);

    return style;
}
