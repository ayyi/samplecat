/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. https://ayyi.github.io/samplecat/    |
 | copyright (C) 2020-2025 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#include "widgets/suggestion_entry.h"

extern GtkWidget* test_get_context_menu ();
extern void print_widget_tree (GtkWidget* widget);


bool
window_is_open ()
{
	return gtk_widget_get_realized((GtkWidget*)gtk_application_get_active_window(GTK_APPLICATION(app)));
}


GtkWidget*
find_menuitem_by_name (GtkWidget* root, const char* name)
{
	GtkWidget* result = NULL;

	void find (GtkWidget* widget, GtkWidget** result)
	{
		GtkWidget* child = gtk_widget_get_first_child (widget);
		for (; child; child = gtk_widget_get_next_sibling (child)) {
			if (!strcmp(G_OBJECT_CLASS_NAME(G_OBJECT_GET_CLASS(child)), "GtkModelButton")) {
				GtkWidget* label = find_widget_by_type (child, GTK_TYPE_LABEL);
				if (label && !strcmp(gtk_label_get_text(GTK_LABEL(label)), name)) {
					*result = child;
					break;
				}
			}
			find(child, result);

			if (*result) break;
		}
	}

	find (root, &result);

	return result;
}


GtkWidget*
get_view_menu ()
{
	GtkWidget* menu = test_get_context_menu ();

	return find_menuitem_by_name(menu, "View");
}


void
select_view_menu_item (const char* name)
{
	GtkWidget* context_menu = test_get_context_menu();
	GtkWidget* item = get_view_menu();

	gtk_widget_activate(item);
	GtkWidget* target = find_menuitem_by_name (context_menu, name);

	gtk_widget_activate(target);
}


bool
menu_is_visible (gpointer _)
{
	GtkWidget* menu = test_get_context_menu ();
	return gtk_widget_get_visible(menu);
}


bool
submenu_is_visible (gpointer name)
{
	GtkWidget* context_menu = test_get_context_menu();
	GtkWidget* submenu_item = find_menuitem_by_name (context_menu, "Library");

	return gtk_widget_get_visible(submenu_item) && gtk_widget_get_width(submenu_item) > 0;
}


#ifdef GTK4_TODO
void
click_on_menu_item (GtkWidget* item)
{
	guint button = 1;
	if(!gdk_test_simulate_button (item->window, item->allocation.x + 10, item->allocation.y + 5, button, 0, GDK_BUTTON_PRESS)){
		dbg(0, "click failed");
	}
}
#endif


void
open_menu (WaitCallback callback, gpointer user_data)
{
	GtkWidget* context_menu = test_get_context_menu();

	gtk_popover_popup(GTK_POPOVER(context_menu));

	void then (gpointer _callback)
	{
		WaitCallback callback = _callback;
		callback(NULL);
	}

	wait_for(menu_is_visible, then, callback);
}


void
open_submenu (WaitCallback callback, gpointer user_data)
{
	GtkWidget* item1 = get_view_menu();
	gtk_widget_activate(item1);
	wait_for(submenu_is_visible, callback, "Library");
}


#ifdef GTK4_TODO
GtkWidget*
find_item_in_view_menu (const char* name)
{
	GtkWidget* item = NULL;

	GtkWidget* submenu = gtk_menu_item_get_submenu((GtkMenuItem*)get_view_menu());

	GList* items = gtk_container_get_children((GtkContainer*)submenu);
	for(GList* l=items;l;l=l->next){
		GtkWidget* panel_item = l->data;
		assert_null(GTK_IS_WIDGET(panel_item), "expected GtkWidget");
		assert_null((GTK_IS_CHECK_MENU_ITEM(panel_item)), "expected GtkCheckMenuItem, got %s", G_OBJECT_CLASS_NAME(G_OBJECT_GET_CLASS(panel_item)));
		const gchar* text = gtk_menu_item_get_label((GtkMenuItem*)panel_item);
		if(!strcmp(text, name)){
			item = panel_item;
			break;
		}
	}
	g_list_free(items);

	return item;
}
#endif


bool
view_is_visible (gpointer name)
{
	GdlDockItem* item = find_panel(name);
	return item && gtk_widget_get_visible((GtkWidget*)item);
}


bool
view_not_visible (gpointer name)
{
	GdlDockItem* item = find_panel(name);
	return !item || !gtk_widget_get_visible((GtkWidget*)item);
}


void
search (const char* text)
{
	GtkWidget* search = find_widget_by_type((GtkWidget*)find_panel("Search"), SUGGESTION_TYPE_ENTRY);

	gtk_widget_grab_focus(search);
	GtkEditable* editable = gtk_editable_get_delegate (GTK_EDITABLE(search));
	GtkEntryBuffer* buffer = gtk_text_get_buffer (GTK_TEXT(editable));
	gtk_entry_buffer_insert_text (buffer, -1, text, -1);

	gtk_widget_activate(search);
}
