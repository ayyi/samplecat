
gboolean    window_new            ();
GtkWidget*  left_pane             ();
void        colour_box_update     ();
GtkWidget*  dir_tree_new          ();
gboolean    tag_selector_new      ();
gboolean    tagshow_selector_new  ();
GtkWidget*  message_panel__add_msg(const gchar* msg, const gchar* stock_id);
#ifdef HAVE_FFTW3
void        show_spectrogram      (gboolean enable);
#endif

