#ifndef __inspector_h__
#define __inspector_h__
#include <gtk/gtk.h>
#include "typedefs.h"

GtkWidget* inspector_new                  ();
void       inspector_free                 (Inspector*);
void       inspector_set_labels           (Sample*);

#endif
