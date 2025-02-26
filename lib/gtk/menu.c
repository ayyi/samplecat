/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. https://ayyi.github.io/samplecat/    |
 | copyright (C) 2007-2025 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#include "config.h"
#include "gtk/menu.h"
#include "debug/debug.h"


#ifdef GTK4_TODO
static void
menu_item_image_from_stock (GtkWidget* menu, GtkWidget* item, char* stock_id)
{
	GtkWidget* icon = gtk_image_new_from_icon_name(stock_id);

	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), icon);
}
#endif


GtkWidget*
make_menu (int size, MenuDef menu_def[size], gpointer user_data)
{
	GMenuModel* model = (GMenuModel*)g_menu_new ();
	GtkWidget* menu = gtk_popover_menu_new_from_model_full (model, GTK_POPOVER_MENU_NESTED);

	add_menu_items_from_defn (menu, model, size, menu_def, user_data);

	return menu;
}


void
add_menu_items_from_defn (GtkWidget* widget, GMenuModel* model, int size, MenuDef defn[size], gpointer user_data)
{
	int s = 0;
	GMenuModel* section[3] = { model, };
	gboolean in_section = false;

	for (int i=0;i<size;i++) {
		MenuDef* item = &defn[i];
		switch (item->name[0]) {
			case '-':
				if (!in_section) s++;
				in_section = true;
				section[s] = (GMenuModel*)g_menu_new ();
				g_menu_append_section (G_MENU(model), NULL, section[s]);
				break;
			case '<':
				s--;
				break;
			case '>':
				item = &defn[++i]; // following item must be the submenu parent item.
				if (!in_section) s++;
				in_section = false;

				section[s] = (GMenuModel*)g_menu_new ();
				if (!item->action) {
					g_menu_append_submenu (G_MENU(section[s-1]), item->name, section[s]);
				} else {
					GMenuItem* mi = g_menu_item_new_submenu(item->name,  section[s]);
					g_menu_item_set_attribute (mi, "submenu-action", "s", item->action);
					if (false && item->icon) {
						g_menu_item_set_attribute (mi, "touch-icon", "s", item->icon);
						g_menu_item_set_attribute (mi, "icon", "s", item->icon); // setting an icon changes the layout but does not show an icon
					}
					g_menu_append_item(G_MENU(section[s - 1]), mi);
					g_object_unref(mi);
				}

				break;
			default:
				if (item->icon) {
					add_icon_menu_item(POPOVER_MENU(widget), G_MENU(section[s]), item->name, item->action, item->icon);
				} else {
					if (item->target) {
						GMenuItem* mi = g_menu_item_new (item->name, "dummy");
						//g_menu_item_set_action_and_target (mi, item->action, "i", item->target);
						g_menu_item_set_action_and_target_value (mi, item->action, g_variant_new_int32(item->target));
						g_menu_append_item (G_MENU(section[s]), mi);
						g_object_unref(mi);
					} else {
						g_menu_append (G_MENU(section[s]), item->name, item->action);
					}
				}
#ifdef GTK4_TODO
				if (item->callback)
					g_signal_connect (G_OBJECT(menu_item), "activate", G_CALLBACK(item->callback), item->callback_data ? GINT_TO_POINTER(item->callback_data) : user_data);
#endif
		}
	}
}


void
add_icon_menu_item (PopoverMenu* widget, GMenu* menu, const char* name, const char* action, const char* icon)
{
	GMenuItem* mi = g_menu_item_new (name, "dummy");
	g_menu_item_set_attribute (mi, "custom", "s", action, NULL); // connect the menu item to the action
	g_menu_append_item (menu, mi);
	g_object_unref(mi);

	GtkWidget* box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
	GtkWidget* button = gtk_button_new();
	gtk_actionable_set_action_name (GTK_ACTIONABLE(button), action);
	gtk_button_set_child(GTK_BUTTON(button), box);
	{
		GtkWidget* image = gtk_image_new_from_icon_name(icon);
		gtk_box_append(GTK_BOX(box), image);
		gtk_box_append(GTK_BOX(box), gtk_label_new(name));
	}
#ifdef CUSTOM_MENU
	if (!ayyi_popover_menu_add_child(AYYI_POPOVER_MENU(widget), button, action)) dbg(0, "not added %s", action);
#else
	if (!gtk_popover_menu_add_child(GTK_POPOVER_MENU(widget), button, action)) dbg(0, "not added %s", action);
#endif
}


void
add_icon_menu_item2 (PopoverMenu* widget, GMenu* menu, MenuDef* def)
{
	GMenuItem* mi = g_menu_item_new (def->name, "dummy");
	g_menu_item_set_attribute (mi, "custom", "s", def->action, NULL); // connect the menu item to the action
	g_menu_append_item (menu, mi);
	g_object_unref(mi);

	if (!gtk_popover_menu_add_child(
		GTK_POPOVER_MENU(widget),
		({
			GtkWidget* button = gtk_button_new();

			void activate (GtkButton* self, gpointer user_data)
			{
			}
			g_signal_connect(button, "activate", (gpointer)activate, NULL);
			gtk_actionable_set_action_name (GTK_ACTIONABLE(button), def->action);

			gtk_button_set_child(GTK_BUTTON(button), ({
				GtkWidget* box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
				GtkWidget* image = gtk_image_new_from_icon_name(def->icon);
				gtk_box_append(GTK_BOX(box), image);
				gtk_box_append(GTK_BOX(box), gtk_label_new(def->name));
				box;
			}));

			button;
		}),
		def->action)
	) pwarn("not added %s", def->action);
}


static char*
to_kebab_case (char* str)
{
	for (int i=0;i<strlen(str);i++) {
		str[i] = g_ascii_tolower(str[i]);
		if (str[i] == ' ') str[i] = '-';
	}
	return str;
}


/*
 *  Given the menu definition, create actions and (optionally) menu items.
 */
void
make_menu_actions (GtkWidget* widget, Accel keys[], int count, void (*add_to_menu)(GMenuModel*, GAction*, char*), GMenuModel* section)
{
	GSimpleActionGroup* group = g_simple_action_group_new ();
	gtk_widget_insert_action_group (widget, "app", G_ACTION_GROUP(group));
#ifdef GTK4_TODO
	accel_group = gtk_accel_group_new();
#endif

	for (int k=0;k<count;k++) {
		Accel* key = &keys[k];

		const GVariantType* parameter_type = NULL;
		g_autofree char* name = to_kebab_case(g_strdup(key->name));
		GSimpleAction* action = g_simple_action_new(name, parameter_type);
		g_signal_connect (action, "activate", key->callback, key->user_data);
		g_action_map_add_action(G_ACTION_MAP(group), G_ACTION(action));

#ifdef GTK4_TODO
		GClosure* closure = g_cclosure_new(G_CALLBACK(key->callback), key->user_data, NULL);
		g_signal_connect_closure(G_OBJECT(action), "activate", closure, FALSE);
		gchar path[64]; sprintf(path, "<%s>/Categ/%s", gtk_action_group_get_name(GTK_ACTION_GROUP(group)), key->name);
#if 0
		gtk_accel_group_connect(accel_group, key->key[0].code, key->key[0].mask, GTK_ACCEL_MASK, closure);
#else
		gtk_accel_group_connect_by_path(accel_group, path, closure);
#endif
		gtk_accel_map_add_entry(path, key->key[0].code, key->key[0].mask);
		gtk_action_set_accel_path(action, path);
		gtk_action_set_accel_group(action, accel_group);
#endif

		if (add_to_menu) add_to_menu(section, G_ACTION(action), key->name);
	}
}
