
void
test_9_show_inspector ()
{
	START_TEST;

	assert(view_not_visible("Inspector"), "expected inspector panel not visible");

	void then (gpointer _)
	{
		void on_submenu_visible (gpointer _)
		{
			select_view_menu_item ("Inspector");

			void on_show (gpointer _)
			{
				const char* items[] = {
					"5.4 LU",
					"short term max",
					"14.0 LU",
					"momentary max",
					"-177.0 LU",
					"range max",
					"-177.0 LU",
					"range min",
					"-177.0 LU",
					"range threshold",
					"-177.0 LU",
					"intgd threshold",
					"0.0 LU",
					"loudness range",
					"-177.0 LU",
					"intgd loudness",
					"16 b/sample",
					"bitdepth",
					"172.3 kB/s",
					"bitrate",
					"-0.01 dBA",
					"level",
					"audio/vnd.wave",
					"mimetype",
					"stereo",
					"channels",
					"44.1 kHz",
					"samplerate",
					"44100",
					"frames",
					"00:00:01.000",
					"length",
				};
				GtkWidget* window = (GtkWidget*)gtk_application_get_active_window(GTK_APPLICATION(app));
				GtkWidget* inspector = find_widget_by_name (window, "Inspector");
				GtkWidget* table = find_widget_by_name(inspector, "GtkSimpleTable");
				assert(table, "Table not found");

				GtkWidget* child = gtk_widget_get_first_child(table);
				if (child) {
					for (int i = 0; child; child = gtk_widget_get_next_sibling(child), i++) {
						if (!strcmp("GtkLabel", gtk_widget_get_name(child))) {
							if (i >= G_N_ELEMENTS(items)) break;
							const gchar* text = gtk_label_get_text(GTK_LABEL(child));
							assert(!strcmp(text, items[i]), "expected %s got %s", items[i], text);
						}
					}
				}

				FINISH_TEST;
			}

			wait_for(view_is_visible, on_show, "Inspector");
		}

		open_submenu (on_submenu_visible, NULL);
	}

	open_menu(then, NULL);
}
