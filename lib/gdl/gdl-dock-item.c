/*
 * gdl-dock-item.c
 *
 * Author: Gustavo Giráldez <gustavo.giraldez@gmx.net>
 *         Naba Kumar  <naba@gnome.org>
 *
 * Based on GnomeDockItem/BonoboDockItem.  Original copyright notice follows.
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
#include <gdk/gdkkeysyms.h>

#include "il8n.h"
#include "debug.h"
#include "gdl-dock.h"
#include "gdl-dock-item.h"
#include "gdl-dock-item-grip.h"
#include "gdl-dock-notebook.h"
#include "gdl-dock-paned.h"
#include "switcher.h"
#include "master.h"
#include "libgdltypebuiltins.h"
#include "libgdlmarshal.h"

/**
 * SECTION:gdl-dock-item
 * @title: GdlDockItem
 * @short_description: Adds docking capability to its child widget.
 * @see_also: #GdlDockItem
 * @stability: Unstable
 *
 * A dock item is a container widget that can be docked at different place.
 * It accepts a single child and adds a grip allowing the user to click on it
 * to drag and drop the widget.
 *
 * The grip is implemented as a #GdlDockItemGrip.
 */

#define NEW_DOCK_ITEM_RATIO 0.3

/* ----- Private prototypes ----- */

static void  gdl_dock_item_base_class_init (GdlDockItemClass *class);
static void  gdl_dock_item_class_init     (GdlDockItemClass *class);
static void  gdl_dock_item_init		  (GdlDockItem *item);

static GObject *gdl_dock_item_constructor (GType                  type,
                                           guint                  n_construct_properties,
                                           GObjectConstructParam *construct_param);

static void  gdl_dock_item_set_property  (GObject      *object,
                                          guint         prop_id,
                                          const GValue *value,
                                          GParamSpec   *pspec);
static void  gdl_dock_item_get_property  (GObject      *object,
                                          guint         prop_id,
                                          GValue       *value,
                                          GParamSpec   *pspec);

static void  gdl_dock_item_dispose       (GObject      *object);

static void gdl_dock_item_measure        (GtkWidget* widget, GtkOrientation orientation, int for_size, int* min, int* natural, int* min_baseline, int* natural_baseline);

static void  gdl_dock_item_set_focus_child (GtkWidget *container,
                                            GtkWidget    *widget);

static void  gdl_dock_item_size_allocate (GtkWidget *widget,
                                          int, int, int);
static void  gdl_dock_item_map           (GtkWidget *widget);
static void  gdl_dock_item_unmap         (GtkWidget *widget);

static void  gdl_dock_item_move_focus_child (GdlDockItem      *item,
                                             GtkDirectionType  dir);
 
static void gdl_dock_item_click_gesture_pressed (GtkGestureClick *gesture, int n_press, double widget_x, double widget_y, GdlDockItem *item);
static void gdl_dock_item_click_gesture_released (GtkGestureClick *gesture, int n_press, double widget_x, double widget_y, GdlDockItem *item);
#ifdef GTK4_TODO
static gint     gdl_dock_item_motion        (GtkWidget *widget,
                                             GdkEventMotion *event);
static gboolean gdl_dock_item_key_press     (GtkWidget *widget,
                                             GdkEventKey *event);
#endif

static gboolean gdl_dock_item_dock_request  (GdlDockObject    *object,
                                             gint              x,
                                             gint              y,
                                             GdlDockRequest   *request);
static void     gdl_dock_item_dock          (GdlDockObject    *object,
                                             GdlDockObject    *requestor,
                                             GdlDockPlacement  position,
                                             GValue           *other_data);
static void     gdl_dock_item_remove        (GdlDockObject    *object,
                                             GtkWidget        *widget);
static void     gdl_dock_item_remove_widgets(GdlDockObject    *object);
static void     gdl_dock_item_destroy       (GdlDockObject    *object);
static void     gdl_dock_item_present       (GdlDockObject     *object,
                                             GdlDockObject     *child);


static void     gdl_dock_item_popup_menu    (GdlDockItem *item,
                                             guint        button,
                                             double       x,
                                             double       y);
#ifdef GTK4_TODO
static void     gdl_dock_item_drag_start    (GdlDockItem *item);
#endif
static gboolean gdl_dock_item_drag_end      (GdlDockItem *item, gboolean cancel);

#ifdef GTK4_TODO
static void     gdl_dock_item_tab_button    (GtkWidget      *widget,
                                             GdkEventButton *event,
                                             gpointer        data);
#endif

static void     gdl_dock_item_hide_cb       (GSimpleAction*, GVariant*, gpointer);
static void     gdl_dock_item_lock_cb       (GSimpleAction*, GVariant*, gpointer);
static void     gdl_dock_item_unlock_cb     (GSimpleAction*, GVariant*, gpointer);

static void  gdl_dock_item_showhide_grip    (GdlDockItem *item);

static void  gdl_dock_item_real_set_orientation (GdlDockItem    *item,
                                                 GtkOrientation  orientation);

static void gdl_dock_param_export_gtk_orientation (const GValue *src,
                                                   GValue       *dst);
static void gdl_dock_param_import_gtk_orientation (const GValue *src,
                                                   GValue       *dst);



/* ----- Class variables and definitions ----- */

enum {
    PROP_0,
    PROP_ORIENTATION,
    PROP_RESIZE,
    PROP_EXPAND,
    PROP_BEHAVIOR,
    PROP_LOCKED,
    PROP_PREFERRED_WIDTH,
    PROP_PREFERRED_HEIGHT,
    PROP_ICONIFIED,
    PROP_CLOSED,
};

enum {
    DOCK_DRAG_BEGIN,
    DOCK_DRAG_MOTION,
    DOCK_DRAG_END,
    SELECTED,
    DESELECTED,
    MOVE_FOCUS_CHILD,
    LAST_SIGNAL
};

static guint gdl_dock_item_signals [LAST_SIGNAL] = { 0 };

#define GDL_DOCK_ITEM_GRIP_SHOWN(item) \
    (GDL_DOCK_ITEM_HAS_GRIP (item))

struct _GdlDockItemPrivate {
    GdlDockItemBehavior behavior;
    GtkOrientation      orientation;

    guint               iconified: 1;
    guint               resize : 1;
    guint               expand : 1;
    guint               in_predrag : 1;
    guint               in_drag : 1;

    gint                dragoff_x, dragoff_y;

    GtkWidget*          menu;
    GtkWidget*          menu_item_hide;

    gboolean            grip_shown;
    GtkWidget*          grip;
    guint               grip_size;

    GtkWidget*          tab_label;
    gboolean            intern_tab_label;
    guint               notify_label;
    guint               notify_stock_id;

    gint                preferred_width;
    gint                preferred_height;

    GtkGesture*         click;

    gint                start_x, start_y;
};

struct _GdlDockItemClassPrivate {
    gboolean        has_grip;

    GtkCssProvider *css;
};

/* FIXME: implement the rest of the behaviors */

#define SPLIT_RATIO  0.4


/* ----- Private functions ----- */

/* It is not possible to use G_DEFINE_TYPE_* macro for GdlDockItem because it
 * has some class private data. The priv pointer has to be initialized in
 * the base class initialization function because this function is called for
 * each derived type.
 */
static gpointer gdl_dock_item_parent_class = NULL;
//static gint GdlDockItem_private_offset;

GType
gdl_dock_item_get_type (void)
{
	static GType gtype = 0;
    //static GType g_define_type_id = 0;

	if (G_UNLIKELY (gtype == 0)) {
		const GTypeInfo gtype_info = {
			sizeof (GdlDockItemClass),
			(GBaseInitFunc) gdl_dock_item_base_class_init,
			NULL,
			(GClassInitFunc) gdl_dock_item_class_init,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (GdlDockItem),
			0,          /* n_preallocs */
			(GInstanceInitFunc) gdl_dock_item_init,
			NULL,		/* value_table */
		};

		gtype = g_type_register_static (GDL_TYPE_DOCK_OBJECT, "GdlDockItem", &gtype_info, 0);

		//g_define_type_id = gtype;
		//G_ADD_PRIVATE (GdlDockItem);
		g_type_add_class_private (gtype, sizeof (GdlDockItemClassPrivate));
	}

	return gtype;
}


#ifdef GTK4_TODO
static void
add_tab_bindings (GtkBindingSet *binding_set, GdkModifierType modifiers, GtkDirectionType direction)
{
    gtk_binding_entry_add_signal (binding_set, GDK_KEY_Tab, modifiers, "move_focus_child", 1, GTK_TYPE_DIRECTION_TYPE, direction);
    gtk_binding_entry_add_signal (binding_set, GDK_KEY_KP_Tab, modifiers, "move_focus_child", 1, GTK_TYPE_DIRECTION_TYPE, direction);
}

static void
add_arrow_bindings (GtkBindingSet *binding_set, guint keysym, GtkDirectionType direction)
{
    guint keypad_keysym = keysym - GDK_KEY_Left + GDK_KEY_KP_Left;

    gtk_binding_entry_add_signal (binding_set, keysym, 0,
                                  "move_focus_child", 1,
                                  GTK_TYPE_DIRECTION_TYPE, direction);
    gtk_binding_entry_add_signal (binding_set, keysym, GDK_CONTROL_MASK,
                                  "move_focus_child", 1,
                                  GTK_TYPE_DIRECTION_TYPE, direction);
    gtk_binding_entry_add_signal (binding_set, keysym, GDK_CONTROL_MASK,
                                  "move_focus_child", 1,
                                  GTK_TYPE_DIRECTION_TYPE, direction);
    gtk_binding_entry_add_signal (binding_set, keypad_keysym, GDK_CONTROL_MASK,
                                  "move_focus_child", 1,
                                  GTK_TYPE_DIRECTION_TYPE, direction);
}
#endif

static void
gdl_dock_item_base_class_init (GdlDockItemClass *klass)
{
    klass->priv = G_TYPE_CLASS_GET_PRIVATE (klass, GDL_TYPE_DOCK_ITEM, GdlDockItemClassPrivate);
}

