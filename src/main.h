#ifndef __samplecat_main_h__
#define __samplecat_main_h__

#include <gtk/gtk.h>
#include <pthread.h>
#include "typedefs.h"
#include "types.h"
#include "model.h"
#include "application.h"

#define START_EDITING 1

#define MAX_DISPLAY_ROWS 1000


void        do_search();

gboolean    new_search(GtkWidget*, gpointer userdata);

void        add_dir(const char* path, int* added_count);
void        delete_selected_rows();

gboolean    on_overview_done(gpointer sample);
gboolean    on_peaklevel_done(gpointer sample);
gboolean    on_ebur128_done(gpointer _sample);

void        on_quit(GtkMenuItem*, gpointer user_data);

#endif
