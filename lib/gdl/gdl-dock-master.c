/*
 * gdl-dock-master.c - Object which manages a dock ring
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

#include "gdl-dock-master.h"
#include "gdl-dock.h"
#include "gdl-dock-item.h"
#include "gdl-dock-notebook.h"
#include "switcher.h"
#include "debug.h"
#include "libgdlmarshal.h"
#include "libgdltypebuiltins.h"

/**
 * SECTION:gdl-dock-master
 * @title: GdlDockMaster
 * @short_description: Manage all dock widgets
 * @stability: Unstable
 * @see_also: #GdlDockObject, #GdlDockNotebook, #GdlDockPaned
 *
 * For the toplevel docks to be able to interact with each other, when the user
 * drags items from one place to another, they're all kept in a user-invisible
 * and automatic object called the master. To participate in docking operations
 * every #GdlDockObject must have the same master, the binding to the master is
 * done automatically.
 *
 * The master also keeps track of the manual items,
 * mostly those created with gdl_dock_*_new functions which are in the dock.
 * This is so the user doesn't need to keep track of them, but can perform
 * operations like hiding and such.
 *
 * The master is responsible for creating automatically compound widgets.
 * When the user drops a widget on a simple one, a notebook or a paned compound
 * widget containing both widgets is created and replace it.
 * Such widgets are hidden automatically when they have less than two
 * children.
 *
 * One of the top level dock item of the master is considered as the controller.
 * This controller is an user visible representation of the master. A floating
 * dock widget will use a dock object having the same properties than this
 * controller. You can show or hide all dock widgets of the master by showing
 * or hiding the controller. This controller is assigned to the master
 * automatically. If the controller dock widget is removed, a new top level dock
 * widget will be automatically used as controller. If there is no other dock
 * widget, the master will be destroyed.
 */

/* ----- Private prototypes ----- */

static void     gdl_dock_master_class_init    (GdlDockMasterClass *klass);

static void     gdl_dock_master_dispose       (GObject            *g_object);
static void     gdl_dock_master_finalize      (GObject            *g_object);
static void     gdl_dock_master_set_property  (GObject            *object,
                                               guint               prop_id,
                                               const GValue       *value,
                                               GParamSpec         *pspec);
static void     gdl_dock_master_get_property  (GObject            *object,
                                               guint               prop_id,
                                               GValue             *value,
                                               GParamSpec         *pspec);

static void     _gdl_dock_master_remove       (GdlDockObject      *object,
                                               GdlDockMaster      *master);

static void     gdl_dock_master_drag_begin    (GdlDockItem        *item,
                                               gpointer            data);
#if 0
static void     gdl_dock_master_drag_motion   (GdlDockItem        *item,
                                               GdkDevice          *device,
                                               gint                x,
                                               gint                y,
                                               gpointer            data);
#endif

static void     _gdl_dock_master_foreach      (gpointer            key,
                                               gpointer            value,
                                               gpointer            user_data);

#if 0
static void     gdl_dock_master_show_preview   (GdlDockMaster      *master);
static void     gdl_dock_master_hide_preview   (GdlDockMaster      *master);
#endif

static void     gdl_dock_master_layout_changed (GdlDockMaster     *master);
static void     gdl_dock_master_dock_item_added(GdlDockMaster     *master, gpointer);

static void gdl_dock_master_set_switcher_style (GdlDockMaster *master,
                                                GdlSwitcherStyle switcher_style);
static void gdl_dock_master_set_tab_pos        (GdlDockMaster     *master,
                                                GtkPositionType    pos);
static void gdl_dock_master_set_tab_reorderable (GdlDockMaster    *master,
                                                gboolean           reorderable);
static void gdl_dock_master_create_overlay     (GdlDockMaster*     master);

/* ----- Private data types and variables ----- */

enum {
    PROP_0,
    PROP_DEFAULT_TITLE,
    PROP_LOCKED,
    PROP_SWITCHER_STYLE,
    PROP_TAB_POS,
    PROP_TAB_REORDERABLE
};

enum {
    LAYOUT_CHANGED,
    DOCK_ITEM_ADDED,
    DOCK_ITEM_REMOVED,
    LAST_SIGNAL
};

struct _GdlDockMasterPrivate {
    GHashTable     *dock_objects;
    GList          *toplevel_docks;
    GdlDockObject  *controller;      /* GUI root object */

    gint            dock_number;     /* for toplevel dock numbering */

    gint            number;          /* for naming nameless manual objects */
    gchar          *default_title;

    GdlDock        *rect_owner;

    GdlDockRequest *drag_request;

    /* source id for the idle handler to emit a layout_changed signal */
    guint           idle_layout_changed_id;

    /* hashes to quickly calculate the overall locked status: i.e.
     * if size(unlocked_items) == 0 then locked = 1
     * else if size(locked_items) == 0 then locked = 0
     * else locked = -1
     */
    GHashTable     *locked_items;
    GHashTable     *unlocked_items;

    GdlSwitcherStyle switcher_style;
    GtkPositionType tab_pos;
    gboolean        tab_reorderable;

    GtkWidget*      overlay;
};

#define COMPUTE_LOCKED(master)                                          \
    (g_hash_table_size ((master)->priv->unlocked_items) == 0 ? 1 :     \
     (g_hash_table_size ((master)->priv->locked_items) == 0 ? 0 : -1))

#define GBOOLEAN_TO_POINTER(i) (GINT_TO_POINTER ((i) ? 2 : 1))
#define GPOINTER_TO_BOOLEAN(i) ((gboolean) ((GPOINTER_TO_INT(i) == 2) ? TRUE : FALSE))

static guint master_signals [LAST_SIGNAL] = { 0 };

/* ----- Private interface ----- */

G_DEFINE_TYPE_WITH_CODE (GdlDockMaster, gdl_dock_master, G_TYPE_OBJECT, G_ADD_PRIVATE (GdlDockMaster))

