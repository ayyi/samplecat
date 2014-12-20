#ifndef __samplecat_main_h__
#define __samplecat_main_h__

#include <gtk/gtk.h>
#include <pthread.h>
#include "typedefs.h"
#include "types.h"
#include "model.h"
#include "application.h"

#define START_EDITING 1


gboolean    new_search(GtkWidget*, gpointer userdata);

void        delete_selected_rows();

void        on_quit(GtkMenuItem*, gpointer user_data);

#endif
