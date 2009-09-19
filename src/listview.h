
#include "listmodel.h"

void        listview__new                   ();
int         listview__path_get_id           (GtkTreePath*);
void        listview__show_db_missing       ();
gboolean    listview__item_set_colour       (GtkTreePath*, unsigned colour);

void        treeview_block_motion_handler   ();
void        treeview_unblock_motion_handler ();

