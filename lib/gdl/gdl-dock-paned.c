/*
 * gdl-dock-paned.h
 *
 * This file is part of the GNOME Devtools Libraries.
 *
 * Copyright (C) 2002 Gustavo Giráldez <gustavo.giraldez@gmx.net>
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

#include <string.h>
#include <gtk/gtk.h>
#include "il8n.h"
#include "utils.h"
#include "gdl/debug.h"
#include "gdl-dock-paned.h"

#include "gdl-gtk-paned.c"


/**
 * SECTION:gdl-dock-paned
 * @title: GdlDockPaned
 * @short_description: Arrange dock widget in two adjustable panes
 * @stability: Unstable
 * @see_also: #GdlDockNotebook, #GdlDockMaster
 *
 * A #GdlDockPaned is a compound dock widget. It can dock one or two children,
 * including another compound widget like a #GdlDockPaned or a #GdlDockNotebook.
 * The children are displayed in two panes using a #GtkPaned widget.
 * A #GdlDockPaned is normally created automatically by the master when docking
 * a child on any edge: top, bottom, left or right.
 */


/* Private prototypes */

static void     gdl_dock_paned_class_init     (GdlDockPanedClass *klass);
static void     gdl_dock_paned_init           (GdlDockPaned      *paned);
static GObject *gdl_dock_paned_constructor    (GType              type,
                                               guint              n_construct_properties,
                                               GObjectConstructParam *construct_param);
static void     gdl_dock_paned_set_property   (GObject           *object,
                                               guint              prop_id,
                                               const GValue      *value,
                                               GParamSpec        *pspec);
static void     gdl_dock_paned_get_property   (GObject           *object,
                                               guint              prop_id,
                                               GValue            *value,
                                               GParamSpec        *pspec);

static void     gdl_dock_paned_dispose         (GObject*          object);

static gboolean gdl_dock_paned_dock_request    (GdlDockObject*    object,
                                                gint              x,
                                                gint              y,
                                                GdlDockRequest*   request);
static void     gdl_dock_paned_dock            (GdlDockObject*    object,
                                                GdlDockObject*    requestor,
                                                GdlDockPlacement  position,
                                                GValue*           other_data);

static void     gdl_dock_paned_set_orientation (GdlDockItem*      item,
                                                GtkOrientation    orientation);

static gboolean gdl_dock_paned_child_placement (GdlDockObject*    object,
                                                GdlDockObject*    child,
                                                GdlDockPlacement* placement);
static GList*   gdl_dock_paned_children        (GdlDockObject*    object);
static void     gdl_dock_paned_foreach_child   (GdlDockObject*    object,
                                                GdlDockObjectFn   fn,
                                                gpointer          user_data);
static void     gdl_dock_paned_remove_child    (GdlDockObject*, GtkWidget*);
static void     gdl_dock_paned_remove_widgets  (GdlDockObject*    object);
#ifdef DEBUG
static bool     gdl_dock_paned_validate        (GdlDockObject* object);
#endif


/* ----- Class variables and definitions ----- */

#define SPLIT_RATIO  0.3

enum {
    PROP_0,
    PROP_POSITION
};

struct _GdlDockPanedPrivate {
    gboolean    user_action;
    gboolean    position_changed;
};

/* ----- Private functions ----- */

G_DEFINE_TYPE_WITH_CODE (GdlDockPaned, gdl_dock_paned, GDL_TYPE_DOCK_ITEM, G_ADD_PRIVATE (GdlDockPaned));

