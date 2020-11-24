
void
test_1 ()
{
	START_TEST;

	void do_search (gpointer _)
	{
		assert(gtk_widget_get_realized(app->window), "window not realized");

		gboolean _do_search (gpointer _)
		{
#if 0
			extern void print_widget_tree (GtkWidget* widget);
			print_widget_tree(app->window);
#endif

			GtkWidget* search = find_widget_by_name(app->window, "search-entry");

			int n_rows = gtk_tree_model_iter_n_children(gtk_tree_view_get_model((GtkTreeView*)app->libraryview->widget), NULL);
			assert_and_stop(n_rows > 0, "no library items");
#if 0
			send_key(search->window, GDK_KEY_H, 0);
			send_key(search->window, GDK_KEY_E, 0);
			send_key(search->window, GDK_KEY_Return, 0);
#else
			gtk_test_text_set(search, "Hello");
#endif
			gtk_widget_activate(search);

			bool is_empty ()
			{
				return gtk_tree_model_iter_n_children(gtk_tree_view_get_model((GtkTreeView*)app->libraryview->widget), NULL) == 0;
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

	bool window_is_open ()
	{
		return gtk_widget_get_realized(app->window);
	}

	wait_for(window_is_open, do_search, NULL);
}


