#ifndef __LISTMODEL_H_
#define __LISTMODEL_H_
#include <gtk/gtk.h>
#include "typedefs.h"

enum
{
  COL_ICON = 0,
#ifdef USE_AYYI
  COL_AYYI_ICON,
#endif
  COL_IDX,
  COL_NAME,
  COL_FNAME,
  COL_KEYWORDS,
  COL_OVERVIEW,
  COL_LENGTH,
  COL_SAMPLERATE,
  COL_CHANNELS,
  COL_MIMETYPE,
  COL_PEAKLEVEL,
  //from here on items are the store but not the view.
  COL_COLOUR,
  COL_SAMPLEPTR,
  NUM_COLS, ///< end of columns in the store
  // these are NOT in the store but in the sample-struct (COL_SAMPLEPTR)
  COLX_EBUR,
  COLX_NOTES
};

GtkListStore* listmodel__new                   ();
void          listmodel__clear                 ();
void          listmodel__add_result            (Sample*);

void          listmodel__update_result         (Sample*, int what);
gboolean      listmodel__update_by_ref         (GtkTreeIter *iter, int what, void *data);
gboolean      listmodel__update_by_rowref      (GtkTreeRowReference *row_ref, int what, void *data);

char*         listmodel__get_filename_from_id  (int);
void          listmodel__move_files            (GList *list, const gchar *dest_path);

void          listmodel__set_overview          (GtkTreeRowReference*, GdkPixbuf*);
void          listmodel__set_peaklevel         (GtkTreeRowReference*, float);
#endif
