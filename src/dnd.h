#ifndef __SAMPLECAT_DND_H_
#define __SAMPLECAT_DND_H_
#include <gtk/gtk.h>
enum {
  TARGET_APP_COLLECTION_MEMBER,
  TARGET_URI_LIST,
  TARGET_TEXT_PLAIN
};

#ifdef __dnd_c__
GtkTargetEntry dnd_file_drag_types[] = {
      { "text/uri-list", 0, TARGET_URI_LIST },
    //{ "text/plain", 0, TARGET_TEXT_PLAIN }
};
gint dnd_file_drag_types_count = 1;
#else
extern GtkTargetEntry dnd_file_drag_types[1];
extern gint dnd_file_drag_types_count;
#endif

void     dnd_setup    ();
gint     drag_received(GtkWidget*, GdkDragContext*, gint x, gint y, GtkSelectionData*, guint info, guint time, gpointer user_data);
gint     drag_motion  (GtkWidget*, GdkDragContext*, gint x, gint y, guint time, gpointer user_data);
#endif
