
void
test_5_show_waveform ()
{
	START_TEST;

	assert(view_not_visible("Waveform"), "expected waveform panel not visible");

	void then (gpointer _)
	{
		void on_submenu_visible (gpointer _)
		{
			GtkWidget* menu_item = find_item_in_view_menu("Waveform");

			assert(!gtk_check_menu_item_get_active((GtkCheckMenuItem*)menu_item), "expected GtkCheckMenuItem active");

			void on_show (gpointer _)
			{
				FINISH_TEST;
			}

			click_on_menu_item(menu_item);
			gtk_menu_item_activate((GtkMenuItem*)menu_item);
			gtk_menu_popdown(GTK_MENU(app->context_menu));

			wait_for(view_is_visible, on_show, "Waveform");
		}

		open_submenu (on_submenu_visible, NULL);
	}

	open_menu(then, NULL);
}
