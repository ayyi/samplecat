
void
test_1_empty_search ()
{
	START_TEST;

	assert(gtk_widget_get_realized((GtkWidget*)gtk_application_get_active_window(GTK_APPLICATION(app))), "window not realized");

	gboolean _do_search (gpointer _)
	{
#if 0
		extern void print_widget_tree (GtkWidget* widget);
		print_widget_tree((GtkWidget*)gtk_application_get_active_window(GTK_APPLICATION(app)));
#endif

		int n_rows = samplecat.store->row_count;
		assert_and_stop(n_rows > 0, "no library items");

		search("Hello");

		bool is_empty ()
		{
			return samplecat.store->row_count == 0;
		}

		void then (gpointer _)
		{
			FINISH_TEST;
		}

		wait_for((ReadyTest)is_empty, then, NULL);

		return G_SOURCE_REMOVE;
	}

	g_timeout_add(1000, _do_search, NULL);
}