static void
gdl_dock_master_class_init (GdlDockMasterClass *klass)
{
    GObjectClass      *object_class;

    object_class = G_OBJECT_CLASS (klass);

    object_class->dispose = gdl_dock_master_dispose;
    object_class->finalize = gdl_dock_master_finalize;
    object_class->set_property = gdl_dock_master_set_property;
    object_class->get_property = gdl_dock_master_get_property;

    klass->layout_changed = gdl_dock_master_layout_changed;
    klass->dock_item_added = gdl_dock_master_dock_item_added;

    g_object_class_install_property (
        object_class, PROP_DEFAULT_TITLE,
        g_param_spec_string ("default-title", _("Default title"),
                             _("Default title for newly created floating docks"),
                             NULL,
                             G_PARAM_READWRITE));

    g_object_class_install_property (
        object_class, PROP_LOCKED,
        g_param_spec_int ("locked", _("Locked"),
                          _("If is set to 1, all the dock items bound to the master "
                            "are locked; if it's 0, all are unlocked; -1 indicates "
                            "inconsistency among the items"),
                          -1, 1, 0,
                          G_PARAM_READWRITE));

    g_object_class_install_property (
        object_class, PROP_SWITCHER_STYLE,
        g_param_spec_enum ("switcher-style", _("Switcher Style"),
                           _("Switcher buttons style"),
                           GDL_TYPE_SWITCHER_STYLE,
                           GDL_SWITCHER_STYLE_BOTH,
                           G_PARAM_READWRITE));

    g_object_class_install_property (
        object_class, PROP_TAB_POS,
        g_param_spec_enum ("tab-pos", _("Tab Position"),
                           _("Which side of the notebook holds the tabs"),
                           GTK_TYPE_POSITION_TYPE,
                           GTK_POS_BOTTOM,
                           G_PARAM_READWRITE));

    g_object_class_install_property (
        object_class, PROP_TAB_REORDERABLE,
        g_param_spec_boolean ("tab-reorderable", _("Tab reorderable"),
                              _("Whether the tab is reorderable by user action"),
                              FALSE,
                              G_PARAM_READWRITE));

    /**
     * GdlDockMaster::layout-changed:
     *
     * Signals that the layout has changed, one or more widgets have been moved,
     * added or removed.
     */
    master_signals [LAYOUT_CHANGED] =
        g_signal_new ("layout-changed",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (GdlDockMasterClass, layout_changed),
                      NULL, /* accumulator */
                      NULL, /* accu_data */
                      gdl_marshal_VOID__VOID,
                      G_TYPE_NONE, /* return type */
                      0);

    master_signals [DOCK_ITEM_ADDED] =
        g_signal_new ("dock-item-added",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (GdlDockMasterClass, dock_item_added),
                      NULL, /* accumulator */
                      NULL, /* accu_data */
                      g_cclosure_marshal_VOID__POINTER,
                      G_TYPE_NONE, /* return type */
                      1, /* n params */
                      G_TYPE_POINTER);

    master_signals [DOCK_ITEM_REMOVED] =
        g_signal_new ("dock-item-removed",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (GdlDockMasterClass, dock_item_added),
                      NULL, /* accumulator */
                      NULL, /* accu_data */
                      g_cclosure_marshal_VOID__POINTER,
                      G_TYPE_NONE, /* return type */
                      1, /* n params */
                      G_TYPE_POINTER);
}

static void
gdl_dock_master_init (GdlDockMaster *master)
{
	master->priv = gdl_dock_master_get_instance_private (master);

    master->priv->dock_objects = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
    master->priv->toplevel_docks = NULL;
    master->priv->controller = NULL;
    master->priv->dock_number = 1;

    master->priv->number = 1;
    master->priv->switcher_style = GDL_SWITCHER_STYLE_BOTH;
    master->priv->tab_pos = GTK_POS_BOTTOM;
    master->priv->tab_reorderable = FALSE;
    master->priv->locked_items = g_hash_table_new (g_direct_hash, g_direct_equal);
    master->priv->unlocked_items = g_hash_table_new (g_direct_hash, g_direct_equal);
}

static void
_gdl_dock_master_remove (GdlDockObject *object, GdlDockMaster *master)
{
    g_return_if_fail (master != NULL && object != NULL);

    if (GDL_IS_DOCK (object)) {
        GList *found_link;

        found_link = g_list_find (master->priv->toplevel_docks, object);
        if (found_link)
            master->priv->toplevel_docks = g_list_delete_link (master->priv->toplevel_docks, found_link);
        if (object == master->priv->controller) {
            GList *last;
            GdlDockObject *new_controller = NULL;

            /* now find some other non-automatic toplevel to use as a
               new controller.  start from the last dock, since it's
               probably a non-floating and manual */
            last = g_list_last (master->priv->toplevel_docks);
            while (last) {
                if (!gdl_dock_object_is_automatic (last->data)) {
                    new_controller = GDL_DOCK_OBJECT (last->data);
                    break;
                }
                last = last->prev;
            };

            if (new_controller) {
                /* the new controller gets the ref (implicitly of course) */
                master->priv->controller = new_controller;
            } else {
                master->priv->controller = NULL;
                /* no controller, no master */
                g_object_unref (master);
            }
        }
    }
    /* disconnect dock object signals */
    g_signal_handlers_disconnect_matched (object, G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, master);

    /* unref the object from the hash if it's there */
    if (gdl_dock_object_get_name (object) != NULL) {
        GdlDockObject *found_object = g_hash_table_lookup (master->priv->dock_objects, gdl_dock_object_get_name (object));
        if (found_object == object) {
            g_hash_table_remove (master->priv->dock_objects, gdl_dock_object_get_name (object));
            g_object_unref (object);
        }
    }
}

static void
ht_foreach_build_slist (gpointer key, gpointer value, GSList **slist)
{
    *slist = g_slist_prepend (*slist, value);
}

