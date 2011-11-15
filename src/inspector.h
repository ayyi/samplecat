#ifndef __INSPECTOR_H_
#define __INSPECTOR_H_
#include <gtk/gtk.h>
#include "typedefs.h"

GtkWidget* inspector_new();
void       inspector_free(Inspector*);
void       inspector_update_from_result(Sample*);
void       inspector_set_labels(Sample* sample);
void       inspector_update_from_fileview(GtkTreeView*);

void       show_player();

#endif
