
void
test_10_directories ()
{
	START_TEST;

	void on_directories (gpointer _)
	{
		GtkWidget* treeview = app->dir_treeview;

		int n_rows = gtk_tree_model_iter_n_children(gtk_tree_view_get_model((GtkTreeView*)treeview), NULL);
		assert(n_rows > 0, "no directory items");

		FINISH_TEST;
	}

	wait_for(view_is_visible, on_directories, "Directories");
}
