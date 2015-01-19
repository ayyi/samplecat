#ifndef __LISTMODEL_H_
#define __LISTMODEL_H_
#include <gtk/gtk.h>
#include "typedefs.h"
#include "list_store.h"

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
  COL_LEN,
  NUM_COLS, ///< end of columns in the store
  // these are NOT in the store but in the sample-struct (COL_SAMPLEPTR)
  COL_X_EBUR,
  COL_X_NOTES
};

GtkListStore* listmodel__new                   ();
void          listmodel__clear                 ();
void          listmodel__add_result            (Sample*);

char*         listmodel__get_filename_from_id  (int);
void          listmodel__move_files            (GList*, const gchar* dest_path);

#endif