static void
gdl_dock_paned_class_init (GdlDockPanedClass *klass)
{
    GObjectClass* g_object_class = G_OBJECT_CLASS (klass);
    GdlDockObjectClass* object_class = GDL_DOCK_OBJECT_CLASS (klass);
    GdlDockItemClass* item_class = GDL_DOCK_ITEM_CLASS (klass);

    g_object_class->set_property = gdl_dock_paned_set_property;
    g_object_class->get_property = gdl_dock_paned_get_property;
    g_object_class->constructor = gdl_dock_paned_constructor;
    g_object_class->dispose = gdl_dock_paned_dispose;

    gdl_dock_object_class_set_is_compound (object_class, TRUE);
    object_class->dock_request = gdl_dock_paned_dock_request;
    object_class->dock = gdl_dock_paned_dock;
    object_class->child_placement = gdl_dock_paned_child_placement;
    object_class->children = gdl_dock_paned_children;
    object_class->foreach_child = gdl_dock_paned_foreach_child;
    object_class->remove = gdl_dock_paned_remove_child;
    object_class->remove_widgets = gdl_dock_paned_remove_widgets;
#ifdef DEBUG
    object_class->validate = gdl_dock_paned_validate;
#endif

    gdl_dock_item_class_set_has_grip (item_class, FALSE);
    item_class->set_orientation = gdl_dock_paned_set_orientation;

    g_object_class_install_property (
        g_object_class, PROP_POSITION,
        g_param_spec_uint ("position", _("Position"),
                           _("Position of the divider in pixels"),
                           0, G_MAXINT, 0,
                           G_PARAM_READWRITE |
                           GDL_DOCK_PARAM_EXPORT | GDL_DOCK_PARAM_AFTER));

	gtk_widget_class_set_css_name (GTK_WIDGET_CLASS(klass), "dock-paned");
}

static void
gdl_dock_paned_init (GdlDockPaned *paned)
{
	paned->priv = gdl_dock_paned_get_instance_private (paned);

    paned->priv->user_action = FALSE;
    paned->priv->position_changed = FALSE;
}

static void
gdl_dock_paned_notify_cb (GObject *g_object, GParamSpec *pspec, gpointer user_data)
{
    g_return_if_fail (user_data != NULL && GDL_IS_DOCK_PANED (user_data));

    /* chain the notification to the GdlDockPaned */
    g_object_notify (G_OBJECT (user_data), pspec->name);

    GdlDockPaned *paned = GDL_DOCK_PANED (user_data);

    if (paned->priv->user_action && !strcmp (pspec->name, "position"))
        paned->priv->position_changed = TRUE;
}

#ifdef GTK4_TODO
static gboolean
gdl_dock_paned_button_cb (GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
    g_return_val_if_fail (user_data != NULL && GDL_IS_DOCK_PANED (user_data), FALSE);

    GdlDockPaned *paned = GDL_DOCK_PANED (user_data);

    if (event->button == 1) {
        if (event->type == GDK_BUTTON_PRESS)
            paned->priv->user_action = TRUE;
        else {
            paned->priv->user_action = FALSE;
            if (paned->priv->position_changed) {
                /* emit pending layout changed signal to track separator position */
                gdl_dock_object_layout_changed_notify (GDL_DOCK_OBJECT (paned));
                paned->priv->position_changed = FALSE;
            }
        }
    }

    return FALSE;
}
#endif

#if 0
static void
gdl_dock_item_drag_begin (GtkGestureDrag *gesture, double start_x, double start_y, GdlDockItem *item)
{
}

static void
gdl_dock_item_drag_end (GtkGestureDrag *gesture, double offset_x, double offset_y, GdlDockItem *item)
{
}
#endif

static void
gdl_dock_paned_create_child (GdlDockPaned *paned, GtkOrientation orientation)
{
    GdlDockItem* item = GDL_DOCK_ITEM (paned);

    /* create the container paned */
    GtkWidget* child = gdl_gtk_paned_new (orientation);
    gdl_dock_item_set_child (item, child);

    /* get notification for propagation */
    g_signal_connect (child, "notify::position", (GCallback) gdl_dock_paned_notify_cb, (gpointer) item);
#ifdef GTK4_TODO
    g_signal_connect (child, "button-press-event", (GCallback) gdl_dock_paned_button_cb, (gpointer) item);
    g_signal_connect (child, "button-release-event", (GCallback) gdl_dock_paned_button_cb, (gpointer) item);
#else
																// TODO is called twice?
#if 0
	g_signal_connect (GDL_GTK_PANED(child)->drag_gesture, "drag-begin", G_CALLBACK (gdl_dock_item_drag_begin), item);
	g_signal_connect (GDL_GTK_PANED(child)->drag_gesture, "drag-end", G_CALLBACK (gdl_dock_item_drag_end), item);
#endif
#endif
}

