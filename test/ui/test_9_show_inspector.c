
void
test_9_show_inspector ()
{
	START_TEST;

	assert(view_not_visible("Inspector"), "expected inspector panel not visible");

	void then (gpointer _)
	{
		void on_submenu_visible (gpointer _)
		{
			GtkWidget* library_item = find_item_in_view_menu("Inspector");

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
				GtkWidget* inspector = find_dock_item("Inspector")->child;
				GtkWidget* table = find_widget_by_name(inspector, "GtkSimpleTable");
				assert(table, "Table not found");

				GList* children = gtk_container_get_children(GTK_CONTAINER(table));
				int i = 0;
				for (GList* l = children; l; l = l->next, i++) {
					if (!strcmp("GtkLabel", gtk_widget_get_name(l->data))) {
						if (i >= G_N_ELEMENTS(items)) break;
						const gchar* text = gtk_label_get_text(GTK_LABEL(l->data));
						assert(!strcmp(text, items[i]), "expected %s got %s", items[i], text);
					}
				}

				FINISH_TEST;
			}

			click_on_menu_item(library_item);
			gtk_menu_item_activate((GtkMenuItem*)library_item);
			gtk_menu_popdown(GTK_MENU(app->context_menu));

			wait_for(view_is_visible, on_show, "Inspector");
		}

		open_submenu (on_submenu_visible, NULL);
	}

	open_menu(then, NULL);
}
