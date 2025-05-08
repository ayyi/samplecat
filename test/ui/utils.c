/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2020-2020 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/


void
send_key (GdkWindow* window, int keyval, GdkModifierType modifiers)
{
	assert(gdk_test_simulate_key (window, -1, -1, keyval, modifiers, GDK_KEY_PRESS), "%i", keyval);
	assert(gdk_test_simulate_key (window, -1, -1, keyval, modifiers, GDK_KEY_RELEASE), "%i", keyval);
}


#ifdef USE_GDL
static GdlDockItem*
find_dock_item (const char* name)
{
	typedef struct
	{
		const char*  name;
		GdlDockItem* item;
	} C;

	GtkWidget* get_first_child (GtkWidget* widget)
	{
		GList* items = gtk_container_get_children((GtkContainer*)widget);
		GtkWidget* item = items->data;
		g_list_free(items);
		return item;
	}

	void gdl_dock_layout_foreach_object (GdlDockObject* object, gpointer user_data)
	{
		g_return_if_fail (object && GDL_IS_DOCK_OBJECT (object));

		C* c = user_data;
		char* name  = object->name;

		char* properties = NULL;
		if(!name){
			name = (char*)G_OBJECT_CLASS_NAME(G_OBJECT_GET_CLASS(object));
		}
		if(!strcmp(name, c->name)){
			c->item = (GdlDockItem*)object;
			return;
		}

		if (gdl_dock_object_is_compound (object)) {
			gtk_container_foreach(GTK_CONTAINER(object), (GtkCallback)gdl_dock_layout_foreach_object, c);
		}

		if(properties) g_free(properties);
	}

	GtkWidget* window = app->window;
	GtkWidget* vbox = get_first_child(window);
	GtkWidget* dock = get_first_child(vbox);

	C c = {name};
	gdl_dock_master_foreach_toplevel((GdlDockMaster*)GDL_DOCK_OBJECT(dock)->master, TRUE, (GFunc) gdl_dock_layout_foreach_object, &c);

	return c.item;
}
#endif


bool
window_is_open (void* _)
{
	return gtk_widget_get_realized(app->window);
}


GtkWidget*
get_view_menu ()
{
	GList* menu_items = gtk_container_get_children((GtkContainer*)app->context_menu);
	GtkWidget* item = g_list_nth_data(menu_items, 8);
	assert_null(GTK_IS_MENU_ITEM(item), "not menu item");
	g_list_free(menu_items);

	GList* items = gtk_container_get_children((GtkContainer*)item);
	for(GList* l=items;l;l=l->next){
		assert_null(!strcmp(gtk_label_get_text(l->data), "View"), "Expected View menu");
	}
	g_list_free(items);

	return item;
}


bool
menu_is_visible (gpointer _)
{
	return gtk_widget_get_visible(app->context_menu);
}


bool
submenu_is_visible (gpointer _)
{
	GtkWidget* item = get_view_menu();
	GtkWidget* submenu = gtk_menu_item_get_submenu((GtkMenuItem*)item);
	return gtk_widget_get_visible(submenu);
}


void
click_on_menu_item (GtkWidget* item)
{
	guint button = 1;
	if(!gdk_test_simulate_button (item->window, item->allocation.x + 10, item->allocation.y + 5, button, 0, GDK_BUTTON_PRESS)){
		dbg(0, "click failed");
	}
}


void
open_menu (WaitCallback callback, gpointer user_data)
{
	guint button = 3;
	if (!gdk_test_simulate_button (app->window->window, 350, 250, button, 0, GDK_BUTTON_PRESS)) {
		dbg(0, "click failed");
	}

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
	GtkWidget* item = get_view_menu();
	GtkWidget* submenu = gtk_menu_item_get_submenu((GtkMenuItem*)item);

	// open the View submenu
	if(!gtk_widget_get_visible(submenu)){
		click_on_menu_item(item);
	}

	wait_for(submenu_is_visible, callback, NULL);
}


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


bool
view_is_visible (gpointer name)
{
#ifdef USE_GDL
	GdlDockItem* item = find_dock_item(name);
	return item && gtk_widget_get_visible((GtkWidget*)item);
#else
	return true;
#endif
}


bool
view_not_visible (gpointer name)
{
#ifdef USE_GDL
	GdlDockItem* item = find_dock_item(name);
	return !item || !gtk_widget_get_visible((GtkWidget*)item);
#else
	return true;
#endif
}


void
search (const char* text)
{
	GtkWidget* search = find_widget_by_name(app->window, "search-entry");

#if 0
	send_key(search->window, GDK_KEY_H, 0);
	send_key(search->window, GDK_KEY_E, 0);
	send_key(search->window, GDK_KEY_Return, 0);
#else
	gtk_test_text_set(search, text);
#endif

	gtk_widget_activate(search);
}


