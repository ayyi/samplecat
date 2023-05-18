/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. https://ayyi.github.io/samplecat/    |
 | copyright (C) 2007-2023 Tim Orford <tim@orford.org>                  |
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
				if (!item->target) {
					g_menu_append_submenu (G_MENU(section[s-1]), item->name, section[s]);
				} else {
					GMenuItem* mi = g_menu_item_new_submenu(item->name,  section[s]);
					g_menu_item_set_action_and_target (mi, item->action, "s", item->target);
					g_menu_append_item(G_MENU(section[s - 1]), mi);
					g_object_unref(mi);
				}

				break;
			default:
				if (item->icon) {
					GMenuItem* mi = g_menu_item_new ("Label", "item->action");
					g_menu_item_set_attribute (mi, "custom", "s", item->action, NULL); // connect the menu item to the action
					g_menu_append_item (G_MENU(section[s]), mi);

					GtkWidget* box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
					GtkWidget* button = gtk_button_new();
					gtk_actionable_set_action_name (GTK_ACTIONABLE(button), item->action);
					gtk_button_set_child(GTK_BUTTON(button), box);
					{
						GtkWidget* image = gtk_image_new_from_icon_name(item->icon);
						gtk_box_append(GTK_BOX(box), image);
						gtk_box_append(GTK_BOX(box), gtk_label_new(item->name));
					}
					if (!gtk_popover_menu_add_child(GTK_POPOVER_MENU(widget), button, item->action)) dbg(0, "not added %i", item->action);
				} else {
					g_menu_append (G_MENU(section[s]), item->name, item->action);
				}
#ifdef GTK4_TODO
				if (item->callback)
					g_signal_connect (G_OBJECT(menu_item), "activate", G_CALLBACK(item->callback), item->callback_data ? GINT_TO_POINTER(item->callback_data) : user_data);
#endif
		}
	}
}

