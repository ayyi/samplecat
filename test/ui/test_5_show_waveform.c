
void
test_5_show_waveform ()
{
	START_TEST;

	assert(view_not_visible("Waveform"), "expected waveform panel not visible");

	void then (gpointer _)
	{
		void on_submenu_visible (gpointer _)
		{
			select_view_menu_item ("Waveform");

			void on_show (gpointer _)
			{
				FINISH_TEST;
			}

			wait_for(view_is_visible, on_show, "Waveform");
		}

		open_submenu (on_submenu_visible, NULL);
	}

	open_menu(then, NULL);
}
