#pragma once

void     gdk_source_set_static_name_by_id (guint tag, const char *name);

gboolean gtk_widget_focus_move       (GtkWidget*, GtkDirectionType);
gboolean gtk_widget_grab_focus_child (GtkWidget*);
void     gtk_widget_focus_sort       (GtkWidget*, GtkDirectionType, GPtrArray* focus_order);
