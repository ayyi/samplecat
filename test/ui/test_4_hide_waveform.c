
/*
 *  Hide the Waveform view via the View menu.
 */
void
test_4_hide_waveform ()
{
	START_TEST;

	assert(view_is_visible("Waveform"), "expected waveform visible");

	void then (gpointer _)
	{
		void on_submenu_visible (gpointer _)
		{
			select_view_menu_item ("Waveform");

			void on_waveform_hide (gpointer _)
			{
				FINISH_TEST;
			}

			wait_for(view_not_visible, on_waveform_hide, "Waveform");
		}

		open_submenu (on_submenu_visible, NULL);
	}

	open_menu(then, NULL);
}

