/* GTK - The GIMP Toolkit
 * Copyright © 2014 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"
#include <gtk/gtk.h>
#include "debug/debug.h"
#include "popovermenu.h"
#include "popovermenuprivate.h"
#include "menusectionboxprivate.h"
#include "widgetfocus.h"
#include "modelbutton.h"

/**
 * AyyiPopoverMenu:
 *
 * `AyyiPopoverMenu` is a subclass of `AyyiPopover` that implements menu
 * behavior.
 *
 * ![An example AyyiPopoverMenu](menu.png)
 *
 * `AyyiPopoverMenu` treats its children like menus and allows switching
 * between them. It can open submenus as traditional, nested submenus,
 * or in a more touch-friendly sliding fashion.
 * The property [property@Gtk.PopoverMenu:flags] controls this appearance.
 *
 * `AyyiPopoverMenu` is meant to be used primarily with menu models,
 * using [ctor@Gtk.PopoverMenu.new_from_model]. If you need to put
 * other widgets such as a `GtkSpinButton` or a `GtkSwitch` into a popover,
 * you can use [method@Gtk.PopoverMenu.add_child].
 *
 * For more dialog-like behavior, use a plain `AyyiPopover`.
 *
 * ## Menu models
 *
 * The XML format understood by `GtkBuilder` for `GMenuModel` consists
 * of a toplevel `<menu>` element, which contains one or more `<item>`
 * elements. Each `<item>` element contains `<attribute>` and `<link>`
 * elements with a mandatory name attribute. `<link>` elements have the
 * same content model as `<menu>`. Instead of `<link name="submenu">`
 * or `<link name="section">`, you can use `<submenu>` or `<section>`
 * elements.
 *
 * ```xml
 * <menu id='app-menu'>
 *   <section>
 *     <item>
 *       <attribute name='label' translatable='yes'>_New Window</attribute>
 *       <attribute name='action'>app.new</attribute>
 *     </item>
 *     <item>
 *       <attribute name='label' translatable='yes'>_About Sunny</attribute>
 *       <attribute name='action'>app.about</attribute>
 *     </item>
 *     <item>
 *       <attribute name='label' translatable='yes'>_Quit</attribute>
 *       <attribute name='action'>app.quit</attribute>
 *     </item>
 *   </section>
 * </menu>
 * ```
 *
 * Attribute values can be translated using gettext, like other `GtkBuilder`
 * content. `<attribute>` elements can be marked for translation with a
 * `translatable="yes"` attribute. It is also possible to specify message
 * context and translator comments, using the context and comments attributes.
 * To make use of this, the `GtkBuilder` must have been given the gettext
 * domain to use.
 *
 * The following attributes are used when constructing menu items:
 *
 * - "label": a user-visible string to display
 * - "use-markup": whether the text in the menu item includes [Pango markup](https://docs.gtk.org/Pango/pango_markup.html)
 * - "action": the prefixed name of the action to trigger
 * - "target": the parameter to use when activating the action
 * - "icon" and "verb-icon": names of icons that may be displayed
 * - "submenu-action": name of an action that may be used to track
 *      whether a submenu is open
 * - "hidden-when": a string used to determine when the item will be hidden.
 *      Possible values include "action-disabled", "action-missing", "macos-menubar".
 *      This is mainly useful for exported menus, see [method@Gtk.Application.set_menubar].
 * - "custom": a string used to match against the ID of a custom child added with
 *      [method@Gtk.PopoverMenu.add_child], [method@Gtk.PopoverMenuBar.add_child],
 *      or in the ui file with `<child type="ID">`.
 *
 * The following attributes are used when constructing sections:
 *
 * - "label": a user-visible string to use as section heading
 * - "display-hint": a string used to determine special formatting for the section.
 *     Possible values include "horizontal-buttons", "circular-buttons" and
 *     "inline-buttons". They all indicate that section should be
 *     displayed as a horizontal row of buttons.
 * - "text-direction": a string used to determine the `GtkTextDirection` to use
 *     when "display-hint" is set to "horizontal-buttons". Possible values
 *     include "rtl", "ltr", and "none".
 *
 * The following attributes are used when constructing submenus:
 *
 * - "label": a user-visible string to display
 * - "icon": icon name to display
 *
 * Menu items will also show accelerators, which are usually associated
 * with actions via [method@Gtk.Application.set_accels_for_action],
 * [method@WidgetClass.add_binding_action] or
 * [method@Gtk.ShortcutController.add_shortcut].
 *
 * # Shortcuts and Gestures
 *
 * `AyyiPopoverMenu` supports the following keyboard shortcuts:
 *
 * - <kbd>Space</kbd> activates the default widget.
 *
 * # CSS Nodes
 *
 * `AyyiPopoverMenu` is just a subclass of `AyyiPopover` that adds custom content
 * to it, therefore it has the same CSS nodes. It is one of the cases that add
 * a `.menu` style class to the main `popover` node.
 *
 * Menu items have nodes with name `button` and class `.model`. If a section
 * display-hint is set, the section gets a node `box` with class `horizontal`
 * plus a class with the same text as the display hint. Note that said box may
 * not be the direct ancestor of the item `button`s. Thus, for example, to style
 * items in an `inline-buttons` section, select `.inline-buttons button.model`.
 * Other things that may be of interest to style in menus include `label` nodes.
 *
 * # Accessibility
 *
 * `AyyiPopoverMenu` uses the %GTK_ACCESSIBLE_ROLE_MENU role, and its
 * items use the %GTK_ACCESSIBLE_ROLE_MENU_ITEM,
 * %GTK_ACCESSIBLE_ROLE_MENU_ITEM_CHECKBOX or
 * %GTK_ACCESSIBLE_ROLE_MENU_ITEM_RADIO roles, depending on the
 * action they are connected to.
 */