static void
gdl_dock_item_class_init (GdlDockItemClass *klass)
{
    static const gchar style[] =
       "* {\n"
           "padding: 0;\n"
       "}";

    gdl_dock_item_parent_class = g_type_class_peek_parent (klass);

    GObjectClass* object_class = G_OBJECT_CLASS (klass);
    GtkWidgetClass* widget_class = GTK_WIDGET_CLASS (klass);
    GdlDockObjectClass* dock_object_class = GDL_DOCK_OBJECT_CLASS (klass);

    object_class->constructor = gdl_dock_item_constructor;
    object_class->set_property = gdl_dock_item_set_property;
    object_class->get_property = gdl_dock_item_get_property;
    object_class->dispose = gdl_dock_item_dispose;

    widget_class->map = gdl_dock_item_map;
    widget_class->unmap = gdl_dock_item_unmap;
	widget_class->measure = gdl_dock_item_measure;
    widget_class->size_allocate = gdl_dock_item_size_allocate;
#ifdef GTK4_TODO
    widget_class->button_release_event = gdl_dock_item_button_changed;
    widget_class->motion_notify_event = gdl_dock_item_motion;
    widget_class->key_press_event = gdl_dock_item_key_press;
#endif
    widget_class->set_focus_child = gdl_dock_item_set_focus_child;

    gdl_dock_object_class_set_is_compound (dock_object_class, FALSE);
    dock_object_class->dock_request = gdl_dock_item_dock_request;
    dock_object_class->dock = gdl_dock_item_dock;
    dock_object_class->present = gdl_dock_item_present;
    dock_object_class->remove = gdl_dock_item_remove;
    dock_object_class->remove_widgets = gdl_dock_item_remove_widgets;
    dock_object_class->destroy = gdl_dock_item_destroy;

    klass->priv->has_grip = TRUE;
    klass->dock_drag_begin = NULL;
    klass->dock_drag_motion = NULL;
    klass->dock_drag_end = NULL;
    klass->move_focus_child = gdl_dock_item_move_focus_child;
    klass->set_orientation = gdl_dock_item_real_set_orientation;

    /* properties */

    /**
     * GdlDockItem:orientation:
     *
     * The orientation of the docking item. If the orientation is set to
     * #GTK_ORIENTATION_VERTICAL, the grip widget will be shown along
     * the top of the edge of item (if it is not hidden). If the
     * orientation is set to #GTK_ORIENTATION_HORIZONTAL, the grip
     * widget will be shown down the left edge of the item (even if the
     * widget text direction is set to RTL).
     */
    g_object_class_install_property (
        object_class, PROP_ORIENTATION,
        g_param_spec_enum ("orientation", _("Orientation"),
                           _("Orientation of the docking item"),
                           GTK_TYPE_ORIENTATION,
                           GTK_ORIENTATION_VERTICAL,
                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                           GDL_DOCK_PARAM_EXPORT));

    /* --- register exporter/importer for GTK_ORIENTATION */
    g_value_register_transform_func (GTK_TYPE_ORIENTATION, GDL_TYPE_DOCK_PARAM,
                                     gdl_dock_param_export_gtk_orientation);
    g_value_register_transform_func (GDL_TYPE_DOCK_PARAM, GTK_TYPE_ORIENTATION,
                                     gdl_dock_param_import_gtk_orientation);
    /* --- end of registration */

    g_object_class_install_property (
        object_class, PROP_RESIZE,
        g_param_spec_boolean ("resize", _("Resizable"),
                              _("If set, the dock item can be resized when "
                                "docked in a GtkPanel widget"),
                              TRUE,
                              G_PARAM_READWRITE | GDL_DOCK_PARAM_EXPORT));

    g_object_class_install_property (
        object_class, PROP_EXPAND,
        g_param_spec_boolean ("expand", _("Expandable"),
                              _("If unset, the allocation will not be allowed to be"
                                "greater than the natural one when docked in a Paned"),
                              TRUE,
                              G_PARAM_READWRITE | GDL_DOCK_PARAM_EXPORT));

    g_object_class_install_property (
        object_class, PROP_BEHAVIOR,
        g_param_spec_flags ("behavior", _("Item behavior"),
                            _("General behavior for the dock item (i.e. "
                              "whether it can float, if it's locked, etc.)"),
                            GDL_TYPE_DOCK_ITEM_BEHAVIOR,
                            GDL_DOCK_ITEM_BEH_NORMAL,
                            G_PARAM_READWRITE));

    g_object_class_install_property (
        object_class, PROP_LOCKED,
        g_param_spec_boolean ("locked", _("Locked"),
                              _("If set, the dock item cannot be dragged around "
                                "and it doesn't show a grip"),
                              FALSE,
                              G_PARAM_READWRITE |
                              GDL_DOCK_PARAM_EXPORT));

    g_object_class_install_property (
        object_class, PROP_PREFERRED_WIDTH,
        g_param_spec_int ("preferred-width", _("Preferred width"),
                          _("Preferred width for the dock item"),
                          -1, G_MAXINT, -1,
                          G_PARAM_READWRITE));

    g_object_class_install_property (
        object_class, PROP_PREFERRED_HEIGHT,
        g_param_spec_int ("preferred-height", _("Preferred height"),
                          _("Preferred height for the dock item"),
                          -1, G_MAXINT, -1,
                          G_PARAM_READWRITE));

    /**
     * GdlDockItem:iconified:
     *
     * If set, the dock item is hidden but it has a corresponding icon in the
     * dock bar allowing to show it again.
     *
     * Since: 3.6
     */
    g_object_class_install_property (
        object_class, PROP_ICONIFIED,
        g_param_spec_boolean ("iconified", _("Iconified"),
                              _("If set, the dock item is hidden but it has a "
                                "corresponding icon in the dock bar allowing to "
                                "show it again."),
                              FALSE,
                              G_PARAM_READWRITE |
                              GDL_DOCK_PARAM_EXPORT));

    /**
     * GdlDockItem:closed:
     *
     * If set, the dock item is closed.
     *
     * Since: 3.6
     */
    g_object_class_install_property (
        object_class, PROP_CLOSED,
        g_param_spec_boolean ("closed", _("Closed"),
                              _("Whether the widget is closed."),
                              FALSE,
                              G_PARAM_READWRITE |
                              GDL_DOCK_PARAM_EXPORT));

    /**
     * GdlDockItem::dock-drag-begin:
     * @item: The dock item which is being dragged.
     *
     * Signals that the dock item has begun to be dragged.
     **/
    gdl_dock_item_signals [DOCK_DRAG_BEGIN] =
        g_signal_new ("dock-drag-begin",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (GdlDockItemClass, dock_drag_begin),
                      NULL, /* accumulator */
                      NULL, /* accu_data */
                      gdl_marshal_VOID__VOID,
                      G_TYPE_NONE,
                      0);

    /**
     * GdlDockItem::dock-drag-motion:
     * @item: The dock item which is being dragged.
     * @device: The device used.
     * @x: The x-position that the dock item has been dragged to.
     * @y: The y-position that the dock item has been dragged to.
     *
     * Signals that a dock item dragging motion event has occured.
     **/
    gdl_dock_item_signals [DOCK_DRAG_MOTION] =
        g_signal_new ("dock-drag-motion",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (GdlDockItemClass, dock_drag_motion),
                      NULL, /* accumulator */
                      NULL, /* accu_data */
                      gdl_marshal_VOID__OBJECT_INT_INT,
                      G_TYPE_NONE,
                      3,
                      GDK_TYPE_DEVICE,
                      G_TYPE_INT,
                      G_TYPE_INT);

    /**
     * GdlDockItem::dock-drag-end:
     * @item: The dock item which is no longer being dragged.
     * @cancel: This value is set to TRUE if the drag was cancelled by
     * the user. #cancel is set to FALSE if the drag was accepted.
     *
     * Signals that the dock item dragging has ended.
     **/
    gdl_dock_item_signals [DOCK_DRAG_END] =
        g_signal_new ("dock_drag_end",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (GdlDockItemClass, dock_drag_end),
                      NULL, /* accumulator */
                      NULL, /* accu_data */
                      gdl_marshal_VOID__BOOLEAN,
                      G_TYPE_NONE,
                      1,
                      G_TYPE_BOOLEAN);

    /**
     * GdlDockItem::selected:
     *
     * Signals that this dock has been selected from a switcher.
     */
    gdl_dock_item_signals [SELECTED] =
        g_signal_new ("selected",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_FIRST,
                      0,
                      NULL,
                      NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE,
                      0);

    /**
     * GdlDockItem::move-focus-child:
     * @gdldockitem: The dock item in which a change of focus is requested
     * @dir: The direction in which to move focus
     *
     * The ::move-focus-child signal is emitted when a change of focus is
     * requested for the child widget of a dock item.  The @dir parameter
     * specifies the direction in which focus is to be shifted.
     *
     * Since: 3.3.2
     */
    gdl_dock_item_signals [MOVE_FOCUS_CHILD] =
        g_signal_new ("move_focus_child",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                      G_STRUCT_OFFSET (GdlDockItemClass, move_focus_child),
                      NULL, /* accumulator */
                      NULL, /* accu_data */
                      gdl_marshal_VOID__ENUM,
                      G_TYPE_NONE,
                      1,
                      GTK_TYPE_DIRECTION_TYPE);


    /**
     * GdlDockItem::deselected:
     *
     * Signals that this dock has been deselected in a switcher.
     */
    gdl_dock_item_signals [DESELECTED] =
        g_signal_new ("deselected",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_FIRST,
                      0,
                      NULL,
                      NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE,
                      0);

    /* key bindings */

#ifdef GTK4_TODO
    GtkBindingSet *binding_set = gtk_binding_set_by_class (klass);

    add_arrow_bindings (binding_set, GDK_KEY_Up, GTK_DIR_UP);
    add_arrow_bindings (binding_set, GDK_KEY_Down, GTK_DIR_DOWN);
    add_arrow_bindings (binding_set, GDK_KEY_Left, GTK_DIR_LEFT);
    add_arrow_bindings (binding_set, GDK_KEY_Right, GTK_DIR_RIGHT);
 
    add_tab_bindings (binding_set, 0, GTK_DIR_TAB_FORWARD);
    add_tab_bindings (binding_set, GDK_CONTROL_MASK, GTK_DIR_TAB_FORWARD);
    add_tab_bindings (binding_set, GDK_SHIFT_MASK, GTK_DIR_TAB_BACKWARD);
    add_tab_bindings (binding_set, GDK_CONTROL_MASK | GDK_SHIFT_MASK, GTK_DIR_TAB_BACKWARD);
#endif

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	g_type_class_add_private (object_class, sizeof (GdlDockItemPrivate));
#pragma GCC diagnostic warning "-Wdeprecated-declarations"

	gtk_widget_class_set_accessible_role (widget_class, GTK_ACCESSIBLE_ROLE_REGION);

    /* set the style */
    klass->priv->css = gtk_css_provider_new ();
	gtk_css_provider_load_from_string(klass->priv->css, style);
	gtk_widget_class_set_css_name (widget_class, "dock-item");
}

static void
gdl_dock_item_init (GdlDockItem *item)
{
	//item->priv = gdl_dock_item_get_instance_private (item);
	item->priv = (GdlDockItemPrivate*) g_type_instance_get_private ((GTypeInstance*)item, GDL_TYPE_DOCK_ITEM);

	gtk_widget_set_focusable (GTK_WIDGET (item), TRUE);
	gtk_widget_set_overflow (GTK_WIDGET (item), GTK_OVERFLOW_HIDDEN);

	item->child = NULL;

	item->priv->orientation = GTK_ORIENTATION_VERTICAL;
	item->priv->behavior = GDL_DOCK_ITEM_BEH_NORMAL;

	item->priv->iconified = FALSE;

	item->priv->resize = TRUE;
	item->priv->expand = TRUE;

	item->priv->dragoff_x = item->priv->dragoff_y = 0;
	item->priv->in_predrag = item->priv->in_drag = FALSE;

	item->priv->menu = NULL;
	item->priv->menu_item_hide = NULL;

	item->priv->preferred_width = item->priv->preferred_height = -1;
	item->priv->tab_label = NULL;
	item->priv->intern_tab_label = FALSE;
}

static void
on_long_name_changed (GObject* item, GParamSpec* spec, gpointer user_data)
{
    gchar* long_name;
    g_object_get (item, "long-name", &long_name, NULL);
    gtk_label_set_label (GTK_LABEL (user_data), long_name);
    g_free(long_name);
}

static void
on_stock_id_changed (GObject* item, GParamSpec* spec, gpointer user_data)
{
    gchar* icon_name;
    g_object_get (item, "stock_id", &icon_name, NULL);
	gtk_image_set_from_icon_name (GTK_IMAGE (user_data), icon_name);
    g_free(icon_name);
}

