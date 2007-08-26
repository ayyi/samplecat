
//dnd:
enum {
  TARGET_APP_COLLECTION_MEMBER,
  TARGET_URI_LIST,
  TARGET_TEXT_PLAIN
};

#ifdef IS_DND_C
GtkTargetEntry dnd_file_drag_types[] = {
      { "text/uri-list", 0, TARGET_URI_LIST },
      { "text/plain", 0, TARGET_TEXT_PLAIN }
};
gint dnd_file_drag_types_count = 2;
#else
GtkTargetEntry dnd_file_drag_types[2];
gint dnd_file_drag_types_count;
#endif

void        dnd_setup();
gint        drag_received(GtkWidget *widget, GdkDragContext *drag_context, gint x, gint y,
                          GtkSelectionData *data, guint info, guint time, gpointer user_data);
gint        drag_motion(GtkWidget *widget, GdkDragContext *drag_context, gint x, gint y,
                               guint time, gpointer user_data);