typedef struct _AyyiPopoverMenuClass AyyiPopoverMenuClass;

struct _AyyiPopoverMenuClass
{
  GtkPopoverClass parent_class;
};

enum {
  PROP_VISIBLE_SUBMENU = 1,
  PROP_MENU_MODEL,
  PROP_FLAGS
};

static void ayyi_popover_menu_buildable_iface_init (GtkBuildableIface *iface);

G_DEFINE_TYPE_WITH_CODE (AyyiPopoverMenu, ayyi_popover_menu, GTK_TYPE_POPOVER,
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_BUILDABLE, ayyi_popover_menu_buildable_iface_init))

GtkWidget*
ayyi_popover_menu_get_parent_menu (AyyiPopoverMenu *menu)
{
	return menu->parent_menu;
}

void
ayyi_popover_menu_set_parent_menu (AyyiPopoverMenu* menu, GtkWidget* parent)
{
	menu->parent_menu = parent;
}

void
ayyi_popover_menu_set_open_submenu (AyyiPopoverMenu* menu, GtkWidget* submenu)
{
  menu->open_submenu = submenu;
}

void
ayyi_popover_menu_close_submenus (AyyiPopoverMenu *menu)
{
	GtkWidget* submenu = menu->open_submenu;
	if (submenu) {
		ayyi_popover_menu_close_submenus (AYYI_POPOVER_MENU (submenu));
		gtk_widget_set_visible (submenu, FALSE);
		ayyi_popover_menu_set_open_submenu (menu, NULL);
	}
}

GtkWidget *
ayyi_popover_menu_get_active_item (AyyiPopoverMenu *menu)
{
	return menu->active_item;
}

void
ayyi_popover_menu_set_active_item (AyyiPopoverMenu *menu, GtkWidget *item)
{
  if (menu->active_item != item)
    {
      if (menu->active_item)
        {
          gtk_widget_unset_state_flags (menu->active_item, GTK_STATE_FLAG_SELECTED);
          g_object_remove_weak_pointer (G_OBJECT (menu->active_item), (gpointer *)&menu->active_item);
        }

      menu->active_item = item;

      if (menu->active_item)
        {
          GtkWidget *popover;

          g_object_add_weak_pointer (G_OBJECT (menu->active_item), (gpointer *)&menu->active_item);

          gtk_widget_set_state_flags (menu->active_item, GTK_STATE_FLAG_SELECTED, FALSE);
          if (GTK_IS_MODEL_BUTTON (item))
            g_object_get (item, "popover", &popover, NULL);
          else
            popover = NULL;

          if (!popover || popover != menu->open_submenu)
            gtk_widget_grab_focus (menu->active_item);

          g_clear_object (&popover);
       }
    }
}

