
/*
 *  Hide the Library view via the View menu.
 */
void
test_2_hide_library ()
{
	START_TEST;

	assert(view_is_visible("Library"), "expected library panel visible");

	void then (gpointer _)
	{
		// context menu is now open

#if 0
		extern void print_widget_tree (GtkWidget* widget);
		print_widget_tree((GtkWidget*)item);
#endif

		void on_submenu_visible (gpointer _)
		{
			GtkWidget* context_menu = test_get_context_menu();
			GtkWidget* item = get_view_menu();

			gtk_widget_activate(item);
			GtkWidget* target = find_menuitem_by_name (context_menu, "Library");

			gtk_widget_activate(target);

			void on_library_hide (gpointer _)
			{
				FINISH_TEST;
			}

			wait_for(view_not_visible, on_library_hide, "Library");
		}

		open_submenu (on_submenu_visible, NULL);
	}

	open_menu (then, NULL);
}
