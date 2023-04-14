/*
 * gdl-dock-placeholder.c - Placeholders for docking items
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

#include "il8n.h"

#undef GDL_DISABLE_DEPRECATED

#include "gdl-dock-placeholder.h"
#include "gdl-dock-item.h"
#include "gdl-dock-paned.h"
#include "gdl-dock-master.h"
#include "libgdltypebuiltins.h"

/**
 * SECTION:gdl-dock-placeholder
 * @title: GdlDockPlaceHolder
 * @short_description: A widget marking a docking place.
 * @see_also:
 * @stability: Unstable
 *
 * A dock placeholder is a widget allowing to keep track of a docking place.
 * Unfortunately, all the details of the initial goal have been forgotten and
 * the code has still some issues.
 *
 * In GDL 3.6, this part has been deprecated. Placeholder widgets are not
 * used anymore. Instead, closed widgets are hidden but kept in the widget
 * hierarchy.
 */

#undef PLACEHOLDER_DEBUG

/* ----- Private prototypes ----- */

static void     gdl_dock_placeholder_class_init     (GdlDockPlaceholderClass *klass);

static void     gdl_dock_placeholder_set_property   (GObject                 *g_object,
                                                     guint                    prop_id,
                                                     const GValue            *value,
                                                     GParamSpec              *pspec);
static void     gdl_dock_placeholder_get_property   (GObject                 *g_object,
                                                     guint                    prop_id,
                                                     GValue                  *value,
                                                     GParamSpec              *pspec);

static void     gdl_dock_placeholder_dispose        (GObject                 *object);

#ifdef GTK4_TODO
static void     gdl_dock_placeholder_add            (GtkWidget               *container,
                                                     GtkWidget               *widget);
#endif

static void     gdl_dock_placeholder_detach         (GdlDockObject           *object,
                                                     gboolean                 recursive);
static void     gdl_dock_placeholder_reduce         (GdlDockObject           *object);
static void     gdl_dock_placeholder_dock           (GdlDockObject           *object,
                                                     GdlDockObject           *requestor,
                                                     GdlDockPlacement         position,
                                                     GValue                  *other_data);

static void     gdl_dock_placeholder_weak_notify    (gpointer                 data,
                                                     GObject                 *old_object);

static void     disconnect_host                     (GdlDockPlaceholder      *ph);
static void     connect_host                        (GdlDockPlaceholder      *ph,
                                                     GdlDockObject           *new_host);
static void     do_excursion                        (GdlDockPlaceholder      *ph);

static void     gdl_dock_placeholder_present        (GdlDockObject           *object,
                                                     GdlDockObject           *child);

static void     detach_cb                           (GdlDockObject           *object,
                                                     gboolean                 recursive,
                                                     gpointer                 user_data);

/* ----- Private variables and data structures ----- */

enum {
    PROP_0,
    PROP_STICKY,
    PROP_HOST,
    PROP_NEXT_PLACEMENT,
    PROP_WIDTH,
    PROP_HEIGHT,
    PROP_FLOATING,
    PROP_FLOAT_X,
    PROP_FLOAT_Y
};

struct _GdlDockPlaceholderPrivate {
    /* current object this placeholder is pinned to */
    GdlDockObject    *host;
    gboolean          sticky;

    /* when the placeholder is moved up the hierarchy, this stack
       keeps track of the necessary dock positions needed to get the
       placeholder to the original position */
    GSList           *placement_stack;

    /* Width and height of the attachments */
    gint              width;
    gint              height;

    /* connected signal handlers */
    guint             host_detach_handler;
    guint             host_dock_handler;

    /* Window Coordinates if Dock was floating */
    gboolean          floating;
    gint              floatx;
    gint              floaty;
};


/* ----- Private interface ----- */

G_DEFINE_TYPE_WITH_CODE (GdlDockPlaceholder, gdl_dock_placeholder, GDL_TYPE_DOCK_OBJECT, G_ADD_PRIVATE (GdlDockPlaceholder));