static GObject *
gdl_dock_item_constructor (GType type, guint n_construct_properties, GObjectConstructParam *construct_param)
{
    GObject* g_object = G_OBJECT_CLASS (gdl_dock_item_parent_class)-> constructor (type, n_construct_properties, construct_param);
    if (g_object) {
        GdlDockItem* item = GDL_DOCK_ITEM (g_object);
        gchar* long_name;
        gchar* stock_id;

        if (GDL_DOCK_ITEM_HAS_GRIP (item)) {
            item->priv->grip_shown = TRUE;
            item->priv->grip = gdl_dock_item_grip_new (item);
            gtk_widget_set_parent (item->priv->grip, GTK_WIDGET (item));

			item->priv->click = gtk_gesture_click_new ();
			g_signal_connect (item->priv->click, "pressed", G_CALLBACK (gdl_dock_item_click_gesture_pressed), item);
			g_signal_connect (item->priv->click, "released", G_CALLBACK (gdl_dock_item_click_gesture_released), item);
			gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (item->priv->click), 3);
			gtk_widget_add_controller (GTK_WIDGET (item->priv->grip), GTK_EVENT_CONTROLLER (item->priv->click));

        } else {
            item->priv->grip_shown = FALSE;
        }

        g_object_get (g_object, "long-name", &long_name, "stock-id", &stock_id, NULL);

        GtkWidget* hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
        GtkWidget* label = gtk_label_new (long_name);
        GtkWidget* icon = gtk_image_new ();
        if (stock_id)
            gtk_image_set_from_icon_name (GTK_IMAGE (icon), stock_id);
        gtk_box_append (GTK_BOX (hbox), icon);
        gtk_box_append (GTK_BOX (hbox), label);
        gtk_accessible_update_relation (GTK_ACCESSIBLE(g_object), GTK_ACCESSIBLE_RELATION_LABELLED_BY, label, NULL, -1);

        item->priv->notify_label = g_signal_connect (item, "notify::long-name", G_CALLBACK (on_long_name_changed), label);
        item->priv->notify_stock_id = g_signal_connect (item, "notify::stock-id", G_CALLBACK (on_stock_id_changed), icon);

        gdl_dock_item_set_tablabel (item, hbox);
        item->priv->intern_tab_label = TRUE;

        g_free (long_name);
        g_free (stock_id);
    }

    return g_object;
}

static void
gdl_dock_item_set_property (GObject *g_object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	GdlDockItem *item = GDL_DOCK_ITEM (g_object);

	switch (prop_id) {
		case PROP_ORIENTATION:
			gdl_dock_item_set_orientation (item, g_value_get_enum (value));
			break;
		case PROP_RESIZE:
			item->priv->resize = g_value_get_boolean (value);
			{
				GObject* parent = (GObject*) gtk_widget_get_parent (GTK_WIDGET (item));
				// if we docked, update "resize" child_property of our parent
				if (parent) {
					gboolean resize;
					g_object_get (g_object, "resize", &resize, NULL);
					if (resize != item->priv->resize) {
#ifdef GTK4_TODO
						gtk_container_child_set (GTK_CONTAINER(parent), GTK_WIDGET(item), "resize", item->priv->resize, NULL);
#endif
					}
				}
			}
			gtk_widget_queue_resize (GTK_WIDGET (item));
			break;
		case PROP_EXPAND:
		{
			gboolean expand = g_value_get_boolean (value);
			if (expand != item->priv->expand) {
#ifdef DEBUG
				GdlDockObject* parent = gdl_dock_object_get_parent_object(GDL_DOCK_OBJECT(item));
				if (parent && !GDL_IS_DOCK_PANED(parent)) {
					g_warning("`expand` property only applies to Paned children");
				}
#endif
				item->priv->expand = expand;
			}
			break;
		}
		case PROP_BEHAVIOR:
		{
			gdl_dock_item_set_behavior_flags (item, g_value_get_flags (value), TRUE);
			break;
		}
		case PROP_LOCKED:
		{
			GdlDockItemBehavior old_beh = item->priv->behavior;

			if (g_value_get_boolean (value))
				item->priv->behavior |= GDL_DOCK_ITEM_BEH_LOCKED;
			else
				item->priv->behavior &= ~GDL_DOCK_ITEM_BEH_LOCKED;

			if (old_beh ^ item->priv->behavior) {
				gdl_dock_item_showhide_grip (item);
				g_object_notify (g_object, "behavior");

				gdl_dock_object_layout_changed_notify (GDL_DOCK_OBJECT (item));
			}
			break;
		}
		case PROP_PREFERRED_WIDTH:
			item->priv->preferred_width = g_value_get_int (value);
			break;
		case PROP_PREFERRED_HEIGHT:
			item->priv->preferred_height = g_value_get_int (value);
			break;
		case PROP_ICONIFIED:
			if (g_value_get_boolean (value)) {
				if (!item->priv->iconified) gdl_dock_item_iconify_item (item);
			} else if (item->priv->iconified) {
				item->priv->iconified = FALSE;
				gtk_widget_set_visible (GTK_WIDGET (item), true);
				gtk_widget_queue_resize (GTK_WIDGET (item));
			}
			break;
		case PROP_CLOSED:
			if (g_value_get_boolean (value)) {
				gtk_widget_set_visible (GTK_WIDGET (item), false);
			} else {
				if (!item->priv->iconified && !gdl_dock_item_is_placeholder (item))
					gtk_widget_set_visible (GTK_WIDGET (item), true);
			}
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (g_object, prop_id, pspec);
			break;
	}
}

static void
gdl_dock_item_get_property (GObject *g_object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    GdlDockItem *item = GDL_DOCK_ITEM (g_object);

    switch (prop_id) {
        case PROP_ORIENTATION:
            g_value_set_enum (value, item->priv->orientation);
            break;
        case PROP_RESIZE:
            g_value_set_boolean (value, item->priv->resize);
            break;
        case PROP_EXPAND:
            g_value_set_boolean (value, item->priv->expand);
            break;
        case PROP_BEHAVIOR:
            g_value_set_flags (value, item->priv->behavior);
            break;
        case PROP_LOCKED:
            g_value_set_boolean (value, !GDL_DOCK_ITEM_NOT_LOCKED (item));
            break;
        case PROP_PREFERRED_WIDTH:
            g_value_set_int (value, item->priv->preferred_width);
            break;
        case PROP_PREFERRED_HEIGHT:
            g_value_set_int (value, item->priv->preferred_height);
            break;
        case PROP_ICONIFIED:
            g_value_set_boolean (value, gdl_dock_item_is_iconified (item));
            break;
        case PROP_CLOSED:
            g_value_set_boolean (value, gdl_dock_item_is_closed (item));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (g_object, prop_id, pspec);
            break;
    }
}

static void
gdl_dock_item_dispose (GObject *object)
{
    GdlDockItem *item = GDL_DOCK_ITEM (object);
    GdlDockItemPrivate *priv = item->priv;

    if (priv->tab_label) {
        gdl_dock_item_set_tablabel (item, NULL);
    }

    if (priv->menu) {
#ifdef GTK4_TODO
        gtk_menu_detach (GTK_MENU (priv->menu));
#endif
        priv->menu = NULL;
        priv->menu_item_hide = NULL;
    }

    if (priv->grip) {
		g_clear_pointer(&priv->grip, gtk_widget_unparent);
    }

    if (item->child) {
		g_clear_pointer(&item->child, gtk_widget_unparent);
    }

	if (!gdl_dock_object_is_compound(GDL_DOCK_OBJECT(object))) {
		g_signal_emit_by_name(gdl_dock_object_get_master(GDL_DOCK_OBJECT(object)), "dock-item-removed", object);
	}

    G_OBJECT_CLASS (gdl_dock_item_parent_class)->dispose (object);
}

void
gdl_dock_item_add (GdlDockItem* item, GtkWidget *widget)
{
    g_return_if_fail (GDL_IS_DOCK_ITEM (item));

    if (GDL_IS_DOCK_ITEM (widget)) {
        g_warning (_("You can't add a dock object (%p of type %s) inside a %s. Use a GdlDock or some other compound dock object."), widget, G_OBJECT_TYPE_NAME (widget), G_OBJECT_TYPE_NAME (item));
        return;
    }

    if (item->child) {
        g_warning (_("Attempting to add a widget with type %s to a %s, but it can only contain one widget at a time; it already contains a widget of type %s"), G_OBJECT_TYPE_NAME (widget), G_OBJECT_TYPE_NAME (item), G_OBJECT_TYPE_NAME (item->child));
        return;
    }

	gtk_widget_insert_before (widget, GTK_WIDGET (item), NULL);
    item->child = widget;
}

static void
gdl_dock_item_remove_widgets (GdlDockObject* object)
{
    GdlDockItem* item = GDL_DOCK_ITEM (object);

    if (item->priv->grip) {
        gboolean grip_was_visible = gtk_widget_get_visible (item->priv->grip);
        g_clear_pointer(&item->priv->grip, gtk_widget_unparent);
        if (grip_was_visible)
            gtk_widget_queue_resize (GTK_WIDGET (item));
    }

	if (!gdl_dock_object_is_compound(object))
		g_clear_pointer(&item->child, gtk_widget_unparent);

	GDL_DOCK_OBJECT_CLASS (gdl_dock_item_parent_class)->remove_widgets (object);
}

static void
gdl_dock_item_destroy (GdlDockObject* object)
{
	GtkWidget* grip = gdl_dock_item_get_grip(GDL_DOCK_ITEM(object));
	if (grip) {
		GListModel* controllers = gtk_widget_observe_controllers(grip);
		for (int i = 0; i < g_list_model_get_n_items(controllers); i++) {
			GtkEventController* controller = g_list_model_get_item(controllers, i);
			const char* name = gtk_event_controller_get_name(controller);
			if (name && !strcmp(name, "drag")) {
#if 0
				gtk_widget_remove_controller(grip, controller);
#else
				g_object_unref(object); // workaround for ref_count not being reduced following removal of the controller
#endif
			}
		}
	}

	GDL_DOCK_OBJECT_CLASS (gdl_dock_item_parent_class)->destroy (object);
}

static void
gdl_dock_item_remove (GdlDockObject *container, GtkWidget *widget)
{
    g_return_if_fail (GDL_IS_DOCK_ITEM (container));

    GdlDockItem* item = GDL_DOCK_ITEM (container);

	gdl_dock_item_remove_widgets (container);

    gdl_dock_item_drag_end (item, TRUE);

    g_return_if_fail (widget == item->child);

    gboolean was_visible = gtk_widget_get_visible (widget);

    gtk_widget_unparent (widget);
	if (widget == item->child)
	    item->child = NULL;

    if (was_visible)
        gtk_widget_set_visible (GTK_WIDGET (container), false);
}

static void
gdl_dock_item_set_focus_child (GtkWidget *container, GtkWidget *child)
{
    g_return_if_fail (GDL_IS_DOCK_ITEM (container));

    if (GTK_WIDGET_CLASS (gdl_dock_item_parent_class)->set_focus_child) {
        (* GTK_WIDGET_CLASS (gdl_dock_item_parent_class)->set_focus_child) (container, child);
    }
}

