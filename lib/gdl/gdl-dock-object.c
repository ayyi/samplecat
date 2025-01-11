/*
 * gdl-dock-object.c - Abstract base class for all dock related objects
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
#include <stdlib.h>
#include <string.h>
#include "debug.h"

#include "gdl-dock-object.h"
#include "gdl-dock-master.h"
#include "libgdltypebuiltins.h"
#include "libgdlmarshal.h"

/* for later use by the registry */
#include "gdl-dock.h"
#include "gdl-dock-item.h"
#include "gdl-dock-paned.h"
#include "gdl-dock-notebook.h"
#include "gdl-dock-placeholder.h"
					#include "gdl-switcher.h"

/**
 * SECTION:gdl-dock-object
 * @title: GdlDockObject
 * @short_description: Base class for all dock objects
 * @stability: Unstable
 * @see_also: #GdlDockMaster
 *
 * A #GdlDockObject is an abstract class which defines the basic interface
 * for docking widgets.
 */

/* ----- Private prototypes ----- */

static void    gdl_dock_object_base_class_init     (GdlDockObjectClass* klass);
static void    gdl_dock_object_class_init          (GdlDockObjectClass* klass);
static void    gdl_dock_object_init                (GdlDockObject*      object);

static void     gdl_dock_object_set_property       (GObject            *g_object,
                                                    guint               prop_id,
                                                    const GValue       *value,
                                                    GParamSpec         *pspec);
static void     gdl_dock_object_get_property       (GObject            *g_object,
                                                    guint               prop_id,
                                                    GValue             *value,
                                                    GParamSpec         *pspec);
static void     gdl_dock_object_finalize           (GObject            *g_object);

static void     gdl_dock_object_destroy            (GObject            *dock_object);

static void     gdl_dock_object_show               (GtkWidget          *widget);
static void     gdl_dock_object_hide               (GtkWidget          *widget);

static void     gdl_dock_object_real_detach        (GdlDockObject      *object,
                                                    gboolean            recursive);
static void     gdl_dock_object_real_reduce        (GdlDockObject      *object);
static void     gdl_dock_object_dock_unimplemented (GdlDockObject      *object,
                                                    GdlDockObject      *requestor,
                                                    GdlDockPlacement    position,
                                                    GValue             *other_data);
static void     gdl_dock_object_real_present       (GdlDockObject      *object,
                                                    GdlDockObject      *child);
static GList*   gdl_dock_object_children           (GdlDockObject      *object);


/* ----- Private data types and variables ----- */

enum {
    PROP_0,
    PROP_NAME,
    PROP_LONG_NAME,
    PROP_STOCK_ID,
    PROP_PIXBUF_ICON,
    PROP_MASTER,
    PROP_EXPORT_PROPERTIES,
    PROP_LAST
};

enum {
    DETACH,
    DOCK,
    LAST_SIGNAL
};

static guint gdl_dock_object_signals [LAST_SIGNAL] = { 0 };

static GParamSpec *properties[PROP_LAST];

struct _GdlDockObjectPrivate {
    guint               automatic : 1;
    guint               attached : 1;
    gint                freeze_count;

    gchar              *name;
    gchar              *long_name;
    gchar              *stock_id;
    GdkPixbuf          *pixbuf_icon;

    gboolean            reduce_pending;
};

struct _GdlDockObjectClassPrivate {
    gboolean           is_compound;
};

struct DockRegisterItem {
	gchar* nick;
	gpointer type;
};

static GArray *dock_register = NULL;
//static gint GdlDockObject_private_offset;

/* ----- Private interface ----- */

/* It is not possible to use G_DEFINE_TYPE_* macro for GdlDockObject because it
 * has some class private data. The priv pointer has to be initialized in
 * the base class initialization function because this function is called for
 * each derived type.
 */
static gpointer gdl_dock_object_parent_class = NULL;

GType
gdl_dock_object_get_type (void)
{
	static GType gtype = 0;
    //static GType g_define_type_id = 0;

	if (G_UNLIKELY (gtype == 0)) {
		const GTypeInfo gtype_info = {
			sizeof (GdlDockObjectClass),
			(GBaseInitFunc) gdl_dock_object_base_class_init,
			NULL,
			(GClassInitFunc) gdl_dock_object_class_init,
			NULL,       /* class_finalize */
			NULL,       /* class_data */
			sizeof (GdlDockObject),
			0,          /* n_preallocs */
			(GInstanceInitFunc) gdl_dock_object_init,
			NULL,       /* value_table */
		};

		gtype = g_type_register_static (GTK_TYPE_WIDGET, "GdlDockObject", &gtype_info, 0);

		g_type_add_class_private (gtype, sizeof (GdlDockObjectClassPrivate));
		//g_define_type_id = gtype;
		//G_ADD_PRIVATE (GdlDockObject);
	}

	return gtype;
}

static void
gdl_dock_object_base_class_init (GdlDockObjectClass *klass)
{
    klass->priv = G_TYPE_CLASS_GET_PRIVATE (klass, GDL_TYPE_DOCK_OBJECT, GdlDockObjectClassPrivate);
}

