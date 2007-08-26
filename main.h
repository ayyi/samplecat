char err [32];
char warn[32];

#define EXPAND_TRUE 1
#define EXPAND_FALSE 0
#define FILL_TRUE 1
#define FILL_FALSE 0

#define OVERVIEW_WIDTH 150
#define OVERVIEW_HEIGHT 20

#define PALETTE_SIZE 12

#define ASSERT_POINTER(A, B, C) if((unsigned)A < 1024){ errprintf("%s(): bad %s pointer (%p).\n", B, C, A); return; }
#define ASSERT_POINTER_FALSE(A, B, C) if(GPOINTER_TO_UINT(B) < 1024){ errprintf("%s(): bad %s pointer (%p).\n", A, C, B); return FALSE; }
#define POINTER_OK_NULL(A, B, C) if((unsigned)A < 1024){ errprintf("%s(): bad %s pointer (%p).\n", B, C, A); return NULL; }
#define PF printf("%s()...\n", __func__);
#define dbg(A, B, ...) debug_printf(__func__, A, B, __VA_ARGS__)


typedef struct _inspector
{
	unsigned       row_id;
	GtkTreeRowReference* row_ref;
	GtkWidget*     name;
	GtkWidget*     filename;
	GtkWidget*     tags;
	GtkWidget*     tags_ev;    //event box for mouse clicks
	GtkWidget*     length;
	GtkWidget*     samplerate;
	GtkWidget*     channels;
	GtkWidget*     mimetype;
	GtkWidget*     image;
	GtkTextBuffer* notes;
	GtkWidget*     edit;
} inspector;


struct _config
{
	char      database_host[64];
	char      database_user[64];
	char      database_pass[64];
	char      database_name[64];
	char      show_dir[256];
	char      window_width[64];
	char      window_height[64];
	char      colour[PALETTE_SIZE][8];
	gboolean  add_recursive;
};

struct _app
{
	char      home_dir[256];
	char      config_filename[256];
	struct _config config;
	char      search_phrase[256];
	char*     search_dir;
	gchar*    search_category;
	gboolean  add_recursive;

	GKeyFile* key_file;   //config file data.

	int       playing_id; //database index of the file that is currently playing, or zero if none playing.

	GtkListStore *store;
	inspector    *inspector;
	
	GtkWidget *window;
	GtkWidget *vbox;
	GtkWidget *toolbar;
	GtkWidget *toolbar2;
	GtkWidget *scroll;
	GtkWidget *view;
	GtkWidget *statusbar;
	GtkWidget *statusbar2;
	GtkWidget *search;
	GtkWidget *category;
	GtkWidget *view_category;
	GtkWidget *context_menu;

	GtkWidget *colour_button[PALETTE_SIZE];
	gboolean  colourbox_dirty;

	GtkCellRenderer*   cell1;     //sample name.
	GtkCellRenderer*   cell_tags;
	GtkTreeViewColumn* col_pixbuf;
	GtkTreeViewColumn* col_tags;

	GtkTreeRowReference* mouseover_row_ref;

	GNode*     dir_tree;
	GtkWidget* dir_treeview;
	GtkWidget* vpaned;        //vertical divider on lhs between the dir_tree and inspector

	GdkColor   fg_colour;
	GdkColor   bg_colour;
	GdkColor   bg_colour_mod1;
	GdkColor   base_colour;
	GdkColor   text_colour;

	MYSQL mysql;

	//nasty!
	gint     mouse_x;
	gint     mouse_y;

	GMutex*  mutex;
};

typedef struct _sample
{
	int          id;            //database index.
	GtkTreeRowReference* row_ref;
	char         filename[256]; //full path.
	char         filetype;      //see enum.
	unsigned int sample_rate;
	unsigned int length;        //milliseconds
	unsigned int frames;        //total number of frames (eg a frame for 16bit stereo is 4 bytes).
	unsigned int channels;
	int          bitdepth;
	short        minmax[OVERVIEW_WIDTH]; //peak data before it is copied onto the pixbuf.
	GdkPixbuf*   pixbuf;

} sample;

struct _palette {
  guint red[8];
  guint grn[8];
  guint blu[8];
};

enum {
	TYPE_SNDFILE=1,
	TYPE_FLAC,
};

