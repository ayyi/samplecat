
void
test_3_show_library ()
{
#ifdef GTK4_TODO
	START_TEST;

	assert(view_not_visible("Library"), "expected library panel not visible");

	void then (gpointer _)
	{
		void on_submenu_visible (gpointer _)
		{
			GtkWidget* library_item = find_item_in_view_menu("Library");

			void on_show (gpointer _)
			{
				FINISH_TEST;
			}

			click_on_menu_item(library_item);
			gtk_menu_item_activate((GtkMenuItem*)library_item);

			wait_for(view_is_visible, on_show, "Library");
		}

		open_submenu (on_submenu_visible, NULL);
	}

	open_menu(then, NULL);
#endif
}