static void
gdl_dock_master_dispose (GObject *object)
{
    GdlDockMaster *master = GDL_DOCK_MASTER (object);

    if (master->priv->toplevel_docks) {
        g_list_foreach (master->priv->toplevel_docks, (GFunc) gdl_dock_object_unbind, NULL);
        g_list_free (master->priv->toplevel_docks);
        master->priv->toplevel_docks = NULL;
    }

    if (master->priv->dock_objects) {
        GSList *alive_docks = NULL;
        g_hash_table_foreach (master->priv->dock_objects, (GHFunc) ht_foreach_build_slist, &alive_docks);
        while (alive_docks) {
            gdl_dock_object_unbind (GDL_DOCK_OBJECT (alive_docks->data));
            alive_docks = g_slist_delete_link (alive_docks, alive_docks);
        }

        g_hash_table_unref (master->priv->dock_objects);
        master->priv->dock_objects = NULL;
    }

    if (master->priv->idle_layout_changed_id) {
        g_source_remove (master->priv->idle_layout_changed_id);
        master->priv->idle_layout_changed_id = 0;
    }

    if (master->priv->drag_request) {
        if (G_IS_VALUE (&master->priv->drag_request->extra))
            g_value_unset (&master->priv->drag_request->extra);

        g_free (master->priv->drag_request);
        master->priv->drag_request = NULL;
    }

    if (master->priv->locked_items) {
        g_hash_table_unref (master->priv->locked_items);
        master->priv->locked_items = NULL;
    }

    if (master->priv->unlocked_items) {
        g_hash_table_unref (master->priv->unlocked_items);
        master->priv->unlocked_items = NULL;
    }

#if 0
    if (master->priv->area_window) {
        gtk_window_destroy (GTK_WINDOW (master->priv->area_window));
        master->priv->area_window = NULL;
    }
#endif

    G_OBJECT_CLASS (gdl_dock_master_parent_class)->dispose (object);
}

static void
gdl_dock_master_finalize (GObject *object)
{
    GdlDockMaster *master = GDL_DOCK_MASTER (object);

    g_free (master->priv->default_title);

    G_OBJECT_CLASS (gdl_dock_master_parent_class)->finalize (object);
}

static void
foreach_lock_unlock (GdlDockObject *item, gboolean locked)
{
    if (!GDL_IS_DOCK_ITEM (item))
        return;

    g_object_set (item, "locked", locked, NULL);
    if (gdl_dock_object_is_compound (GDL_DOCK_OBJECT (item)))
        gdl_dock_object_foreach_child (item, (GdlDockObjectFn)foreach_lock_unlock, GINT_TO_POINTER (locked));
}

static void
gdl_dock_master_lock_unlock (GdlDockMaster *master, gboolean locked)
{
    for (GList* l = master->priv->toplevel_docks; l; l = l->next) {
        GdlDock *dock = GDL_DOCK (l->data);
        GdlDockObject *root = gdl_dock_get_root (dock);
        if (root != NULL)
            foreach_lock_unlock (root, locked);
    }

    /* just to be sure hidden items are set too */
    gdl_dock_master_foreach (master, (GFunc) foreach_lock_unlock, GINT_TO_POINTER (locked));
}

