/*
 * This file is part of the GNOME Devtools Libraries.
 *
 * Copyright (C) 2002 Gustavo Giráldez <gustavo.giraldez@gmx.net>
 *               2007 Naba Kumar  <naba@gnome.org>
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

#include "il8n.h"
#include <stdlib.h>
#include <string.h>
#include "debug.h"

#include "gdl-dock.h"
#include "gdl-dock-master.h"
#include "gdl-dock-paned.h"
#include "gdl-dock-notebook.h"
//#include "gdl-preview-window.h"

#include "libgdlmarshal.h"

/**
 * SECTION:gdl-dock
 * @title: GdlDock
 * @short_description: A docking area widget.
 * @see_also: #GdlDockItem, #GdlDockMaster
 * @stability: Unstable
 *
 * A #GdlDock is the toplevel widget which in turn hold a tree of #GdlDockItem
 * widgets.
 *
 * Several dock widgets can exchange widgets if they share the same master.
 */

/* ----- Private prototypes ----- */

static void  gdl_dock_class_init      (GdlDockClass *class);

static GObject *gdl_dock_constructor  (GType                  type,
                                       guint                  n_construct_properties,
                                       GObjectConstructParam *construct_param);
static void  gdl_dock_set_property    (GObject      *object,
                                       guint         prop_id,
                                       const GValue *value,
                                       GParamSpec   *pspec);
static void  gdl_dock_get_property    (GObject      *object,
                                       guint         prop_id,
                                       GValue       *value,
                                       GParamSpec   *pspec);
static void  gdl_dock_notify_cb       (GObject      *object,
                                       GParamSpec   *pspec,
                                       gpointer      user_data);

static void  gdl_dock_set_title       (GdlDock      *dock);

static void  gdl_dock_dispose         (GObject      *object);

static void gdl_dock_measure          (GtkWidget* widget, GtkOrientation orientation, int for_size, int* min, int* natural, int* min_baseline, int* natural_baseline);

static void  gdl_dock_size_allocate   (GtkWidget      *widget, int, int, int);
static void  gdl_dock_map             (GtkWidget      *widget);
static void  gdl_dock_unmap           (GtkWidget      *widget);
static void  gdl_dock_show            (GtkWidget      *widget);
static void  gdl_dock_hide            (GtkWidget      *widget);

static void     gdl_dock_detach       (GdlDockObject    *object,
                                       gboolean          recursive);
static void     gdl_dock_reduce       (GdlDockObject    *object);
static gboolean gdl_dock_dock_request (GdlDockObject    *object,
                                       gint              x,
                                       gint              y,
                                       GdlDockRequest   *request);
static void     gdl_dock_dock         (GdlDockObject    *object,
                                       GdlDockObject    *requestor,
                                       GdlDockPlacement  position,
                                       GValue           *other_data);
static gboolean gdl_dock_reorder      (GdlDockObject    *object,
                                       GdlDockObject    *requestor,
                                       GdlDockPlacement  new_position,
                                       GValue           *other_data);

#ifdef GTK4_TODO
static gboolean gdl_dock_floating_window_delete_event_cb (GtkWidget *widget);
#endif

static gboolean gdl_dock_child_placement  (GdlDockObject    *object,
                                           GdlDockObject    *child,
                                           GdlDockPlacement *placement);

static void     gdl_dock_present          (GdlDockObject    *object,
                                           GdlDockObject    *child);


/* ----- Class variables and definitions ----- */

struct _GdlDockPrivate
{
    GdlDockObject   *root;

    /* for floating docks */
    gboolean            floating;
    GtkWidget          *window;
    gboolean            auto_title;

    gint                float_x;
    gint                float_y;
    gint                width;
    gint                height;

    GtkWidget          *area_window;

    gboolean            skip_taskbar;
};

enum {
    LAYOUT_CHANGED,
    LAST_SIGNAL
};

enum {
    PROP_0,
    PROP_FLOATING,
    PROP_DEFAULT_TITLE,
    PROP_WIDTH,
    PROP_HEIGHT,
    PROP_FLOAT_X,
    PROP_FLOAT_Y,
    PROP_SKIP_TASKBAR
};

static guint dock_signals [LAST_SIGNAL] = { 0 };

#define SPLIT_RATIO  0.3


/* ----- Private functions ----- */

G_DEFINE_TYPE_WITH_CODE (GdlDock, gdl_dock, GDL_TYPE_DOCK_OBJECT, G_ADD_PRIVATE (GdlDock))