static void
gdl_dock_item_get_preferred_width (GtkWidget *widget, gint *minimum, gint *natural)
{
    gint child_min = 0;
	gint child_nat = 0;

    g_return_if_fail (GDL_IS_DOCK_ITEM (widget));

    GdlDockItem *item = GDL_DOCK_ITEM (widget);

    /* If our child is not visible, we still request its size, since
       we won't have any useful hint for our size otherwise.  */
    if (item->child)
		gtk_widget_measure (item->child, GTK_ORIENTATION_HORIZONTAL, -1, &child_min, &child_nat, NULL, NULL);

    if (item->priv->orientation == GTK_ORIENTATION_HORIZONTAL) {
        if (GDL_DOCK_ITEM_GRIP_SHOWN (item)) {
			gtk_widget_measure (item->priv->grip, GTK_ORIENTATION_HORIZONTAL, -1, minimum, natural, NULL, NULL);
        } else
            *minimum = *natural = 0;

        if (item->child) {
            *minimum += child_min;
            *natural += child_nat;
        }
    } else {
        if (item->child) {
            *minimum = child_min;
            *natural = child_nat;
        } else
            *minimum = *natural = 0;
    }

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    GtkStyleContext *context = gtk_widget_get_style_context (widget);
    GtkBorder padding;
    gtk_style_context_get_padding (context, &padding);
#pragma GCC diagnostic warning "-Wdeprecated-declarations"

    *minimum += padding.left + padding.right;
    *minimum = 0; // this prevents gtk from complaining when the available space is small.
    *natural += padding.left + padding.right;
}

static void
gdl_dock_item_get_preferred_height (GtkWidget *widget, gint *minimum, gint *natural)
{
    gint child_min, child_nat;

    g_return_if_fail (GDL_IS_DOCK_ITEM (widget));

    GdlDockItem* item = GDL_DOCK_ITEM (widget);

    /* If our child is not visible, we still request its size, since
       we won't have any useful hint for our size otherwise.  */
    if (item->child)
		// we have to ignore the measured height because in many cases the reported min height is the same as the natural height. We use the grip height as the min height.
		gtk_widget_measure (item->child, GTK_ORIENTATION_VERTICAL, -1, NULL, &child_nat, NULL, NULL);
    else
        child_min = child_nat = 0;

    if (item->priv->orientation == GTK_ORIENTATION_HORIZONTAL) {
        if (item->child) {
            *minimum = child_min;
            *natural = child_nat;
        } else
            *minimum = *natural = 0;
    } else {
        if (GDL_DOCK_ITEM_GRIP_SHOWN (item)) {
			gtk_widget_measure (item->priv->grip, GTK_ORIENTATION_VERTICAL, -1, minimum, natural, NULL, NULL);
        } else
            *minimum = *natural = 0;

        if (item->child) {
            *minimum += child_min;
            *natural += child_nat;
        }
    }

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    GtkStyleContext* context = gtk_widget_get_style_context (widget);
    GtkBorder padding;
    gtk_style_context_get_padding (context, &padding);
#pragma GCC diagnostic warning "-Wdeprecated-declarations"

    if (*minimum) *minimum += padding.top + padding.bottom;
    *natural += padding.top + padding.bottom;
}

static void
gdl_dock_item_measure (GtkWidget* widget, GtkOrientation orientation, int for_size, int* min, int* natural, int* min_baseline, int* natural_baseline)
{
	if (orientation == GTK_ORIENTATION_HORIZONTAL)
		gdl_dock_item_get_preferred_width (widget, min, natural);
	else
		gdl_dock_item_get_preferred_height (widget, min, natural);
}

static void
gdl_dock_item_size_allocate (GtkWidget *widget, int w, int h, int baseline)
{
    g_return_if_fail (GDL_IS_DOCK_ITEM (widget));

    GdlDockItem* item = GDL_DOCK_ITEM (widget);

    /* Once size is allocated, preferred size is no longer necessary */
    item->priv->preferred_height = -1;
    item->priv->preferred_width = -1;

#ifdef GTK4_TODO
    if (gtk_widget_get_realized (widget))
        gdk_surface_move_resize (gtk_widget_get_window (widget), allocation->x, allocation->y, allocation->width, allocation->height);
#endif

	if (item->child) {
		GtkRequisition grip_req = {0,};

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
		GtkStyleContext *context = gtk_widget_get_style_context (widget);
		GtkBorder padding;
		gtk_style_context_get_padding (context, &padding);
#pragma GCC diagnostic warning "-Wdeprecated-declarations"

		if (GDL_DOCK_ITEM_GRIP_SHOWN (item)) {
			GtkAllocation grip_alloc = {
				.x = padding.left,
				.y = padding.top,
				.width = w - padding.left - padding.right,
				.height = h - padding.top - padding.bottom,
			};

			gtk_widget_get_preferred_size (item->priv->grip, &grip_req, NULL);
			if (item->priv->orientation == GTK_ORIENTATION_HORIZONTAL) {
				grip_alloc.width = grip_req.width;
			} else {
				grip_alloc.height = grip_req.height;
			}
			gtk_widget_size_allocate (item->priv->grip, &grip_alloc, -1);
		}

		if (gtk_widget_get_visible (item->child)) {

			GtkAllocation child_allocation = {
				.x = padding.left,
				.y = padding.top,
				.width = w - padding.left - padding.right,
				.height = h - padding.top - padding.bottom,
			};

			if (item->priv->orientation == GTK_ORIENTATION_HORIZONTAL) {
				child_allocation.x += grip_req.width;
				child_allocation.width -= grip_req.width;
			} else {
				child_allocation.y += grip_req.height;
				child_allocation.height -= grip_req.height;
			}

			/* Allocation can't be negative */
			if (child_allocation.width < 0)
				child_allocation.width = 0;
			if (child_allocation.height < 0)
				child_allocation.height = 0;

			gtk_widget_size_allocate (item->child, &child_allocation, -1);
		}
	}
}

static void
gdl_dock_item_map (GtkWidget *widget)
{
    g_return_if_fail (widget != NULL);
    g_return_if_fail (GDL_IS_DOCK_ITEM (widget));
 
    GTK_WIDGET_CLASS (gdl_dock_item_parent_class)->map (widget);

    GdlDockItem* item = GDL_DOCK_ITEM (widget);

    if (item->child
        && gtk_widget_get_visible (item->child)
        && !gtk_widget_get_mapped (item->child)
	)
        gtk_widget_map (item->child);

    if (item->priv->grip
        && gtk_widget_get_visible (GTK_WIDGET (item->priv->grip))
        && !gtk_widget_get_mapped (GTK_WIDGET (item->priv->grip))
	)
        gtk_widget_map (item->priv->grip);
}

static void
gdl_dock_item_unmap (GtkWidget *widget)
{
	g_return_if_fail (widget);
	g_return_if_fail (GDL_IS_DOCK_ITEM (widget));

	GdlDockItem *item = GDL_DOCK_ITEM (widget);

    GTK_WIDGET_CLASS (gdl_dock_item_parent_class)->unmap (widget);

	if (item->child)
		gtk_widget_unmap (item->child);

	if (item->priv->grip)
		gtk_widget_unmap (item->priv->grip);
}

static void
gdl_dock_item_move_focus_child (GdlDockItem *item, GtkDirectionType dir)
{
    g_return_if_fail (GDL_IS_DOCK_ITEM (item));
    gtk_widget_child_focus (GTK_WIDGET (item->child), dir);
}

#define EVENT_IN_GRIP_EVENT_WINDOW(ev,gr) \
    ((gr) != NULL && gdl_dock_item_grip_has_event (GDL_DOCK_ITEM_GRIP (gr), (GdkEvent *)(ev)))

static void
gdl_dock_item_click_gesture_pressed (GtkGestureClick *gesture, int n_press, double widget_x, double widget_y, GdlDockItem *item)
{
    g_return_if_fail (item);
    g_return_if_fail (GDL_IS_DOCK_ITEM (item));

	guint button = gtk_gesture_single_get_current_button (GTK_GESTURE_SINGLE (gesture));

#ifdef GTK4_TODO
    if (!EVENT_IN_GRIP_EVENT_WINDOW (event, item->priv->grip))
        return;
#endif

    gboolean locked = !GDL_DOCK_ITEM_NOT_LOCKED (item);
    gboolean event_handled = FALSE;

	graphene_rect_t allocation;
#pragma GCC diagnostic ignored "-Wunused-result"
	gtk_widget_compute_bounds(item->priv->grip, item->priv->grip, &allocation);
#pragma GCC diagnostic warning "-Wunused-result"

    /* Check if user clicked on the drag handle. */
    gboolean in_handle = (item->priv->orientation == GTK_ORIENTATION_HORIZONTAL)
		? widget_x < allocation.size.width
		: (item->priv->orientation == GTK_ORIENTATION_VERTICAL)
			? widget_y < allocation.size.height
			: false;

	switch (button) {
		case GDK_BUTTON_PRIMARY:
			if (!locked) {
				if (!gdl_dock_item_or_child_has_focus (item))
					gtk_widget_grab_focus (GTK_WIDGET (item));

				/* Set in_drag flag, grab pointer and call begin drag operation. */
				if (in_handle) {
					item->priv->start_x = widget_x;
					item->priv->start_y = widget_y;

					item->priv->in_predrag = TRUE;

					gdl_dock_item_grip_set_cursor (GDL_DOCK_ITEM_GRIP (item->priv->grip), TRUE);

					event_handled = TRUE;
				};
			}
			break;
		case GDK_BUTTON_SECONDARY:
        	gdl_dock_item_popup_menu (item, button, widget_x, widget_y);
	        event_handled = TRUE;
			break;
		default:
	}

	if (event_handled) gtk_gesture_set_state (GTK_GESTURE(gesture), GTK_EVENT_SEQUENCE_CLAIMED);
}

static void
gdl_dock_item_click_gesture_released (GtkGestureClick *gesture, int n_press, double widget_x, double widget_y, GdlDockItem *item)
{
	guint button = gtk_gesture_single_get_current_button (GTK_GESTURE_SINGLE (gesture));
    gboolean locked = !GDL_DOCK_ITEM_NOT_LOCKED (item);

    if (!locked && button == GDK_BUTTON_PRIMARY) {
        /* User dropped widget somewhere */
        if (gdl_dock_item_drag_end (item, FALSE))
			gtk_gesture_set_state (GTK_GESTURE(gesture), GTK_EVENT_SEQUENCE_CLAIMED);
	}
}