static void
gdl_dock_object_class_init (GdlDockObjectClass *klass)
{
    gdl_dock_object_parent_class = g_type_class_peek_parent (klass);

    GObjectClass* object_class = G_OBJECT_CLASS (klass);
    GtkWidgetClass* widget_class = GTK_WIDGET_CLASS (klass);

    klass->children = gdl_dock_object_children;

    object_class->set_property = gdl_dock_object_set_property;
    object_class->get_property = gdl_dock_object_get_property;
    object_class->finalize = gdl_dock_object_finalize;

    /**
     * GdlDockObject:name:
     *
     * The object name.  If the object is manual the name can be used
     * to recall the object from any other object in the ring
     */
    properties[PROP_NAME] =
        g_param_spec_string (GDL_DOCK_NAME_PROPERTY, _("Name"),
                             _("Unique name for identifying the dock object"),
                             NULL,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                             GDL_DOCK_PARAM_EXPORT);

    g_object_class_install_property (object_class, PROP_NAME, properties[PROP_NAME]);

    /**
     * GdlDockObject:long-name:
     *
     * A long descriptive name.
     */
    properties[PROP_LONG_NAME] =
        g_param_spec_string ("long-name", _("Long name"),
                             _("Human readable name for the dock object"),
                             NULL,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

    g_object_class_install_property (object_class, PROP_LONG_NAME, properties[PROP_LONG_NAME]);

    /**
     * GdlDockObject:stock-id:
     *
     * A stock id to use for the icon of the dock object.
     */
    properties[PROP_STOCK_ID] =
        g_param_spec_string ("stock-id", _("Stock Icon"),
                             _("Stock icon for the dock object"),
                             NULL,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

    g_object_class_install_property (object_class, PROP_STOCK_ID, properties[PROP_STOCK_ID]);

    /**
     * GdlDockObject:pixbuf-icon:
     *
     * A GdkPixbuf to use for the icon of the dock object.
     *
     * Since: 3.3.2
     */
    properties[PROP_PIXBUF_ICON] =
        g_param_spec_pointer ("pixbuf-icon", _("Pixbuf Icon"),
                              _("Pixbuf icon for the dock object"),
                              G_PARAM_READWRITE);

    g_object_class_install_property (object_class, PROP_PIXBUF_ICON, properties[PROP_PIXBUF_ICON]);

    /**
     * GdlDockObject:master:
     *
     * The master which manages all the objects in a dock ring
     */
    properties[PROP_MASTER] =
        g_param_spec_object ("master", _("Dock master"),
                             _("Dock master this dock object is bound to"),
                             GDL_TYPE_DOCK_MASTER,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

    g_object_class_install_property (object_class, PROP_MASTER, properties[PROP_MASTER]);

    object_class->dispose = gdl_dock_object_destroy;

    widget_class->show = gdl_dock_object_show;
    widget_class->hide = gdl_dock_object_hide;

    klass->priv->is_compound = TRUE;

    klass->detach = gdl_dock_object_real_detach;
    klass->reduce = gdl_dock_object_real_reduce;
    klass->dock_request = NULL;
    klass->dock = gdl_dock_object_dock_unimplemented;
    klass->reorder = NULL;
    klass->present = gdl_dock_object_real_present;
    klass->child_placement = NULL;

    /**
     * GdlDockObject::detach:
     * @item: The detached dock object.
     * @recursive: %TRUE if children have to be detached too.
     *
     * Signals that the #GdlDockObject is detached.
     **/
    gdl_dock_object_signals [DETACH] =
        g_signal_new ("detach",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (GdlDockObjectClass, detach),
                      NULL,
                      NULL,
                      gdl_marshal_VOID__BOOLEAN,
                      G_TYPE_NONE,
                      1,
                      G_TYPE_BOOLEAN);

    /**
     * GdlDockObject::dock:
     * @item: The docked dock object.
     * @requestor: The widget to dock
     * @position: The position for the child
     * @other_data: (allow-none): Optional data giving additional information
     *
     * Signals that the #GdlDockObject has been docked.
     **/
    gdl_dock_object_signals [DOCK] =
        g_signal_new ("dock",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (GdlDockObjectClass, dock),
                      NULL,
                      NULL,
                      gdl_marshal_VOID__OBJECT_ENUM_BOXED,
                      G_TYPE_NONE,
                      3,
                      GDL_TYPE_DOCK_OBJECT,
                      GDL_TYPE_DOCK_PLACEMENT,
                      G_TYPE_VALUE);

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	g_type_class_add_private (object_class, sizeof (GdlDockObjectPrivate));
#pragma GCC diagnostic warning "-Wdeprecated-declarations"
}

static void
gdl_dock_object_init (GdlDockObject *object)
{
	object->priv = (GdlDockObjectPrivate*) g_type_instance_get_private ((GTypeInstance*)object, GDL_TYPE_DOCK_OBJECT);

    object->priv->automatic = TRUE;
    object->priv->freeze_count = 0;
#ifndef GDL_DISABLE_DEPRECATED
    object->deprecated_flags = 0;
#endif
}

static void
gdl_dock_object_set_property  (GObject *g_object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    GdlDockObject *object = GDL_DOCK_OBJECT (g_object);

    switch (prop_id) {
    case PROP_NAME:
        gdl_dock_object_set_name (object, g_value_get_string (value));
        break;
    case PROP_LONG_NAME:
        gdl_dock_object_set_long_name (object, g_value_get_string (value));
        break;
    case PROP_STOCK_ID:
        gdl_dock_object_set_stock_id (object, g_value_get_string (value));
        break;
    case PROP_PIXBUF_ICON:
        gdl_dock_object_set_pixbuf (object,  g_value_get_pointer (value));
        break;
    case PROP_MASTER:
        if (g_value_get_object (value))
            gdl_dock_object_bind (object, g_value_get_object (value));
        else
            gdl_dock_object_unbind (object);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
gdl_dock_object_get_property  (GObject *g_object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	GdlDockObject *object = GDL_DOCK_OBJECT (g_object);

	switch (prop_id) {
		case PROP_NAME:
			g_value_set_string (value, object->priv->name);
			break;
		case PROP_LONG_NAME:
			g_value_set_string (value, object->priv->long_name);
			break;
		case PROP_STOCK_ID:
			g_value_set_string (value, object->priv->stock_id);
			break;
		case PROP_PIXBUF_ICON:
			g_value_set_pointer (value, object->priv->pixbuf_icon);
			break;
		case PROP_MASTER:
			g_value_set_object (value, object->master);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gdl_dock_object_finalize (GObject *g_object)
{
	GdlDockObject *object = GDL_DOCK_OBJECT (g_object);

	g_free (object->priv->name);
	g_free (object->priv->long_name);
	g_free (object->priv->stock_id);
	g_clear_pointer (&object->priv->pixbuf_icon, g_object_unref);

	G_OBJECT_CLASS (gdl_dock_object_parent_class)->finalize (g_object);
}

static void
gdl_dock_object_foreach_detach (GdlDockObject *object, gpointer user_data)
{
    gdl_dock_object_detach (object, TRUE);
}

static void
gdl_dock_object_destroy (GObject* g_object)
{
	GtkWidget *dock_object = (GtkWidget*)g_object;

    g_return_if_fail (GDL_IS_DOCK_OBJECT (dock_object));

    GdlDockObject *object = GDL_DOCK_OBJECT (dock_object);
    if (gdl_dock_object_is_compound (object)) {
        /* detach our dock object children if we have some, and even if we are not attached, so they can get notification */
        gdl_dock_object_freeze (object);
		gdl_dock_object_foreach_child (object, gdl_dock_object_foreach_detach, NULL);
        object->priv->reduce_pending = FALSE;
        gdl_dock_object_thaw (object);
    }
    /* detach ourselves */
    gdl_dock_object_detach (object, FALSE);

    /* finally unbind us */
    if (object->master)
        gdl_dock_object_unbind (object);

    G_OBJECT_CLASS(gdl_dock_object_parent_class)->dispose (g_object);
}

static void
gdl_dock_object_foreach_is_visible (GdlDockObject *object, gpointer user_data)
{
	gboolean *visible = (gboolean *)user_data;

	if (!*visible && gtk_widget_get_visible (GTK_WIDGET (object))) *visible = TRUE;
}

static void
gdl_dock_object_update_visibility (GdlDockObject *object)
{
	g_return_if_fail (object != NULL);

	if (object && gdl_dock_object_is_automatic (object)) {
		gboolean visible = FALSE;

		gdl_dock_object_foreach_child (object, gdl_dock_object_foreach_is_visible, &visible);
		object->priv->attached = visible;
#ifndef GDL_DISABLE_DEPRECATED
		if (visible)
			object->deprecated_flags |= GDL_DOCK_ATTACHED;
		else
			object->deprecated_flags &= ~GDL_DOCK_ATTACHED;
#endif
		gtk_widget_set_visible (GTK_WIDGET (object), visible);
	}
	gdl_dock_object_layout_changed_notify (object);
}

static void
gdl_dock_object_update_parent_visibility (GdlDockObject *object)
{
    g_return_if_fail (object);

    GdlDockObject* parent = gdl_dock_object_get_parent_object (object);
    if (parent != NULL) gdl_dock_object_update_visibility (parent);
}

#if 0
static void
gdl_dock_object_foreach_automatic (GdlDockObject *object, gpointer user_data)
{
    void (* function) (GtkWidget *) = user_data;

    if (gdl_dock_object_is_automatic (object))
        (* function) (GTK_WIDGET (object));
}
#endif

static void
gdl_dock_object_show (GtkWidget *widget)
{
    GDL_DOCK_OBJECT (widget)->priv->attached = TRUE;
#ifndef GDL_DISABLE_DEPRECATED
    GDL_DOCK_OBJECT (widget)->deprecated_flags |= GDL_DOCK_ATTACHED;
#endif
    GTK_WIDGET_CLASS (gdl_dock_object_parent_class)->show (widget);

    /* Update visibility of automatic parents */
    gdl_dock_object_update_parent_visibility (GDL_DOCK_OBJECT (widget));
}

static void
gdl_dock_object_hide (GtkWidget *widget)
{
    GDL_DOCK_OBJECT (widget)->priv->attached = FALSE;
#ifndef GDL_DISABLE_DEPRECATED
    GDL_DOCK_OBJECT (widget)->deprecated_flags &= ~GDL_DOCK_ATTACHED;
#endif
   GTK_WIDGET_CLASS (gdl_dock_object_parent_class)->hide (widget);

    /* Update visibility of automatic parents */
    gdl_dock_object_update_parent_visibility (GDL_DOCK_OBJECT (widget));
}

static void
gdl_dock_object_real_detach (GdlDockObject *object, gboolean recursive)
{
    g_return_if_fail (object);

    /* detach children */
    if (recursive && gdl_dock_object_is_compound (object)) {
        gdl_dock_object_foreach_child (object, (GdlDockObjectFn)gdl_dock_object_detach, GINT_TO_POINTER (recursive));
    }

    /* detach the object itself */
    object->priv->attached = FALSE;
#ifndef GDL_DISABLE_DEPRECATED
    object->deprecated_flags &= ~GDL_DOCK_ATTACHED;
#endif

   	GdlDockObject* dock_parent = gdl_dock_object_get_parent_object (object);
	{
    	GtkWidget* widget = GTK_WIDGET (object);
		GtkWidget* parent = gtk_widget_get_parent (widget);
	    if (parent) {
			if (GTK_IS_STACK(parent)) {
				gtk_stack_remove(GTK_STACK (parent), widget);
			} else {
				if (!GDL_IS_DOCK_OBJECT(parent))
					parent = gtk_widget_get_parent(parent);
				if (GTK_IS_NOTEBOOK (parent)) {
					parent = gtk_widget_get_parent(parent);
				}
				gdl_dock_object_remove_child (GDL_DOCK_OBJECT (parent), widget);
			}
		}
	}

	{
	    if (dock_parent)
    	    gdl_dock_object_reduce (dock_parent);
	}
}

static void
gdl_dock_object_real_reduce (GdlDockObject *object)
{
	 g_return_if_fail (object != NULL);

	if (!gdl_dock_object_is_compound (object))
		return;

	ENTER;

	GdlDockObject* parent = gdl_dock_object_get_parent_object (object);
    GList* children = gdl_dock_object_get_children (object);
    if (g_list_length (children) <= 1) {
        GList *dchildren = NULL;

        /* detach ourselves and then re-attach our children to our
           current parent.  if we are not currently attached, the
           children are detached */
        if (parent)
            gdl_dock_object_freeze (parent);
        gdl_dock_object_freeze (object);
        /* Detach the children before detaching this object, since in this
         * way the children can have access to the whole object hierarchy.
         * Set the InDetach flag now, so the children know that this object
         * is going to be detached. */

        for (GList* l = children; l; l = l->next) {

            if (!GDL_IS_DOCK_OBJECT (l->data))
                continue;

            GdlDockObject *child = GDL_DOCK_OBJECT (l->data);

            g_object_ref (child);
            gdl_dock_object_detach (child, FALSE);
            if (parent)
                dchildren = g_list_append (dchildren, child);
        }
        /* Now it can be detached */
        gdl_dock_object_detach (object, FALSE);

        /* After detaching the reduced object, we can add the
        children (the only child in fact) to the new parent */
        for (GList* l = dchildren; l; l = l->next) {
            gdl_dock_object_add_child (parent, l->data);
            g_object_unref (l->data);
        }
        g_list_free (dchildren);


        /* sink the widget, so any automatic floating widget is destroyed */
        g_object_ref_sink (object);
        /* don't reenter */
        object->priv->reduce_pending = FALSE;
        gdl_dock_object_thaw (object);
        if (parent)
            gdl_dock_object_thaw (parent);
    }
    g_list_free (children);
}

static void
gdl_dock_object_dock_unimplemented (GdlDockObject *object, GdlDockObject *requestor, GdlDockPlacement position, GValue *other_data)
{
    g_warning (_("Call to gdl_dock_object_dock in a dock object %p "
                 "(object type is %s) which hasn't implemented this method"),
               object, G_OBJECT_TYPE_NAME (object));
}

static void
gdl_dock_object_real_present (GdlDockObject *object, GdlDockObject *child)
{
    gtk_widget_set_visible (GTK_WIDGET (object), true);
}


static GList*
gdl_dock_object_children (GdlDockObject* object)
{
	GList* children = NULL;

	GtkWidget* child = gtk_widget_get_first_child (GTK_WIDGET (object));
	for (; child; child = gtk_widget_get_next_sibling (child)) {
		children = g_list_append(children, child);
	}

	return children;
}

/* ----- Public interface ----- */

/**
 * gdl_dock_object_is_compound:
 * @object: A #GdlDockObject
 *
 * Check if an object is a compound object, accepting children widget or not.
 *
 * Returns: %TRUE if @object is a compound object.
 */
gboolean
gdl_dock_object_is_compound (GdlDockObject *object)
{
    g_return_val_if_fail (object != NULL, FALSE);
    g_return_val_if_fail (GDL_IS_DOCK_OBJECT (object), FALSE);

    return GDL_DOCK_OBJECT_GET_CLASS (object)->priv->is_compound;
}

/**
 * gdl_dock_object_detach:
 * @object: A #GdlDockObject
 * @recursive: %TRUE to detach children
 *
 * Dissociate a dock object from its parent, including or not its children.
 */
void
gdl_dock_object_detach (GdlDockObject *object, gboolean recursive)
{
    g_return_if_fail (object != NULL);

	ENTER;
    if (!GDL_IS_DOCK_OBJECT (object))
        return;

    if (!object->priv->attached && (gtk_widget_get_parent (GTK_WIDGET (object)) == NULL))
        return;

    /* freeze the object to avoid reducing while detaching children */
    gdl_dock_object_freeze (object);
    g_signal_emit (object, gdl_dock_object_signals [DETACH], 0, recursive);
    gdl_dock_object_thaw (object);
}

void
gdl_dock_object_add_child (GdlDockObject* object, GtkWidget* child)
{
	ENTER;

#if 0
	if (GDL_IS_DOCK_PANED (object) && GDL_IS_DOCK_ITEM(child)) {
		cdbg(0, "cannot put dock-item in a GdlPaned ?");
		g_assert(false);
	}
#endif

	if (GDL_IS_DOCK_PANED (object)) {
		gdl_dock_paned_add (GDL_DOCK_PANED(object), child);
	}
	else if (GDL_IS_DOCK_NOTEBOOK (object)) {
		gdl_dock_notebook_add (object, child);
	}
	else if (GDL_IS_DOCK_ITEM (object) && !GDL_IS_DOCK_PANED (object)) {
		gdl_dock_item_add (GDL_DOCK_ITEM (object), child);
	}
	else if (GDL_IS_DOCK (object)) {
		gdl_dock_add (GDL_DOCK (object), child);
		gtk_widget_insert_before (child, GTK_WIDGET (object), NULL);
	}
}

void
gdl_dock_object_remove_child (GdlDockObject* object, GtkWidget* child)
{
	ENTER;

	/*
	if (GTK_IS_NOTEBOOK (object)) {
		gdl_dock_notebook_remove (object, child);
	} else */if (GDL_IS_DOCK_PANED (object)) {
		gdl_dock_paned_remove_child (object, GDL_DOCK_ITEM (child));
	} else if (GDL_IS_DOCK_ITEM (object)) {
		gtk_widget_unparent(child);
		gdl_dock_item_remove (GDL_DOCK_ITEM (object), child);
	} else if (GDL_IS_DOCK (object)) {
		gtk_widget_unparent(child);
		gdl_dock_remove (GDL_DOCK (object), child);
	}
}

/**
 * gdl_dock_object_get_parent_object:
 * @object: A #GdlDockObject
 *
 * Returns a parent #GdlDockObject if it exists.
 *
 * Returns: (allow-none) (transfer none): a #GdlDockObject or %NULL if such object does not exist.
 */
GdlDockObject *
gdl_dock_object_get_parent_object (GdlDockObject *object)
{
    g_return_val_if_fail (object != NULL, NULL);

    GtkWidget* parent = gtk_widget_get_parent (GTK_WIDGET (object));
    while (parent && !GDL_IS_DOCK_OBJECT (parent)) {
        parent = gtk_widget_get_parent (parent);
    }

    return parent ? GDL_DOCK_OBJECT (parent) : NULL;
}

GList*
gdl_dock_object_get_children (GdlDockObject *object)
{
    return GDL_DOCK_OBJECT_GET_CLASS (object)->children (object);
}

void
gdl_dock_object_foreach_child (GdlDockObject *object, GdlDockObjectFn fn, gpointer data)
{
    g_return_if_fail (object);

	if (GDL_IS_DOCK_PANED (object)/* || GDL_DOCK_NOTEBOOK (object)*/) {
		GDL_DOCK_OBJECT_GET_CLASS (object)->foreach_child (object, fn, data);
		return;
	}
	GtkWidget* child = gtk_widget_get_first_child (GTK_WIDGET (object));
	for (; child; child = gtk_widget_get_next_sibling (child)) {
		if (GDL_IS_DOCK_OBJECT (child)) {
			fn (GDL_DOCK_OBJECT (child), data);
		} else if (GDL_IS_SWITCHER (child)) {
			GDL_DOCK_OBJECT_GET_CLASS (object)->foreach_child (object, fn, data);
		}
	}
}

/**
 * gdl_dock_object_freeze:
 * @object: A #GdlDockObject
 *
 * Temporarily freezes a dock object, any call to reduce on the object has no
 * immediate effect. If gdl_dock_object_freeze() has been called more than once,
 * gdl_dock_object_thaw() must be called an equal number of times.
 */
void
gdl_dock_object_freeze (GdlDockObject *object)
{
    g_return_if_fail (object != NULL);

    if (object->priv->freeze_count == 0) {
        g_object_ref (object);   /* dock objects shouldn't be destroyed if they are frozen */
    }
    object->priv->freeze_count++;
}

/**
 * gdl_dock_object_thaw:
 * @object: A #GdlDockObject
 *
 * Thaws a dock object frozen with gdl_dock_object_freeze().
 * Any pending reduce calls are made, maybe leading to the destruction of
 * the object.
 */
void
gdl_dock_object_thaw (GdlDockObject *object)
{
    g_return_if_fail (object != NULL);
    g_return_if_fail (object->priv->freeze_count > 0);

    object->priv->freeze_count--;
    if (object->priv->freeze_count == 0) {
        if (object->priv->reduce_pending) {
            object->priv->reduce_pending = FALSE;
            gdl_dock_object_reduce (object);
        }
        g_object_unref (object);
    }
}

/**
 * gdl_dock_object_is_frozen:
 * @object: A #GdlDockObject
 *
 * Determine if an object is frozen and is not removed immediately from the
 * widget hierarchy when it is reduced.
 *
 * Return value: %TRUE if the object is frozen.
 *
 * Since: 3.6
 */
gboolean
gdl_dock_object_is_frozen (GdlDockObject *object)
{
    g_return_val_if_fail (GDL_IS_DOCK_OBJECT (object), FALSE);

    return object->priv->freeze_count > 0;
}

/**
 * gdl_dock_object_reduce:
 * @object: A #GdlDockObject
 *
 * Remove a compound object if it is not longer useful to hold the child. The
 * object has to be removed and the child reattached to the parent.
 */
void
gdl_dock_object_reduce (GdlDockObject *object)
{
	g_return_if_fail (object);

	if (gdl_dock_object_is_frozen (object)) {
		object->priv->reduce_pending = TRUE;
		return;
	}

	if (GDL_DOCK_OBJECT_GET_CLASS (object)->reduce)
		GDL_DOCK_OBJECT_GET_CLASS (object)->reduce (object);
}

/**
 * gdl_dock_object_dock_request:
 * @object: A #GdlDockObject
 * @x: X coordinate
 * @y: Y coordinate
 * @request: A #GdlDockRequest with information about the docking position
 *
 * Dock a dock widget in @object at the defined position.
 *
 * Returns: %TRUE if @object has been docked.
 */
gboolean
gdl_dock_object_dock_request (GdlDockObject *object, gint x, gint y, GdlDockRequest *request)
{
    g_return_val_if_fail (object != NULL && request != NULL, FALSE);

    if (GDL_DOCK_OBJECT_GET_CLASS (object)->dock_request)
        return GDL_DOCK_OBJECT_GET_CLASS (object)->dock_request (object, x, y, request);
    else
        return FALSE;
}

/**
 * gdl_dock_object_dock:
 * @object: A #GdlDockObject
 * @requestor: The widget to dock
 * @position: The position for the child
 * @other_data: (allow-none): Optional data giving additional information
 * depending on the dock object.
 *
 * Dock a dock widget in @object at the defined position.
 */
void
gdl_dock_object_dock (GdlDockObject *object, GdlDockObject *requestor, GdlDockPlacement position, GValue *other_data)
{
	g_return_if_fail (object != NULL && requestor != NULL);

	if (object == requestor)
		return;

	if (!object->master)
		g_warning (_("Dock operation requested in a non-bound object %p. The application might crash"), object);

	ENTER;

    if (!gdl_dock_object_is_bound (requestor))
        gdl_dock_object_bind (requestor, object->master);

    if (requestor->master != object->master) {
        g_warning (_("Cannot dock %p to %p because they belong to different masters"), requestor, object);
        return;
    }

    /* first, see if we can optimize things by reordering */
    if (position != GDL_DOCK_NONE) {
        GdlDockObject* parent = gdl_dock_object_get_parent_object (object);
        if (
			gdl_dock_object_reorder (object, requestor, position, other_data)
	        || (parent && gdl_dock_object_reorder (parent, requestor, position, other_data))
		)
            return;
    }

    /* freeze the object, since under some conditions it might be destroyed when
       detaching the requestor */
    gdl_dock_object_freeze (object);

    /* detach the requestor before docking */
    g_object_ref (requestor);
    GdlDockObject *parent = gdl_dock_object_get_parent_object (requestor);
    if (parent) g_object_ref (parent);
    gdl_dock_object_detach (requestor, FALSE);

    if (position != GDL_DOCK_NONE)
        g_signal_emit (object, gdl_dock_object_signals [DOCK], 0, requestor, position, other_data);

    g_object_unref (requestor);
    gdl_dock_object_thaw (object);

    if (gtk_widget_get_visible (GTK_WIDGET (requestor))) {
        requestor->priv->attached = TRUE;
#ifndef GDL_DISABLE_DEPRECATED
        requestor->deprecated_flags |= GDL_DOCK_ATTACHED;
#endif
    }
    /* Update visibility of automatic parents */
    if (parent != NULL) {
        gdl_dock_object_update_visibility (parent);
        g_object_unref (parent);
    }
    gdl_dock_object_update_parent_visibility (GDL_DOCK_OBJECT (requestor));
}

/**
 * gdl_dock_object_bind:
 * @object: A #GdlDockObject
 * @master: A #GdlDockMaster
 *
 * Add a link between a #GdlDockObject and a master. It is normally not used
 * directly because it is automatically called when a new object is docked.
 */
void
gdl_dock_object_bind (GdlDockObject *object, GObject *master)
{
    g_return_if_fail (object != NULL && master != NULL);
    g_return_if_fail (GDL_IS_DOCK_MASTER (master));

    if (object->master == master)
        /* nothing to do here */
        return;

    if (object->master) {
        g_warning (_("Attempt to bind to %p an already bound dock object %p (current master: %p)"), master, object, object->master);
        return;
    }

    gdl_dock_master_add (GDL_DOCK_MASTER (master), object);
    object->master = master;
    g_object_add_weak_pointer (master, (gpointer *) &object->master);

    g_object_notify (G_OBJECT (object), "master");
}

/**
 * gdl_dock_object_unbind:
 * @object: A #GdlDockObject
 *
 * This removes the link between an dock object and its master.
 */
void
gdl_dock_object_unbind (GdlDockObject *object)
{
    g_return_if_fail (object != NULL);

    g_object_ref (object);

    /* detach the object first */
    gdl_dock_object_detach (object, TRUE);

    if (object->master) {
        GObject *master = object->master;
        g_object_remove_weak_pointer (master, (gpointer *) &object->master);
        object->master = NULL;
        gdl_dock_master_remove (GDL_DOCK_MASTER (master), object);
        g_object_notify (G_OBJECT (object), "master");
    }
    g_object_unref (object);
}

/**
 * gdl_dock_object_is_bound:
 * @object: A #GdlDockObject
 *
 * Check if the object is bound to a master.
 *
 * Returns: %TRUE if @object has a master
 */
gboolean
gdl_dock_object_is_bound (GdlDockObject *object)
{
    g_return_val_if_fail (object != NULL, FALSE);
    return (object->master != NULL);
}

/**
 * gdl_dock_object_get_master:
 * @object: a #GdlDockObject
 *
 * Retrieves the master of the object.
 *
 * Return value: (transfer none): a #GdlDockMaster object
 *
 * Since: 3.6
 */
GObject *
gdl_dock_object_get_master (GdlDockObject *object)
{
    g_return_val_if_fail (GDL_IS_DOCK_OBJECT (object), NULL);

    return object->master;
}

/**
 * gdl_dock_object_get_controller:
 * @object: a #GdlDockObject
 *
 * Retrieves the controller of the object.
 *
 * Return value: (transfer none): a #GdlDockObject object
 *
 * Since: 3.6
 */
GdlDockObject *
gdl_dock_object_get_controller (GdlDockObject *object)
{
    g_return_val_if_fail (GDL_IS_DOCK_OBJECT (object), NULL);

    return gdl_dock_master_get_controller (GDL_DOCK_MASTER (object->master));
}

/**
 * gdl_dock_object_layout_changed_notify:
 * @object: a #GdlDockObject
 *
 * Emits the #GdlDockMaster::layout-changed signal on the master of the object
 * if existing.
 *
 * Since: 3.6
 **/
void
gdl_dock_object_layout_changed_notify (GdlDockObject *object)
{
    if (object->master)
        g_signal_emit_by_name (object->master, "layout-changed");
}


/**
 * gdl_dock_object_reorder:
 * @object: A #GdlDockObject
 * @child: The child widget to reorder
 * @new_position: New position for the child
 * @other_data: (allow-none): Optional data giving additional information
 * depending on the dock object.
 *
 * Move the @child widget at another place.
 *
 * Returns: %TRUE if @child has been moved
 */
gboolean
gdl_dock_object_reorder (GdlDockObject *object, GdlDockObject *child, GdlDockPlacement new_position, GValue *other_data)
{
	g_return_val_if_fail (object != NULL && child != NULL, FALSE);

	if (GDL_DOCK_OBJECT_GET_CLASS (object)->reorder)
		return GDL_DOCK_OBJECT_GET_CLASS (object)->reorder (object, child, new_position, other_data);
	else
		return FALSE;
}

/**
 * gdl_dock_object_present:
 * @object: A #GdlDockObject
 * @child: (allow-none): The child widget to present or %NULL
 *
 * Presents the GDL object to the user. By example, this will select the
 * corresponding page if the object is in a notebook. If @child is missing,
 * only the @object will be show.
 */
void
gdl_dock_object_present (GdlDockObject *object, GdlDockObject *child)
{
    g_return_if_fail (object != NULL && GDL_IS_DOCK_OBJECT (object));

    GdlDockObject *parent = gdl_dock_object_get_parent_object (object);
    if (parent)
        /* chain the call to our parent */
        gdl_dock_object_present (parent, object);

   if (GDL_DOCK_OBJECT_GET_CLASS (object)->present)
        GDL_DOCK_OBJECT_GET_CLASS (object)->present (object, child);
}

/**
 * gdl_dock_object_child_placement:
 * @object: the dock object we are asking for child placement
 * @child: the child of the @object we want the placement for
 * @placement: (allow-none): where to return the placement information
 *
 * This function returns information about placement of a child dock
 * object inside another dock object.  The function returns %TRUE if
 * @child is effectively a child of @object.  @placement should
 * normally be initially setup to %GDL_DOCK_NONE.  If it's set to some
 * other value, this function will not touch the stored value if the
 * specified placement is "compatible" with the actual placement of
 * the child.
 *
 * @placement can be %NULL, in which case the function simply tells if
 * @child is attached to @object.
 *
 * Returns: %TRUE if @child is a child of @object.
 */
gboolean
gdl_dock_object_child_placement (GdlDockObject *object, GdlDockObject *child, GdlDockPlacement *placement)
{
    g_return_val_if_fail (object != NULL && child != NULL, FALSE);

    /* simple case */
    if (!gdl_dock_object_is_compound (object))
        return FALSE;

    if (GDL_DOCK_OBJECT_GET_CLASS (object)->child_placement)
        return GDL_DOCK_OBJECT_GET_CLASS (object)->child_placement (object, child, placement);
    else
        return FALSE;
}

/* Access functions
 *---------------------------------------------------------------------------*/
/**
 * gdl_dock_object_is_closed:
 * @object: The dock object to be checked
 *
 * Checks whether a given #GdlDockObject is closed. It can be only hidden and
 * still in the widget hierarchy or detached.
  *
 * Returns: %TRUE if the dock object is closed.
 *
 * Since: 3.6
 */
gboolean
gdl_dock_object_is_closed (GdlDockObject *object)
{
    return !object->priv->attached;
}

/**
 * gdl_dock_object_is_automatic:
 * @object: a #GdlDockObject
 *
 * Determine if an object is managed by the dock master, such object is
 * destroyed automatically when it is not needed anymore.
 *
 * Return value: %TRUE if the object is managed automatically by the dock master.
 *
 * Since: 3.6
 */
gboolean
gdl_dock_object_is_automatic (GdlDockObject *object)
{
    g_return_val_if_fail (GDL_IS_DOCK_OBJECT (object), FALSE);

    return object->priv->automatic;
}

/**
 * gdl_dock_object_set_manual:
 * @object: a #GdlDockObject
 *
 * A #GdlDockObject is managed by default by the dock master, use this function
 * to make it a manual object if you want to manage the destruction of the
 * object.
 *
 * Since: 3.6
 */
void
gdl_dock_object_set_manual (GdlDockObject *object)
{
    g_return_if_fail (GDL_IS_DOCK_OBJECT (object));

    object->priv->automatic = FALSE;
}

/**
 * gdl_dock_object_get_name:
 * @object: a #GdlDockObject
 *
 * Retrieves the name of the object. This name is used to identify the object.
 *
 * Return value: the name of the object.
 *
 * Since: 3.6
 */
const gchar *
gdl_dock_object_get_name (GdlDockObject *object)
{
    g_return_val_if_fail (GDL_IS_DOCK_OBJECT (object), NULL);

    return object->priv->name;
}

/**
 * gdl_dock_object_set_name:
 * @object: a #GdlDockObject
 * @name: a name for the object
 *
 * Set the name of the object used to identify it.
 *
 * Since: 3.6
 */
void
gdl_dock_object_set_name (GdlDockObject *object, const gchar *name)
{
    g_return_if_fail (GDL_IS_DOCK_OBJECT (object));

    g_free (object->priv->name);
    object->priv->name = g_strdup (name);

    g_object_notify_by_pspec (G_OBJECT (object), properties[PROP_NAME]);
}

/**
 * gdl_dock_object_get_long_name:
 * @object: a #GdlDockObject
 *
 * Retrieves the long name of the object. This name is an human readable string
 * which can be displayed in the user interface.
 *
 * Return value: the name of the object.
 *
 * Since: 3.6
 */
const gchar *
gdl_dock_object_get_long_name (GdlDockObject *object)
{
    g_return_val_if_fail (GDL_IS_DOCK_OBJECT (object), NULL);

    return object->priv->long_name;
}

/**
 * gdl_dock_object_set_long_name:
 * @object: a #GdlDockObject
 * @name: a name for the object
 *
 * Set the long name of the object. This name is an human readable string
 * which can be displayed in the user interface.
 *
 * Since: 3.6
 */
void
gdl_dock_object_set_long_name (GdlDockObject *object, const gchar *name)
{
    g_return_if_fail (GDL_IS_DOCK_OBJECT (object));

    g_free (object->priv->long_name);
    object->priv->long_name = g_strdup (name);

    g_object_notify_by_pspec (G_OBJECT (object), properties[PROP_LONG_NAME]);
}

/**
 * gdl_dock_object_get_stock_id:
 * @object: a #GdlDockObject
 *
 * Retrieves the a stock id used as the object icon.
 *
 * Return value: A stock id corresponding to the object icon.
 *
 * Since: 3.6
 */
const gchar *
gdl_dock_object_get_stock_id (GdlDockObject *object)
{
    g_return_val_if_fail (GDL_IS_DOCK_OBJECT (object), NULL);

    return object->priv->stock_id;
}

/**
 * gdl_dock_object_set_stock_id:
 * @object: a #GdlDockObject
 * @stock_id: a stock id
 *
 * Set an icon for the dock object using a stock id.
 *
 * Since: 3.6
 */
void
gdl_dock_object_set_stock_id (GdlDockObject *object, const gchar *stock_id)
{
    g_return_if_fail (GDL_IS_DOCK_OBJECT (object));

    g_free (object->priv->stock_id);
    object->priv->stock_id = g_strdup (stock_id);

    g_object_notify_by_pspec (G_OBJECT (object), properties[PROP_STOCK_ID]);
}

/**
 * gdl_dock_object_get_pixbuf:
 * @object: a #GdlDockObject
 *
 * Retrieves a pixbuf used as the dock object icon.
 *
 * Return value: (transfer none): icon for dock object
 *
 * Since: 3.6
 */
GdkPixbuf *
gdl_dock_object_get_pixbuf (GdlDockObject *object)
{
    g_return_val_if_fail (GDL_IS_DOCK_OBJECT (object), NULL);

    return object->priv->pixbuf_icon;
}

/**
 * gdl_dock_object_set_pixbuf:
 * @object: a #GdlDockObject
 * @icon: (allow-none): a icon or %NULL
 *
 * Set a icon for a dock object using a #GdkPixbuf.
 *
 * Since: 3.6
 */
void
gdl_dock_object_set_pixbuf (GdlDockObject *object, GdkPixbuf *icon)
{
    g_return_if_fail (GDL_IS_DOCK_OBJECT (object));
    g_return_if_fail (icon == NULL || GDK_IS_PIXBUF (icon));

    object->priv->pixbuf_icon =icon;

    g_object_notify_by_pspec (G_OBJECT (object), properties[PROP_PIXBUF_ICON]);
}



/* Dock param type functions
 *---------------------------------------------------------------------------*/

static void
gdl_dock_param_export_int (const GValue *src, GValue *dst)
{
    dst->data [0].v_pointer = g_strdup_printf ("%d", src->data [0].v_int);
}

static void
gdl_dock_param_export_uint (const GValue *src,
                            GValue       *dst)
{
    dst->data [0].v_pointer = g_strdup_printf ("%u", src->data [0].v_uint);
}

static void
gdl_dock_param_export_string (const GValue *src,
                              GValue       *dst)
{
    dst->data [0].v_pointer = g_strdup (src->data [0].v_pointer);
}

static void
gdl_dock_param_export_bool (const GValue *src,
                            GValue       *dst)
{
    dst->data [0].v_pointer = g_strdup_printf ("%s", src->data [0].v_int ? "yes" : "no");
}

static void
gdl_dock_param_export_placement (const GValue *src, GValue *dst)
{
    switch (src->data [0].v_int) {
        case GDL_DOCK_NONE:
            dst->data [0].v_pointer = g_strdup ("");
            break;
        case GDL_DOCK_TOP:
            dst->data [0].v_pointer = g_strdup ("top");
            break;
        case GDL_DOCK_BOTTOM:
            dst->data [0].v_pointer = g_strdup ("bottom");
            break;
        case GDL_DOCK_LEFT:
            dst->data [0].v_pointer = g_strdup ("left");
            break;
        case GDL_DOCK_RIGHT:
            dst->data [0].v_pointer = g_strdup ("right");
            break;
        case GDL_DOCK_CENTER:
            dst->data [0].v_pointer = g_strdup ("center");
            break;
        case GDL_DOCK_FLOATING:
            dst->data [0].v_pointer = g_strdup ("floating");
            break;
    }
}

static void
gdl_dock_param_import_int (const GValue *src,
                           GValue       *dst)
{
    dst->data [0].v_int = atoi (src->data [0].v_pointer);
}

static void
gdl_dock_param_import_uint (const GValue *src,
                            GValue       *dst)
{
    dst->data [0].v_uint = (guint) atoi (src->data [0].v_pointer);
}

static void
gdl_dock_param_import_string (const GValue *src,
                              GValue       *dst)
{
    dst->data [0].v_pointer = g_strdup (src->data [0].v_pointer);
}

static void
gdl_dock_param_import_bool (const GValue *src,
                            GValue       *dst)
{
    dst->data [0].v_int = !strcmp (src->data [0].v_pointer, "yes");
}

static void
gdl_dock_param_import_placement (const GValue *src,
                                 GValue       *dst)
{
    if (!strcmp (src->data [0].v_pointer, "top"))
        dst->data [0].v_int = GDL_DOCK_TOP;
    else if (!strcmp (src->data [0].v_pointer, "bottom"))
        dst->data [0].v_int = GDL_DOCK_BOTTOM;
    else if (!strcmp (src->data [0].v_pointer, "center"))
        dst->data [0].v_int = GDL_DOCK_CENTER;
    else if (!strcmp (src->data [0].v_pointer, "left"))
        dst->data [0].v_int = GDL_DOCK_LEFT;
    else if (!strcmp (src->data [0].v_pointer, "right"))
        dst->data [0].v_int = GDL_DOCK_RIGHT;
    else if (!strcmp (src->data [0].v_pointer, "floating"))
        dst->data [0].v_int = GDL_DOCK_FLOATING;
    else
        dst->data [0].v_int = GDL_DOCK_NONE;
}

GType
gdl_dock_param_get_type (void)
{
    static GType our_type = 0;

    if (our_type == 0) {
        GTypeInfo tinfo = { 0, };
        our_type = g_type_register_static (G_TYPE_STRING, "GdlDockParam", &tinfo, 0);

        /* register known transform functions */
        /* exporters */
        g_value_register_transform_func (G_TYPE_INT, our_type, gdl_dock_param_export_int);
        g_value_register_transform_func (G_TYPE_UINT, our_type, gdl_dock_param_export_uint);
        g_value_register_transform_func (G_TYPE_STRING, our_type, gdl_dock_param_export_string);
        g_value_register_transform_func (G_TYPE_BOOLEAN, our_type, gdl_dock_param_export_bool);
        g_value_register_transform_func (GDL_TYPE_DOCK_PLACEMENT, our_type, gdl_dock_param_export_placement);
        /* importers */
        g_value_register_transform_func (our_type, G_TYPE_INT, gdl_dock_param_import_int);
        g_value_register_transform_func (our_type, G_TYPE_UINT, gdl_dock_param_import_uint);
        g_value_register_transform_func (our_type, G_TYPE_STRING, gdl_dock_param_import_string);
        g_value_register_transform_func (our_type, G_TYPE_BOOLEAN, gdl_dock_param_import_bool);
        g_value_register_transform_func (our_type, GDL_TYPE_DOCK_PLACEMENT, gdl_dock_param_import_placement);
    }

    return our_type;
}



/* Nick <-> Type conversion
 *---------------------------------------------------------------------------*/

static void
gdl_dock_object_register_init (void)
{
#ifdef GDL_DISABLE_DEPRECATED
    const size_t n_default = 4;
#else
    const size_t n_default = 5;
#endif
    guint i = 0;
    struct DockRegisterItem default_items[n_default];

    if (dock_register)
        return;

    dock_register = g_array_new (FALSE, FALSE, sizeof (struct DockRegisterItem));

    /* add known types */
    default_items[0].nick = "dock";
    default_items[0].type = (gpointer) GDL_TYPE_DOCK;
    default_items[1].nick = "item";
    default_items[1].type = (gpointer) GDL_TYPE_DOCK_ITEM;
    default_items[2].nick = "paned";
    default_items[2].type = (gpointer) GDL_TYPE_DOCK_PANED;
    default_items[3].nick = "notebook";
    default_items[3].type = (gpointer) GDL_TYPE_DOCK_NOTEBOOK;
#ifndef GDL_DISABLE_DEPRECATED
    default_items[4].nick = "placeholder";
    default_items[4].type = (gpointer) GDL_TYPE_DOCK_PLACEHOLDER;
#endif

    for (i = 0; i < n_default; i++)
        g_array_append_val (dock_register, default_items[i]);
}

/**
 * gdl_dock_object_nick_from_type:
 * @type: The type for which to find the nickname
 *
 * Finds the nickname for a given type
 *
 * Returns: If the object has a nickname, then it is returned.
 *   Otherwise, the type name.
 */
const gchar *
gdl_dock_object_nick_from_type (GType type)
{
    gchar *nick = NULL;

    if (!dock_register)
        gdl_dock_object_register_init ();

    for (int i=0; i < dock_register->len; i++) {
        struct DockRegisterItem item = g_array_index (dock_register, struct DockRegisterItem, i);

        if (g_direct_equal (item.type, (gpointer) type))
            nick = item.nick;
    }

    return nick ? nick : g_type_name (type);
}

/**
 * gdl_dock_object_type_from_nick:
 * @nick: The nickname for the object type
 *
 * Finds the object type assigned to a given nickname.
 *
 * Returns: If the nickname has previously been assigned, then the corresponding
 * object type is returned.  Otherwise, %G_TYPE_NONE.
 */
GType
gdl_dock_object_type_from_nick (const gchar *nick)
{
	GType type = G_TYPE_NONE;
	gboolean nick_is_in_register = FALSE;

	if (!dock_register)
		gdl_dock_object_register_init ();

	for (int i = 0; i < dock_register->len; i++) {
		struct DockRegisterItem item = g_array_index (dock_register, struct DockRegisterItem, i);

		if (!g_strcmp0 (nick, item.nick)) {
			nick_is_in_register = TRUE;
			type = (GType) item.type;
		}
	}
	if (!nick_is_in_register) {
		/* try searching in the glib type system */
		type = g_type_from_name (nick);
	}

	return type;
}

/**
 * gdl_dock_object_set_type_for_nick:
 * @nick: The nickname for the object type
 * @type: The object type
 *
 * Assigns an object type to a given nickname.  If the nickname already exists,
 * then it reassigns it to a new object type.
 *
 * Returns: If the nick was previously assigned, the old type is returned.
 * Otherwise, %G_TYPE_NONE.
 */
GType
gdl_dock_object_set_type_for_nick (const gchar *nick, GType type)
{
	GType old_type = G_TYPE_NONE;

	struct DockRegisterItem new_item = {
		.nick = g_strdup(nick),
		.type = (gpointer) type
	};

	if (!dock_register)
		gdl_dock_object_register_init ();

	g_return_val_if_fail (g_type_is_a (type, GDL_TYPE_DOCK_OBJECT), G_TYPE_NONE);

	for (int i = 0; i < dock_register->len; i++) {
		struct DockRegisterItem item = g_array_index (dock_register, struct DockRegisterItem, i);

		if (!g_strcmp0 (nick, item.nick)) {
			old_type = (GType) item.type;
			g_array_insert_val (dock_register, i, new_item);
		}
	}

	return old_type;
}

/**
 * gdl_dock_object_class_set_is_compound:
 * @item: a #GdlDockObjectClass
 * @is_compound: %TRUE is the dock object contains other objects
 *
 * Define in the corresponding kind of dock object can contains children.
 *
 * Since: 3.6
 */
void
gdl_dock_object_class_set_is_compound (GdlDockObjectClass *object_class, gboolean is_compound)
{
    g_return_if_fail (GDL_IS_DOCK_OBJECT_CLASS (object_class));

    object_class->priv->is_compound = is_compound;
}
