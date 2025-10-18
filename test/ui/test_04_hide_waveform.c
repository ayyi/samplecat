
/*
 *  Hide the Waveform view via the View menu.
 */
void
test_04_hide_waveform ()
{
	START_TEST;

	assert(view_is_visible("Waveform"), "expected waveform visible");

	void then (gpointer _)
	{
		void on_submenu_visible (gpointer _)
		{
			GtkWidget* submenu = gtk_menu_item_get_submenu((GtkMenuItem*)get_view_menu());
			assert(gtk_widget_get_visible(submenu), "submenu not visible");

			GtkWidget* waveform_item = find_item_in_view_menu("Waveform");

			assert(gtk_check_menu_item_get_active((GtkCheckMenuItem*)waveform_item), "expected GtkCheckMenuItem active");

			void on_waveform_hide (gpointer _)
			{
				FINISH_TEST;
			}

			click_on_menu_item(waveform_item);
			gtk_menu_item_activate((GtkMenuItem*)waveform_item);

			wait_for(view_not_visible, on_waveform_hide, "Waveform");
		}

		open_submenu (on_submenu_visible, NULL);
	}

	open_menu(then, NULL);
}