#ifdef GTK4_TODO
static gint
gdl_dock_item_motion (GtkWidget *widget, GdkEventMotion *event)
{
    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (GDL_IS_DOCK_ITEM (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);

    GdlDockItem* item = GDL_DOCK_ITEM (widget);

    /* motion drag events are coming from the grip window because there is an
     * automatic pointer grab when clicking on the grip widget to start drag. */
    if (!EVENT_IN_GRIP_EVENT_WINDOW (event, item->priv->grip))
        return FALSE;

    if (item->priv->in_predrag) {
        if (gtk_drag_check_threshold (widget, item->priv->start_x, item->priv->start_y, event->x, event->y)) {
            item->priv->in_predrag = FALSE;
            item->priv->dragoff_x = item->priv->start_x;
            item->priv->dragoff_y = item->priv->start_y;

            gdl_dock_item_drag_start (item);
        }
    }

    if (!item->priv->in_drag)
        return FALSE;

    gint new_x = event->x_root;
    gint new_y = event->y_root;

    g_signal_emit (item, gdl_dock_item_signals [DOCK_DRAG_MOTION], 0, event->device, new_x, new_y);

    return TRUE;
}

static gboolean
gdl_dock_item_key_press (GtkWidget *widget, GdkEventKey *event)
{
    /* Cancel drag */
    if (gdl_dock_item_drag_end (GDL_DOCK_ITEM (widget), TRUE))
        return TRUE;
    else
        return GTK_WIDGET_CLASS (gdl_dock_item_parent_class)->key_press_event (widget, event);
}
#endif

static gboolean
gdl_dock_item_dock_request (GdlDockObject *object, gint x, gint y, GdlDockRequest *request)
{
    /* we get (x,y) in our allocation coordinates system */

	graphene_rect_t alloc;
#pragma GCC diagnostic ignored "-Wunused-result"
	gtk_widget_compute_bounds(GTK_WIDGET(object), GTK_WIDGET(gdl_dock_object_get_toplevel(object)), &alloc);
#pragma GCC diagnostic warning "-Wunused-result"

    /* Get coordinates relative to our window. */
    int rel_x = x - alloc.origin.x;
    int rel_y = y - alloc.origin.y;

    /* Location is inside. */
    if (
		rel_x > 0 && rel_x < alloc.size.width &&
		rel_y > 0 && rel_y < alloc.size.height
	) {
        float rx, ry;
        GtkRequisition my, other;
        gint divider = -1;

        /* this are for calculating the extra docking parameter */
        gdl_dock_item_preferred_size (GDL_DOCK_ITEM (request->applicant), &other);
        gdl_dock_item_preferred_size (GDL_DOCK_ITEM (object), &my);

        /* Calculate location in terms of the available space (0-100%). */
        rx = (float) rel_x / alloc.size.width;
        ry = (float) rel_y / alloc.size.height;

        /* Determine dock location. */
        if (rx < SPLIT_RATIO) {
            request->position = GDL_DOCK_LEFT;
            divider = other.width;
        }
        else if (rx > (1 - SPLIT_RATIO)) {
            request->position = GDL_DOCK_RIGHT;
            rx = 1 - rx;
            divider = MAX (0, my.width - other.width);
        }
        else if (ry < SPLIT_RATIO && ry < rx) {
            request->position = GDL_DOCK_TOP;
            divider = other.height;
        }
        else if (ry > (1 - SPLIT_RATIO) && (1 - ry) < rx) {
            request->position = GDL_DOCK_BOTTOM;
            divider = MAX (0, my.height - other.height);
        }
        else
            request->position = GDL_DOCK_CENTER;

        /* Reset rectangle coordinates to entire item. */
		request->rect = (graphene_rect_t){ .size = { alloc.size.width, alloc.size.height }};

        GdlDockItemBehavior behavior = GDL_DOCK_ITEM(object)->priv->behavior;

        /* Calculate docking indicator rectangle size for new locations. Only
           do this when we're not over the item's current location. */
        if (request->applicant != object) {
            switch (request->position) {
                case GDL_DOCK_TOP:
                    if (behavior & GDL_DOCK_ITEM_BEH_CANT_DOCK_TOP)
                        return FALSE;
                    request->rect.size.height *= SPLIT_RATIO;
                    break;
                case GDL_DOCK_BOTTOM:
                    if (behavior & GDL_DOCK_ITEM_BEH_CANT_DOCK_BOTTOM)
                        return FALSE;
                    request->rect.origin.y += request->rect.size.height * (1 - SPLIT_RATIO);
                    request->rect.size.height *= SPLIT_RATIO;
                    break;
                case GDL_DOCK_LEFT:
                    if (behavior & GDL_DOCK_ITEM_BEH_CANT_DOCK_LEFT)
                        return FALSE;
                    request->rect.size.width *= SPLIT_RATIO;
                    break;
                case GDL_DOCK_RIGHT:
                    if (behavior & GDL_DOCK_ITEM_BEH_CANT_DOCK_RIGHT)
                        return FALSE;
                    request->rect.origin.x += request->rect.size.width * (1 - SPLIT_RATIO);
                    request->rect.size.width *= SPLIT_RATIO;
                    break;
                case GDL_DOCK_CENTER:
                    if (behavior & GDL_DOCK_ITEM_BEH_CANT_DOCK_CENTER)
                        return FALSE;
                    request->rect.origin.x = request->rect.size.width * SPLIT_RATIO/2;
                    request->rect.origin.y = request->rect.size.height * SPLIT_RATIO/2;
                    request->rect.size.width = (request->rect.size.width * (1 - SPLIT_RATIO/2)) - request->rect.origin.x;
                    request->rect.size.height = (request->rect.size.height * (1 - SPLIT_RATIO/2)) - request->rect.origin.y;
                    break;
                default:
                    break;
            }
        }

        /* adjust returned coordinates so they are have the same
           origin as our window */
        request->rect.origin.x += alloc.origin.x;
        request->rect.origin.y += alloc.origin.y;

        /* Set possible target location and return TRUE. */
        request->target = object;

        /* fill-in other dock information */
        if (request->position != GDL_DOCK_CENTER && divider >= 0) {
            if (G_IS_VALUE (&request->extra))
                g_value_unset (&request->extra);
            g_value_init (&request->extra, G_TYPE_UINT);
            g_value_set_uint (&request->extra, (guint) divider);
        }

        return TRUE;
    }
    else /* No docking possible at this location. */
        return FALSE;
}

static void
gdl_dock_item_dock (GdlDockObject *object, GdlDockObject *requestor, GdlDockPlacement  position, GValue *other_data)
{
    GdlDockObject *new_parent = NULL;
    GdlDockObject *requestor_parent;
    gboolean       add_ourselves_first = FALSE;

    guint	   available_space=0;
    gint	   pref_size=-1;
    guint	   splitpos=0;
    GtkRequisition req, object_req, parent_req;

    GdlDockObject* parent = gdl_dock_object_get_parent_object (object);
    gdl_dock_item_preferred_size (GDL_DOCK_ITEM (requestor), &req);
    gdl_dock_item_preferred_size (GDL_DOCK_ITEM (object), &object_req);
    if (GDL_IS_DOCK_ITEM (parent)) {
        gdl_dock_item_preferred_size (GDL_DOCK_ITEM (parent), &parent_req);
    } else {
		graphene_rect_t allocation;
#pragma GCC diagnostic ignored "-Wunused-result"
		gtk_widget_compute_bounds (GTK_WIDGET(parent), GTK_WIDGET(parent), &allocation);
#pragma GCC diagnostic warning "-Wunused-result"
        parent_req.height = allocation.size.height;
        parent_req.width = allocation.size.width;
    }

    /* If preferred size is not set on the requestor (perhaps a new item),
     * then estimate and set it. The default value (either 0 or 1 pixels) is
     * not any good.
     */
    switch (position) {
        case GDL_DOCK_TOP:
        case GDL_DOCK_BOTTOM:
            if (req.width < 2)
            {
                req.width = object_req.width;
                g_object_set (requestor, "preferred-width", req.width, NULL);
            }
            if (req.height < 2)
            {
                req.height = NEW_DOCK_ITEM_RATIO * object_req.height;
                g_object_set (requestor, "preferred-height", req.height, NULL);
            }
            if (req.width > 1)
                g_object_set (object, "preferred-width", req.width, NULL);
            if (req.height > 1)
                g_object_set (object, "preferred-height",
                              object_req.height - req.height, NULL);
            break;
        case GDL_DOCK_LEFT:
        case GDL_DOCK_RIGHT:
            if (req.height < 2)
            {
                req.height = object_req.height;
                g_object_set (requestor, "preferred-height", req.height, NULL);
            }
            if (req.width < 2)
            {
                req.width = NEW_DOCK_ITEM_RATIO * object_req.width;
                g_object_set (requestor, "preferred-width", req.width, NULL);
            }
            if (req.height > 1)
                g_object_set (object, "preferred-height", req.height, NULL);
            if (req.width > 1)
                g_object_set (object, "preferred-width",
                          object_req.width - req.width, NULL);
            break;
        case GDL_DOCK_CENTER:
            if (req.height < 2)
            {
                req.height = object_req.height;
                g_object_set (requestor, "preferred-height", req.height, NULL);
            }
            if (req.width < 2)
            {
                req.width = object_req.width;
                g_object_set (requestor, "preferred-width", req.width, NULL);
            }
            if (req.height > 1)
                g_object_set (object, "preferred-height", req.height, NULL);
            if (req.width > 1)
                g_object_set (object, "preferred-width", req.width, NULL);
            break;
        default:
        {
            GEnumClass *enum_class = G_ENUM_CLASS (g_type_class_ref (GDL_TYPE_DOCK_PLACEMENT));
            GEnumValue *enum_value = g_enum_get_value (enum_class, position);
            const gchar *name = enum_value ? enum_value->value_name : NULL;

            g_warning (_("Unsupported docking strategy %s in dock object of type %s"),
                       name,  G_OBJECT_TYPE_NAME (object));
            g_type_class_unref (enum_class);
            return;
        }
    }
    switch (position) {
        case GDL_DOCK_TOP:
        case GDL_DOCK_BOTTOM:
            /* get a paned style dock object */
            new_parent = g_object_new (gdl_dock_object_type_from_nick ("paned"),
                                       "orientation", GTK_ORIENTATION_VERTICAL,
                                       "preferred-width", object_req.width,
                                       "preferred-height", object_req.height,
                                       NULL);
            add_ourselves_first = (position == GDL_DOCK_BOTTOM);
            if (parent)
                available_space = parent_req.height;
            pref_size = req.height;
            break;
        case GDL_DOCK_LEFT:
        case GDL_DOCK_RIGHT:
            new_parent = g_object_new (gdl_dock_object_type_from_nick ("paned"),
                                       "orientation", GTK_ORIENTATION_HORIZONTAL,
                                       "preferred-width", object_req.width,
                                       "preferred-height", object_req.height,
                                       NULL);
            add_ourselves_first = (position == GDL_DOCK_RIGHT);
            if(parent)
                available_space = parent_req.width;
            pref_size = req.width;
            break;
        case GDL_DOCK_CENTER:
            /* If the parent is already a DockNotebook, we don't need
             to create a new one. */
            if (!GDL_IS_DOCK_NOTEBOOK (parent))
            {
                new_parent = g_object_new (gdl_dock_object_type_from_nick ("notebook"),
                                           "preferred-width", object_req.width,
                                           "preferred-height", object_req.height,
                                           NULL);
                add_ourselves_first = TRUE;
            }
            break;
        default:
        {
            GEnumClass *enum_class = G_ENUM_CLASS (g_type_class_ref (GDL_TYPE_DOCK_PLACEMENT));
            GEnumValue *enum_value = g_enum_get_value (enum_class, position);
            const gchar *name = enum_value ? enum_value->value_name : NULL;

            g_warning (_("Unsupported docking strategy %s in dock object of type %s"),
                       name,  G_OBJECT_TYPE_NAME (object));
            g_type_class_unref (enum_class);
            return;
        }
    }

    /* freeze the parent so it doesn't reduce automatically */
    if (parent)
        gdl_dock_object_freeze (parent);


    if (new_parent) {
        /* ref ourselves since we could be destroyed when detached */
        g_object_ref (object);
        gdl_dock_object_detach (object, FALSE);

        /* freeze the new parent, so reduce won't get called before it's actually added to our parent */
        gdl_dock_object_freeze (new_parent);

        /* bind the new parent to our master, so the following adds work */
        gdl_dock_object_bind (new_parent, gdl_dock_object_get_master (GDL_DOCK_OBJECT (object)));

        /* add the objects */
        if (add_ourselves_first) {
            gdl_dock_object_add_child (new_parent, GTK_WIDGET (object));
            gdl_dock_object_add_child (new_parent, GTK_WIDGET (requestor));
            splitpos = available_space - pref_size;
        } else {
            gdl_dock_object_add_child (new_parent, GTK_WIDGET (requestor));
            gdl_dock_object_add_child (new_parent, GTK_WIDGET (object));
            splitpos = pref_size;
        }

        /* add the new parent to the parent */
        if (parent)
            gdl_dock_object_add_child (parent, GTK_WIDGET (new_parent));

        gdl_dock_object_thaw (new_parent);

        /* use extra docking parameter */
        if (position != GDL_DOCK_CENTER && other_data &&
            G_VALUE_HOLDS (other_data, G_TYPE_UINT))
		{
            g_object_set (G_OBJECT (new_parent), "position", g_value_get_uint (other_data), NULL);

        } else if (splitpos > 0 && splitpos < available_space) {
            g_object_set (G_OBJECT (new_parent), "position", splitpos, NULL);
        }

        g_object_unref (object);
    } else {
        /* If the parent is already a DockNotebook, we don't need to create a new one. */
        gdl_dock_object_add_child (parent, GTK_WIDGET (requestor));
    }

    requestor_parent = gdl_dock_object_get_parent_object (requestor);
    if (GDL_IS_DOCK_NOTEBOOK (requestor_parent) && gtk_widget_get_visible (GTK_WIDGET (requestor))) {
        /* Activate the page we just added */
        GdlDockItem* notebook = GDL_DOCK_ITEM (gdl_dock_object_get_parent_object (requestor));
		GtkNotebook* nb = GTK_NOTEBOOK (GDL_SWITCHER(notebook->child)->notebook);
        gtk_notebook_set_current_page (nb, gtk_notebook_page_num (nb, GTK_WIDGET (requestor)));
    }

    if (parent)
        gdl_dock_object_thaw (parent);
}

static void
gdl_dock_item_present (GdlDockObject *object, GdlDockObject *child)
{
    GdlDockItem *item = GDL_DOCK_ITEM (object);

    gdl_dock_item_show_item (item);
    GDL_DOCK_OBJECT_CLASS (gdl_dock_item_parent_class)->present (object, child);
}

#ifdef GTK4_TODO
static void
gdl_dock_item_detach_menu (GtkWidget *widget, GtkMenu *menu)
{
    GdlDockItem *item = GDL_DOCK_ITEM (widget);

    item->priv->menu = NULL;
}
#endif

static void
gdl_dock_item_popup_menu (GdlDockItem *item, guint button, double x, double y)
{
	GdlDockItemPrivate* _item = item->priv;

#ifdef GTK4_TODO
    if (item->priv->menu_item_hide)
        gtk_widget_set_visible(item->priv->menu_item_hide, !GDL_DOCK_ITEM_CANT_CLOSE(item));
#endif

    if (!_item->menu) {
		GMenuModel* model = (GMenuModel*)g_menu_new ();

		_item->menu = gtk_popover_menu_new_from_model (model);
		gtk_widget_set_parent (_item->menu, GTK_WIDGET(item));
		gtk_popover_set_has_arrow (GTK_POPOVER(_item->menu), false);

		GMenu* menu = G_MENU (model);

		GSimpleActionGroup *group = g_simple_action_group_new ();
		gtk_widget_insert_action_group (_item->menu, "dock-item", G_ACTION_GROUP(group));

        if (_item->behavior & GDL_DOCK_ITEM_BEH_LOCKED) {
			GSimpleAction* action = g_simple_action_new ("unlock", NULL);
      		g_action_map_add_action (G_ACTION_MAP (group), G_ACTION (action));
            g_signal_connect (action, "activate", G_CALLBACK (gdl_dock_item_unlock_cb), item);
			g_menu_append (menu, _("UnLock"), "dock-item.lock");
        } else {
      		GSimpleAction* action = g_simple_action_new_stateful ("hide", NULL, g_variant_new_boolean (FALSE));
      		g_action_map_add_action (G_ACTION_MAP (group), G_ACTION (action));
            g_signal_connect (action, "activate", G_CALLBACK (gdl_dock_item_hide_cb), item);
			g_menu_append (menu, _("Hide"), "dock-item.hide");

			action = g_simple_action_new ("lock", NULL);
      		g_action_map_add_action (G_ACTION_MAP (group), G_ACTION (action));
            g_signal_connect (action, "activate", G_CALLBACK (gdl_dock_item_lock_cb), item);
			g_menu_append (menu, _("Lock"), "dock-item.lock");
        }
	}

#ifdef GTK4_TODO
	gtk_popover_set_position (GTK_POPOVER(_item->menu), GTK_POS_LEFT);
#endif
	gtk_popover_popup(GTK_POPOVER(_item->menu));

#ifdef GTK4_TODO
	const GdkRectangle rect = {0,};
	gtk_popover_set_pointing_to (GTK_POPOVER(popover), &rect);
#endif
}

#ifdef GTK4_TODO
static void
gdl_dock_item_drag_start (GdlDockItem *item)
{
    if (!gtk_widget_get_realized (GTK_WIDGET (item)))
        gtk_widget_realize (GTK_WIDGET (item));

    item->priv->in_drag = TRUE;

    /* grab the keyboard & pointer. The pointer has already been grabbed by the grip
     * window when it has received a press button event. See gdk_pointer_grab. */
#ifdef GTK4_TODO
    gtk_grab_add (GTK_WIDGET (item));
#endif

    g_signal_emit (item, gdl_dock_item_signals [DOCK_DRAG_BEGIN], 0);
}
#endif

/* Terminate a drag operation, return TRUE if a drag or pre-drag was running */
static gboolean
gdl_dock_item_drag_end (GdlDockItem *item, gboolean cancel)
{
    if (item->priv->in_drag) {
        /* Release pointer & keyboard. */
#ifdef GTK4_TODO
        gtk_grab_remove (GTK_WIDGET (item));
#endif
        g_signal_emit (item, gdl_dock_item_signals [DOCK_DRAG_END], 0, cancel);
        gtk_widget_grab_focus (GTK_WIDGET (item));

        item->priv->in_drag = FALSE;
    }
    else if (item->priv->in_predrag) {
        item->priv->in_predrag = FALSE;
    }
    else {
        /* No drag not pre-drag has been started */
        return FALSE;
    }

#ifdef GTK4_TODO
    /* Restore old cursor */
    gdl_dock_item_grip_set_cursor (GDL_DOCK_ITEM_GRIP (item->priv->grip), FALSE);
#endif

    return TRUE;
}

#ifdef GTK4_TODO
static void
gdl_dock_item_tab_button (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    GtkAllocation allocation;

    GdlDockItem* item = GDL_DOCK_ITEM (data);

    if (!GDL_DOCK_ITEM_NOT_LOCKED (item))
        return;

    switch (event->button) {
        case 1:
            /* set dragoff_{x,y} as we the user clicked on the middle of the drag handle */
            switch (item->priv->orientation) {
            case GTK_ORIENTATION_HORIZONTAL:
                gtk_widget_get_allocation (GTK_WIDGET (data), &allocation);
                /*item->priv->dragoff_x = item->priv->grip_size / 2;*/
                item->priv->dragoff_y = allocation.height / 2;
                break;
            case GTK_ORIENTATION_VERTICAL:
                /*item->priv->dragoff_x = GTK_WIDGET (data)->allocation.width / 2;*/
                item->priv->dragoff_y = item->priv->grip_size / 2;
                break;
            };
            gdl_dock_item_drag_start (item);
            break;

        case 3:
            gdl_dock_item_popup_menu (item, button, event->button.x, event->button.y);
            break;

        default:
            break;
    }
}
#endif

static void
gdl_dock_item_hide_cb (GSimpleAction* self, GVariant* parameter, gpointer item)
{
    g_return_if_fail (item);

    gdl_dock_item_hide_item (item);
}

static void
gdl_dock_item_lock_cb (GSimpleAction* self, GVariant* parameter, gpointer item)
{
    g_return_if_fail (item);

    gdl_dock_item_lock (item);
}

static void
gdl_dock_item_unlock_cb (GSimpleAction* self, GVariant* parameter, gpointer item)
{
    g_return_if_fail (item);

    gdl_dock_item_unlock (item);
}

static void
gdl_dock_item_showhide_grip (GdlDockItem *item)
{
#ifdef GTK4_TODO
    gdl_dock_item_detach_menu (GTK_WIDGET (item), NULL);
#endif

    if (item->priv->grip && GDL_DOCK_ITEM_NOT_LOCKED(item) && GDL_DOCK_ITEM_HAS_GRIP(item)) {
        if (item->priv->grip_shown) {
            gdl_dock_item_grip_show_handle (GDL_DOCK_ITEM_GRIP (item->priv->grip));
        } else {
            gdl_dock_item_grip_hide_handle (GDL_DOCK_ITEM_GRIP (item->priv->grip));
        }
    }
}

static void
gdl_dock_item_real_set_orientation (GdlDockItem *item, GtkOrientation orientation)
{
    item->priv->orientation = orientation;

    if (gtk_widget_is_drawable (GTK_WIDGET (item)))
        gtk_widget_queue_draw (GTK_WIDGET (item));
    gtk_widget_queue_resize (GTK_WIDGET (item));
}


/* ----- Public interface ----- */

/**
 * gdl_dock_item_new:
 * @name: Unique name for identifying the dock object.
 * @long_name: Human readable name for the dock object.
 * @behavior: General behavior for the dock item (i.e. whether it can
 *            float, if it's locked, etc.), as specified by
 *            #GdlDockItemBehavior flags.
 *
 * Creates a new dock item widget.
 * Returns: The newly created dock item widget.
 **/
GtkWidget *
gdl_dock_item_new (const gchar *name, const gchar *long_name, GdlDockItemBehavior  behavior)
{
    GdlDockItem *item = GDL_DOCK_ITEM (g_object_new (GDL_TYPE_DOCK_ITEM, "name", name, "long-name", long_name, "behavior", behavior, NULL));
    gdl_dock_object_set_manual (GDL_DOCK_OBJECT (item));

    return GTK_WIDGET (item);
}

/**
 * gdl_dock_item_new_with_stock:
 * @name: Unique name for identifying the dock object.
 * @long_name: Human readable name for the dock object.
 * @stock_id: Stock icon for the dock object.
 * @behavior: General behavior for the dock item (i.e. whether it can
 *            float, if it's locked, etc.), as specified by
 *            #GdlDockItemBehavior flags.
 *
 * Creates a new dock item widget with a given stock id.
 * Returns: The newly created dock item widget.
 **/
GtkWidget *
gdl_dock_item_new_with_icon (const gchar *name, const gchar *long_name, const gchar *icon, GdlDockItemBehavior behavior)
{
	GdlDockItem *item = GDL_DOCK_ITEM (g_object_new (GDL_TYPE_DOCK_ITEM, "name", name, "long-name", long_name, "stock-id", icon, "behavior", behavior, NULL));
	gdl_dock_object_set_manual (GDL_DOCK_OBJECT (item));

	return GTK_WIDGET (item);
}

/**
 * gdl_dock_item_new_with_pixbuf_icon:
 * @name: Unique name for identifying the dock object.
 * @long_name: Human readable name for the dock object.
 * @pixbuf_icon: Pixbuf icon for the dock object.
 * @behavior: General behavior for the dock item (i.e. whether it can
 *            float, if it's locked, etc.), as specified by
 *            #GdlDockItemBehavior flags.
 *
 * Creates a new dock item grip widget with a given pixbuf icon.
 * Returns: The newly created dock item grip widget.
 *
 * Since: 3.3.2
 **/
GtkWidget *
gdl_dock_item_new_with_pixbuf_icon (const gchar *name, const gchar *long_name, const GdkPixbuf *pixbuf_icon, GdlDockItemBehavior behavior)
{
    GdlDockItem *item = GDL_DOCK_ITEM (g_object_new (GDL_TYPE_DOCK_ITEM, "name", name, "long-name", long_name, "pixbuf-icon", pixbuf_icon, "behavior", behavior, NULL));

    gdl_dock_object_set_manual (GDL_DOCK_OBJECT (item));
    gdl_dock_item_set_tablabel (item, gtk_label_new (long_name));

    return GTK_WIDGET (item);
}

/* convenient function (and to preserve source compat) */
/**
 * gdl_dock_item_dock_to:
 * @item: The dock item that will be relocated to the dock position.
 * @target: (allow-none): The dock item that will be used as the point of reference.
 * @position: The position to dock #item, relative to #target.
 * @docking_param: This value is unused, and will be ignored.
 *
 * Relocates a dock item to a new location relative to another dock item.
 **/
void
gdl_dock_item_dock_to (GdlDockItem *item, GdlDockItem *target, GdlDockPlacement position, gint docking_param)
{
    g_return_if_fail (item != NULL);
    g_return_if_fail (item != target);
    g_return_if_fail (target != NULL || position == GDL_DOCK_FLOATING);
    g_return_if_fail ((item->priv->behavior & GDL_DOCK_ITEM_BEH_NEVER_FLOATING) == 0 || position != GDL_DOCK_FLOATING);

    if (position == GDL_DOCK_FLOATING || !target) {
        GdlDockObject *controller;

        if (!gdl_dock_object_is_bound (GDL_DOCK_OBJECT (item))) {
            g_warning (_("Attempt to bind an unbound item %p"), item);
            return;
        }

        controller = gdl_dock_object_get_controller (GDL_DOCK_OBJECT (item));

        /* FIXME: save previous docking position for later
           re-docking... does this make sense now? */

        /* Create new floating dock for widget. */
        item->priv->dragoff_x = item->priv->dragoff_y = 0;
        gdl_dock_add_floating_item (GDL_DOCK (controller), item, 0, 0, -1, -1);

    } else
        gdl_dock_object_dock (GDL_DOCK_OBJECT (target), GDL_DOCK_OBJECT (item), position, NULL);
}

/**
 * gdl_dock_item_set_orientation:
 * @item: The dock item which will get it's orientation set.
 * @orientation: The orientation to set the item to. If the orientation
 * is set to #GTK_ORIENTATION_VERTICAL, the grip widget will be shown
 * along the top of the edge of item (if it is not hidden). If the
 * orientation is set to #GTK_ORIENTATION_HORIZONTAL, the grip widget
 * will be shown down the left edge of the item (even if the widget
 * text direction is set to RTL).
 *
 * This function sets the layout of the dock item.
 **/
void
gdl_dock_item_set_orientation (GdlDockItem *item, GtkOrientation orientation)
{
    g_return_if_fail (item);

    if (item->priv->orientation != orientation) {
        /* push the property down the hierarchy if our child supports it */
        if (item->child != NULL) {
            GParamSpec* pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (item->child), "orientation");
            if (pspec && pspec->value_type == GTK_TYPE_ORIENTATION)
                g_object_set (G_OBJECT (item->child), "orientation", orientation, NULL);
        };
        if (GDL_DOCK_ITEM_GET_CLASS (item)->set_orientation)
            GDL_DOCK_ITEM_GET_CLASS (item)->set_orientation (item, orientation);
        g_object_notify (G_OBJECT (item), "orientation");
    }
}

