
//treeview/store layout:
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
  COL_NOTES, //this is in the store but not the view.
  COL_COLOUR,
  NUM_COLS
};

void        listview__new();
int         listview__path_get_id(GtkTreePath*);
void        listview__show_db_missing();
gboolean    listview__item_set_colour(GtkTreePath*, unsigned colour);

void        treeview_block_motion_handler();
void        treeview_unblock_motion_handler();

void        listmodel__update();
void        listmodel__add_result(SamplecatResult*);

