#include <gtk/gtk.h>
gboolean    window_new            ();
void        colour_box_update     ();
gboolean    tag_selector_new      ();
gboolean    tagshow_selector_new  ();
GtkWidget*  message_panel__add_msg(const gchar* msg, const gchar* stock_id);
void        show_waveform         (gboolean enable);
#ifdef HAVE_FFTW3
void        show_spectrogram      (gboolean enable);
#endif
void        show_filemanager      (gboolean enable);

