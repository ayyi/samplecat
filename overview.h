
gpointer    overview_thread(gpointer data);
GdkPixbuf*  make_overview(sample* sample);
GdkPixbuf*  make_overview_sndfile(sample* sample);
GdkPixbuf*  make_overview_flac(sample* sample);
GdkPixbuf*  overview_pixbuf_new();
void        overview_draw_line(GdkPixbuf* pixbuf, int x, short max, short min);