static void
gdl_dock_class_init (GdlDockClass *klass)
{
	GObjectClass* g_object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass* widget_class = GTK_WIDGET_CLASS (klass);
	GdlDockObjectClass* object_class = GDL_DOCK_OBJECT_CLASS (klass);

	g_object_class->constructor = gdl_dock_constructor;
	g_object_class->set_property = gdl_dock_set_property;
	g_object_class->get_property = gdl_dock_get_property;
	g_object_class->dispose = gdl_dock_dispose;

	/* properties */

    g_object_class_install_property (
        g_object_class, PROP_FLOATING,
        g_param_spec_boolean ("floating", _("Floating"),
                              _("Whether the dock is floating in its own window"),
                              FALSE,
                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                              GDL_DOCK_PARAM_EXPORT));

    g_object_class_install_property (
        g_object_class, PROP_DEFAULT_TITLE,
        g_param_spec_string ("default-title", _("Default title"),
                             _("Default title for the newly created floating docks"),
                             NULL,
                             G_PARAM_READWRITE));

    g_object_class_install_property (
        g_object_class, PROP_WIDTH,
        g_param_spec_int ("width", _("Width"),
                          _("Width for the dock when it's of floating type"),
                          -1, G_MAXINT, -1,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                          GDL_DOCK_PARAM_EXPORT));

    g_object_class_install_property (
        g_object_class, PROP_HEIGHT,
        g_param_spec_int ("height", _("Height"),
                          _("Height for the dock when it's of floating type"),
                          -1, G_MAXINT, -1,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                          GDL_DOCK_PARAM_EXPORT));

    g_object_class_install_property (
        g_object_class, PROP_FLOAT_X,
        g_param_spec_int ("floatx", _("Float X"),
                          _("X coordinate for a floating dock"),
                          G_MININT, G_MAXINT, 0,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                          GDL_DOCK_PARAM_EXPORT));

    g_object_class_install_property (
        g_object_class, PROP_FLOAT_Y,
        g_param_spec_int ("floaty", _("Float Y"),
                          _("Y coordinate for a floating dock"),
                          G_MININT, G_MAXINT, 0,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                          GDL_DOCK_PARAM_EXPORT));

    /**
     * GdlDock:skip-taskbar:
     *
     * Whether or not to prevent a floating dock window from appearing in the
     * taskbar. Note that this only affects floating windows that are created
     * after this flag is set; existing windows are not affected.  Usually,
     * this property is used when you create the dock.
     *
     * Since: 3.6
     */
    g_object_class_install_property (
        g_object_class, PROP_SKIP_TASKBAR,
        g_param_spec_boolean ("skip-taskbar",
                              _("Skip taskbar"),
                              _("Whether or not to prevent a floating dock window from appearing in the taskbar"),
                              TRUE,
                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                              GDL_DOCK_PARAM_EXPORT));

	widget_class->measure = gdl_dock_measure;
    widget_class->size_allocate = gdl_dock_size_allocate;
    widget_class->map = gdl_dock_map;
    widget_class->unmap = gdl_dock_unmap;
    widget_class->show = gdl_dock_show;
    widget_class->hide = gdl_dock_hide;

    gdl_dock_object_class_set_is_compound (object_class, TRUE);
    object_class->detach = gdl_dock_detach;
    object_class->reduce = gdl_dock_reduce;
    object_class->dock_request = gdl_dock_dock_request;
    object_class->dock = gdl_dock_dock;
    object_class->reorder = gdl_dock_reorder;
    object_class->child_placement = gdl_dock_child_placement;
    object_class->present = gdl_dock_present;

    /* signals */

    /**
     * GdlDock::layout-changed:
     *
     * Signals that the layout has changed, one or more widgets have been moved,
     * added or removed.
     */
    dock_signals [LAYOUT_CHANGED] =
        g_signal_new ("layout-changed",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (GdlDockClass, layout_changed),
                      NULL, /* accumulator */
                      NULL, /* accu_data */
                      gdl_marshal_VOID__VOID,
                      G_TYPE_NONE, /* return type */
                      0);

    klass->layout_changed = NULL;
}

static void
gdl_dock_init (GdlDock *dock)
{
	dock->priv = gdl_dock_get_instance_private(dock);

    dock->priv->width = -1;
    dock->priv->height = -1;
}

#ifdef GTK4_TODO
static gboolean
gdl_dock_floating_configure_event_cb (GtkWidget *widget, GdkEventConfigure *event, gpointer user_data)
{
    g_return_val_if_fail (user_data != NULL && GDL_IS_DOCK (user_data), TRUE);

    GdlDock* dock = GDL_DOCK (user_data);
    dock->priv->float_x = event->x;
    dock->priv->float_y = event->y;
    dock->priv->width = event->width;
    dock->priv->height = event->height;

    return FALSE;
}
#endif

static GObject *
gdl_dock_constructor (GType type, guint n_construct_properties, GObjectConstructParam *construct_param)
{
    GObject* g_object = G_OBJECT_CLASS (gdl_dock_parent_class)-> constructor (type, n_construct_properties, construct_param);
    if (g_object) {
        GdlDock *dock = GDL_DOCK (g_object);

        /* create a master for the dock if none was provided in the construction */
        GdlDockMaster* master = GDL_DOCK_MASTER (gdl_dock_object_get_master (GDL_DOCK_OBJECT (dock)));
        if (!master) {
            gdl_dock_object_set_manual (GDL_DOCK_OBJECT (dock));
            master = g_object_new (GDL_TYPE_DOCK_MASTER, NULL);
            /* the controller owns the master ref */
            gdl_dock_object_bind (GDL_DOCK_OBJECT (dock), G_OBJECT (master));
        }

        if (dock->priv->floating) {
            /* create floating window for this dock */
            dock->priv->window = gtk_window_new ();
            g_object_set_data (G_OBJECT (dock->priv->window), "dock", dock);

            /* set position and default size */
#ifdef GTK4_TODO
            gtk_window_set_position (GTK_WINDOW (dock->priv->window), GTK_WIN_POS_MOUSE);
#endif
            gtk_window_set_default_size (GTK_WINDOW (dock->priv->window), dock->priv->width, dock->priv->height);
#ifdef GTK4_TODO
            gtk_window_set_type_hint (GTK_WINDOW (dock->priv->window), GDK_WINDOW_TYPE_HINT_TOOLBAR);
#endif

            gdl_dock_set_skip_taskbar (dock, dock->priv->skip_taskbar);

#ifdef GTK4_TODO
            gtk_window_move (GTK_WINDOW (dock->priv->window), dock->priv->float_x, dock->priv->float_y);
#endif

            /* connect to the configure event so we can track down window geometry */
#ifdef GTK4_TODO
            g_signal_connect (dock->priv->window, "configure_event", (GCallback) gdl_dock_floating_configure_event_cb, dock);
#endif

            /* set the title and connect to the long_name notify queue
             so we can reset the title when this prop changes */
            gdl_dock_set_title (dock);
            g_signal_connect (dock, "notify::long-name", (GCallback) gdl_dock_notify_cb, NULL);

            gtk_window_set_child (GTK_WINDOW (dock->priv->window), GTK_WIDGET (dock));

#ifdef GTK4_TODO
            g_signal_connect (dock->priv->window, "delete_event", G_CALLBACK (gdl_dock_floating_window_delete_event_cb), NULL);
#endif
		}

		gtk_widget_set_hexpand (GTK_WIDGET (dock), true);
		gtk_widget_set_vexpand (GTK_WIDGET (dock), true);
	}

	return g_object;
}

