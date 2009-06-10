
gpointer    overview_thread        (gpointer data);
GdkPixbuf*  make_overview          (sample*);
GdkPixbuf*  make_overview_sndfile  (sample*);
GdkPixbuf*  make_overview_flac     (sample*);
GdkPixbuf*  overview_pixbuf_new    ();
void        overview_draw_line     (GdkPixbuf*, int x, short max, short min);

