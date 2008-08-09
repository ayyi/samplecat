
//treeview/store layout:
enum
{
  COL_ICON = 0,
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
void        list__update();
int         listview__path_get_id(GtkTreePath* path);
void        listview__show_db_missing();
gboolean    listview__item_set_colour(GtkTreePath* path, unsigned colour);

void        treeview_block_motion_handler();
void        treeview_unblock_motion_handler();
