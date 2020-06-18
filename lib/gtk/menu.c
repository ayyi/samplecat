/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2007-2020 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/

#include "config.h"
#include "gtk/menu.h"
#include "debug/debug.h"


static void
menu_item_image_from_stock (GtkWidget* menu, GtkWidget* item, char* stock_id)
{
	GtkWidget* icon = gtk_image_new_from_stock(stock_id, GTK_ICON_SIZE_MENU);

	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), icon);
}


static GtkWidget*
menu_separator_new (GtkWidget* container)
{
	GtkWidget* separator = gtk_menu_item_new();
	gtk_widget_set_sensitive(separator, FALSE);
	gtk_container_add(GTK_CONTAINER(container), separator);

	return separator;
}


GtkWidget*
make_menu (int size, MenuDef menu_def[size], gpointer user_data)
{
	GtkWidget* menu = gtk_menu_new();

	add_menu_items_from_defn (menu, size, menu_def, user_data);

	return menu;
}


void
add_menu_items_from_defn (GtkWidget* menu, int size, MenuDef defn[size], gpointer user_data)
{
	GtkWidget* parent = menu;
	GtkWidget* sub = NULL;

	for(int i=0;i<size;i++){
		MenuDef* item = &defn[i];
		switch(item->label[0]){
			case '-':
				menu_separator_new(menu);
				break;
			case '<':
				gtk_menu_item_set_submenu(GTK_MENU_ITEM(sub), parent);
				parent = menu;
				break;
			case '>':
				item = &defn[++i]; // following item must be the submenu parent item.
				parent = gtk_menu_new();

				GtkWidget* _sub = sub = gtk_image_menu_item_new_with_label(item->label);
				if(item->stock_id){
					menu_item_image_from_stock(menu, _sub, item->stock_id);
				}
				gtk_container_add(GTK_CONTAINER(menu), _sub);

				break;
			default:
				;GtkWidget* menu_item = gtk_image_menu_item_new_with_label (item->label);
				gtk_menu_shell_append (GTK_MENU_SHELL(parent), menu_item);
				if(item->stock_id){
					menu_item_image_from_stock(menu, menu_item, item->stock_id);
				}
				if(item->callback)
					g_signal_connect (G_OBJECT(menu_item), "activate", G_CALLBACK(item->callback), item->callback_data ? GINT_TO_POINTER(item->callback_data) : user_data);
		}
	}

	gtk_widget_show_all((GtkWidget*)menu);
}