/**
 * gdl_dock_item_get_orientation:
 * @item: a #GdlDockItem
 *
 * Retrieves the orientation of the object.
 *
 * Return value: the orientation of the object.
 *
 * Since: 3.6
 */
GtkOrientation
gdl_dock_item_get_orientation (GdlDockItem *item)
{
    g_return_val_if_fail (GDL_IS_DOCK_ITEM (item), GTK_ORIENTATION_HORIZONTAL);

    return item->priv->orientation;
}

/**
 * gdl_dock_item_set_behavior_flags:
 * @item: The dock item which will get it's behavior set.
 * @behavior: Behavior flags to turn on
 * @clear: Whether to clear state before turning on @flags
 *
 * This function sets the behavior of the dock item.
 *
 * Since: 3.6
 */
void
gdl_dock_item_set_behavior_flags (GdlDockItem *item, GdlDockItemBehavior behavior, gboolean clear)
{
    GdlDockItemBehavior old_beh = item->priv->behavior;
    g_return_if_fail (GDL_IS_DOCK_ITEM (item));

    if (clear)
        item->priv->behavior = behavior;
    else
        item->priv->behavior |= behavior;

    if ((old_beh ^ behavior) & GDL_DOCK_ITEM_BEH_LOCKED) {
        gdl_dock_object_layout_changed_notify (GDL_DOCK_OBJECT (item));
        g_object_notify (G_OBJECT (item), "locked");
        gdl_dock_item_showhide_grip (item);
    }
}

