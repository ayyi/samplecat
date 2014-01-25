#ifndef __listview_h__
#define __listview_h__

#include "listmodel.h"

struct _libraryview {
   GtkWidget*         widget;         // treeview
   GtkWidget*         scroll;

   struct {
      GtkCellRenderer* name;
      GtkCellRenderer* tags;
   }                   cells;
   GtkTreeViewColumn* col_name;
   GtkTreeViewColumn* col_path;
   GtkTreeViewColumn* col_pixbuf;
   GtkTreeViewColumn* col_tags;

   int                  selected;
   GtkTreeRowReference* mouseover_row_ref;
};

GtkWidget*  listview__new                        ();
void        listview__show_db_missing            ();
gboolean    listview__item_set_colour            (GtkTreePath*, unsigned colour);
void        listview__add_item                   ();
Sample*     listview__get_first_selected_sample  ();
gchar*      listview__get_first_selected_filename();
gchar*      listview__get_first_selected_filepath();
Sample*     listview__get_sample_by_rowref       (GtkTreeRowReference*);
void        listview__reset_colours              ();
void        listview__edit_row                   (GtkWidget*, gpointer);

void        highlight_playing_by_path            (GtkTreePath*);
void        highlight_playing_by_ref             (GtkTreeRowReference*);

void        listview__block_motion_handler       ();
gint        listview__get_mouseover_row          ();


#endif
