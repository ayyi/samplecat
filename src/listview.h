
#include "listmodel.h"

void        listview__new                        ();
void        listview__show_db_missing            ();
gboolean    listview__item_set_colour            (GtkTreePath*, unsigned colour);
void        listview__add_item                   ();
Sample*     listview__get_first_selected_result  ();
gchar*      listview__get_first_selected_filename();
gchar*      listview__get_first_selected_filepath();
void        listview__reset_colours              ();

void        highlight_playing_by_path            (GtkTreePath *path);
void        highlight_playing_by_ref             (GtkTreeRowReference *ref);

void        treeview_block_motion_handler        ();
void        treeview_unblock_motion_handler      ();

