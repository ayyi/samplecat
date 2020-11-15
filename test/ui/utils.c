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