static void
visible_submenu_changed (GObject *object, GParamSpec *pspec, AyyiPopoverMenu *popover)
{
	g_object_notify (G_OBJECT (popover), "visible-submenu");
}

static void
focus_out (GtkEventController *controller, AyyiPopoverMenu *menu)
{
	GtkRoot* root = gtk_widget_get_root (GTK_WIDGET (menu));
	if (!root)
		return;

	GtkWidget* new_focus = gtk_root_get_focus (root);

	if (!gtk_event_controller_focus_contains_focus (GTK_EVENT_CONTROLLER_FOCUS (controller)) && new_focus != NULL) {
		if (menu->parent_menu && AYYI_POPOVER_MENU (menu->parent_menu)->open_submenu == (GtkWidget*)menu)
			AYYI_POPOVER_MENU (menu->parent_menu)->open_submenu = NULL;
		gtk_popover_popdown (GTK_POPOVER (menu));
	}
}

static void
leave_cb (GtkEventController *controller, gpointer data)
{
	if (!gtk_event_controller_motion_contains_pointer (GTK_EVENT_CONTROLLER_MOTION (controller))) {
		GtkWidget *target = gtk_event_controller_get_widget (controller);

		ayyi_popover_menu_set_active_item (AYYI_POPOVER_MENU (target), NULL);
	}
}

static void
ayyi_popover_menu_init (AyyiPopoverMenu *popover)
{
	GtkWidget* sw = gtk_scrolled_window_new ();
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_propagate_natural_width (GTK_SCROLLED_WINDOW (sw), TRUE);
	gtk_scrolled_window_set_propagate_natural_height (GTK_SCROLLED_WINDOW (sw), TRUE);
	gtk_popover_set_child (GTK_POPOVER (popover), sw);

	GtkWidget* stack = gtk_stack_new ();
	gtk_stack_set_vhomogeneous (GTK_STACK (stack), FALSE);
	gtk_stack_set_transition_type (GTK_STACK (stack), GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
	gtk_stack_set_interpolate_size (GTK_STACK (stack), TRUE);
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (sw), stack);
	g_signal_connect (stack, "notify::visible-child-name", G_CALLBACK (visible_submenu_changed), popover);

	gtk_widget_add_css_class (GTK_WIDGET (popover), "menu");

	GtkEventController* controller = gtk_event_controller_focus_new ();
	g_signal_connect (controller, "leave", G_CALLBACK (focus_out), popover);
	gtk_widget_add_controller (GTK_WIDGET (popover), controller);

	controller = gtk_event_controller_motion_new ();
	g_signal_connect (controller, "notify::contains-pointer", G_CALLBACK (leave_cb), popover);
	gtk_widget_add_controller (GTK_WIDGET (popover), controller);

#ifdef GTK4_TODO
	extern GtkEventController **gtk_widget_list_controllers (GtkWidget *widget, GtkPropagationPhase phase, guint *out_n_controllers);
	GtkEventController** controllers = gtk_widget_list_controllers (GTK_WIDGET (popover), GTK_PHASE_CAPTURE, &n_controllers);
	guint n_controllers;
	for (int i = 0; i < n_controllers; i ++) {
		controller = controllers[i];
		if (GTK_IS_SHORTCUT_CONTROLLER (controller) && strcmp (gtk_event_controller_get_name (controller), "gtk-shortcut-manager-capture") == 0)
			gtk_shortcut_controller_set_mnemonics_modifiers (GTK_SHORTCUT_CONTROLLER (controller), 0);
	}
	g_free (controllers);

	extern void gtk_popover_disable_auto_mnemonics (GtkPopover*);
	gtk_popover_disable_auto_mnemonics (GTK_POPOVER (popover));
#endif
	gtk_popover_set_cascade_popdown (GTK_POPOVER (popover), TRUE);
}