/**
 * gdl_dock_item_unset_behavior_flags:
 * @item: The dock item which will get it's behavior set.
 * @behavior: Behavior flags to turn off
 *
 * This function sets the behavior of the dock item.
 *
 * Since: 3.6
 */
void
gdl_dock_item_unset_behavior_flags (GdlDockItem *item, GdlDockItemBehavior behavior)
{
    GdlDockItemBehavior old_beh = item->priv->behavior;
    g_return_if_fail (GDL_IS_DOCK_ITEM (item));

    item->priv->behavior &= ~behavior;

    if ((old_beh ^ behavior) & GDL_DOCK_ITEM_BEH_LOCKED) {
        gdl_dock_object_layout_changed_notify (GDL_DOCK_OBJECT (item));
        g_object_notify (G_OBJECT (item), "locked");
        gdl_dock_item_showhide_grip (item);
    }
}

/**
 * gdl_dock_item_get_behavior_flags:
 * @item: a #GdlDockItem
 *
 * Retrieves the behavior of the item.
 *
 * Return value: the behavior of the item.
 *
 * Since: 3.6
 */
GdlDockItemBehavior
gdl_dock_item_get_behavior_flags (GdlDockItem *item)
{
    g_return_val_if_fail (GDL_IS_DOCK_ITEM (item), GDL_DOCK_ITEM_BEH_NORMAL);

    GdlDockItemBehavior behavior = item->priv->behavior;

    if (!(behavior & GDL_DOCK_ITEM_BEH_NO_GRIP) && !(GDL_DOCK_ITEM_GET_CLASS (item)->priv->has_grip))
        behavior |= GDL_DOCK_ITEM_BEH_NO_GRIP;
    if (behavior & GDL_DOCK_ITEM_BEH_LOCKED)
        behavior |= GDL_DOCK_ITEM_BEH_CANT_ICONIFY |
                    GDL_DOCK_ITEM_BEH_CANT_ICONIFY |
                    GDL_DOCK_ITEM_BEH_CANT_DOCK_TOP |
                    GDL_DOCK_ITEM_BEH_CANT_DOCK_BOTTOM |
                    GDL_DOCK_ITEM_BEH_CANT_DOCK_LEFT |
                    GDL_DOCK_ITEM_BEH_CANT_DOCK_RIGHT |
                    GDL_DOCK_ITEM_BEH_CANT_DOCK_CENTER;

    return behavior;
}

/**
 * gdl_dock_item_get_tablabel:
 * @item: The dock item from which to get the tab label widget.
 *
 * Gets the current tab label widget. Note that this label widget is
 * only visible when the "switcher-style" property of the #GdlDockMaster
 * is set to #GDL_SWITCHER_STYLE_TABS
 *
 * Returns: (transfer none): Returns the tab label widget.
 **/
GtkWidget *
gdl_dock_item_get_tablabel (GdlDockItem *item)
{
    g_return_val_if_fail (item != NULL, NULL);
    g_return_val_if_fail (GDL_IS_DOCK_ITEM (item), NULL);

    return item->priv->tab_label;
}

/**
 * gdl_dock_item_set_tablabel:
 * @item: The dock item which will get it's tab label widget set.
 * @tablabel: The widget that will become the tab label.
 *
 * Replaces the current tab label widget with another widget. Note that
 * this label widget is only visible when the "switcher-style" property
 * of the #GdlDockMaster is set to #GDL_SWITCHER_STYLE_TABS
 **/
void
gdl_dock_item_set_tablabel (GdlDockItem *item, GtkWidget *tablabel)
{
    g_return_if_fail (item != NULL);

    if (item->priv->intern_tab_label) {
        item->priv->intern_tab_label = FALSE;
        g_signal_handler_disconnect (item, item->priv->notify_label);
        g_signal_handler_disconnect (item, item->priv->notify_stock_id);
    }

    if (item->priv->tab_label) {
        g_clear_pointer (&item->priv->tab_label, g_object_unref);
    }

    if (tablabel) {
        g_object_ref_sink (G_OBJECT (tablabel));
        item->priv->tab_label = tablabel;
    }
}

/**
 * gdl_dock_item_get_grip:
 * @item: The dock item from which to to get the grip of.
 *
 * This function returns the dock item's grip label widget.
 *
 * Returns: (allow-none) (transfer none): Returns the current label widget.
 **/
GtkWidget *
gdl_dock_item_get_grip (GdlDockItem *item)
{
    g_return_val_if_fail (item != NULL, NULL);
    g_return_val_if_fail (GDL_IS_DOCK_ITEM (item), NULL);

    return item->priv->grip;
}

/**
 * gdl_dock_item_hide_grip:
 * @item: The dock item to hide the grip of.
 *
 * This function hides the dock item's grip widget.
 **/
void
gdl_dock_item_hide_grip (GdlDockItem *item)
{
    g_return_if_fail (item != NULL);
    if (item->priv->grip_shown) {
        item->priv->grip_shown = FALSE;
        gdl_dock_item_showhide_grip (item);
    };
}

/**
 * gdl_dock_item_show_grip:
 * @item: The dock item to show the grip of.
 *
 * This function shows the dock item's grip widget.
 **/
void
gdl_dock_item_show_grip (GdlDockItem *item)
{
    g_return_if_fail (item != NULL);
    if (!item->priv->grip_shown) {
        item->priv->grip_shown = TRUE;
        gdl_dock_item_showhide_grip (item);
    };
}

/**
 * gdl_dock_item_notify_selected:
 * @item: the dock item to emit a selected signal on.
 *
 * This function emits the selected signal. It is to be used by #GdlSwitcher
 * to let clients know that this item has been switched to.
 **/
void
gdl_dock_item_notify_selected (GdlDockItem *item)
{
    g_signal_emit (item, gdl_dock_item_signals [SELECTED], 0);
}

/**
 * gdl_dock_item_notify_deselected:
 * @item: the dock item to emit a deselected signal on.
 *
 * This function emits the deselected signal. It is used by #GdlSwitcher
 * to let clients know that this item has been deselected.
 **/
void
gdl_dock_item_notify_deselected (GdlDockItem *item)
{
    g_signal_emit (item, gdl_dock_item_signals [DESELECTED], 0);
}

/* convenient function (and to preserve source compat) */
/**
 * gdl_dock_item_bind:
 * @item: The item to bind.
 * @dock: The #GdlDock widget to bind it to. Note that this widget must
 * be a type of #GdlDock.
 *
 * Binds this dock item to a new dock master.
 **/
