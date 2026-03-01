
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
		void on_submenu_visible (gpointer _)
		{
			select_view_menu_item ("Inspector");

			void on_inspector_hide (gpointer _)
			{
				FINISH_TEST;
			}

			wait_for(view_not_visible, on_inspector_hide, "Inspector");
		}

		open_submenu (on_submenu_visible, NULL);
	}

	open_menu (then, NULL);
}