static GObject *
gdl_dock_paned_constructor (GType type, guint n_construct_properties, GObjectConstructParam *construct_param)
{
	GObject* g_object = G_OBJECT_CLASS (gdl_dock_paned_parent_class)-> constructor (type, n_construct_properties, construct_param);
	GdlDockItem *item = GDL_DOCK_ITEM (g_object);

	if (!gdl_dock_item_get_child (item))
		gdl_dock_paned_create_child (GDL_DOCK_PANED (g_object), gdl_dock_item_get_orientation (item));
		/* otherwise, the orientation was set as a construction
		   parameter and the child is already created */

	return g_object;
}

static void
gdl_dock_paned_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    GdlDockItem *item = GDL_DOCK_ITEM (object);
    GtkWidget *child;

    switch (prop_id) {
        case PROP_POSITION:
            child = gdl_dock_item_get_child (item);
            if (child && GDL_IS_GTK_PANED (child))
                gdl_gtk_paned_set_position (GDL_GTK_PANED (child), g_value_get_uint (value));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
gdl_dock_paned_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    GdlDockItem *item = GDL_DOCK_ITEM (object);

    switch (prop_id) {
        case PROP_POSITION:
    		GtkWidget *child = gdl_dock_item_get_child (item);
            if (child && GDL_IS_GTK_PANED (child))
                g_value_set_uint (value, gdl_gtk_paned_get_position (GDL_GTK_PANED (child)));
            else
                g_value_set_uint (value, 0);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
gdl_dock_paned_dispose (GObject *object)
{
    GdlDockItem *item = GDL_DOCK_ITEM (object);

    /* we need to call the virtual first, since in GdlDockDestroy our children dock objects are detached */
    G_OBJECT_CLASS (gdl_dock_paned_parent_class)->dispose (object);

    /* after that we can remove the GtkNotebook */
    gdl_dock_item_set_child (item, NULL);
}

void
gdl_dock_paned_add (GdlDockPaned *container, GtkWidget *widget)
{
	ENTER;

	GdlDockPlacement pos = GDL_DOCK_NONE;

	g_return_if_fail (container && widget);
	g_return_if_fail (GDL_IS_DOCK_PANED (container));
	g_return_if_fail (GDL_IS_DOCK_ITEM (widget));

	GdlDockItem* item = GDL_DOCK_ITEM (container);
	g_return_if_fail (gdl_dock_item_get_child (item));

	GdlGtkPaned* paned = GDL_GTK_PANED (gdl_dock_item_get_child (item));
	GtkWidget* child1 = gdl_gtk_paned_get_start_child (paned);
	GtkWidget* child2 = gdl_gtk_paned_get_end_child (paned);
	g_return_if_fail (!child1 || !child2);

	if (!child1)
		pos = gdl_dock_item_get_orientation (item) == GTK_ORIENTATION_HORIZONTAL ?  GDL_DOCK_LEFT : GDL_DOCK_TOP;
	else if (!child2)
		pos = gdl_dock_item_get_orientation (item) == GTK_ORIENTATION_HORIZONTAL ?  GDL_DOCK_RIGHT : GDL_DOCK_BOTTOM;

	if (pos != GDL_DOCK_NONE)
		gdl_dock_object_dock (GDL_DOCK_OBJECT (container), GDL_DOCK_OBJECT (widget), pos, NULL);
}

static void
gdl_dock_paned_remove_child (GdlDockObject* object, GtkWidget* child)
{
	GdlGtkPaned* paned = GDL_GTK_PANED (gdl_dock_item_get_child (GDL_DOCK_ITEM (object)));

	if ((GtkWidget*)child == gdl_gtk_paned_get_end_child (paned))
		gdl_gtk_paned_set_end_child (paned, NULL);
	else if ((GtkWidget*)child == gdl_gtk_paned_get_start_child (paned))
		gdl_gtk_paned_set_start_child (paned, NULL);
}

static void
gdl_dock_paned_request_foreach (GdlDockObject *object, gpointer user_data)
{
    struct {
        GtkWidget*      parent;
        gint            x, y;
        GdlDockRequest* request;
        gboolean        may_dock;
    } *data = user_data;

    /* Translate parent coordinate to child coordinate */
    double child_x, child_y;
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    gtk_widget_translate_coordinates (data->parent, GTK_WIDGET (object), data->x, data->y, &child_x, &child_y);
#pragma GCC diagnostic warning "-Wdeprecated-declarations"

    GdlDockRequest my_request = *data->request;
    gboolean may_dock = gdl_dock_object_dock_request (object, child_x, child_y, &my_request);
    if (may_dock) {
        /* Translate request coordinate back to parent coordinate */
		graphene_point_t in = my_request.rect.origin;
		graphene_point_t out;
		if (gtk_widget_compute_point (GTK_WIDGET (object), data->parent, &in, &out)) {
			my_request.rect.origin = out;
		}
        data->may_dock = TRUE;
        *data->request = my_request;
    }
}

static gboolean
gdl_dock_paned_dock_request (GdlDockObject *object, gint x, gint y, GdlDockRequest *request)
{
    gboolean may_dock = FALSE;

    g_return_val_if_fail (GDL_IS_DOCK_ITEM (object), FALSE);

    /* we get (x,y) in our allocation coordinates system */

    GdlDockItem* item = GDL_DOCK_ITEM (object);

    /* Get item's allocation. */
	graphene_rect_t alloc;
#pragma GCC diagnostic ignored "-Wunused-result"
	gtk_widget_compute_bounds(GTK_WIDGET(object), GTK_WIDGET(gtk_widget_get_root(GTK_WIDGET(object))), &alloc);
#pragma GCC diagnostic warning "-Wunused-result"
#ifdef GTK4_TODO
    guint bw = gtk_container_get_border_width (GTK_CONTAINER (object));
#else
	guint bw = 0;
#endif

	/* Get coordinates relative to our window. */
	gint rel_x = x - alloc.origin.x;
	gint rel_y = y - alloc.origin.y;

	GdlDockRequest my_request = request ? *request : (GdlDockRequest){0,};

	/* Check if coordinates are inside the widget. */
	if (rel_x > 0 && rel_x < alloc.size.width &&
		rel_y > 0 && rel_y < alloc.size.height) {
		GtkRequisition my, other;
		gint divider = -1;

		gdl_dock_item_preferred_size (GDL_DOCK_ITEM (my_request.applicant), &other);
		gdl_dock_item_preferred_size (GDL_DOCK_ITEM (object), &my);

		/* It's inside our area. */
		may_dock = TRUE;

		/* Set docking indicator rectangle to the widget size. */
		my_request.rect = (graphene_rect_t){ .origin = {bw, bw}, .size = {alloc.size.width - 2 * bw, alloc.size.height - 2 * bw} };

        my_request.target = object;

        /* See if it's in the border_width band. */
        if (rel_x < bw) {
            my_request.position = GDL_DOCK_LEFT;
            my_request.rect.size.width *= SPLIT_RATIO;
            divider = other.width;
        } else if (rel_x > alloc.size.width - bw) {
            my_request.position = GDL_DOCK_RIGHT;
            my_request.rect.origin.x += my_request.rect.size.width * (1 - SPLIT_RATIO);
            my_request.rect.size.width *= SPLIT_RATIO;
            divider = MAX (0, my.width - other.width);
        } else if (rel_y < bw) {
            my_request.position = GDL_DOCK_TOP;
            my_request.rect.size.height *= SPLIT_RATIO;
            divider = other.height;
        } else if (rel_y > alloc.size.height - bw) {
            my_request.position = GDL_DOCK_BOTTOM;
            my_request.rect.origin.y += my_request.rect.size.height * (1 - SPLIT_RATIO);
            my_request.rect.size.height *= SPLIT_RATIO;
            divider = MAX (0, my.height - other.height);

        } else { /* Otherwise try our children. */
            struct {
                GtkWidget *parent;
                gint            x, y;
                GdlDockRequest *request;
                gboolean        may_dock;
            } data;

            /* give them coordinates in their allocation system... the
               GtkPaned has its own window in Gtk 3.1.6, so our children
               allocation coordinates has to be translated to and from
               our window coordinates. It is done in the
               gdl_dock_paned_request_foreach function. */
            data.parent = GTK_WIDGET (object);
            data.x = rel_x;
            data.y = rel_y;
            data.request = &my_request;
            data.may_dock = FALSE;

            gdl_dock_object_foreach_child (object, (GdlDockObjectFn) gdl_dock_paned_request_foreach, &data);

            may_dock = data.may_dock;
            if (!may_dock) {
                /* the pointer is on the handle, so snap to top/bottom
                   or left/right */
                may_dock = TRUE;
                if (gdl_dock_item_get_orientation (item) == GTK_ORIENTATION_HORIZONTAL) {
                    if (rel_y < alloc.size.height / 2) {
                        my_request.position = GDL_DOCK_TOP;
                        my_request.rect.size.height *= SPLIT_RATIO;
                        divider = other.height;
                    } else {
                        my_request.position = GDL_DOCK_BOTTOM;
                        my_request.rect.origin.y += my_request.rect.size.height * (1 - SPLIT_RATIO);
                        my_request.rect.size.height *= SPLIT_RATIO;
                        divider = MAX (0, my.height - other.height);
                    }
                } else {
                    if (rel_x < alloc.size.width / 2) {
                        my_request.position = GDL_DOCK_LEFT;
                        my_request.rect.size.width *= SPLIT_RATIO;
                        divider = other.width;
                    } else {
                        my_request.position = GDL_DOCK_RIGHT;
                        my_request.rect.origin.x += my_request.rect.size.width * (1 - SPLIT_RATIO);
                        my_request.rect.size.width *= SPLIT_RATIO;
                        divider = MAX (0, my.width - other.width);
                    }
                }
            }
        }

        if (divider >= 0 && my_request.position != GDL_DOCK_CENTER) {
            if (G_IS_VALUE (&my_request.extra))
                g_value_unset (&my_request.extra);
            g_value_init (&my_request.extra, G_TYPE_UINT);
            g_value_set_uint (&my_request.extra, (guint) divider);
        }

        if (may_dock) {
            /* adjust returned coordinates so they are relative to our allocation */
            my_request.rect.origin.x += alloc.origin.x;
            my_request.rect.origin.y += alloc.origin.y;
        }
    }

    if (may_dock && request)
        *request = my_request;

    return may_dock;
}

static void
gdl_dock_paned_dock (GdlDockObject *object, GdlDockObject *requestor, GdlDockPlacement position, GValue *other_data)
{
    gboolean requestor_resize = FALSE;
    gboolean done = FALSE;

    g_return_if_fail (GDL_IS_DOCK_PANED (object));
    g_return_if_fail (gdl_dock_item_get_child (GDL_DOCK_ITEM (object)) != NULL);

	ENTER;

    GdlGtkPaned* paned = GDL_GTK_PANED (gdl_dock_item_get_child (GDL_DOCK_ITEM (object)));

    if (GDL_IS_DOCK_ITEM (requestor)) {
        g_object_get(G_OBJECT (requestor), "resize", &requestor_resize, NULL);
    }

    GtkWidget* child1 = gdl_gtk_paned_get_start_child (paned);
    GtkWidget* child2 = gdl_gtk_paned_get_end_child (paned);
    /* see if we can dock the item in our paned */
    switch (gdl_dock_item_get_orientation (GDL_DOCK_ITEM (object))) {
        case GTK_ORIENTATION_HORIZONTAL:
            if (!child1 && position == GDL_DOCK_LEFT) {
                gdl_gtk_paned_set_start_child (paned, GTK_WIDGET (requestor));
                done = TRUE;
            } else if (!child2 && position == GDL_DOCK_RIGHT) {
                gdl_gtk_paned_set_end_child (paned, GTK_WIDGET (requestor));
                done = TRUE;
            }
            break;
        case GTK_ORIENTATION_VERTICAL:
            if (!child1 && position == GDL_DOCK_TOP) {
                gdl_gtk_paned_set_start_child (paned, GTK_WIDGET (requestor));
                done = TRUE;
            } else if (!child2 && position == GDL_DOCK_BOTTOM) {
                gdl_gtk_paned_set_end_child (paned, GTK_WIDGET (requestor));
                done = TRUE;
            }
            break;
        default:
            break;
    }

    if (!done) {
        /* this will create another paned and reparent us there */
        GDL_DOCK_OBJECT_CLASS (gdl_dock_paned_parent_class)->dock (object, requestor, position, other_data);

    } else {
        if (gtk_widget_get_visible (GTK_WIDGET (requestor)))
            gdl_dock_item_show_grip (GDL_DOCK_ITEM (requestor));
    }
}

static void
gdl_dock_paned_set_orientation (GdlDockItem *item, GtkOrientation orientation)
{
    GdlGtkPaned *old_paned = NULL;
    gboolean child1_resize, child2_resize;

    g_return_if_fail (GDL_IS_DOCK_PANED (item));

    if (gdl_dock_item_get_child (item)) {
        old_paned = GDL_GTK_PANED (gdl_dock_item_get_child (item));
        g_object_ref (old_paned);
        gdl_dock_item_set_child (item, NULL);
    }

    gdl_dock_paned_create_child (GDL_DOCK_PANED (item), orientation);

    if (old_paned) {
        GdlGtkPaned* new_paned = GDL_GTK_PANED (gdl_dock_item_get_child (item));
        GtkWidget* child1 = gdl_gtk_paned_get_start_child (old_paned);
        GtkWidget* child2 = gdl_gtk_paned_get_end_child (old_paned);

        if (child1) {
            g_object_get(G_OBJECT (child1), "resize", &child1_resize, NULL);
            g_object_ref (child1);
            gdl_gtk_paned_set_start_child (new_paned, child1);
            g_object_unref (child1);
        }
        if (child2) {
            g_object_get(G_OBJECT (child1), "resize", &child2_resize, NULL);
            g_object_ref (child2);
            gdl_gtk_paned_set_end_child (new_paned, child2);
            g_object_unref (child2);
        }
    }

    GDL_DOCK_ITEM_CLASS (gdl_dock_paned_parent_class)->set_orientation (item, orientation);
}

static gboolean
gdl_dock_paned_child_placement (GdlDockObject *object, GdlDockObject *child, GdlDockPlacement *placement)
{
    GdlDockItem *item = GDL_DOCK_ITEM (object);
    GdlDockPlacement pos = GDL_DOCK_NONE;

    if (gdl_dock_item_get_child (item)) {
        GdlGtkPaned* paned = GDL_GTK_PANED (gdl_dock_item_get_child (item));
        if (GTK_WIDGET (child) == gdl_gtk_paned_get_start_child (paned))
            pos = gdl_dock_item_get_orientation (item) == GTK_ORIENTATION_HORIZONTAL ?  GDL_DOCK_LEFT : GDL_DOCK_TOP;
        else if (GTK_WIDGET (child) == gdl_gtk_paned_get_end_child (paned))
            pos = gdl_dock_item_get_orientation (item) == GTK_ORIENTATION_HORIZONTAL ?  GDL_DOCK_RIGHT : GDL_DOCK_BOTTOM;
    }

    if (pos != GDL_DOCK_NONE) {
        if (placement)
            *placement = pos;
        return TRUE;
    }
    else
        return FALSE;
}


static GList*
gdl_dock_paned_children (GdlDockObject* object)
{
	GdlGtkPaned* paned = GDL_GTK_PANED (gdl_dock_item_get_child (GDL_DOCK_ITEM (object)));

	GList* children = NULL;
	children = g_list_append(children, gdl_gtk_paned_get_start_child(paned));
	children = g_list_append(children, gdl_gtk_paned_get_end_child(paned));

	return children;
}


static void
gdl_dock_paned_foreach_child (GdlDockObject *object, GdlDockObjectFn fn, gpointer user_data)
{
	GdlGtkPaned* paned = GDL_GTK_PANED(gdl_dock_item_get_child (GDL_DOCK_ITEM (object)));

	GtkWidget* children[] = {
		gdl_gtk_paned_get_start_child(paned),
		gdl_gtk_paned_get_end_child(paned),
	};

	for (int i=0;i<2;i++)
		if (children[i])
			fn (GDL_DOCK_OBJECT (children[i]), user_data);
}

static void
gdl_dock_paned_remove_widgets (GdlDockObject* object)
{
	// no direct widgets other than item->child
}

#ifdef DEBUG
static bool
gdl_dock_paned_validate (GdlDockObject* object)
{
	GdlGtkPaned* paned = GDL_GTK_PANED(gdl_dock_item_get_child (GDL_DOCK_ITEM (object)));

	if (!gdl_gtk_paned_get_start_child(paned))
		return false;
	if (!gdl_gtk_paned_get_end_child(paned)) {
		printf("Invalid layout: Paned is missing a child\n");
		return false;
	}
	return true;
}
#endif
