#pragma once

#include <gtk/gtk.h>

typedef struct _GtkScrolledWindowClass GtkScrolledWindowClass;

struct _GtkScrolledWindow
{
  GtkWidget parent_instance;
};

struct _GtkScrolledWindowClass
{
  GtkWidgetClass parent_class;

  /* Unfortunately, GtkScrollType is deficient in that there is
   * no horizontal/vertical variants for GTK_SCROLL_START/END,
   * so we have to add an additional boolean flag.
   */
  gboolean (*scroll_child) (GtkScrolledWindow *scrolled_window,
                            GtkScrollType      scroll,
                            gboolean           horizontal);

  void (* move_focus_out) (GtkScrolledWindow *scrolled_window,
                           GtkDirectionType   direction);
};

