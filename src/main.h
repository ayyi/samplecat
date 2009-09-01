char err [32];
char warn[32];

#define EXPAND_TRUE 1
#define EXPAND_FALSE 0
#define FILL_TRUE 1
#define FILL_FALSE 0
#define NON_HOMOGENOUS 0
#define START_EDITING 1

#define PALETTE_SIZE 17

#define POINTER_OK_NULL(A, B, C) if((unsigned)A < 1024){ errprintf("%s(): bad %s pointer (%p).\n", B, C, A); return NULL; }
#ifndef USE_AYYI
#define P_GERR if(error){ errprintf2("%s\n", error->message); g_error_free(error); error = NULL; }
#endif


typedef struct _inspector
{
	unsigned       row_id;
	GtkTreeRowReference* row_ref;
	GtkWidget*     widget;
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
	char      column_widths[4][64];
};


struct _backend
{
	gboolean         (*search_iter_new)  (char* search, char* dir);
	SamplecatResult* (*search_iter_next) (unsigned long**);
	void        (*search_iter_free) ();
	int         (*insert)           (sample*, MIME_type*);
	gboolean    (*update_colour)    (int, int);
	gboolean    (*update_keywords)  (int, const char*);
	gboolean    (*update_notes)     (int, const char*);
};
#ifdef __main_c__
struct _backend backend = {NULL, NULL, NULL, NULL};
#else
extern struct _backend backend;
#endif

#define BACKEND_IS_NULL (backend.search_iter_new == NULL)


struct _app
{
	gboolean       loaded;

	char*          config_filename;
	struct _config config;
	char           search_phrase[256];
	char*          search_dir;
	gchar*         search_category;
	gboolean       add_recursive;
	gboolean       no_gui;
	struct _args {
		char*      search;
		char*      add;
	}              args;

	GKeyFile*      key_file;   //config file data.

	GList*         backends;

	int            playing_id; //database index of the file that is currently playing, or zero if none playing.

	GtkListStore*  store;
	inspector*     inspector;
	
	GtkWidget*     window;
	GtkWidget*     vbox;
	GtkWidget*     toolbar;
	GtkWidget*     toolbar2;
	GtkWidget*     scroll;
	GtkWidget*     hpaned;
	GtkWidget*     view;
	GtkWidget*     msg_panel;
	GtkWidget*     statusbar;
	GtkWidget*     statusbar2;
	GtkWidget*     search;
	GtkWidget*     category;
	GtkWidget*     view_category;
	GtkWidget*     context_menu;

	GtkWidget*     colour_button[PALETTE_SIZE];
	gboolean       colourbox_dirty;

	GtkCellRenderer*     cell1;          //sample name.
	GtkCellRenderer*     cell_tags;
	GtkTreeViewColumn*   col_name;
	GtkTreeViewColumn*   col_path;
	GtkTreeViewColumn*   col_pixbuf;
	GtkTreeViewColumn*   col_tags;

	GtkTreeRowReference* mouseover_row_ref;

	GNode*               dir_tree;
	GtkWidget*           dir_treeview;
	ViewDirTree*         dir_treeview2;
	GtkWidget*           vpaned;        //vertical divider on lhs between the dir_tree and inspector
	GtkWidget*           vpaned2;

	GtkWidget*           fm_view;

	GdkColor             fg_colour;
	GdkColor             bg_colour;
	GdkColor             bg_colour_mod1;
	GdkColor             base_colour;
	GdkColor             text_colour;

	MYSQL                mysql;

	GAsyncQueue*         msg_queue;

	//nasty!
	gint       mouse_x;
	gint       mouse_y;

	GMutex*    mutex;
};

struct _palette {
  guint red[8];
  guint grn[8];
  guint blu[8];
};

enum {
	TYPE_SNDFILE=1,
	TYPE_FLAC,
};

void        on_view_category_changed(GtkComboBox*, gpointer user_data);
void        on_category_changed(GtkComboBox*, gpointer user_data);
gboolean    row_set_tags(GtkTreeIter*, int id, const char* tags_new);
gboolean    row_set_tags_from_id(int id, GtkTreeRowReference* row_ref, const char* tags_new);

void        do_search(char* search, char* dir);

gboolean    new_search(GtkWidget*, gpointer userdata);

void        scan_dir(const char* path, int* added_count);

gboolean    add_file(char* path);
gboolean    add_dir(char* uri);
gboolean    get_file_info(sample*);
gboolean    get_file_info_sndfile(sample* sample);
gboolean    on_overview_done(gpointer sample);

void        db_update_pixbuf(sample*);
void        update_dir_node_list();

void        delete_row(GtkWidget*, gpointer user_data);
void        update_row(GtkWidget*, gpointer user_data);
void        edit_row  (GtkWidget*, gpointer user_data);
void        clear_store();
gboolean    treeview_get_cell(GtkTreeView *view, guint x, guint y, GtkCellRenderer **cell);
gboolean    treeview_get_tags_cell(GtkTreeView *view, guint x, guint y, GtkCellRenderer **cell);

gint        get_mouseover_row();

void        update_search_dir(gchar* uri);
gboolean    dir_tree_update(gpointer data);
void        set_search_dir(char* dir);

void        on_entry_activate(GtkEntry*, gpointer);

gboolean    keyword_is_dupe(char* new, char* existing);

gboolean	on_directory_list_changed();
gboolean    toggle_recursive_add(GtkWidget*, gpointer user_data);

void        on_file_moved(GtkTreeIter);

void        tag_edit_start(int tnum);
void        tag_edit_stop(GtkWidget*, GdkEventCrossing*, gpointer user_data);

gboolean    config_load();
void        config_new();
gboolean    config_save();
void        on_quit(GtkMenuItem *menuitem, gpointer user_data);

void        set_backend(BackendType);