gboolean	window_new();
GtkWidget*  left_pane();
GtkWidget*  inspector_pane();
void        inspector_update(GtkTreePath*);
gboolean    inspector_on_tags_clicked(GtkWidget*, GdkEventButton*, gpointer user_data);
GtkWidget*  colour_box_new(GtkWidget* parent);
void        colour_box_update();
gboolean    colour_box_exists(GdkColor* colour);
gboolean    colour_box_add(GdkColor* colour);
void        window_on_allocate(GtkWidget *win, gpointer user_data);
void        window_on_realise(GtkWidget *win, gpointer user_data);
gboolean    window_on_configure(GtkWidget *widget, GdkEventConfigure *event, gpointer user_data);
void        make_listview();
gboolean	filter_new();
gboolean    tag_selector_new();
gboolean    tagshow_selector_new();
void        on_view_category_changed(GtkComboBox *widget, gpointer user_data);
void        on_category_changed(GtkComboBox *widget, gpointer user_data);
void        on_category_set_clicked(GtkComboBox *widget, gpointer user_data);
gboolean    row_set_tags(GtkTreeIter* iter, int id, const char* tags_new);
gboolean    row_set_tags_from_id(int id, GtkTreeRowReference* row_ref, const char* tags_new);
gboolean    row_clear_tags(GtkTreeIter* iter, int id);


void        do_search(char *search, char *dir);

gboolean    new_search(GtkWidget *widget, gpointer userdata);
void        on_notes_insert(GtkTextView *textview, gchar *arg1, gpointer user_data);
gboolean    on_notes_focus_out(GtkWidget *widget, gpointer userdata);

void        scan_dir();

sample*     sample_new();
sample*     sample_new_from_model(GtkTreePath *path);
void        sample_free(sample* sample);

gboolean    add_file(char *uri);
gboolean    add_dir(char *uri);
gboolean    get_file_info(sample* sample);
gboolean    get_file_info_sndfile(sample* sample);
gboolean    on_overview_done(gpointer sample);

void        db_update_pixbuf(sample *sample);
void        db_get_dirs();

void        keywords_on_edited(GtkCellRendererText *cell, gchar *path_string, gchar *new_text, gpointer user_data);
void        delete_row(GtkWidget*, gpointer user_data);
void        update_row(GtkWidget*, gpointer user_data);
void        edit_row  (GtkWidget*, gpointer user_data);
GtkWidget*  make_context_menu();
gboolean    on_row_clicked(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
gboolean    treeview_on_motion(GtkWidget *widget, GdkEventMotion *event, gpointer user_data);
void        clear_store();
void        treeview_block_motion_handler();
void        treeview_unblock_motion_handler();
gboolean    treeview_get_cell(GtkTreeView *view, guint x, guint y, GtkCellRenderer **cell);
gboolean    treeview_get_tags_cell(GtkTreeView *view, guint x, guint y, GtkCellRenderer **cell);

gint        get_mouseover_row();
void        path_cell_data_func(GtkTreeViewColumn *tree_column, GtkCellRenderer *cell, GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer data);
void        path_cell_bg_lighter(GtkTreeViewColumn *tree_column, GtkCellRenderer *cell, GtkTreeModel *tree_model, GtkTreeIter *iter);
void        tag_cell_data(GtkTreeViewColumn *tree_column, GtkCellRenderer *cell, GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer data);

GtkWidget*  dir_tree_new();
gboolean    dir_tree_on_link_selected(GObject *ignored, DhLink *link, gpointer data);
gboolean    dir_tree_update(gpointer data);
void        set_search_dir(char* dir);

void        on_entry_activate(GtkEntry *entry, gpointer user_data);

gboolean    keyword_is_dupe(char* new, char* existing);

int         colour_drag_dataget(GtkWidget *widget, GdkDragContext *drag_context, GtkSelectionData *data, guint info, guint time, gpointer user_data);
//int         colour_drag_datareceived(GtkWidget *widget, GdkDragContext *drag_context, gint x, gint y, GtkSelectionData *data, guint info, guint time, gpointer user_data);
gint        treeview_drag_received(GtkWidget *widget, GdkDragContext *drag_context, gint x, gint y, GtkSelectionData *data, guint info, guint time, gpointer user_data);
gboolean    item_set_colour(GtkTreePath* path, unsigned colour);

int         path_get_id(GtkTreePath* path);
gboolean	on_directory_list_changed();
gboolean    toggle_recursive_add(GtkWidget *widget, gpointer user_data);

void        tag_edit_start(int tnum);
void        tag_edit_stop(GtkWidget *widget, GdkEventCrossing *event, gpointer user_data);

gboolean    config_load();
void        config_new();
gboolean    config_save();
void        on_quit(GtkMenuItem *menuitem, gpointer user_data);