static void
gdl_dock_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    GdlDock *dock = GDL_DOCK (object);

    switch (prop_id) {
        case PROP_FLOATING:
            dock->priv->floating = g_value_get_boolean (value);
            break;
        case PROP_DEFAULT_TITLE:
            if (gdl_dock_object_get_master (GDL_DOCK_OBJECT (object)) != NULL)
                g_object_set (gdl_dock_object_get_master (GDL_DOCK_OBJECT (object)), "default-title", g_value_get_string (value), NULL);
            break;
        case PROP_WIDTH:
            dock->priv->width = g_value_get_int (value);
            break;
        case PROP_HEIGHT:
            dock->priv->height = g_value_get_int (value);
            break;
        case PROP_FLOAT_X:
            dock->priv->float_x = g_value_get_int (value);
            break;
        case PROP_FLOAT_Y:
            dock->priv->float_y = g_value_get_int (value);
            break;
	case PROP_SKIP_TASKBAR:
            gdl_dock_set_skip_taskbar (dock, g_value_get_boolean (value));
	    break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }

    switch (prop_id) {
        case PROP_WIDTH:
        case PROP_HEIGHT:
        case PROP_FLOAT_X:
        case PROP_FLOAT_Y:
            if (dock->priv->floating && dock->priv->window) {
#ifdef GTK4_TODO
                gtk_window_resize (GTK_WINDOW (dock->priv->window), dock->priv->width, dock->priv->height);
#endif
            }
            break;
    }
}

