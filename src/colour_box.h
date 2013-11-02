#ifndef __samplecat_colorbox_h__
#define __samplecat_colorbox_h__

#include <gtk/gtk.h>
struct _colour_box
{
	GtkWidget* menu;
};

void       colour_box_init      ();
GtkWidget* colour_box_new       (GtkWidget* parent);
gboolean   colour_box_add       (GdkColor*);
void       colour_box_colourise ();

#endif
