
#include "listmodel.h"

void        listview__new                        ();
void        listview__show_db_missing            ();
gboolean    listview__item_set_colour            (GtkTreePath*, unsigned colour);
void        listview__add_item                   ();
Result*     listview__get_first_selected_result  ();
gchar*      listview__get_first_selected_filename();
gchar*      listview__get_first_selected_filepath();

void        treeview_block_motion_handler        ();
void        treeview_unblock_motion_handler      ();

