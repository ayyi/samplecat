
/*
 *  Hide the Library view via the View menu.
 */
void
test_2_hide_library ()
{
	START_TEST;

	guint button = 3;
	if(!gdk_test_simulate_button (app->window->window, 400, 400, button, 0, GDK_BUTTON_PRESS)){
		dbg(0, "click failed");
	}

	bool menu_is_visible (gpointer _)
	{
		return gtk_widget_get_visible(app->context_menu);
	}

	GtkWidget* get_view_menu ()
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

	void then (gpointer _)
	{
		// context menu is now open

		GtkWidget* item = get_view_menu();

		GtkWidget* submenu = gtk_menu_item_get_submenu((GtkMenuItem*)item);
		assert(!gtk_widget_get_visible(submenu), "submenu unexpectedly visible");

		void click_on_item (GtkWidget* item)
		{
			guint button = 1;
			if(!gdk_test_simulate_button (item->window, item->allocation.x + 10, item->allocation.y + 5, button, 0, GDK_BUTTON_PRESS)){
				dbg(0, "click failed");
			}
		}

		// open the View submenu
		click_on_item(item);

		bool submenu_is_visible (gpointer _)
		{
			GtkWidget* item = get_view_menu();
			GtkWidget* submenu = gtk_menu_item_get_submenu((GtkMenuItem*)item);
			return gtk_widget_get_visible(submenu);
		}

		void on_submenu_visible (gpointer _)
		{
			GtkWidget* item = get_view_menu();
			GtkWidget* submenu = gtk_menu_item_get_submenu((GtkMenuItem*)item);
			assert(gtk_widget_get_visible(submenu), "submenu not visible");

			// library
			GtkWidget* library_item = NULL;
			GList* items = gtk_container_get_children((GtkContainer*)submenu);
			for(GList* l=items;l;l=l->next){
				GtkWidget* panel_item = l->data;
				assert(GTK_IS_WIDGET(panel_item), "expected GtkWidget");
				assert((GTK_IS_CHECK_MENU_ITEM(panel_item)), "expected GtkCheckMenuItem");
				assert(gtk_check_menu_item_get_active((GtkCheckMenuItem*)panel_item), "expected GtkCheckMenuItem");
				library_item = panel_item;
				break;
			}
			g_list_free(items);


			bool library_is_visible (gpointer _)
			{
				GdlDockItem* item = find_dock_item("Library");
				return item && gtk_widget_get_visible((GtkWidget*)item);
			}

			bool library_not_visible (gpointer _)
			{
				GdlDockItem* item = find_dock_item("Library");
				return !item || !gtk_widget_get_visible((GtkWidget*)item);
			}

			void on_library_hide (gpointer _)
			{

				FINISH_TEST;
			}

			assert(library_is_visible(NULL), "expected library visible");

			// TODO why menu not closed ?
			click_on_item(library_item);
			gtk_menu_item_activate((GtkMenuItem*)library_item);
			wait_for(library_not_visible, on_library_hide, NULL);
		}

		wait_for(submenu_is_visible, on_submenu_visible, NULL);
	}

	wait_for(menu_is_visible, then, NULL);
}

