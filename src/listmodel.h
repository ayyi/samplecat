#ifndef __LISTMODEL_H_
#define __LISTMODEL_H_
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
  COL_MISC,
  COL_SAMPLEPTR,
  NUM_COLS
};

GtkListStore* listmodel__new                   ();
void          listmodel__update                ();
void          listmodel__clear                 ();
void          listmodel__add_result            (Sample*);

char*         listmodel__get_filename_from_id  (int);

void          listmodel__set_overview          (GtkTreeRowReference*, GdkPixbuf*);
void          listmodel__set_peaklevel         (GtkTreeRowReference*, float);
#endif