static void
gdl_dock_master_set_property  (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    GdlDockMaster *master = GDL_DOCK_MASTER (object);

    switch (prop_id) {
        case PROP_DEFAULT_TITLE:
            g_free (master->priv->default_title);
            master->priv->default_title = g_value_dup_string (value);
            break;
        case PROP_LOCKED:
            if (g_value_get_int (value) >= 0)
                gdl_dock_master_lock_unlock (master, (g_value_get_int (value) > 0));
            break;
        case PROP_SWITCHER_STYLE:
            gdl_dock_master_set_switcher_style (master, g_value_get_enum (value));
            break;
        case PROP_TAB_POS:
            gdl_dock_master_set_tab_pos (master, g_value_get_enum (value));
            break;
        case PROP_TAB_REORDERABLE:
            gdl_dock_master_set_tab_reorderable (master, g_value_get_boolean (value));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
gdl_dock_master_get_property  (GObject      *object,
                               guint         prop_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
    GdlDockMaster *master = GDL_DOCK_MASTER (object);

    switch (prop_id) {
        case PROP_DEFAULT_TITLE:
            g_value_set_string (value, master->priv->default_title);
            break;
        case PROP_LOCKED:
            g_value_set_int (value, COMPUTE_LOCKED (master));
            break;
        case PROP_SWITCHER_STYLE:
            g_value_set_enum (value, master->priv->switcher_style);
            break;
        case PROP_TAB_POS:
            g_value_set_enum (value, master->priv->tab_pos);
            break;
        case PROP_TAB_REORDERABLE:
            g_value_set_enum (value, master->priv->tab_reorderable);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
gdl_dock_master_drag_begin (GdlDockItem *item, gpointer data)
{
    g_return_if_fail (data != NULL);
    g_return_if_fail (item != NULL);

    GdlDockMaster* master = GDL_DOCK_MASTER (data);

	if (!master->priv->overlay) {
		gdl_dock_master_create_overlay(master);
	}

    if (!master->priv->drag_request)
        master->priv->drag_request = g_new0 (GdlDockRequest, 1);

    GdlDockRequest* request = master->priv->drag_request;

    /* Set the target to itself so it won't go floating with just a click. */
    request->applicant = GDL_DOCK_OBJECT (item);
    request->target = GDL_DOCK_OBJECT (item);
    request->position = GDL_DOCK_FLOATING;
    if (G_IS_VALUE (&request->extra))
        g_value_unset (&request->extra);

    master->priv->drag_request->dock = GTK_WIDGET(gdl_dock_object_get_toplevel(GDL_DOCK_OBJECT(item)));

    master->priv->rect_owner = NULL;
}

#if 0
static void
gdl_dock_master_drag_motion (GdlDockItem *item, GdkDevice *device, gint root_x, gint root_y, gpointer data)
{
    GdkWindow      *widget_window;
    gint            win_x, win_y;
    gint            x, y;
    GdlDock        *dock = NULL;
    gboolean        may_dock = FALSE;

    g_return_if_fail (item != NULL && data != NULL);

    GdlDockMaster* master = GDL_DOCK_MASTER (data);
    GdlDockRequest* request = master->priv->drag_request;

    g_return_if_fail (GDL_DOCK_OBJECT (item) == request->applicant);

    GdlDockRequest my_request = *request;

    /* first look under the pointer */
    GdkWindow* window = gdk_device_get_window_at_position (device, &win_x, &win_y);
    if (window) {
        GtkWidget *widget;
        /* ok, now get the widget who owns that window and see if we can
           get to a GdlDock by walking up the hierarchy */
        gdk_window_get_user_data (window, (gpointer) &widget);
        if (GTK_IS_WIDGET (widget)) {
            while (widget && (!GDL_IS_DOCK (widget) || gdl_dock_object_get_master (GDL_DOCK_OBJECT (widget)) != G_OBJECT (master)))
                widget = gtk_widget_get_parent (widget);
            if (widget) {
                gint win_w, win_h;

                widget_window = gtk_widget_get_window (widget);

                /* verify that the pointer is still in that dock
                   (the user could have moved it) */
                gdk_window_get_geometry (widget_window, NULL, NULL, &win_w, &win_h);
                gdk_window_get_origin (widget_window, &win_x, &win_y);
                if (root_x >= win_x && root_x < win_x + win_w &&
                    root_y >= win_y && root_y < win_y + win_h)
                    dock = GDL_DOCK (widget);
            }
        }
    }

    if (dock) {
        GdkWindow *dock_window = gtk_widget_get_window (GTK_WIDGET (dock));

        /* translate root coordinates into dock object coordinates
           (i.e. widget coordinates) */
        gdk_window_get_origin (dock_window, &win_x, &win_y);
        x = root_x - win_x;
        y = root_y - win_y;
        may_dock = gdl_dock_object_dock_request (GDL_DOCK_OBJECT (dock),
                                                 x, y, &my_request);
    }
    else {
        /* try to dock the item in all the docks in the ring in turn */
        for (GList* l = master->priv->toplevel_docks; l; l = l->next) {
            dock = GDL_DOCK (l->data);
            GdkWindow *dock_window = gtk_widget_get_window (GTK_WIDGET (dock));
            if (!dock_window)
                continue;

            /* translate root coordinates into dock object coordinates
               (i.e. widget coordinates) */
            gdk_window_get_origin (dock_window, &win_x, &win_y);
            x = root_x - win_x;
            y = root_y - win_y;
            may_dock = gdl_dock_object_dock_request (GDL_DOCK_OBJECT (dock), x, y, &my_request);
            if (may_dock)
                break;
        }
    }


    if (!may_dock) {
		/* Special case for GdlDockItems : they must respect the flags */
		if (GDL_IS_DOCK_ITEM(item) && gdl_dock_item_get_behavior_flags (GDL_DOCK_ITEM(item)) & GDL_DOCK_ITEM_BEH_NEVER_FLOATING)
		    return;

        dock = NULL;
        my_request.target = GDL_DOCK_OBJECT (gdl_dock_object_get_toplevel (request->applicant));
        my_request.position = GDL_DOCK_FLOATING;

        gdl_dock_item_get_drag_area (GDL_DOCK_ITEM (request->applicant), &my_request.rect);
        my_request.rect.x = root_x - my_request.rect.x;
        my_request.rect.y = root_y - my_request.rect.y;

        /* setup extra docking information */
        if (G_IS_VALUE (&my_request.extra))
            g_value_unset (&my_request.extra);

        g_value_init (&my_request.extra, GDK_TYPE_RECTANGLE);
        g_value_set_boxed (&my_request.extra, &my_request.rect);
    }

	/* if we want to enforce GDL_DOCK_ITEM_BEH_NEVER_FLOATING
	 * the item must remain attached to the controller, otherwise
	 * it could be inserted in another floating dock
	 * so check for the flag at this moment
	 */
	else if(GDL_IS_DOCK_ITEM(item)
		&& gdl_dock_item_get_behavior_flags (GDL_DOCK_ITEM(item)) & GDL_DOCK_ITEM_BEH_NEVER_FLOATING
		&& dock != GDL_DOCK(master->priv->controller)
	)
		return;

	/* the previous windows is drawn by the dock master object if the preview
	 * is floating or by the corresponding dock object. It is not necessary
	 * the dock master could handle all cases but then it would be more
	 * difficult to implement different preview window in both cases
	 * erase previous preview window if necessary
	 */
	if (master->priv->rect_owner != dock) {
        gdl_dock_master_hide_preview (master);
        master->priv->rect_owner = dock;
    }

    /* draw the preview window */
    *request = my_request;
    gdl_dock_master_show_preview (master);
}
#endif

static void
_gdl_dock_master_foreach (gpointer key, gpointer value, gpointer user_data)
{
    struct {
        GFunc    function;
        gpointer user_data;
    } *data = user_data;

    (* data->function) (GTK_WIDGET (value), data->user_data);
}

#if 0
static void
gdl_dock_master_show_preview (GdlDockMaster *master)
{
    if (!master->priv || !master->priv->drag_request)
        return;

    if (master->priv->rect_owner) {
        gdl_dock_show_preview (master->priv->rect_owner, &master->priv->drag_request->rect);
    } else {
        cairo_rectangle_int_t *rect;

        rect = &master->priv->drag_request->rect;

        if (!master->priv->area_window) {
            master->priv->area_window = gdl_preview_window_new ();
        }

        gdl_preview_window_update (GDL_PREVIEW_WINDOW (master->priv->area_window), rect);
    }
}
#endif

#if 0
static void
gdl_dock_master_hide_preview (GdlDockMaster *master)
{
    if (!master->priv)
        return;

    if (master->priv->rect_owner)
    {
        gdl_dock_hide_preview (master->priv->rect_owner);
        master->priv->rect_owner = NULL;
    }
    if (master->priv->area_window)
    {
        gtk_widget_set_visible (master->priv->area_window, false);
    }
}
#endif

static void
gdl_dock_master_layout_changed (GdlDockMaster *master)
{
    g_return_if_fail (GDL_IS_DOCK_MASTER (master));

    /* emit "layout-changed" on the controller to notify the user who
     * normally shouldn't have access to us */
    if (master->priv->controller)
        g_signal_emit_by_name (master->priv->controller, "layout-changed");

    /* remove the idle handler if there is one */
    if (master->priv->idle_layout_changed_id) {
        g_source_remove (master->priv->idle_layout_changed_id);
        master->priv->idle_layout_changed_id = 0;
    }
}

static void
gdl_dock_master_dock_item_added (GdlDockMaster *master, gpointer item)
{
}

static gboolean
idle_emit_layout_changed (gpointer user_data)
{
	ENTER;

    GdlDockMaster *master = user_data;

    g_return_val_if_fail (master && GDL_IS_DOCK_MASTER (master), FALSE);

    master->priv->idle_layout_changed_id = 0;
    g_signal_emit (master, master_signals [LAYOUT_CHANGED], 0);

    return FALSE;
}

static void
item_dock_cb (GdlDockObject* object, GdlDockObject* requestor, GdlDockPlacement position, GValue* other_data, gpointer user_data)
{
    GdlDockMaster *master = user_data;

    g_return_if_fail (requestor && GDL_IS_DOCK_OBJECT (requestor));
    g_return_if_fail (master && GDL_IS_DOCK_MASTER (master));

    /* here we are in fact interested in the requestor, since it's
     * assumed that object will not change its visibility... for the
     * requestor, however, could mean that it's being shown */
    if (!gdl_dock_object_is_frozen (requestor) &&
        !gdl_dock_object_is_automatic (requestor)) {
        if (!master->priv->idle_layout_changed_id)
            master->priv->idle_layout_changed_id =
                g_idle_add (idle_emit_layout_changed, master);
    }
}

static void
item_detach_cb (GdlDockObject *object, gboolean recursive, gpointer user_data)
{
    GdlDockMaster *master = user_data;

    g_return_if_fail (object && GDL_IS_DOCK_OBJECT (object));
    g_return_if_fail (master && GDL_IS_DOCK_MASTER (master));

    if (!gdl_dock_object_is_frozen (object) &&
        !gdl_dock_object_is_automatic (object)) {
        if (!master->priv->idle_layout_changed_id)
            master->priv->idle_layout_changed_id =
                g_idle_add (idle_emit_layout_changed, master);
    }
}

static void
item_notify_cb (GdlDockObject *object, GParamSpec *pspec, gpointer user_data)
{
    GdlDockMaster *master = user_data;
    gint locked = COMPUTE_LOCKED (master);
    gboolean item_locked;

    g_object_get (object, "locked", &item_locked, NULL);

    if (item_locked) {
        g_hash_table_remove (master->priv->unlocked_items, object);
        g_hash_table_insert (master->priv->locked_items, object, NULL);
    } else {
        g_hash_table_remove (master->priv->locked_items, object);
        g_hash_table_insert (master->priv->unlocked_items, object, NULL);
    }

    if (COMPUTE_LOCKED (master) != locked)
        g_object_notify (G_OBJECT (master), "locked");
}

/* ----- Public interface ----- */

/**
 * gdl_dock_master_add:
 * @master: a #GdlDockMaster
 * @object: a #GdlDockObject
 *
 * Add a new dock widget to the master.
 */
void
gdl_dock_master_add (GdlDockMaster *master, GdlDockObject *object)
{
    g_return_if_fail (master != NULL && object != NULL);

    if (!gdl_dock_object_is_automatic (object)) {
        GdlDockObject *found_object;

        /* create a name for the object if it doesn't have one */
        if (gdl_dock_object_get_name (object) == NULL) {
            /* directly set the name, since it's a construction only
               property */
            gchar *name = g_strdup_printf ("__dock_%u", master->priv->number++);
            gdl_dock_object_set_name (object, name);
            g_free (name);
        }

        /* add the object to our hash list */
        if ((found_object = g_hash_table_lookup (master->priv->dock_objects, gdl_dock_object_get_name (object)))) {
            g_warning (_("master %p: unable to add object %p[%s] to the hash.  "
                         "There already is an item with that name (%p)."),
                       master, object, gdl_dock_object_get_name (object), found_object);
        }
        else {
            g_object_ref_sink (object);
            g_hash_table_insert (master->priv->dock_objects, g_strdup (gdl_dock_object_get_name (object)), object);
        }
    }

    if (GDL_IS_DOCK (object)) {
        /* if this is the first toplevel we are adding, name it controller */
        if (!master->priv->toplevel_docks)
            /* the dock should already have the ref */
            master->priv->controller = object;

        /* add dock to the toplevel list */
        gboolean floating;
        g_object_get (object, "floating", &floating, NULL);
        if (floating)
            master->priv->toplevel_docks = g_list_prepend (master->priv->toplevel_docks, object);
        else
            master->priv->toplevel_docks = g_list_append (master->priv->toplevel_docks, object);

        /* we are interested in the dock request this toplevel
         * receives to update the layout */
        g_signal_connect (object, "dock", G_CALLBACK (item_dock_cb), master);
    }

    else if (GDL_IS_DOCK_ITEM (object)) {
        /* we need to connect the item's signals */
        g_signal_connect (object, "dock_drag_begin", G_CALLBACK (gdl_dock_master_drag_begin), master);
#if 0
        g_signal_connect (object, "dock_drag_motion", G_CALLBACK (gdl_dock_master_drag_motion), master);
        g_signal_connect (object, "dock_drag_end", G_CALLBACK (gdl_dock_master_drag_end), master);
#endif
        g_signal_connect (object, "dock", G_CALLBACK (item_dock_cb), master);
        g_signal_connect (object, "detach", G_CALLBACK (item_detach_cb), master);

        /* register to "locked" notification if the item has a grip,
         * and add the item to the corresponding hash */
        if (GDL_DOCK_ITEM_HAS_GRIP (GDL_DOCK_ITEM (object))) {
            g_signal_connect (object, "notify::locked", G_CALLBACK (item_notify_cb), master);
            item_notify_cb (object, NULL, master);

			{
				GtkDragSource* src = gtk_drag_source_new ();
				GdkContentProvider* content = gdk_content_provider_new_typed (GDL_TYPE_DOCK_ITEM, object);
				gtk_drag_source_set_content (src, content);
				g_object_unref (content);
				gtk_widget_add_controller (gdl_dock_item_get_grip(GDL_DOCK_ITEM (object)), GTK_EVENT_CONTROLLER (src));

				void begin (GtkDragSource* src, GdkDrag* drag, gpointer item)
				{
					PF;
					GdlDock* dock = GDL_DOCK(gdl_dock_object_get_toplevel(GDL_DOCK_OBJECT(item)));

					gdl_dock_master_drag_begin (GDL_DOCK_ITEM(item), GDL_DOCK_OBJECT(item)->master);
					gdl_dock_show_overlay (dock, ((GdlDockMaster*)GDL_DOCK_OBJECT(item)->master)->priv->overlay);
				}
				g_signal_connect(src, "drag-begin", (void*)begin, object);

				void drag_end (GtkDragSource* self, GdkDrag* drag, gboolean delete_data, gpointer _item)
				{
					g_return_if_fail (_item);

					GdlDockItem *item = _item;
					GdlDockMaster* master = GDL_DOCK_MASTER(GDL_DOCK_OBJECT(item)->master);
					GdlDockRequest* request = master->priv->drag_request;

					bool cancelled = false; // TODO

					g_return_if_fail (GDL_DOCK_OBJECT (item) == request->applicant);

#if 1
					gdl_dock_hide_overlay (GDL_DOCK(request->dock));
#else
					/* Hide the preview window */
					gdl_dock_master_hide_preview (master);
#endif

					/* cancel conditions */
					if (cancelled || request->applicant == request->target)
						return;

					/* dock the object at the requested position */
					gdl_dock_object_dock (request->target, request->applicant, request->position, &request->extra);

					g_signal_emit (master, master_signals [LAYOUT_CHANGED], 0);
				}
				g_signal_connect(src, "drag-end", (void*)drag_end, object);
			}
        }

        /* If the item is notebook, set the switcher style and notebook
         * settings. */
        if (GDL_IS_DOCK_NOTEBOOK (object) && GDL_IS_SWITCHER (gdl_dock_item_get_child (GDL_DOCK_ITEM (object)))) {
            GtkWidget *child = gdl_dock_item_get_child (GDL_DOCK_ITEM (object));
            g_object_set (child, "switcher-style", master->priv->switcher_style, NULL);
            g_object_set (child, "tab-pos", master->priv->tab_pos, NULL);
            g_object_set (child, "tab-reorderable", master->priv->tab_reorderable, NULL);
        }

        /* post a layout_changed emission if the item is not automatic
         * (since it should be added to the items model) */
        if (!gdl_dock_object_is_automatic (object)) {
            if (!master->priv->idle_layout_changed_id)
                master->priv->idle_layout_changed_id = g_idle_add (idle_emit_layout_changed, master);
        }
    }
}

/**
 * gdl_dock_master_remove:
 * @master: a #GdlDockMaster
 * @object: a #GdlDockObject
 *
 * Remove one dock widget from the master.
 */
void
gdl_dock_master_remove (GdlDockMaster *master, GdlDockObject *object)
{
    g_return_if_fail (master != NULL && object != NULL);

    /* remove from locked/unlocked hashes and property change if
     * that's the case */
    if (GDL_IS_DOCK_ITEM (object) && GDL_DOCK_ITEM_HAS_GRIP (GDL_DOCK_ITEM (object))) {
        gint locked = COMPUTE_LOCKED (master);
        if (g_hash_table_remove (master->priv->locked_items, object) ||
            g_hash_table_remove (master->priv->unlocked_items, object)) {
            if (COMPUTE_LOCKED (master) != locked)
                g_object_notify (G_OBJECT (master), "locked");
        }
    }

    /* ref the master, since removing the controller could cause master disposal */
    g_object_ref (master);

    /* all the interesting stuff happens in _gdl_dock_master_remove */
    _gdl_dock_master_remove (object, master);

    /* post a layout_changed emission if the item is not automatic
     * (since it should be removed from the items model) */
    if (!gdl_dock_object_is_automatic (object)) {
        if (!master->priv->idle_layout_changed_id)
            master->priv->idle_layout_changed_id =
                g_idle_add (idle_emit_layout_changed, master);
    }

    /* balance ref count */
    g_object_unref (master);
}

/**
 * gdl_dock_master_foreach:
 * @master: a #GdlDockMaster
 * @function: (scope call): the function to call with each element's data
 * @user_data: user data to pass to the function
 *
 * Call @function on each dock widget of the master.
 */
void
gdl_dock_master_foreach (GdlDockMaster *master, GFunc function, gpointer user_data)
{
    struct {
        GFunc    function;
        gpointer user_data;
    } data;

    g_return_if_fail (master != NULL && function != NULL);

    data.function = function;
    data.user_data = user_data;
    g_hash_table_foreach (master->priv->dock_objects, _gdl_dock_master_foreach, &data);
}

/**
 * gdl_dock_master_foreach_toplevel:
 * @master: a #GdlDockMaster
 * @include_controller: %TRUE to include the controller
 * @function: (scope call): the function to call with each element's data
 * @user_data: user data to pass to the function
 *
 * Call @function on each top level dock widget of the master, including or not
 * the controller.
 */
void
gdl_dock_master_foreach_toplevel (GdlDockMaster *master, gboolean include_controller, GFunc function, gpointer user_data)
{
    g_return_if_fail (master != NULL && function != NULL);

    for (GList* l = master->priv->toplevel_docks; l; ) {
        GdlDockObject *object = GDL_DOCK_OBJECT (l->data);
        l = l->next;
        if (object != master->priv->controller || include_controller)
            (* function) (GTK_WIDGET (object), user_data);
    }
}

/**
 * gdl_dock_master_get_object:
 * @master: a #GdlDockMaster
 * @nick_name: the name of the dock widget.
 *
 * Looks for a #GdlDockObject named @nick_name.
 *
 * Returns: (allow-none) (transfer none): A #GdlDockObject named @nick_name or %NULL if it does not exist.
 */
GdlDockObject *
gdl_dock_master_get_object (GdlDockMaster *master, const gchar *nick_name)
{
    g_return_val_if_fail (master != NULL, NULL);

    if (!nick_name)
        return NULL;

    gpointer* found = g_hash_table_lookup (master->priv->dock_objects, nick_name);

    return found ? GDL_DOCK_OBJECT (found) : NULL;
}

/**
 * gdl_dock_master_get_controller:
 * @master: a #GdlDockMaster
 *
 * Retrieves the #GdlDockObject acting as the controller.
 *
 * Returns: (transfer none): A #GdlDockObject.
 */
GdlDockObject *
gdl_dock_master_get_controller (GdlDockMaster *master)
{
    g_return_val_if_fail (master != NULL, NULL);

    return master->priv->controller;
}

/**
 * gdl_dock_master_set_controller:
 * @master: a #GdlDockMaster
 * @new_controller: a #GdlDockObject
 *
 * Set a new controller. The controller must be a top level #GdlDockObject.
 */
void
gdl_dock_master_set_controller (GdlDockMaster *master, GdlDockObject *new_controller)
{
    g_return_if_fail (master != NULL);

    if (new_controller) {
        if (gdl_dock_object_is_automatic (new_controller))
            g_warning (_("The new dock controller %p is automatic.  Only manual "
                         "dock objects should be named controller."), new_controller);

        /* check that the controller is in the toplevel list */
        if (!g_list_find (master->priv->toplevel_docks, new_controller))
            gdl_dock_master_add (master, new_controller);
        master->priv->controller = new_controller;

    } else {
        master->priv->controller = NULL;
        /* no controller, no master */
        g_object_unref (master);
    }
}

/**
 * gdl_dock_master_get_dock_name:
 * @master: a #GdlDockMaster
 *
 * Return an unique translated dock name.
 *
 * Returns: (transfer full): a new translated name. The string has to be freed
 * with g_free().
 *
 * Since: 3.6
 */
gchar *
gdl_dock_master_get_dock_name (GdlDockMaster *master)
{
    g_return_val_if_fail (GDL_IS_DOCK_MASTER (master), NULL);

    return g_strdup_printf (_("Dock #%d"), master->priv->dock_number++);
}


static void
set_switcher_style_foreach (GtkWidget *obj, gpointer user_data)
{
    GdlSwitcherStyle style = GPOINTER_TO_INT (user_data);

    if (!GDL_IS_DOCK_ITEM (obj))
        return;

    if (GDL_IS_DOCK_NOTEBOOK (obj)) {

        GtkWidget *child = gdl_dock_item_get_child (GDL_DOCK_ITEM (obj));
        if (GDL_IS_SWITCHER (child)) {

            g_object_set (child, "switcher-style", style, NULL);
        }
    } else if (gdl_dock_object_is_compound (GDL_DOCK_OBJECT (obj))) {

        gdl_dock_object_foreach_child (GDL_DOCK_OBJECT(obj), (GdlDockObjectFn)set_switcher_style_foreach, user_data);
    }
}

static void
gdl_dock_master_set_switcher_style (GdlDockMaster *master, GdlSwitcherStyle switcher_style)
{
    g_return_if_fail (GDL_IS_DOCK_MASTER (master));

    master->priv->switcher_style = switcher_style;
    for (GList* l = master->priv->toplevel_docks; l; l = l->next) {
        GdlDock *dock = GDL_DOCK (l->data);
        GtkWidget *root = GTK_WIDGET (gdl_dock_get_root (dock));
        if (root != NULL)
            set_switcher_style_foreach (root, GINT_TO_POINTER (switcher_style));
    }

    /* just to be sure hidden items are set too */
    gdl_dock_master_foreach (master, (GFunc) set_switcher_style_foreach, GINT_TO_POINTER (switcher_style));
}

static void
set_tab_pos_foreach (GtkWidget *obj, gpointer user_data)
{
    GtkPositionType tab_pos = GPOINTER_TO_INT (user_data);

    if (!GDL_IS_DOCK_ITEM (obj))
        return;

    if (GDL_IS_DOCK_NOTEBOOK (obj)) {

        GtkWidget *child = gdl_dock_item_get_child (GDL_DOCK_ITEM (obj));
        if (GDL_IS_SWITCHER (child)) {

            g_object_set (child, "tab-pos", tab_pos, NULL);
        }
    } else if (gdl_dock_object_is_compound (GDL_DOCK_OBJECT (obj))) {

        gdl_dock_object_foreach_child (GDL_DOCK_OBJECT (obj), (GdlDockObjectFn)set_tab_pos_foreach, user_data);
    }
}

static void
gdl_dock_master_set_tab_pos (GdlDockMaster *master, GtkPositionType tab_pos)
{
    GList *l;
    g_return_if_fail (GDL_IS_DOCK_MASTER (master));

    master->priv->tab_pos = tab_pos;
    for (l = master->priv->toplevel_docks; l; l = l->next) {
        GdlDock *dock = GDL_DOCK (l->data);
        GtkWidget *root = GTK_WIDGET (gdl_dock_get_root (dock));
        if (root != NULL)
            set_tab_pos_foreach (root, GINT_TO_POINTER (tab_pos));
    }

    /* just to be sure hidden items are set too */
    gdl_dock_master_foreach (master, (GFunc) set_tab_pos_foreach, GINT_TO_POINTER (tab_pos));
}

static void
set_tab_reorderable_foreach (GtkWidget *obj, gpointer user_data)
{
    gboolean tab_reorderable = GPOINTER_TO_BOOLEAN (user_data);

    if (!GDL_IS_DOCK_ITEM (obj))
        return;

    if (GDL_IS_DOCK_NOTEBOOK (obj)) {

        GtkWidget *child = gdl_dock_item_get_child (GDL_DOCK_ITEM (obj));
        if (GDL_IS_SWITCHER (child)) {
            g_object_set (child, "tab-reorderable", tab_reorderable, NULL);
        }
    } else if (gdl_dock_object_is_compound (GDL_DOCK_OBJECT (obj))) {

        gdl_dock_object_foreach_child (GDL_DOCK_OBJECT (obj), (GdlDockObjectFn)set_tab_reorderable_foreach, user_data);
    }
}

static void
gdl_dock_master_set_tab_reorderable (GdlDockMaster *master, gboolean tab_reorderable)
{
    GList *l;
    g_return_if_fail (GDL_IS_DOCK_MASTER (master));

    master->priv->tab_reorderable = tab_reorderable;
    for (l = master->priv->toplevel_docks; l; l = l->next) {
        GdlDock *dock = GDL_DOCK (l->data);
        GtkWidget *root = GTK_WIDGET (gdl_dock_get_root (dock));
        if (root != NULL)
            set_tab_reorderable_foreach (root, GBOOLEAN_TO_POINTER (tab_reorderable));
    }

    /* just to be sure hidden items are set too */
    gdl_dock_master_foreach (master, (GFunc) set_tab_reorderable_foreach, GBOOLEAN_TO_POINTER (tab_reorderable));
}

static void
gdl_dock_master_create_overlay (GdlDockMaster* master)
{
	GtkWidget* overlay = master->priv->overlay = gtk_drawing_area_new();

	void overlay_draw (GtkDrawingArea* area, cairo_t* cr, int width, int height, gpointer master)
	{
		graphene_rect_t rect = ((GdlDockMaster*)master)->priv->drag_request->rect;

		cairo_set_line_width (cr, 1.0);
		const double dashes[] = {1};
		cairo_set_dash (cr, (double*)&dashes, 1, 0);

		cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
		cairo_rectangle (cr, rect.origin.x + 1, rect.origin.y + 1, rect.size.width, rect.size.height);
		cairo_stroke (cr);

		cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
		cairo_rectangle (cr, rect.origin.x, rect.origin.y, rect.size.width, rect.size.height);
		cairo_stroke (cr);
	}

	gtk_drawing_area_set_draw_func (GTK_DRAWING_AREA (overlay), overlay_draw, master, NULL);

	{
		GtkDropTarget* target = gtk_drop_target_new (G_TYPE_INVALID, GDK_ACTION_MOVE | GDK_ACTION_COPY);
		gtk_drop_target_set_gtypes (target, (GType [1]) { GDL_TYPE_DOCK_ITEM }, 1);

		GdkDragAction on_motion (GtkDropTarget* self, gdouble x, gdouble y, gpointer master)
		{
			GdlDockMasterPrivate* priv = ((GdlDockMaster*)master)->priv;

			GtkWidget* widget = gtk_widget_pick (gtk_widget_get_first_child(GTK_WIDGET(priv->drag_request->dock)), x, y, GTK_PICK_DEFAULT);
			GtkWidget* item = gtk_widget_get_ancestor(widget, GDL_TYPE_DOCK_ITEM);
			dbg(2, "%.0fx%.0f %s %s", x, y, G_OBJECT_CLASS_NAME(G_OBJECT_GET_CLASS(widget)), G_OBJECT_CLASS_NAME(G_OBJECT_GET_CLASS(item)));

			if (item) {
#pragma GCC diagnostic ignored "-Wunused-result"
				gtk_widget_compute_bounds(GTK_WIDGET(item), GTK_WIDGET(priv->drag_request->dock), &priv->drag_request->rect);
#pragma GCC diagnostic warning "-Wunused-result"

				GdlDockPlacement position;

				if (GDL_DOCK_OBJECT(item) == priv->drag_request->applicant) {
					position = GDL_DOCK_NONE;
					priv->drag_request->rect = (graphene_rect_t){0,};

				}
				else if (GDL_IS_DOCK_NOTEBOOK(item)) {
    				position = GDL_DOCK_CENTER;

				} else {
					GtkWidget* notebook = gtk_widget_get_ancestor(widget, GDL_TYPE_DOCK_NOTEBOOK);
					if (notebook) {
    					position = GDL_DOCK_CENTER;
#pragma GCC diagnostic ignored "-Wunused-result"
						gtk_widget_compute_bounds(GTK_WIDGET(notebook), GTK_WIDGET(priv->drag_request->dock), &priv->drag_request->rect);
#pragma GCC diagnostic warning "-Wunused-result"
					} else {
						graphene_point_t* origin = &priv->drag_request->rect.origin;
						graphene_size_t* size = &priv->drag_request->rect.size;

						bool is_top = y < origin->y + size->height / 3;
						bool is_bottom = y > origin->y + 2 * size->height / 3;
						bool is_left = x < origin->x + size->width / 3;
						bool is_right = x > origin->x + 2 * size->width / 3;
						if (!is_top && !is_bottom && !is_left && !is_right) {
							position = GDL_DOCK_CENTER;
							origin->x += size->width / 8;
							origin->y += size->height / 8;
							size->width = 3 * size->width / 4;
							size->height = 3 * size->height / 4;
						} else if (is_top) {
							position = GDL_DOCK_TOP;
							size->height /= 3;
						} else if (is_left) {
							position = GDL_DOCK_LEFT;
							size->width /= 3;
						} else if (is_bottom) {
							position = GDL_DOCK_BOTTOM;
							size->height /= 3;
							origin->y += 2 * size->height;
						} else {
							position = GDL_DOCK_RIGHT;
							size->width /= 3;
							origin->x += 2 * size->width;
						}
					}
				}

   				priv->drag_request->target = GDL_DOCK_OBJECT (item);
				priv->drag_request->position = position;

				gtk_widget_queue_draw(priv->overlay);
			}

			return GDK_ACTION_MOVE;
		}
		g_signal_connect (target, "motion", G_CALLBACK (on_motion), master);

		gboolean on_drop (GtkDropTarget* target, const GValue* value, double x, double y, gpointer data)
		{
			if (G_VALUE_HOLDS (value, GDL_TYPE_DOCK_ITEM)) {
				return true;
			}

			return false;
		}
		g_signal_connect (target, "drop", G_CALLBACK (on_drop), overlay);

		gtk_widget_add_controller (GTK_WIDGET (overlay), GTK_EVENT_CONTROLLER (target));
	}
}
