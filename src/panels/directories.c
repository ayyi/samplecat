/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. https://ayyi.github.io/samplecat/    |
 | copyright (C) 2007-2024 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#include "config.h"
#include <gtk/gtk.h>
#include <string.h>
#include "debug/debug.h"
#include "dh_link.h"
#include "support.h"
#include "application.h"
#include "model.h"
#include "dir_list.h"
#include "directories.h"


GtkWidget*
dir_panel_new ()
{
	dir_list_update(); // because this is slow, it is not done until a consumer needs it.

	GtkWidget* widget = scrolled_window_new();
	gtk_widget_add_css_class(widget, "directories");

	gtk_scrolled_window_set_child((GtkScrolledWindow*)widget, ({
		void setup_row (GtkSignalListItemFactory* self, GtkListItem* list_item, gpointer user_data)
		{
			GtkWidget* expander = gtk_tree_expander_new ();

			GtkWidget* box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
			GtkWidget* icon = gtk_image_new_from_icon_name ("inode-directory");
			GtkWidget* label = gtk_label_new ("");
			gtk_box_append(GTK_BOX(box), icon);
			gtk_box_append(GTK_BOX(box), label);
			gtk_tree_expander_set_child (GTK_TREE_EXPANDER(expander), box);
			gtk_list_item_set_child (list_item, expander);
		}

		void bind_row (GtkSignalListItemFactory* self, GtkListItem* list_item, gpointer user_data)
		{
			GtkTreeListRow* row = gtk_list_item_get_item (list_item);
			GtkWidget* expander = gtk_list_item_get_child (list_item);
			gtk_tree_expander_set_list_row (GTK_TREE_EXPANDER(expander), row);
			GtkStringObject* obj = gtk_tree_list_row_get_item (row);

			GtkWidget* box = gtk_tree_expander_get_child (GTK_TREE_EXPANDER(expander));
			GtkWidget* icon = gtk_widget_get_first_child(box);
			GtkWidget* label = gtk_widget_get_next_sibling(icon);
			gtk_label_set_text (GTK_LABEL(label), gtk_string_object_get_string (gtk_tree_list_row_get_item (row)));

			void click_gesture_pressed (GtkGestureClick* gesture, int n_press, double x, double y, gpointer obj)
			{
				GNode* node = g_object_get_data (G_OBJECT(GTK_STRING_OBJECT(obj)), "node");
				DhLink* link = (DhLink*) node->data;
				observable_string_set(samplecat.model->filters2.dir, g_strdup(link->uri));
			}
			GtkGesture* click = gtk_gesture_click_new ();
			g_signal_connect (click, "pressed", G_CALLBACK (click_gesture_pressed), obj);
			gtk_widget_add_controller (box, GTK_EVENT_CONTROLLER (click));
		}

		void add_node_to_store (GNode* nodes, GListStore* store)
		{
			GNode* node = g_node_first_child (nodes);
			for (; node; node = g_node_next_sibling (node)) {
				DhLink* link = (DhLink*) node->data;
				g_autoptr(GtkStringObject) obj = gtk_string_object_new (link->name);
				g_object_set_data (G_OBJECT(obj), "node", node);
				g_list_store_append (store, obj);
			}
		}

		GListModel* create_store_for_node (GNode* nodes)
		{
			GNode* node = g_node_first_child (nodes);
			if (!node)
				return NULL;

			GListStore* store = g_list_store_new (GTK_TYPE_STRING_OBJECT);

			for (; node; node = g_node_next_sibling (node)) {
				DhLink* link = (DhLink*) node->data;
				g_autoptr(GtkStringObject) obj = gtk_string_object_new (link->name);
				g_object_set_data (G_OBJECT(obj), "node", node);
				g_list_store_append (store, obj);
			}

			return G_LIST_MODEL(store);
		}

		GListModel* tree_list_model_creator (gpointer item, gpointer user_data)
		{
			GNode* node = g_object_get_data (G_OBJECT(GTK_STRING_OBJECT(item)), "node");
			return create_store_for_node (node);
		}

		GtkListItemFactory* create_factory ()
		{
			GtkListItemFactory* factory = gtk_signal_list_item_factory_new ();

			g_signal_connect (factory, "setup", G_CALLBACK(setup_row), NULL);
			g_signal_connect (factory, "bind", G_CALLBACK(bind_row), NULL);

			return factory;
		}

		GListModel* store = create_store_for_node (samplecat.model->dir_tree);
		GtkTreeListModel* treemodel = gtk_tree_list_model_new (store, false, false, tree_list_model_creator, NULL, NULL);
		GtkSelectionModel* selection = GTK_SELECTION_MODEL(gtk_single_selection_new (G_LIST_MODEL(treemodel)));
		GtkWidget* view = gtk_list_view_new (selection, create_factory());

		void on_dir_list_changed (GObject* _model, gpointer view)
		{
			GtkSelectionModel* selection = gtk_list_view_get_model (GTK_LIST_VIEW(view));
			GListStore* store = G_LIST_STORE (gtk_single_selection_get_model (GTK_SINGLE_SELECTION(selection)));
			g_list_store_remove_all (store);
			add_node_to_store (samplecat.model->dir_tree, store);
		}
		g_signal_connect(samplecat.model, "dir-list-changed", G_CALLBACK(on_dir_list_changed), view);

		view;
	}));


#ifdef GTK4_TODO
	void dir_on_theme_changed (Application* a, char* theme, gpointer _tree)
	{
		GtkWidget* container = gtk_widget_get_parent(a->dir_treeview);
		gtk_widget_unparent(a->dir_treeview);
		a->dir_treeview = dh_book_tree_new(&samplecat.model->dir_tree);
		gtk_scrolled_window_set_child((GtkScrolledWindow*)container, a->dir_treeview);
	}
	g_signal_connect(app, "icon-theme", G_CALLBACK(dir_on_theme_changed), &tree);
#endif

	return widget;
}