static void
gdl_dock_placeholder_class_init (GdlDockPlaceholderClass *klass)
{
    GObjectClass       *object_class;
    GdlDockObjectClass *dock_object_class;

    object_class = G_OBJECT_CLASS (klass);
    dock_object_class = GDL_DOCK_OBJECT_CLASS (klass);

    object_class->get_property = gdl_dock_placeholder_get_property;
    object_class->set_property = gdl_dock_placeholder_set_property;
    object_class->dispose = gdl_dock_placeholder_dispose;

#ifdef GTK4_TODO
    container_class->add = gdl_dock_placeholder_add;
#endif

    gdl_dock_object_class_set_is_compound (dock_object_class, FALSE);
    dock_object_class->detach = gdl_dock_placeholder_detach;
    dock_object_class->reduce = gdl_dock_placeholder_reduce;
    dock_object_class->dock = gdl_dock_placeholder_dock;
    dock_object_class->present = gdl_dock_placeholder_present;

    g_object_class_install_property (
        object_class, PROP_STICKY,
        g_param_spec_boolean ("sticky", _("Sticky"),
                              _("Whether the placeholder will stick to its host or "
                                "move up the hierarchy when the host is redocked"),
                              FALSE,
                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    g_object_class_install_property (
        object_class, PROP_HOST,
        g_param_spec_object ("host", _("Host"),
                            _("The dock object this placeholder is attached to"),
                            GDL_TYPE_DOCK_OBJECT,
                            G_PARAM_READWRITE));

    /* this will return the top of the placement stack */
    g_object_class_install_property (
        object_class, PROP_NEXT_PLACEMENT,
        g_param_spec_enum ("next-placement", _("Next placement"),
         					_("The position an item will be docked to our host if a "
           					"request is made to dock to us"),
                           	GDL_TYPE_DOCK_PLACEMENT,
                           	GDL_DOCK_CENTER,
                           	G_PARAM_READWRITE |
                           	GDL_DOCK_PARAM_EXPORT | GDL_DOCK_PARAM_AFTER));

    g_object_class_install_property (
        object_class, PROP_WIDTH,
        g_param_spec_int ("width", _("Width"),
                          	_("Width for the widget when it's attached to the placeholder"),
                          	-1, G_MAXINT, -1,
                          	G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                          	GDL_DOCK_PARAM_EXPORT));

    g_object_class_install_property (
        object_class, PROP_HEIGHT,
        g_param_spec_int ("height", _("Height"),
                     		_("Height for the widget when it's attached to the placeholder"),
                          	-1, G_MAXINT, -1,
                          	G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                          	GDL_DOCK_PARAM_EXPORT));
    g_object_class_install_property (
        object_class, PROP_FLOATING,
        g_param_spec_boolean ("floating", _("Floating Toplevel"),
                            _("Whether the placeholder is standing in for a "
                            "floating toplevel dock"),
                            FALSE,
                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (
        object_class, PROP_FLOAT_X,
        g_param_spec_int ("floatx", _("X Coordinate"),
                          	_("X coordinate for dock when floating"),
                          	-1, G_MAXINT, -1,
                          	G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                          	GDL_DOCK_PARAM_EXPORT));
    g_object_class_install_property (
        object_class, PROP_FLOAT_Y,
        g_param_spec_int ("floaty", _("Y Coordinate"),
                          	_("Y coordinate for dock when floating"),
                          	-1, G_MAXINT, -1,
                          	G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                          	GDL_DOCK_PARAM_EXPORT));
}

static void
gdl_dock_placeholder_init (GdlDockPlaceholder *ph)
{
	ph->priv = gdl_dock_placeholder_get_instance_private (ph);

#ifdef GTK4_TODO
    gtk_widget_set_has_window (GTK_WIDGET (ph), FALSE);
#endif
    gtk_widget_set_can_focus (GTK_WIDGET (ph), FALSE);
}

static void
gdl_dock_placeholder_set_property (GObject *g_object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    GdlDockPlaceholder *ph = GDL_DOCK_PLACEHOLDER (g_object);

    switch (prop_id) {
        case PROP_STICKY:
            if (ph->priv)
                ph->priv->sticky = g_value_get_boolean (value);
            break;
        case PROP_HOST:
            gdl_dock_placeholder_attach (ph, g_value_get_object (value));
            break;
        case PROP_NEXT_PLACEMENT:
            if (ph->priv) {
            	ph->priv->placement_stack = g_slist_prepend (ph->priv->placement_stack, GINT_TO_POINTER (g_value_get_enum (value)));
            }
            break;
        case PROP_WIDTH:
            ph->priv->width = g_value_get_int (value);
            break;
        case PROP_HEIGHT:
            ph->priv->height = g_value_get_int (value);
            break;
		case PROP_FLOATING:
			ph->priv->floating = g_value_get_boolean (value);
			break;
		case PROP_FLOAT_X:
			ph->priv->floatx = g_value_get_int (value);
			break;
		case PROP_FLOAT_Y:
			ph->priv->floaty = g_value_get_int (value);
			break;
        default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (g_object, prop_id, pspec);
			break;
    }
}

static void
gdl_dock_placeholder_get_property (GObject *g_object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    GdlDockPlaceholder *ph = GDL_DOCK_PLACEHOLDER (g_object);

    switch (prop_id) {
        case PROP_STICKY:
            if (ph->priv)
                g_value_set_boolean (value, ph->priv->sticky);
            else
                g_value_set_boolean (value, FALSE);
            break;
        case PROP_HOST:
            if (ph->priv)
                g_value_set_object (value, ph->priv->host);
            else
                g_value_set_object (value, NULL);
            break;
        case PROP_NEXT_PLACEMENT:
            if (ph->priv && ph->priv->placement_stack)
                g_value_set_enum (value, (GdlDockPlacement) ph->priv->placement_stack->data);
            else
                g_value_set_enum (value, GDL_DOCK_CENTER);
            break;
        case PROP_WIDTH:
            g_value_set_int (value, ph->priv->width);
            break;
        case PROP_HEIGHT:
            g_value_set_int (value, ph->priv->height);
            break;
		case PROP_FLOATING:
			g_value_set_boolean (value, ph->priv->floating);
			break;
		case PROP_FLOAT_X:
            g_value_set_int (value, ph->priv->floatx);
            break;
        case PROP_FLOAT_Y:
            g_value_set_int (value, ph->priv->floaty);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (g_object, prop_id, pspec);
            break;
    }
}

static void
gdl_dock_placeholder_dispose (GObject *object)
{
    GdlDockPlaceholder *ph = GDL_DOCK_PLACEHOLDER (object);

    if (ph->priv->host)
        gdl_dock_placeholder_detach (GDL_DOCK_OBJECT (object), FALSE);

    G_OBJECT_CLASS (gdl_dock_placeholder_parent_class)->dispose (object);
}

#ifdef GTK4_TODO
static void
gdl_dock_placeholder_add (GtkWidget *container, GtkWidget *widget)
{
    GdlDockPlaceholder *ph;
    GdlDockPlacement    pos = GDL_DOCK_CENTER;   /* default position */

    g_return_if_fail (GDL_IS_DOCK_PLACEHOLDER (container));
    g_return_if_fail (GDL_IS_DOCK_ITEM (widget));

    ph = GDL_DOCK_PLACEHOLDER (container);
    if (ph->priv->placement_stack)
        pos = (GdlDockPlacement) ph->priv->placement_stack->data;

    gdl_dock_object_dock (GDL_DOCK_OBJECT (ph), GDL_DOCK_OBJECT (widget), pos, NULL);
}
#endif

static void
gdl_dock_placeholder_detach (GdlDockObject *object, gboolean recursive)
{
    GdlDockPlaceholder *ph = GDL_DOCK_PLACEHOLDER (object);

    /* disconnect handlers */
    disconnect_host (ph);

    /* free the placement stack */
    g_slist_free (ph->priv->placement_stack);
    ph->priv->placement_stack = NULL;

    GDL_DOCK_OBJECT_UNSET_FLAGS (object, GDL_DOCK_ATTACHED);
}

static void
gdl_dock_placeholder_reduce (GdlDockObject *object)
{
    /* placeholders are not reduced */
    return;
}

static void
find_biggest_dock_item (GdlDockObject *container, GtkWidget **biggest_child, gint *biggest_child_area)
{
    GtkAllocation allocation;

    GList* children = gdl_dock_object_get_children (container);
    GList* child = children;
    while (child) {
        GtkWidget* child_widget = GTK_WIDGET (child->data);

        if (gdl_dock_object_is_compound (GDL_DOCK_OBJECT(child_widget))) {
            find_biggest_dock_item (GDL_DOCK_OBJECT (child_widget), biggest_child, biggest_child_area);
            child = g_list_next (child);
            continue;
        }
        gtk_widget_get_allocation (child_widget, &allocation);
        gint area = allocation.width * allocation.height;

        if (area > *biggest_child_area) {
            *biggest_child_area = area;
            *biggest_child = child_widget;
        }
        child = g_list_next (child);
    }
}

static void
attempt_to_dock_on_host (GdlDockPlaceholder *ph, GdlDockObject *host, GdlDockObject *requestor, GdlDockPlacement placement, gpointer other_data)
{
    GdlDockObject *parent;
    GtkAllocation  allocation;
    gint host_width;
    gint host_height;

    gtk_widget_get_allocation (GTK_WIDGET (host), &allocation);
    host_width = allocation.width;
    host_height = allocation.height;

    if (placement != GDL_DOCK_CENTER || !GDL_IS_DOCK_PANED (host)) {
        /* we simply act as a proxy for our host */
        gdl_dock_object_dock (host, requestor, placement, other_data);
    } else {
        /* If the requested pos is center, we have to make sure that it
         * does not colapses existing paned items. Find the larget item
         * which is not a paned item to dock to.
         */
        GtkWidget *biggest_child = NULL;
        gint biggest_child_area = 0;

        find_biggest_dock_item (host, &biggest_child, &biggest_child_area);

        if (biggest_child) {
            /* we simply act as a proxy for our host */
            gdl_dock_object_dock (GDL_DOCK_OBJECT (biggest_child), requestor, placement, other_data);
        } else {
            g_warning ("No suitable child found! Should not be here!");
            /* we simply act as a proxy for our host */
            gdl_dock_object_dock (GDL_DOCK_OBJECT (host), requestor, placement, other_data);
        }
    }

    parent = gdl_dock_object_get_parent_object (requestor);

    /* Restore dock item's dimention */
    switch (placement) {
        case GDL_DOCK_LEFT:
            if (ph->priv->width > 0) {
                g_object_set (G_OBJECT (parent), "position", ph->priv->width, NULL);
            }
            break;
        case GDL_DOCK_RIGHT:
            if (ph->priv->width > 0) {
                gint complementary_width = host_width - ph->priv->width;

                if (complementary_width > 0)
                    g_object_set (G_OBJECT (parent), "position", complementary_width, NULL);
            }
            break;
        case GDL_DOCK_TOP:
            if (ph->priv->height > 0) {
                g_object_set (G_OBJECT (parent), "position", ph->priv->height, NULL);
            }
            break;
        case GDL_DOCK_BOTTOM:
            if (ph->priv->height > 0) {
                gint complementary_height = host_height - ph->priv->height;

                if (complementary_height > 0)
                    g_object_set (G_OBJECT (parent), "position", complementary_height, NULL);
            }
            break;
        default:
            /* nothing */
            break;
    }
}

static void
gdl_dock_placeholder_dock (GdlDockObject *object, GdlDockObject *requestor, GdlDockPlacement  position, GValue *other_data)
{
    GdlDockPlaceholder *ph = GDL_DOCK_PLACEHOLDER (object);

    if (ph->priv->host) {
        attempt_to_dock_on_host (ph, ph->priv->host, requestor, position, other_data);
    }
    else {
        GdlDockObject *toplevel;

        if (!gdl_dock_object_is_bound (GDL_DOCK_OBJECT (ph))) {
            g_warning ("%s", _("Attempt to dock a dock object to an unbound placeholder"));
            return;
        }

        /* dock the item as a floating of the controller */
        toplevel = gdl_dock_object_get_controller (GDL_DOCK_OBJECT (ph));
        gdl_dock_object_dock (toplevel, requestor, GDL_DOCK_FLOATING, NULL);
    }
}

#ifdef PLACEHOLDER_DEBUG
static void
print_placement_stack (GdlDockPlaceholder *ph)
{
    GSList *s = ph->priv->placement_stack;
    GEnumClass *enum_class = G_ENUM_CLASS (g_type_class_ref (GDL_TYPE_DOCK_PLACEMENT));
    GEnumValue *enum_value;
    gchar *name;
    GString *message;

    message = g_string_new (NULL);
    g_string_printf (message, "[%p] host: %p (%s), stack: ",
                     ph, ph->priv->host, G_OBJECT_TYPE_NAME (ph->priv->host));
    for (; s; s = s->next) {
        enum_value = g_enum_get_value (enum_class, (GdlDockPlacement) s->data);
        name = enum_value ? enum_value->value_name : NULL;
        g_string_append_printf (message, "%s, ", name);
    }
    g_message ("%s", message->str);

    g_string_free (message, TRUE);
    g_type_class_unref (enum_class);
}
#endif

static void
gdl_dock_placeholder_present (GdlDockObject *object, GdlDockObject *child)
{
    /* do nothing */
    return;
}

/* ----- Public interface ----- */

/**
 * gdl_dock_placeholder_new:
 * @name: Unique name for identifying the dock object.
 * @object: Corresponding #GdlDockObject
 * @position: The position to dock a new item in @object
 * @sticky: %TRUE if the placeholder move with the @object
 *
 * Creates a new dock placeholder at @object place. This is a kind of marker
 * allowing you to dock new items later at this place. It is not completely
 * working though.
 *
 * Returns: The newly created placeholder.
 *
 * Deprecated: 3.6:
 */
GtkWidget *
gdl_dock_placeholder_new (const gchar *name, GdlDockObject *object, GdlDockPlacement position, gboolean sticky)
{
	GdlDockPlaceholder* ph = GDL_DOCK_PLACEHOLDER (g_object_new (GDL_TYPE_DOCK_PLACEHOLDER,
		"name", name,
		"sticky", sticky,
		"next-placement", position,
		"host", object,
		NULL)
	);
	GDL_DOCK_OBJECT_UNSET_FLAGS (ph, GDL_DOCK_AUTOMATIC);

	return GTK_WIDGET (ph);
}

static void
gdl_dock_placeholder_weak_notify (gpointer data, GObject *old_object)
{
    GdlDockPlaceholder *ph;

    g_return_if_fail (data != NULL && GDL_IS_DOCK_PLACEHOLDER (data));

    ph = GDL_DOCK_PLACEHOLDER (data);

#ifdef PLACEHOLDER_DEBUG
    g_message ("The placeholder just lost its host, ph = %p", ph);
#endif

    /* we shouldn't get here, so perform an emergency detach. instead
       we should have gotten a detach signal from our host */
    ph->priv->host = NULL;

    /* We didn't get a detach signal from the host. Detach from the
    supposedly dead host (consequently attaching to the controller) */

    detach_cb (NULL, TRUE, data);
#if 0
    /* free the placement stack */
    g_slist_free (ph->priv->placement_stack);
    ph->priv->placement_stack = NULL;
    GDL_DOCK_OBJECT_UNSET_FLAGS (ph, GDL_DOCK_ATTACHED);
#endif
}

static void
detach_cb (GdlDockObject *object, gboolean recursive, gpointer user_data)
{

    g_return_if_fail (user_data != NULL && GDL_IS_DOCK_PLACEHOLDER (user_data));

    /* we go up in the hierarchy and we store the hinted placement in
     * the placement stack so we can rebuild the docking layout later
     * when we get the host's dock signal.  */

    GdlDockPlaceholder* ph = GDL_DOCK_PLACEHOLDER (user_data);
    GdlDockObject* obj = ph->priv->host;
    if (obj != object) {
        g_warning (_("Got a detach signal from an object (%p) who is not our host %p"), object, ph->priv->host);
        return;
    }

    /* skip sticky objects */
    if (ph->priv->sticky)
        return;

    GdlDockObject* new_host;
    if (obj)
        /* go up in the hierarchy */
        new_host = gdl_dock_object_get_parent_object (obj);
    else
        /* Detaching from the dead host */
        new_host = NULL;

    while (new_host) {
        GdlDockPlacement pos = GDL_DOCK_NONE;

        /* get placement hint from the new host */
        if (gdl_dock_object_child_placement (new_host, obj, &pos)) {
            ph->priv->placement_stack = g_slist_prepend (
                ph->priv->placement_stack, (gpointer) pos);
        }
        else {
            g_warning (_("Something weird happened while getting the child "
                         "placement for %p from parent %p"), obj, new_host);
        }

        if (!GDL_DOCK_OBJECT_IN_DETACH (new_host))
            /* we found a "stable" dock object */
            break;

        obj = new_host;
        new_host = gdl_dock_object_get_parent_object (obj);
    }

    /* disconnect host */
    disconnect_host (ph);

    if (!new_host) {
#ifdef PLACEHOLDER_DEBUG
        g_message ("Detaching from the toplevel. Assignaing to controller");
#endif
        /* the toplevel was detached: we attach ourselves to the
           controller with an initial placement of floating */
        new_host = gdl_dock_object_get_controller (GDL_DOCK_OBJECT (ph));

        /*
        ph->priv->placement_stack = g_slist_prepend (
        ph->priv->placement_stack, (gpointer) GDL_DOCK_FLOATING);
        */
    }
    if (new_host)
        connect_host (ph, new_host);

#ifdef PLACEHOLDER_DEBUG
    print_placement_stack (ph);
#endif
}

/**
 * do_excursion:
 * @ph: placeholder object
 *
 * Tries to shrink the placement stack by examining the host's
 * children and see if any of them matches the placement which is at
 * the top of the stack.  If this is the case, it tries again with the
 * new host.
 *
 * Deprecated: 3.6:
 **/
static void
do_excursion (GdlDockPlaceholder *ph)
{
    if (ph->priv->host && !ph->priv->sticky && ph->priv->placement_stack && gdl_dock_object_is_compound (ph->priv->host)) {

        GdlDockPlacement stack_pos = (GdlDockPlacement) ph->priv->placement_stack->data;
        GdlDockObject* host = ph->priv->host;

        GList* children = gdl_dock_object_get_children (host);
        for (GList* l = children; l; l = l->next) {
            GdlDockPlacement pos = stack_pos;
            gdl_dock_object_child_placement (GDL_DOCK_OBJECT (host), GDL_DOCK_OBJECT (l->data), &pos);
            if (pos == stack_pos) {
                /* remove the stack position */
                ph->priv->placement_stack = g_slist_remove_link (ph->priv->placement_stack, ph->priv->placement_stack);

                /* connect to the new host */
                disconnect_host (ph);
                connect_host (ph, GDL_DOCK_OBJECT (l->data));

                /* recurse... */
                if (!GDL_DOCK_OBJECT_IN_REFLOW (l->data))
                    do_excursion (ph);

                break;
            }
        }
        g_list_free (children);
    }
}

static void
dock_cb (GdlDockObject *object, GdlDockObject *requestor, GdlDockPlacement  position, GValue *other_data, gpointer user_data)
{
    GdlDockPlacement    pos = GDL_DOCK_NONE;
    GdlDockPlaceholder *ph;

    g_return_if_fail (user_data != NULL && GDL_IS_DOCK_PLACEHOLDER (user_data));
    ph = GDL_DOCK_PLACEHOLDER (user_data);
    g_return_if_fail (ph->priv->host == object);

    /* see if the given position is compatible for the stack's top
       element */
    if (!ph->priv->sticky && ph->priv->placement_stack) {
        pos = (GdlDockPlacement) ph->priv->placement_stack->data;
        if (gdl_dock_object_child_placement (object, requestor, &pos)) {
            if (pos == (GdlDockPlacement) ph->priv->placement_stack->data) {
                /* the position is compatible: excurse down */
                do_excursion (ph);
            }
        }
    }
#ifdef PLACEHOLDER_DEBUG
    print_placement_stack (ph);
#endif
}

static void
disconnect_host (GdlDockPlaceholder *ph)
{
    if (!ph->priv->host)
        return;

    if (ph->priv->host_detach_handler)
        g_signal_handler_disconnect (ph->priv->host, ph->priv->host_detach_handler);
    if (ph->priv->host_dock_handler)
        g_signal_handler_disconnect (ph->priv->host, ph->priv->host_dock_handler);
    ph->priv->host_detach_handler = 0;
    ph->priv->host_dock_handler = 0;

    /* remove weak ref to object */
    g_object_weak_unref (G_OBJECT (ph->priv->host),
                         gdl_dock_placeholder_weak_notify, ph);
    ph->priv->host = NULL;

#ifdef PLACEHOLDER_DEBUG
    g_message ("Host just disconnected!, ph = %p", ph);
#endif
}

static void
connect_host (GdlDockPlaceholder *ph, GdlDockObject *new_host)
{
    if (ph->priv->host)
        disconnect_host (ph);

    ph->priv->host = new_host;
    g_object_weak_ref (G_OBJECT (ph->priv->host), gdl_dock_placeholder_weak_notify, ph);

    ph->priv->host_detach_handler =
        g_signal_connect (ph->priv->host, "detach", (GCallback) detach_cb, (gpointer) ph);

    ph->priv->host_dock_handler =
        g_signal_connect (ph->priv->host, "dock", (GCallback) dock_cb, (gpointer) ph);

#ifdef PLACEHOLDER_DEBUG
    g_message ("Host just connected!, ph = %p", ph);
#endif
}

/**
 * gdl_dock_placeholder_attach:
 * @ph: The #GdlDockPlaceholder object
 * @object: A new #GdlDockObject
 *
 * Move the placeholder to the position of @object.
 *
 * Deprecated: 3.6:
 */
void
gdl_dock_placeholder_attach (GdlDockPlaceholder *ph, GdlDockObject *object)
{
    g_return_if_fail (ph != NULL && GDL_IS_DOCK_PLACEHOLDER (ph));
    g_return_if_fail (ph->priv != NULL);
    g_return_if_fail (object != NULL);

    /* object binding */
    if (!gdl_dock_object_is_bound (GDL_DOCK_OBJECT (ph)))
        gdl_dock_object_bind (GDL_DOCK_OBJECT (ph), object->master);

    g_return_if_fail (GDL_DOCK_OBJECT (ph)->master == object->master);

    gdl_dock_object_freeze (GDL_DOCK_OBJECT (ph));

    /* detach from previous host first */
    if (ph->priv->host)
        gdl_dock_object_detach (GDL_DOCK_OBJECT (ph), FALSE);

    connect_host (ph, object);

    GDL_DOCK_OBJECT_SET_FLAGS (ph, GDL_DOCK_ATTACHED);

    gdl_dock_object_thaw (GDL_DOCK_OBJECT (ph));
}