GtkWidget*
ayyi_popover_menu_get_stack (AyyiPopoverMenu *menu)
{
	GtkWidget* sw = gtk_popover_get_child (GTK_POPOVER (menu));
	GtkWidget* vp = gtk_scrolled_window_get_child (GTK_SCROLLED_WINDOW (sw));
	GtkWidget* stack = gtk_viewport_get_child (GTK_VIEWPORT (vp));

	return stack;
}

static void
ayyi_popover_menu_dispose (GObject *object)
{
	AyyiPopoverMenu *popover = AYYI_POPOVER_MENU (object);

	if (popover->active_item) {
		g_object_remove_weak_pointer (G_OBJECT (popover->active_item), (gpointer *)&popover->active_item);
		popover->active_item = NULL;
	}

	g_clear_object (&popover->model);

	G_OBJECT_CLASS (ayyi_popover_menu_parent_class)->dispose (object);
}

static void
ayyi_popover_menu_map (GtkWidget *widget)
{
	ayyi_popover_menu_open_submenu (AYYI_POPOVER_MENU (widget), "main");
	GTK_WIDGET_CLASS (ayyi_popover_menu_parent_class)->map (widget);
}

static void
ayyi_popover_menu_unmap (GtkWidget *widget)
{
	GTK_WIDGET_CLASS (ayyi_popover_menu_parent_class)->unmap (widget);
	ayyi_popover_menu_open_submenu (AYYI_POPOVER_MENU (widget), "main");
}

