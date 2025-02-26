#pragma once

gboolean gtk_widget_focus_move       (GtkWidget*, GtkDirectionType);
gboolean gtk_widget_grab_focus_child (GtkWidget*);
void     gtk_widget_focus_sort       (GtkWidget*, GtkDirectionType, GPtrArray* focus_order);
