#include "config.h"
#include <gtk/gtk.h>
#include "utils.h"

/*
 *  Gdk private function
 */
void
gdk_source_set_static_name_by_id (guint tag, const char *name)
{

	g_return_if_fail (tag > 0);

	GSource* source = g_main_context_find_source_by_id (NULL, tag);
	if (source == NULL)
		return;

	g_source_set_static_name (source, name);
}

gboolean
gtk_widget_grab_focus_child (GtkWidget *widget)
{
  GtkWidget *child;

  for (child = gtk_widget_get_first_child (widget);
       child != NULL;
       child = gtk_widget_get_next_sibling (child))
    {
      if (gtk_widget_grab_focus (child))
        return TRUE;
    }

  return FALSE;
}
