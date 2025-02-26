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

#pragma once

#include <gtk/gtk.h>

#define CUSTOM_MENU

#ifdef CUSTOM_MENU
#include "menu/popovermenu.h"
#define PopoverMenu AyyiPopoverMenu
#define POPOVER_MENU AYYI_POPOVER_MENU
#define popover_menu_new_from_model ayyi_popover_menu_new_from_model
#else
#define PopoverMenu GtkPopoverMenu
#define POPOVER_MENU GTK_POPOVER_MENU
#define popover_menu_new_from_model gtk_popover_menu_new_from_model
#endif

typedef struct
{
	const char* name;
	const char* action;
	const char* icon;
	int         target;
} MenuDef;

typedef struct {
	char          name[16];
#ifdef GTK4_TODO
	GtkStockItem* stock_item;
#else
	void*         stock_item;
#endif
	struct s_key {
		int        code;
		int        mask;
	}             key[2];
	gpointer      callback;
	gpointer      user_data;
} Accel;

GtkWidget* make_menu                (int size, MenuDef[size], gpointer);
void       add_menu_items_from_defn (GtkWidget* menu, GMenuModel*, int size, MenuDef[size], gpointer);
void       add_icon_menu_item       (PopoverMenu*, GMenu*, const char* name, const char* action, const char* icon);
void       add_icon_menu_item2      (PopoverMenu*, GMenu*, MenuDef*);
void       make_menu_actions        (GtkWidget*, Accel[], int, void (*add_to_menu)(GMenuModel*, GAction*, char*), GMenuModel*);
