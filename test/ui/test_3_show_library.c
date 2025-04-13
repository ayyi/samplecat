
void
test_3_show_library ()
{
	START_TEST;

	assert(view_not_visible("Library"), "expected library panel not visible");

	void then (gpointer _)
	{
		void on_submenu_visible (gpointer _)
		{
			select_view_menu_item ("Library");

			void on_show (gpointer _)
			{
				GdlDockItem* library = find_panel("Library");
				GtkWidget* notebook = gtk_widget_get_ancestor(GTK_WIDGET(library), GTK_TYPE_NOTEBOOK);
				if (notebook) {
					int p = gtk_notebook_get_current_page ((GtkNotebook*)notebook);
					int q = gtk_notebook_page_num ((GtkNotebook*)notebook, GTK_WIDGET(library));
					assert(p == q, "library is not current page expected %i but current page is %i", q, p);
				}

				FINISH_TEST;
			}

			wait_for(view_is_visible, on_show, "Library");
		}

		open_submenu (on_submenu_visible, NULL);
	}

	open_menu(then, NULL);
}