static void
ayyi_popover_menu_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
  AyyiPopoverMenu *menu = AYYI_POPOVER_MENU (object);

  switch (property_id)
    {
    case PROP_VISIBLE_SUBMENU:
      g_value_set_string (value, gtk_stack_get_visible_child_name (GTK_STACK (ayyi_popover_menu_get_stack (menu))));
      break;

    case PROP_MENU_MODEL:
      g_value_set_object (value, ayyi_popover_menu_get_menu_model (menu));
      break;

    case PROP_FLAGS:
      g_value_set_flags (value, ayyi_popover_menu_get_flags (menu));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ayyi_popover_menu_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
  AyyiPopoverMenu *menu = AYYI_POPOVER_MENU (object);

  switch (property_id)
    {
    case PROP_VISIBLE_SUBMENU:
      gtk_stack_set_visible_child_name (GTK_STACK (ayyi_popover_menu_get_stack (menu)), g_value_get_string (value));
      break;

    case PROP_MENU_MODEL:
      ayyi_popover_menu_set_menu_model (menu, g_value_get_object (value));
      break;

    case PROP_FLAGS:
      ayyi_popover_menu_set_flags (menu, g_value_get_flags (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
ayyi_popover_menu_focus (GtkWidget *widget, GtkDirectionType direction)
{
	AyyiPopoverMenu *menu = AYYI_POPOVER_MENU (widget);

	if (gtk_widget_get_first_child (widget) == NULL) {
		return FALSE;
	} else {
      if (menu->open_submenu) {
          if (gtk_widget_child_focus (menu->open_submenu, direction))
            return TRUE;

          if (direction == GTK_DIR_LEFT) {
              if (menu->open_submenu) {
                  gtk_popover_popdown (GTK_POPOVER (menu->open_submenu));
                  menu->open_submenu = NULL;
                }

              gtk_widget_grab_focus (menu->active_item);

              return TRUE;
            }
          return FALSE;
        }

      if (gtk_widget_focus_move (widget, direction))
        return TRUE;

      if (direction == GTK_DIR_LEFT || direction == GTK_DIR_RIGHT)
        {
          /* If we are part of a menubar, we want to let the
           * menubar use left/right arrows for cycling, else
           * we eat them.
           */
          if (gtk_widget_get_ancestor (widget, GTK_TYPE_POPOVER_MENU_BAR) ||
              (ayyi_popover_menu_get_parent_menu (menu) &&
               direction == GTK_DIR_LEFT))
            return FALSE;
          else
            return TRUE;
        }
      /* Cycle around with up/down arrows and (Shift+)Tab when modal */
      else if (gtk_popover_get_autohide (GTK_POPOVER (menu)))
        {
          GtkWidget *p = gtk_root_get_focus (gtk_widget_get_root (widget));

          /* In the case where the popover doesn't have any focusable child, if
           * the menu doesn't have any item for example, then the focus will end
           * up out of the popover, hence creating an infinite loop below. To
           * avoid this, just say we had focus and stop here.
           */
          if (!gtk_widget_is_ancestor (p, widget) && p != widget)
            return TRUE;

          /* cycle around */
          for (;
               p != widget;
               p = gtk_widget_get_parent (p))
            {
              gtk_widget_set_focus_child (p, NULL);
            }
          if (gtk_widget_focus_move (widget, direction))
            return TRUE;
        }
    }

  return FALSE;
}

static void
add_tab_bindings (GtkWidgetClass *widget_class, GdkModifierType modifiers, GtkDirectionType direction)
{
	gtk_widget_class_add_binding_signal (widget_class, GDK_KEY_Tab, modifiers, "move-focus", "(i)", direction);
	gtk_widget_class_add_binding_signal (widget_class, GDK_KEY_KP_Tab, modifiers, "move-focus", "(i)", direction);
}

static void
add_arrow_bindings (GtkWidgetClass *widget_class, guint keysym, GtkDirectionType direction)
{
	guint keypad_keysym = keysym - GDK_KEY_Left + GDK_KEY_KP_Left;

	gtk_widget_class_add_binding_signal (widget_class, keysym, 0, "move-focus", "(i)", direction);
	gtk_widget_class_add_binding_signal (widget_class, keysym, GDK_CONTROL_MASK, "move-focus", "(i)", direction);
	gtk_widget_class_add_binding_signal (widget_class, keypad_keysym, 0, "move-focus", "(i)", direction);
	gtk_widget_class_add_binding_signal (widget_class, keypad_keysym, GDK_CONTROL_MASK, "move-focus", "(i)", direction);
}

static void
ayyi_popover_menu_show (GtkWidget *widget)
{
	ayyi_popover_menu_set_open_submenu (AYYI_POPOVER_MENU (widget), NULL);

	GTK_WIDGET_CLASS (ayyi_popover_menu_parent_class)->show (widget);
}

static void
ayyi_popover_menu_move_focus (GtkWidget *widget, GtkDirectionType direction)
{
  gtk_popover_set_mnemonics_visible (GTK_POPOVER (widget), TRUE);

  GTK_WIDGET_CLASS (ayyi_popover_menu_parent_class)->move_focus (widget, direction);
}

static void
ayyi_popover_menu_root (GtkWidget *widget)
{
  GTK_WIDGET_CLASS (ayyi_popover_menu_parent_class)->root (widget);

  gtk_accessible_update_property (GTK_ACCESSIBLE (widget), GTK_ACCESSIBLE_PROPERTY_ORIENTATION, GTK_ORIENTATION_VERTICAL, -1);
}

static void
ayyi_popover_menu_class_init (AyyiPopoverMenuClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = ayyi_popover_menu_dispose;
  object_class->set_property = ayyi_popover_menu_set_property;
  object_class->get_property = ayyi_popover_menu_get_property;

  widget_class->root = ayyi_popover_menu_root;
  widget_class->map = ayyi_popover_menu_map;
  widget_class->unmap = ayyi_popover_menu_unmap;
  widget_class->focus = ayyi_popover_menu_focus;
  widget_class->show = ayyi_popover_menu_show;
  widget_class->move_focus = ayyi_popover_menu_move_focus;

  /**
   * AyyiPopoverMenu:visible-submenu:
   *
   * The name of the visible submenu.
   */
  g_object_class_install_property (object_class,
                                   PROP_VISIBLE_SUBMENU,
                                   g_param_spec_string ("visible-submenu", NULL, NULL,
                                                        NULL,
                                                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * AyyiPopoverMenu:menu-model:
   *
   * The model from which the menu is made.
   */
  g_object_class_install_property (object_class,
                                   PROP_MENU_MODEL,
                                   g_param_spec_object ("menu-model", NULL, NULL,
                                                        G_TYPE_MENU_MODEL,
                                                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * AyyiPopoverMenu:flags:
   *
   * The flags that @popover uses to create/display a menu from its model.
   *
   * If a model is set and the flags change, contents are rebuilt, so if setting
   * properties individually, set flags before model to avoid a redundant rebuild.
   *
   * Since: 4.14
   */
  g_object_class_install_property (object_class,
                                   PROP_FLAGS,
                                   g_param_spec_flags ("flags", NULL, NULL,
                                                       GTK_TYPE_POPOVER_MENU_FLAGS,
                                                       GTK_POPOVER_MENU_SLIDING,
                                                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS
                                                         | G_PARAM_EXPLICIT_NOTIFY));

  add_arrow_bindings (widget_class, GDK_KEY_Up, GTK_DIR_UP);
  add_arrow_bindings (widget_class, GDK_KEY_Down, GTK_DIR_DOWN);
  add_arrow_bindings (widget_class, GDK_KEY_Left, GTK_DIR_LEFT);
  add_arrow_bindings (widget_class, GDK_KEY_Right, GTK_DIR_RIGHT);

  add_tab_bindings (widget_class, 0, GTK_DIR_TAB_FORWARD);
  add_tab_bindings (widget_class, GDK_CONTROL_MASK, GTK_DIR_TAB_FORWARD);
  add_tab_bindings (widget_class, GDK_SHIFT_MASK, GTK_DIR_TAB_BACKWARD);
  add_tab_bindings (widget_class, GDK_CONTROL_MASK | GDK_SHIFT_MASK, GTK_DIR_TAB_BACKWARD);

  gtk_widget_class_add_binding_signal (widget_class, GDK_KEY_Return, 0,
                                       "activate-default", NULL);
  gtk_widget_class_add_binding_signal (widget_class, GDK_KEY_ISO_Enter, 0,
                                       "activate-default", NULL);
  gtk_widget_class_add_binding_signal (widget_class, GDK_KEY_KP_Enter, 0,
                                       "activate-default", NULL);
  gtk_widget_class_add_binding_signal (widget_class, GDK_KEY_space, 0,
                                       "activate-default", NULL);
  gtk_widget_class_add_binding_signal (widget_class, GDK_KEY_KP_Space, 0,
                                       "activate-default", NULL);

  gtk_widget_class_set_accessible_role (widget_class, GTK_ACCESSIBLE_ROLE_MENU);
}

static GtkBuildableIface *parent_buildable_iface;

static void
ayyi_popover_menu_buildable_add_child (GtkBuildable *buildable, GtkBuilder *builder, GObject *child, const char *type)
{
	if (GTK_IS_WIDGET (child)) {
		if (!ayyi_popover_menu_add_child (AYYI_POPOVER_MENU (buildable), GTK_WIDGET (child), type))
			g_warning ("No such custom attribute: %s", type);
	} else {
	    parent_buildable_iface->add_child (buildable, builder, child, type);
	}
}

static void
ayyi_popover_menu_buildable_iface_init (GtkBuildableIface *iface)
{
	parent_buildable_iface = g_type_interface_peek_parent (iface);

	iface->add_child = ayyi_popover_menu_buildable_add_child;
}

static void
ayyi_popover_menu_rebuild_contents (AyyiPopoverMenu *popover)
{
	GtkWidget *child;

	GtkWidget* stack = ayyi_popover_menu_get_stack (popover);
	while ((child = gtk_widget_get_first_child (stack)))
		gtk_stack_remove (GTK_STACK (stack), child);

	if (popover->model)
		ayyi_menu_section_box_new_toplevel (AYYI_POPOVER_MENU(popover), popover->model, popover->flags);
}

/**
 * ayyi_popover_menu_new:
 *
 * Creates a new popover menu.
 *
 * Returns: a new `AyyiPopoverMenu`
 */
GtkWidget *
ayyi_popover_menu_new (void)
{
	GtkWidget *popover = g_object_new (GTK_TYPE_POPOVER_MENU, "autohide", TRUE, NULL);

	return popover;
}

/*<private>
 * ayyi_popover_menu_open_submenu:
 * @popover: a `AyyiPopoverMenu`
 * @name: the name of the menu to switch to
 *
 * Opens a submenu of the @popover. The @name
 * must be one of the names given to the submenus
 * of @popover with `AyyiPopoverMenu:submenu`, or
 * "main" to switch back to the main menu.
 *
 * `GtkModelButton` will open submenus automatically
 * when the `GtkModelButton:menu-name` property is set,
 * so this function is only needed when you are using
 * other kinds of widgets to initiate menu changes.
 */
void
ayyi_popover_menu_open_submenu (AyyiPopoverMenu *popover, const char *name)
{
	g_return_if_fail (GTK_IS_POPOVER_MENU (popover));

	GtkWidget* stack = ayyi_popover_menu_get_stack (popover);
	gtk_stack_set_visible_child_name (GTK_STACK (stack), name);
}

void
ayyi_popover_menu_add_submenu (AyyiPopoverMenu *popover, GtkWidget *submenu, const char *name)
{
	GtkWidget* stack = ayyi_popover_menu_get_stack (popover);
	gtk_stack_add_named (GTK_STACK (stack), submenu, name);
}

/**
 * ayyi_popover_menu_new_from_model:
 * @model: (nullable): a `GMenuModel`
 *
 * Creates a `AyyiPopoverMenu` and populates it according to @model.
 *
 * The created buttons are connected to actions found in the
 * `GtkApplicationWindow` to which the popover belongs - typically
 * by means of being attached to a widget that is contained within
 * the `GtkApplicationWindow`s widget hierarchy.
 *
 * Actions can also be added using [method@Gtk.Widget.insert_action_group]
 * on the menus attach widget or on any of its parent widgets.
 *
 * This function creates menus with sliding submenus.
 * See [ctor@Gtk.PopoverMenu.new_from_model_full] for a way
 * to control this.
 *
 * Returns: the new `AyyiPopoverMenu`
 */
GtkWidget*
ayyi_popover_menu_new_from_model (GMenuModel *model)
{
	return ayyi_popover_menu_new_from_model_full (model, GTK_POPOVER_MENU_SLIDING);
}

/**
 * ayyi_popover_menu_new_from_model_full:
 * @model: a `GMenuModel`
 * @flags: flags that affect how the menu is created
 *
 * Creates a `AyyiPopoverMenu` and populates it according to @model.
 *
 * The created buttons are connected to actions found in the
 * action groups that are accessible from the parent widget.
 * This includes the `GtkApplicationWindow` to which the popover
 * belongs. Actions can also be added using [method@Gtk.Widget.insert_action_group]
 * on the parent widget or on any of its parent widgets.
 *
 * Returns: the new `AyyiPopoverMenu`
 */
GtkWidget *
ayyi_popover_menu_new_from_model_full (GMenuModel* model, GtkPopoverMenuFlags flags)
{
	g_return_val_if_fail (model == NULL || G_IS_MENU_MODEL (model), NULL);

	GtkWidget* popover = ayyi_popover_menu_new ();
	ayyi_popover_menu_set_flags (AYYI_POPOVER_MENU (popover), flags);
	ayyi_popover_menu_set_menu_model (AYYI_POPOVER_MENU (popover), model);

	return popover;
}

/**
 * ayyi_popover_menu_set_menu_model:
 * @popover: a `AyyiPopoverMenu`
 * @model: (nullable): a `GMenuModel`
 *
 * Sets a new menu model on @popover.
 *
 * The existing contents of @popover are removed, and
 * the @popover is populated with new contents according
 * to @model.
 */
void
ayyi_popover_menu_set_menu_model (AyyiPopoverMenu *popover, GMenuModel *model)
{
	g_return_if_fail (GTK_IS_POPOVER_MENU (popover));
	g_return_if_fail (model == NULL || G_IS_MENU_MODEL (model));

	if (g_set_object (&popover->model, model)) {
		ayyi_popover_menu_rebuild_contents (popover);
		g_object_notify (G_OBJECT (popover), "menu-model");
	}
}

/**
 * ayyi_popover_menu_set_flags:
 * @popover: a `AyyiPopoverMenu`
 * @flags: a set of `AyyiPopoverMenuFlags`
 *
 * Sets the flags that @popover uses to create/display a menu from its model.
 *
 * If a model is set and the flags change, contents are rebuilt, so if setting
 * properties individually, set flags before model to avoid a redundant rebuild.
 *
 * Since: 4.14
 */
void
ayyi_popover_menu_set_flags (AyyiPopoverMenu* popover, GtkPopoverMenuFlags flags)
{
	g_return_if_fail (GTK_IS_POPOVER_MENU (popover));

	if (popover->flags == flags)
		return;

	popover->flags = flags;

	/* This shouldn’t happen IRL, but notify test unsets :child, so dodge error */
	if (gtk_popover_get_child (GTK_POPOVER (popover)) != NULL)
		ayyi_popover_menu_rebuild_contents (popover);

	g_object_notify (G_OBJECT (popover), "flags");
}

/**
 * ayyi_popover_menu_get_menu_model:
 * @popover: a `AyyiPopoverMenu`
 *
 * Returns the menu model used to populate the popover.
 *
 * Returns: (transfer none) (nullable): the menu model of @popover
 */
GMenuModel *
ayyi_popover_menu_get_menu_model (AyyiPopoverMenu *popover)
{
  g_return_val_if_fail (GTK_IS_POPOVER_MENU (popover), NULL);

  return popover->model;
}

/**
 * ayyi_popover_menu_get_flags:
 * @popover: a `AyyiPopoverMenu`
 *
 * Returns the flags that @popover uses to create/display a menu from its model.
 *
 * Returns: the `AyyiPopoverMenuFlags`
 *
 * Since: 4.14
 */
GtkPopoverMenuFlags
ayyi_popover_menu_get_flags (AyyiPopoverMenu *popover)
{
  g_return_val_if_fail (GTK_IS_POPOVER_MENU (popover), 0);

  return popover->flags;
}

/**
 * ayyi_popover_menu_add_child:
 * @popover: a `AyyiPopoverMenu`
 * @child: the `GtkWidget` to add
 * @id: the ID to insert @child at
 *
 * Adds a custom widget to a generated menu.
 *
 * For this to work, the menu model of @popover must have
 * an item with a `custom` attribute that matches @id.
 *
 * Returns: %TRUE if @id was found and the widget added
 */
gboolean
ayyi_popover_menu_add_child (AyyiPopoverMenu* popover, GtkWidget* child, const char* id)
{
	g_return_val_if_fail (GTK_IS_POPOVER_MENU (popover), FALSE);
	g_return_val_if_fail (GTK_IS_WIDGET (child), FALSE);
	g_return_val_if_fail (id != NULL, FALSE);

	return ayyi_menu_section_box_add_custom ((AyyiPopoverMenu*)popover, child, id);
}

/**
 * ayyi_popover_menu_remove_child:
 * @popover: a `AyyiPopoverMenu`
 * @child: the `GtkWidget` to remove
 *
 * Removes a widget that has previously been added with
 * [method@Gtk.PopoverMenu.add_child()]
 *
 * Returns: %TRUE if the widget was removed
 */
gboolean
ayyi_popover_menu_remove_child (AyyiPopoverMenu* popover, GtkWidget* child)
{
	g_return_val_if_fail (GTK_IS_POPOVER_MENU (popover), FALSE);
	g_return_val_if_fail (GTK_IS_WIDGET (child), FALSE);

	return ayyi_menu_section_box_remove_custom (popover, child);
}