void
gdl_dock_item_bind (GdlDockItem *item, GtkWidget *dock)
{
    g_return_if_fail (item != NULL);
    g_return_if_fail (dock == NULL || GDL_IS_DOCK (dock));

    gdl_dock_object_bind (GDL_DOCK_OBJECT (item), gdl_dock_object_get_master (GDL_DOCK_OBJECT (dock)));
}

/* convenient function (and to preserve source compat) */
/**
 * gdl_dock_item_unbind:
 * @item: The item to unbind.
 *
 * Unbinds this dock item from it's dock master.
 **/
void
gdl_dock_item_unbind (GdlDockItem *item)
{
    g_return_if_fail (item != NULL);

    gdl_dock_object_unbind (GDL_DOCK_OBJECT (item));
}

/**
 * gdl_dock_item_hide_item:
 * @item: The dock item to hide.
 *
 * This function hides the dock item. Since version 3.6, when dock items
 * are hidden they are not removed from the layout.
 *
 * The dock item close button causes the panel to be hidden.
 */
void
gdl_dock_item_hide_item (GdlDockItem *item)
{
    g_return_if_fail (item != NULL);

    gtk_widget_set_visible (GTK_WIDGET (item), false);

    return;
}

/**
 * gdl_dock_item_iconify_item:
 * @item: The dock item to iconify.
 *
 * This function iconifies the dock item. When dock items are iconified
 * they are hidden, and appear only as icons in dock bars.
 *
 * The dock item iconify button causes the panel to be iconified.
 **/
void
gdl_dock_item_iconify_item (GdlDockItem *item)
{
    g_return_if_fail (item != NULL);

    item->priv->iconified = TRUE;
    gtk_widget_set_visible (GTK_WIDGET (item), FALSE);
}

/**
 * gdl_dock_item_show_item:
 * @item: The dock item to show.
 *
 * This function shows the dock item. When dock items are shown, they
 * are displayed in their normal layout position.
 **/
void
gdl_dock_item_show_item (GdlDockItem *item)
{
    g_return_if_fail (item != NULL);

    /* Check if we need to dock the window */
    if (gtk_widget_get_parent (GTK_WIDGET (item)) == NULL) {
        if (gdl_dock_object_is_bound (GDL_DOCK_OBJECT (item))) {
            GdlDockObject *toplevel;

            toplevel = gdl_dock_object_get_controller (GDL_DOCK_OBJECT (item));
            if (toplevel == GDL_DOCK_OBJECT (item)) return;

            if (item->priv->behavior & GDL_DOCK_ITEM_BEH_NEVER_FLOATING) {
                g_warning("Object %s has no default position and flag GDL_DOCK_ITEM_BEH_NEVER_FLOATING is set.\n", gdl_dock_object_get_name (GDL_DOCK_OBJECT (item)));
                return;
            } else if (toplevel) {
                gdl_dock_object_dock (toplevel, GDL_DOCK_OBJECT (item), GDL_DOCK_FLOATING, NULL);
            } else
                g_warning("There is no toplevel window. GdlDockItem %s cannot be shown.\n", gdl_dock_object_get_name (GDL_DOCK_OBJECT (item)));
            return;
        } else
            g_warning("GdlDockItem %s is not bound. It cannot be shown.\n", gdl_dock_object_get_name (GDL_DOCK_OBJECT (item)));
        return;
    }

    item->priv->iconified = FALSE;
    gtk_widget_set_visible (GTK_WIDGET (item), TRUE);

    return;
}

/**
 * gdl_dock_item_lock:
 * @item: The dock item to lock.
 *
 * This function locks the dock item. When locked the dock item cannot
 * be dragged around and it doesn't show a grip.
 **/
void
gdl_dock_item_lock (GdlDockItem *item)
{
    g_object_set (item, "locked", TRUE, NULL);
}

/**
 * gdl_dock_item_unlock:
 * @item: The dock item to unlock.
 *
 * This function unlocks the dock item. When unlocked the dock item can
 * be dragged around and can show a grip.
 **/
void
gdl_dock_item_unlock (GdlDockItem *item)
{
    g_object_set (item, "locked", FALSE, NULL);
}

/**
 * gdl_dock_item_preferred_size:
 * @item: The dock item to get the preferred size of.
 * @req: A pointer to a #GtkRequisition into which the preferred size
 * will be written.
 *
 * Gets the preferred size of the dock item in pixels.
 **/
void
gdl_dock_item_preferred_size (GdlDockItem *item, GtkRequisition *req)
{
	if (!req)
		return;

	graphene_rect_t allocation;
#pragma GCC diagnostic ignored "-Wunused-result"
	gtk_widget_compute_bounds (GTK_WIDGET(item), GTK_WIDGET(item), &allocation);
#pragma GCC diagnostic warning "-Wunused-result"

	req->width = MAX (item->priv->preferred_width, allocation.size.width);
	req->height = MAX (item->priv->preferred_height, allocation.size.height);
}

/**
 * gdl_dock_item_get_drag_area:
 * @item: The dock item to get the preferred size of.
 * @rect: A pointer to a #GdkRectangle that will receive the drag position
 *
 * Gets the size and the position of the drag window in pixels.
 *
 * Since: 3.6
 */
void
gdl_dock_item_get_drag_area (GdlDockItem *item, GdkRectangle *rect)
{
	g_return_if_fail (GDL_IS_DOCK_ITEM (item));
	g_return_if_fail (rect != NULL);

	graphene_rect_t bounds;
#pragma GCC diagnostic ignored "-Wunused-result"
	gtk_widget_compute_bounds(GTK_WIDGET(item), GTK_WIDGET(item), &bounds);
#pragma GCC diagnostic warning "-Wunused-result"
	*rect = (GdkRectangle){
		.x = item->priv->dragoff_x,
		.y = item->priv->dragoff_y,
		.width = bounds.size.width,
		.height = bounds.size.height,
	};
}

/**
 * gdl_dock_item_or_child_has_focus:
 * @item: The dock item to be checked
 *
 * Checks whether a given #GdlDockItem or its child widget has focus.
 * This check is performed recursively on child widgets.
 *
 * Returns: %TRUE if the dock item or its child widget has focus;
 * %FALSE otherwise.
 *
 * Since: 3.3.2
 */
gboolean
gdl_dock_item_or_child_has_focus (GdlDockItem *item)
{
    g_return_val_if_fail (GDL_IS_DOCK_ITEM (item), FALSE);

    GtkWidget *item_child;
    for (item_child = gtk_widget_get_focus_child (GTK_WIDGET (item));
         item_child && GTK_IS_WIDGET (item_child) && gtk_widget_get_focus_child (GTK_WIDGET (item_child));
         item_child = gtk_widget_get_focus_child (GTK_WIDGET (item_child))) ;

    gboolean item_or_child_has_focus = (gtk_widget_has_focus (GTK_WIDGET (item)) || (GTK_IS_WIDGET (item_child) && gtk_widget_has_focus (item_child)));

    return item_or_child_has_focus;
}

/**
 * gdl_dock_item_is_placeholder:
 * @item: The dock item to be checked
 *
 * Checks whether a given #GdlDockItem is a placeholder created by the
 * #GdlDockLayout object and does not contain a child.
 *
 * Returns: %TRUE if the dock item is a placeholder
 *
 * Since: 3.6
 */
gboolean
gdl_dock_item_is_placeholder (GdlDockItem *item)
{
    return item->child == NULL;
}

/**
 * gdl_dock_item_is_closed:
 * @item: The dock item to be checked
 *
 * Checks whether a given #GdlDockItem is closed. It can be only hidden or
 * detached.
 *
 * Returns: %TRUE if the dock item is closed.
 *
 * Since: 3.6
 */
gboolean
gdl_dock_item_is_closed (GdlDockItem *item)
{
    g_return_val_if_fail (GDL_IS_DOCK_ITEM (item), FALSE);

    return gdl_dock_object_is_closed (GDL_DOCK_OBJECT (item));
}

/**
 * gdl_dock_item_is_iconified:
 * @item: The dock item to be checked
 *
 * Checks whether a given #GdlDockItem is iconified.
 *
 * Returns: %TRUE if the dock item is iconified.
 *
 * Since: 3.6
 */
gboolean
gdl_dock_item_is_iconified (GdlDockItem *item)
{
    g_return_val_if_fail (GDL_IS_DOCK_ITEM (item), FALSE);

    return item->priv->iconified;
}

/**
 * gdl_dock_item_set_child:
 * @item: a #GdlDockItem
 * @child: (allow-none): a #GtkWidget
 *
 * Set a new child for the #GdlDockItem. This child is different from the
 * children using the #GtkWidget interface. It is a private child reserved
 * for the widget implementation.
 *
 * If a child is already present, it will be replaced. If @widget is %NULL the
 * child will be removed.
 *
 * Since: 3.6
 */
void
gdl_dock_item_set_child (GdlDockItem *item, GtkWidget *child)
{
    g_return_if_fail (GDL_IS_DOCK_ITEM (item));

    if (item->child != NULL) {
        gtk_widget_unparent (item->child);
        item->child = NULL;
    }

    if (child != NULL) {
        gtk_widget_set_parent (child, GTK_WIDGET (item));
        item->child = child;
    }
}

/**
 * gdl_dock_item_get_child:
 * @item: a #GdlDockItem
 *
 * Gets the child of the #GdlDockItem, or %NULL if the item contains
 * no child widget. The returned widget does not have a reference
 * added, so you do not need to unref it.
 *
 * Return value: (transfer none): pointer to child of the #GdlDockItem
 *
 * Since: 3.6
 */
GtkWidget*
gdl_dock_item_get_child (GdlDockItem *item)
{
    g_return_val_if_fail (GDL_IS_DOCK_ITEM (item), NULL);

    return item->child;
}


/**
 * gdl_dock_item_class_set_has_grip:
 * @item_class: a #GdlDockItemClass
 * @has_grip: %TRUE is the dock item has a grip
 *
 * Define in the corresponding kind of dock item has a grip. Even if an item
 * has a grip it can be hidden.
 *
 * Since: 3.6
 */
void
gdl_dock_item_class_set_has_grip (GdlDockItemClass *item_class, gboolean has_grip)
{
    g_return_if_fail (GDL_IS_DOCK_ITEM_CLASS (item_class));

    item_class->priv->has_grip = has_grip;
}


gboolean
gdl_dock_item_is_expandable (GdlDockItem* item)
{
	g_return_val_if_fail(item->child, false);

	gboolean expand = gtk_widget_get_visible(item->child);
	if (expand)
		g_object_get ((GObject*)item, "expand", &expand, NULL);
	return expand;
}


/* ----- gtk orientation type exporter/importer ----- */

static void
gdl_dock_param_export_gtk_orientation (const GValue *src, GValue *dst)
{
    dst->data [0].v_pointer = g_strdup_printf ("%s", (src->data [0].v_int == GTK_ORIENTATION_HORIZONTAL) ?  "horizontal" : "vertical");
}

static void
gdl_dock_param_import_gtk_orientation (const GValue *src, GValue *dst)
{
    if (!strcmp (src->data [0].v_pointer, "horizontal"))
        dst->data [0].v_int = GTK_ORIENTATION_HORIZONTAL;
    else
        dst->data [0].v_int = GTK_ORIENTATION_VERTICAL;
}

gboolean
gdl_dock_item_is_active (GdlDockItem* item)
{
	return !! gtk_widget_get_parent((GtkWidget*)item);
}
