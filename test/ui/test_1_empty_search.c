
void
test_1_empty_search ()
{
	START_TEST;

	assert(gtk_widget_get_realized((GtkWidget*)gtk_application_get_active_window(GTK_APPLICATION(app))), "window not realized");

	gboolean _do_search (gpointer _)
	{
#if 0
		extern void print_widget_tree (GtkWidget* widget);
		print_widget_tree(app->window);
#endif

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
		int n_rows = gtk_tree_model_iter_n_children(gtk_tree_view_get_model((GtkTreeView*)APPLICATION(app)->libraryview->widget), NULL);
#pragma GCC diagnostic warning "-Wdeprecated-declarations"
		assert_and_stop(n_rows > 0, "no library items");

		search("Hello");

		bool is_empty ()
		{
			return gtk_tree_model_iter_n_children(gtk_tree_view_get_model((GtkTreeView*)APPLICATION(app)->libraryview->widget), NULL) == 0;
		}

		void then (gpointer _)
		{
			FINISH_TEST;
		}

		wait_for(is_empty, then, NULL);

		return G_SOURCE_REMOVE;
	}

	g_timeout_add(1000, _do_search, NULL);
}
