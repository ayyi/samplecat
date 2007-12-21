
gboolean	window_new();
void        window_on_allocate (GtkWidget *win, gpointer user_data);
void        window_on_realise  (GtkWidget *win, gpointer user_data);
gboolean    window_on_configure(GtkWidget *widget, GdkEventConfigure*, gpointer user_data);
GtkWidget*  left_pane();
void        colour_box_update();
GtkWidget*  dir_tree_new();
