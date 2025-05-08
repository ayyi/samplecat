
/*
 *  Hide the Inspector view via the View menu.
 */
void
test_8_hide_inspector ()
{
	START_TEST;

	assert(view_is_visible("Inspector"), "expected inspector panel visible");

	void then (gpointer _)
	{
		// context menu is now open

		GtkWidget* item = get_view_menu();

		GtkWidget* submenu = gtk_menu_item_get_submenu((GtkMenuItem*)item);
		assert(!gtk_widget_get_visible(submenu), "submenu unexpectedly visible");

		void on_submenu_visible (gpointer _)
		{
			GtkWidget* submenu = gtk_menu_item_get_submenu ((GtkMenuItem*)get_view_menu());
			assert (gtk_widget_get_visible (submenu), "submenu not visible");

			GtkWidget* library_item = find_item_in_view_menu("Inspector");

			void on_library_hide (gpointer _)
			{
				FINISH_TEST;
			}

			click_on_menu_item(library_item);
			gtk_menu_item_activate((GtkMenuItem*)library_item);
			gtk_menu_popdown(GTK_MENU(app->context_menu));

			wait_for(view_not_visible, on_library_hide, "Inspector");
		}

		open_submenu (on_submenu_visible, NULL);
	}

	open_menu (then, NULL);
}

