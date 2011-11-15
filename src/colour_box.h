#ifndef __SAMPLECAT_COLORBOX_H_
#define __SAMPLECAT_COLORBOX_H_

#include <gtk/gtk.h>
struct _colour_box
{
	GtkWidget* menu;
};

GtkWidget* colour_box_new (GtkWidget* parent);
gboolean   colour_box_add (GdkColor*);
#endif