static void
gdl_dock_get_property  (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    GdlDock *dock = GDL_DOCK (object);

    switch (prop_id) {
        case PROP_FLOATING:
            g_value_set_boolean (value, dock->priv->floating);
            break;
        case PROP_DEFAULT_TITLE:
            if (gdl_dock_object_get_master (GDL_DOCK_OBJECT (object)) != NULL) {
                gchar *default_title;
                g_object_get (gdl_dock_object_get_master (GDL_DOCK_OBJECT (object)), "default-title", &default_title, NULL);

                g_value_take_string (value, default_title);
            }
            else
                g_value_set_string (value, NULL);
            break;
        case PROP_WIDTH:
            g_value_set_int (value, dock->priv->width);
            break;
        case PROP_HEIGHT:
            g_value_set_int (value, dock->priv->height);
            break;
        case PROP_FLOAT_X:
            g_value_set_int (value, dock->priv->float_x);
            break;
        case PROP_FLOAT_Y:
            g_value_set_int (value, dock->priv->float_y);
            break;
        case PROP_SKIP_TASKBAR:
            g_value_set_boolean (value, dock->priv->skip_taskbar);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

/**
 * gdl_dock_set_skip_taskbar:
 * @dock: The dock whose property should be set.
 * @skip: %TRUE if floating docks should be prevented from appearing in the taskbar
 *
 * Sets whether or not a floating dock window should be prevented from
 * appearing in the system taskbar.
 *
 * Since: 3.6
 */
void
gdl_dock_set_skip_taskbar (GdlDock *dock, gboolean skip)
{
    dock->priv->skip_taskbar = (skip != FALSE);

    if (dock->priv->window && dock->priv->window) {
#ifdef GTK4_TODO
        gtk_window_set_skip_taskbar_hint (GTK_WINDOW (dock->priv->window), dock->priv->skip_taskbar);
#endif
    }
}

static void
gdl_dock_set_title (GdlDock *dock)
{
    GdlDockObject *object = GDL_DOCK_OBJECT (dock);
    gchar         *title = NULL;

    if (!dock->priv->window)
        return;

    if (!dock->priv->auto_title && gdl_dock_object_get_long_name (object)) {
        title = g_strdup (gdl_dock_object_get_long_name (object));
    }
    else if (gdl_dock_object_get_master (object) != NULL) {
        g_object_get (G_OBJECT (gdl_dock_object_get_master (object)), "default-title", &title, NULL);
    }

    if (!title && dock->priv->root) {
        g_object_get (dock->priv->root, "long-name", &title, NULL);
    }

    if (!title) {
        /* set a default title in the long_name */
        dock->priv->auto_title = TRUE;
        title = gdl_dock_master_get_dock_name (GDL_DOCK_MASTER (gdl_dock_object_get_master (object)));
    }

    gtk_window_set_title (GTK_WINDOW (dock->priv->window), title);

    g_free (title);
}

static void
gdl_dock_notify_cb (GObject *object, GParamSpec *pspec, gpointer user_data)
{
    GdlDock *dock;
    gchar* long_name;

    g_return_if_fail (object != NULL || GDL_IS_DOCK (object));

    g_object_get (object, "long-name", &long_name, NULL);

    if (long_name) {
        dock = GDL_DOCK (object);
        dock->priv->auto_title = FALSE;
        gdl_dock_set_title (dock);
    }
    g_free (long_name);
}

static void
gdl_dock_dispose (GObject *object)
{
    GdlDockPrivate *priv = GDL_DOCK (object)->priv;

    if (priv->window) {
        gtk_window_destroy (GTK_WINDOW (priv->window));
        priv->floating = FALSE;
        priv->window = NULL;
    }

    if (priv->area_window) {
		GtkWindow** area_window = (GtkWindow**)&priv->area_window;
        g_clear_pointer(area_window, gtk_window_destroy);
    }

    G_OBJECT_CLASS (gdl_dock_parent_class)->dispose (object);
}

static void
gdl_dock_get_size (GtkWidget *widget, GtkOrientation orientation, gint *minimum, gint *natural)
{
    g_return_if_fail (widget != NULL);
    g_return_if_fail (GDL_IS_DOCK (widget));

    GdlDock *dock = GDL_DOCK (widget);

    *minimum = *natural = 0;

    /* make request to root */
    if (dock->priv->root && gtk_widget_get_visible (GTK_WIDGET (dock->priv->root))) {
		gtk_widget_measure (GTK_WIDGET (dock->priv->root), orientation, -1, minimum, natural, NULL, NULL);
    }
}

static void
gdl_dock_get_preferred_width (GtkWidget *widget, gint *minimum, gint *natural)
{
    gdl_dock_get_size (widget, GTK_ORIENTATION_HORIZONTAL, minimum, natural);
}

static void
gdl_dock_get_preferred_height (GtkWidget *widget, gint *minimum, gint *natural)
{
    gdl_dock_get_size (widget, GTK_ORIENTATION_VERTICAL, minimum, natural);
}

static void
gdl_dock_measure (GtkWidget* widget, GtkOrientation orientation, int for_size, int* min, int* natural, int* min_baseline, int* natural_baseline)
{
	if (orientation == GTK_ORIENTATION_HORIZONTAL)
		gdl_dock_get_preferred_width (widget, min, natural);
	else
		gdl_dock_get_preferred_height (widget, min, natural);
}

static void
gdl_dock_size_allocate (GtkWidget *widget, int width, int height, int baseline)
{
    g_return_if_fail (widget);
    g_return_if_fail (GDL_IS_DOCK (widget));

    GdlDock *dock = GDL_DOCK (widget);

    //gtk_widget_allocate (widget, width, height, baseline, NULL);

    if (dock->priv->root && gtk_widget_get_visible (GTK_WIDGET (dock->priv->root)))
        gtk_widget_allocate (GTK_WIDGET (dock->priv->root), width, height, baseline, NULL);
}

static void
gdl_dock_map (GtkWidget *widget)
{
    g_return_if_fail (widget);
    g_return_if_fail (GDL_IS_DOCK (widget));

    GdlDock* dock = GDL_DOCK (widget);

    GTK_WIDGET_CLASS (gdl_dock_parent_class)->map (widget);

    if (dock->priv->root) {
        GtkWidget* child = GTK_WIDGET (dock->priv->root);
        if (gtk_widget_get_visible (child) && !gtk_widget_get_mapped (child))
            gtk_widget_map (child);
    }
}

static void
gdl_dock_unmap (GtkWidget *widget)
{
    g_return_if_fail (widget != NULL);
    g_return_if_fail (GDL_IS_DOCK (widget));

    GdlDock* dock = GDL_DOCK (widget);

    GTK_WIDGET_CLASS (gdl_dock_parent_class)->unmap (widget);

    if (dock->priv->root) {
        GtkWidget* child = GTK_WIDGET (dock->priv->root);
        if (gtk_widget_get_visible (child) && gtk_widget_get_mapped (child))
            gtk_widget_unmap (child);
    }

    if (dock->priv->window)
        gtk_widget_unmap (dock->priv->window);
}

static void
gdl_dock_foreach_automatic (GdlDockObject *object, gpointer user_data)
{
    void (* function) (GtkWidget *) = user_data;

    if (gdl_dock_object_is_automatic (object))
        (* function) (GTK_WIDGET (object));
}

static void
gdl_dock_show (GtkWidget *widget)
{
    g_return_if_fail (widget);
    g_return_if_fail (GDL_IS_DOCK (widget));

    GTK_WIDGET_CLASS (gdl_dock_parent_class)->show (widget);

    GdlDock* dock = GDL_DOCK (widget);
    if (dock->priv->floating && dock->priv->window)
        gtk_widget_show (dock->priv->window);

    GdlDockMaster* master = GDL_DOCK_MASTER (gdl_dock_object_get_master (GDL_DOCK_OBJECT (dock)));
    if (GDL_DOCK (gdl_dock_master_get_controller (master)) == dock) {
        gdl_dock_master_foreach_toplevel (master, FALSE, (GFunc) gdl_dock_foreach_automatic, gtk_widget_show);
    }
}

static void
gdl_dock_hide (GtkWidget *widget)
{
    g_return_if_fail (widget != NULL);
    g_return_if_fail (GDL_IS_DOCK (widget));

    GTK_WIDGET_CLASS (gdl_dock_parent_class)->hide (widget);

    GdlDock* dock = GDL_DOCK (widget);
    if (dock->priv->floating && dock->priv->window)
        gtk_widget_hide (dock->priv->window);

    GdlDockMaster* master = GDL_DOCK_MASTER (gdl_dock_object_get_master (GDL_DOCK_OBJECT (dock)));
    if (GDL_DOCK (gdl_dock_master_get_controller (master)) == dock) {
        gdl_dock_master_foreach_toplevel (master, FALSE, (GFunc) gdl_dock_foreach_automatic, gtk_widget_hide);
    }
}

void
gdl_dock_add (GdlDock *container, GtkWidget *widget)
{
    g_return_if_fail (container);
    g_return_if_fail (GDL_IS_DOCK (container));
    g_return_if_fail (GDL_IS_DOCK_ITEM (widget));

    gdl_dock_add_item (container, GDL_DOCK_ITEM (widget), GDL_DOCK_TOP);  /* default position */
}

void
gdl_dock_remove (GdlDock *dock, GtkWidget *widget)
{
    g_return_if_fail (dock);
    g_return_if_fail (widget);

    gboolean was_visible = gtk_widget_get_visible (widget);

    if (GTK_WIDGET (dock->priv->root) == widget) {
        dock->priv->root = NULL;
        gtk_widget_unparent (widget);

        if (was_visible && gtk_widget_get_visible (GTK_WIDGET (dock)))
            gtk_widget_queue_resize (GTK_WIDGET (dock));
    }
}

void
gdl_dock_forall (GdlDock *dock, gboolean include_internals, GtkCallback callback, gpointer callback_data)
{
    g_return_if_fail (dock);
    g_return_if_fail (GDL_IS_DOCK (dock));
    g_return_if_fail (callback);

    if (dock->priv->root)
        (*callback) (GTK_WIDGET (dock->priv->root), callback_data);
}

GType
gdl_dock_child_type (GdlDock *container)
{
    return GDL_TYPE_DOCK_ITEM;
}

static void
gdl_dock_detach (GdlDockObject *object, gboolean recursive)
{
    GdlDock *dock = GDL_DOCK (object);

    /* detach children */
    if (recursive && dock->priv->root) {
        gdl_dock_object_detach (dock->priv->root, recursive);
    }
}

static void
gdl_dock_reduce (GdlDockObject *object)
{
    GdlDock *dock = GDL_DOCK (object);

    if (dock->priv->root)
        return;

    GtkWidget* widget = GTK_WIDGET (object);
    GtkWidget* parent = gtk_widget_get_parent (widget);

    if (gdl_dock_object_is_automatic (GDL_DOCK_OBJECT (dock))) {
        if (parent)
			gtk_widget_unparent (widget);

    } else if (gdl_dock_object_is_closed (GDL_DOCK_OBJECT (dock))) {
        /* if the user explicitly detached the object */
        if (dock->priv->floating) {
            gtk_widget_hide (GTK_WIDGET (dock));
        } else {
            if (parent)
                //gtk_container_remove (GTK_CONTAINER (parent), widget);
				gtk_widget_unparent (widget);
        }
    }
}

static gboolean
gdl_dock_dock_request (GdlDockObject *object, gint x, gint y, GdlDockRequest *request)
{
    gint                rel_x, rel_y;
    GtkAllocation       alloc;
    gboolean            may_dock = FALSE;
    GdlDockRequest      my_request;

    g_return_val_if_fail (GDL_IS_DOCK (object), FALSE);

    /* we get (x,y) in our allocation coordinates system */

    GdlDock* dock = GDL_DOCK (object);

    /* Get dock size. */
    gtk_widget_get_allocation (GTK_WIDGET (dock), &alloc);
#ifdef GTK4_TODO
    guint bw = gtk_container_get_border_width (GTK_CONTAINER (dock));
#else
	guint bw = 0;
#endif

    /* Get coordinates relative to our allocation area. */
    rel_x = x - alloc.x;
    rel_y = y - alloc.y;

    if (request)
        my_request = *request;

    /* Check if coordinates are in GdlDock widget. */
    if (rel_x > 0 && rel_x < alloc.width &&
        rel_y > 0 && rel_y < alloc.height) {

        /* It's inside our area. */
        may_dock = TRUE;

	/* Set docking indicator rectangle to the GdlDock size. */
        my_request.rect.x = alloc.x + bw;
        my_request.rect.y = alloc.y + bw;
        my_request.rect.width = alloc.width - 2*bw;
        my_request.rect.height = alloc.height - 2*bw;

	/* If GdlDock has no root item yet, set the dock itself as
	   possible target. */
        if (!dock->priv->root) {
            my_request.position = GDL_DOCK_TOP;
            my_request.target = object;
        } else {
            my_request.target = dock->priv->root;

            /* See if it's in the border_width band. */
            if (rel_x < bw) {
                my_request.position = GDL_DOCK_LEFT;
                my_request.rect.width *= SPLIT_RATIO;
            } else if (rel_x > alloc.width - bw) {
                my_request.position = GDL_DOCK_RIGHT;
                my_request.rect.x += my_request.rect.width * (1 - SPLIT_RATIO);
                my_request.rect.width *= SPLIT_RATIO;
            } else if (rel_y < bw) {
                my_request.position = GDL_DOCK_TOP;
                my_request.rect.height *= SPLIT_RATIO;
            } else if (rel_y > alloc.height - bw) {
                my_request.position = GDL_DOCK_BOTTOM;
                my_request.rect.y += my_request.rect.height * (1 - SPLIT_RATIO);
                my_request.rect.height *= SPLIT_RATIO;
            } else {
                /* Otherwise try our children. */
                /* give them allocation coordinates (we are a
                   GTK_NO_WINDOW) widget */
                may_dock = gdl_dock_object_dock_request (GDL_DOCK_OBJECT (dock->priv->root), x, y, &my_request);
            }
        }
    }

    if (may_dock && request)
        *request = my_request;

    return may_dock;
}

static void
gdl_dock_dock (GdlDockObject *object, GdlDockObject *requestor, GdlDockPlacement position, GValue *user_data)
{
	ENTER;

    g_return_if_fail (GDL_IS_DOCK (object));
    /* only dock items allowed at this time */
    g_return_if_fail (GDL_IS_DOCK_ITEM (requestor));

    GdlDock *dock = GDL_DOCK (object);

    if (position == GDL_DOCK_FLOATING) {
        GdlDockItem *item = GDL_DOCK_ITEM (requestor);
        gint x, y, width, height;

        if (user_data && G_VALUE_HOLDS (user_data, GDK_TYPE_RECTANGLE)) {
            GdkRectangle *rect;

            rect = g_value_get_boxed (user_data);
            x = rect->x;
            y = rect->y;
            width = rect->width;
            height = rect->height;
        }
        else {
            x = y = 0;
            width = height = -1;
        }

        gdl_dock_add_floating_item (dock, item, x, y, width, height);
    }
    else if (dock->priv->root) {
        /* This is somewhat a special case since we know which item to
           pass the request on because we only have on child */
        gdl_dock_object_dock (dock->priv->root, requestor, position, NULL);
        gdl_dock_set_title (dock);

    }
    else { /* Item about to be added is root item. */
        GtkWidget *widget = GTK_WIDGET (requestor);

        dock->priv->root = requestor;
        gtk_widget_set_parent (widget, GTK_WIDGET (dock));

        gdl_dock_item_show_grip (GDL_DOCK_ITEM (requestor));

        /* Realize the item (create its corresponding GdkWindow) when
           GdlDock has been realized. */
        if (gtk_widget_get_realized (GTK_WIDGET (dock)))
            gtk_widget_realize (widget);

        /* Map the widget if it's visible and the parent is visible and has
           been mapped. This is done to make sure that the GdkWindow is
           visible. */
        if (gtk_widget_get_visible (GTK_WIDGET (dock)) &&
            gtk_widget_get_visible (widget)) {
            if (gtk_widget_get_mapped (GTK_WIDGET (dock)))
                gtk_widget_map (widget);

            /* Make the widget resize. */
            gtk_widget_queue_resize (widget);
        }
        gdl_dock_set_title (dock);
    }
	LEAVE;
}

#ifdef GTK4_TODO
static gboolean
gdl_dock_floating_window_delete_event_cb (GtkWidget *widget)
{
    g_return_val_if_fail (GTK_IS_WINDOW (widget), FALSE);

    GdlDock* dock = GDL_DOCK (g_object_get_data (G_OBJECT (widget), "dock"));
    if (dock->priv->root) {
        /* this will call reduce on ourselves, hiding the window if appropiate */
        if(!GDL_DOCK_ITEM_CANT_CLOSE (GDL_DOCK_ITEM (dock->priv->root))) {
            gdl_dock_item_hide_item (GDL_DOCK_ITEM (dock->priv->root));
        }
    }

    return TRUE;
}
#endif

static void
_gdl_dock_foreach_build_list (GdlDockObject *object, gpointer user_data)
{
    GList **l = (GList **) user_data;

    if (GDL_IS_DOCK_ITEM (object))
        *l = g_list_prepend (*l, object);
}

static gboolean
gdl_dock_reorder (GdlDockObject *object, GdlDockObject *requestor, GdlDockPlacement new_position, GValue *other_data)
{
    GdlDock *dock = GDL_DOCK (object);
    gboolean handled = FALSE;

    if (dock->priv->floating &&
        new_position == GDL_DOCK_FLOATING &&
        dock->priv->root == requestor) {

        if (other_data && G_VALUE_HOLDS (other_data, GDK_TYPE_RECTANGLE)) {
#ifdef GTK4_TODO
            GdkRectangle* rect = g_value_get_boxed (other_data);
            gtk_window_move (GTK_WINDOW (dock->priv->window), rect->x, rect->y);
#endif
            handled = TRUE;
        }
    }

    return handled;
}

static gboolean
gdl_dock_child_placement (GdlDockObject *object, GdlDockObject *child, GdlDockPlacement *placement)
{
    GdlDock *dock = GDL_DOCK (object);
    gboolean retval = TRUE;

    if (dock->priv->root == child) {
        if (placement) {
            if (*placement == GDL_DOCK_NONE || *placement == GDL_DOCK_FLOATING)
                *placement = GDL_DOCK_TOP;
        }
    } else
        retval = FALSE;

    return retval;
}

static void
gdl_dock_present (GdlDockObject *object, GdlDockObject *child)
{
    GdlDock *dock = GDL_DOCK (object);

    if (dock->priv->floating)
        gtk_window_present (GTK_WINDOW (dock->priv->window));
}


/* ----- Public interface ----- */

/**
 * gdl_dock_new:
 *
 * Create a new dock.
 *
 * Returns: (transfer full): A new #GdlDock widget.
 */
GtkWidget *
gdl_dock_new (void)
{
    GObject *dock = g_object_new (GDL_TYPE_DOCK, NULL);
    gdl_dock_object_set_manual (GDL_DOCK_OBJECT (dock));

    return GTK_WIDGET (dock);
}

/**
 * gdl_dock_new_from:
 * @original: The original #GdlDock
 * @floating: %TRUE to create a floating dock
 *
 * Create a new dock widget having the same master than @original.
 *
 * Returns: (transfer full): A new #GdlDock widget
 */
GtkWidget *
gdl_dock_new_from (GdlDock *original, gboolean floating)
{
    g_return_val_if_fail (original != NULL, NULL);

    GObject* new_dock = g_object_new (GDL_TYPE_DOCK, "master", gdl_dock_object_get_master (GDL_DOCK_OBJECT (original)), "floating", floating, NULL);
    gdl_dock_object_set_manual (GDL_DOCK_OBJECT (new_dock));

    return GTK_WIDGET (new_dock);
}

/* Depending on where the dock item (where new item will be docked) locates
 * in the dock, we might need to change the docking placement. If the
 * item is does not touches the center of dock, the new-item-to-dock would
 * require a center dock on this item.
 */
static GdlDockPlacement
gdl_dock_refine_placement (GdlDock *dock, GdlDockItem *dock_item, GdlDockPlacement placement)
{
    GtkAllocation allocation;
    gtk_widget_get_allocation (GTK_WIDGET (dock), &allocation);

	if (allocation.width > 0) {
		g_return_val_if_fail (allocation.height > 0, placement);

	    GtkRequisition object_size;
    	gdl_dock_item_preferred_size (dock_item, &object_size);
		g_return_val_if_fail (object_size.width > 0, placement);
		g_return_val_if_fail (object_size.height > 0, placement);

		if (placement == GDL_DOCK_LEFT || placement == GDL_DOCK_RIGHT) {
			/* Check if dock_object touches center in terms of width */
			if (allocation.width/2 > object_size.width) {
				return GDL_DOCK_CENTER;
			}
		} else if (placement == GDL_DOCK_TOP || placement == GDL_DOCK_BOTTOM) {
			/* Check if dock_object touches center in terms of height */
			if (allocation.height/2 > object_size.height) {
				return GDL_DOCK_CENTER;
			}
		}
	}
    return placement;
}

/* Determines the larger item of the two based on the placement:
 * for left/right placement, height determines it.
 * for top/bottom placement, width determines it.
 * for center placement, area determines it.
 */
static GdlDockItem*
gdl_dock_select_larger_item (GdlDockItem *dock_item_1, GdlDockItem *dock_item_2, GdlDockPlacement placement, gint level /* for debugging */)
{
    GtkRequisition size_1, size_2;

    g_return_val_if_fail (dock_item_1 != NULL, dock_item_2);
    g_return_val_if_fail (dock_item_2 != NULL, dock_item_1);

    gdl_dock_item_preferred_size (dock_item_1, &size_1);
    gdl_dock_item_preferred_size (dock_item_2, &size_2);

#ifdef GTK4_TODO
    g_return_val_if_fail (size_1.width > 0, dock_item_2);
#else
	if (size_1.width < 1) { cdbg(0, "TODO not yet allocated"); return dock_item_2; }
#endif
    g_return_val_if_fail (size_1.height > 0, dock_item_2);
    g_return_val_if_fail (size_2.width > 0, dock_item_1);
    g_return_val_if_fail (size_2.height > 0, dock_item_1);

    if (placement == GDL_DOCK_LEFT || placement == GDL_DOCK_RIGHT) {
        /* For left/right placement, height is what matters */
        return (size_1.height >= size_2.height?  dock_item_1 : dock_item_2);
    } else if (placement == GDL_DOCK_TOP || placement == GDL_DOCK_BOTTOM) {
        /* For top/bottom placement, width is what matters */
        return (size_1.width >= size_2.width ? dock_item_1 : dock_item_2);
    } else if (placement == GDL_DOCK_CENTER) {
        /* For center place, area is what matters */
        return ((size_1.width * size_1.height) >= (size_2.width * size_2.height) ?  dock_item_1 : dock_item_2);
    } else if (placement == GDL_DOCK_NONE) {
        return dock_item_1;
    } else {
        g_warning ("Should not reach here: %s:%d", __func__, __LINE__);
    }
    return dock_item_1;
}

/* Determines the best dock item to dock a new item with the given placement.
 * It traverses the dock tree and (based on the placement) tries to find
 * the best located item wrt to the placement. The approach is to find the
 * largest item on/around the placement side (for side placements) and to
 * find the largest item for center placement. In most situations, this is
 * what user wants and the heuristic should be therefore sufficient.
 */
static GdlDockItem*
gdl_dock_find_best_placement_item (GdlDockItem *dock_item, GdlDockPlacement placement, gint level /* for debugging */)
{
	ENTER;

    GdlDockItem *ret_item = NULL;

    if (GDL_IS_DOCK_PANED (dock_item)) {
    	GList* children = GDL_DOCK_OBJECT_GET_CLASS (dock_item)->children(GDL_DOCK_OBJECT (dock_item));
		g_assert(g_list_length(children) == 2);
		GtkWidget* child1 = children->data;
		GtkWidget* child2 = children->next->data;
		g_list_free(children);

        GtkOrientation orientation;
        g_object_get (dock_item, "orientation", &orientation, NULL);
        if ((orientation == GTK_ORIENTATION_HORIZONTAL &&
             placement == GDL_DOCK_LEFT) ||
            (orientation == GTK_ORIENTATION_VERTICAL &&
             placement == GDL_DOCK_TOP)) {
            /* Return left or top pane widget */
            ret_item = gdl_dock_find_best_placement_item (GDL_DOCK_ITEM (child1), placement, level + 1);
        } else if ((orientation == GTK_ORIENTATION_HORIZONTAL &&
                    placement == GDL_DOCK_RIGHT) ||
                   (orientation == GTK_ORIENTATION_VERTICAL &&
                    placement == GDL_DOCK_BOTTOM)) {
                        /* Return right or top pane widget */
            ret_item = gdl_dock_find_best_placement_item (GDL_DOCK_ITEM (child2), placement, level + 1);
        } else {
            /* Evaluate which of the two sides is bigger */
            GdlDockItem* dock_item_1 = gdl_dock_find_best_placement_item (GDL_DOCK_ITEM (child1), placement, level + 1);
            GdlDockItem* dock_item_2 = gdl_dock_find_best_placement_item (GDL_DOCK_ITEM (child2), placement, level + 1);
            ret_item = gdl_dock_select_larger_item (dock_item_1, dock_item_2, placement, level);
        }
    } else if (GDL_IS_DOCK_ITEM (dock_item)) {
        ret_item = dock_item;
    } else {
        g_warning ("%s: unexpected GdlDockItem type: %s", __func__, G_OBJECT_TYPE_NAME(dock_item));
    }
	LEAVE;
    return ret_item;
}

/**
 * gdl_dock_add_item:
 * @dock: A #GdlDock widget
 * @item: A #GdlDockItem widget
 * @placement: A position for the widget
 *
 * Dock in @dock, the widget @item at the position defined by @placement. The
 * function takes care of finding the right parent widget eventually creating
 * it if needed.
 */
void
gdl_dock_add_item (GdlDock *dock, GdlDockItem *item, GdlDockPlacement placement)
{
	ENTER;

    GdlDockObject *parent = NULL;
    GdlDockPlacement place;

    g_return_if_fail (dock != NULL);
    g_return_if_fail (item != NULL);

    /* Check if a placeholder widget already exist in the same dock */
    GdlDockObject* placeholder = gdl_dock_master_get_object (GDL_DOCK_MASTER (gdl_dock_object_get_master (GDL_DOCK_OBJECT (dock))), gdl_dock_object_get_name (GDL_DOCK_OBJECT (item)));
    if ((placeholder != GDL_DOCK_OBJECT (item)) && (placeholder != NULL)) {
        if (gdl_dock_object_get_toplevel (placeholder) == dock) {
            parent = gdl_dock_object_get_parent_object (placeholder);
        } else {
#ifdef GTK4_TODO
            gtk_widget_destroy (GTK_WIDGET (placeholder));
#else
			cdbg(0, "TODO destroy widget. parent=%p", gtk_widget_get_parent (GTK_WIDGET (placeholder)));
#endif
        }
    }

    if (parent && gdl_dock_object_child_placement (parent, placeholder, &place)) {
        gdl_dock_object_freeze (GDL_DOCK_OBJECT (parent));
#ifdef GTK4_TODO
        gtk_widget_destroy (GTK_WIDGET (placeholder));
#else
		cdbg(0, "TODO destroy widget. parent=%p", gtk_widget_get_parent (GTK_WIDGET (placeholder)));
#endif

        gdl_dock_object_dock (GDL_DOCK_OBJECT (parent), GDL_DOCK_OBJECT (item), place, NULL);
        gdl_dock_object_thaw (GDL_DOCK_OBJECT (parent));
    } else if (placement == GDL_DOCK_FLOATING) {
        /* Add the item to a new floating dock */
        gdl_dock_add_floating_item (dock, item, 0, 0, -1, -1);

    } else {
        /* Non-floating item. */
        if (dock->priv->root) {
            GdlDockItem* best_dock_item = gdl_dock_find_best_placement_item (GDL_DOCK_ITEM (dock->priv->root), placement, 0);
            GdlDockPlacement local_placement = gdl_dock_refine_placement (dock, best_dock_item, placement);
            gdl_dock_object_dock (GDL_DOCK_OBJECT (best_dock_item), GDL_DOCK_OBJECT (item), local_placement, NULL);
        } else {
            gdl_dock_object_dock (GDL_DOCK_OBJECT (dock), GDL_DOCK_OBJECT (item), placement, NULL);
        }
    }
	LEAVE;
}

/**
 * gdl_dock_add_floating_item:
 * @dock: A #GdlDock widget
 * @item: A #GdlDockItem widget
 * @x: X coordinate of the floating item
 * @y: Y coordinate of the floating item
 * @width: width of the floating item
 * @height: height of the floating item
 *
 * Dock an item as a floating item. It creates a new window containing a new
 * dock widget sharing the same master where the item is docked.
 */
void
gdl_dock_add_floating_item (GdlDock *dock, GdlDockItem *item, gint x, gint y, gint width, gint height)
{
    g_return_if_fail (dock);
    g_return_if_fail (item);

    GdlDock* new_dock = GDL_DOCK (g_object_new (GDL_TYPE_DOCK,
                                       "master", gdl_dock_object_get_master (GDL_DOCK_OBJECT (dock)),
                                       "floating", TRUE,
                                       "width", width,
                                       "height", height,
                                       "floatx", x,
                                       "floaty", y,
                                       "skip-taskbar", dock->priv->skip_taskbar,
                                       NULL));

    if (gtk_widget_get_visible (GTK_WIDGET (dock))) {
        gtk_widget_show (GTK_WIDGET (new_dock));
        if (gtk_widget_get_mapped (GTK_WIDGET (dock)))
            gtk_widget_map (GTK_WIDGET (new_dock));

        /* Make the widget resize. */
        gtk_widget_queue_resize (GTK_WIDGET (new_dock));
    }

    gdl_dock_add_item (GDL_DOCK (new_dock), item, GDL_DOCK_TOP);
}

/**
 * gdl_dock_get_item_by_name:
 * @dock: A #GdlDock widget
 * @name: An item name
 *
 * Looks for an #GdlDockItem widget bound to the master of the dock item. It
 * does not search only in the children of this particular dock widget.
 *
 * Returns: (transfer none): A #GdlDockItem widget or %NULL
 */
GdlDockItem *
gdl_dock_get_item_by_name (GdlDock *dock, const gchar *name)
{

    g_return_val_if_fail (dock != NULL && name != NULL, NULL);

    /* proxy the call to our master */
    GdlDockObject* found = gdl_dock_master_get_object (GDL_DOCK_MASTER (gdl_dock_object_get_master (GDL_DOCK_OBJECT (dock))), name);

    return (found && GDL_IS_DOCK_ITEM (found)) ? GDL_DOCK_ITEM (found) : NULL;
}

/**
 * gdl_dock_get_named_items:
 * @dock: A #GdlDock widget
 *
 * Returns a list of all item bound to the master of the dock, not only
 * the children of this particular dock widget.
 *
 * Returns: (element-type GdlDockObject) (transfer container): A list of #GdlDockItem. The list should be freedwith g_list_free(),
 * but the item still belong to the master.
 */
GList *
gdl_dock_get_named_items (GdlDock *dock)
{
    GList *list = NULL;

    g_return_val_if_fail (dock, NULL);

    gdl_dock_master_foreach (GDL_DOCK_MASTER (gdl_dock_object_get_master (GDL_DOCK_OBJECT (dock))), (GFunc) _gdl_dock_foreach_build_list, &list);

    return list;
}

/**
 * gdl_dock_object_get_toplevel:
 * @object: A #GdlDockObject
 *
 * Get the top level #GdlDock widget of @object or %NULL if cannot be found.
 *
 * Returns: (allow-none) (transfer none): A #GdlDock or %NULL.
 */
GdlDock *
gdl_dock_object_get_toplevel (GdlDockObject *object)
{
    GdlDockObject *parent = object;

    g_return_val_if_fail (object != NULL, NULL);

    while (parent && !GDL_IS_DOCK (parent))
        parent = gdl_dock_object_get_parent_object (parent);

    return parent ? GDL_DOCK (parent) : NULL;
}


/**
 * gdl_dock_get_root:
 * @dock: A #GdlDockObject
 *
 * Get the first child of the #GdlDockObject.
 *
 * Returns: (allow-none) (transfer none): A #GdlDockObject or %NULL.
 */
GdlDockObject*
gdl_dock_get_root (GdlDock *dock)
{
    g_return_val_if_fail (GDL_IS_DOCK (dock), NULL);

    return dock->priv->root;
}

/**
 * gdl_dock_show_preview:
 * @dock: A #GdlDock widget
 * @rect: The position and the size of the preview window
 *
 * Show a preview window used to materialize the dock target.
 */
void
gdl_dock_show_preview (GdlDock *dock, cairo_rectangle_int_t *rect)
{
#ifdef GTK4_TODO
    gint x, y;
    GdkWindow* window = gtk_widget_get_window (GTK_WIDGET (dock));
    gdk_window_get_origin (window, &x, &y);

    if (!dock->priv->area_window) {
        dock->priv->area_window = gdl_preview_window_new ();
    }

    rect->x += x;
    rect->y += y;

    gdl_preview_window_update (GDL_PREVIEW_WINDOW (dock->priv->area_window), rect);
#endif
}

/**
 * gdl_dock_hide_preview:
 * @dock: A #GdlDock widget
 *
 * Hide the preview window used to materialize the dock target.
 */
void
gdl_dock_hide_preview (GdlDock *dock)
{
    if (dock->priv->area_window)
        gtk_widget_hide (dock->priv->area_window);
}
