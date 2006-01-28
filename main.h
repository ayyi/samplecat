char err [32];
char warn[32];

#define EXPAND_TRUE 1
#define EXPAND_FALSE 0
#define FILL_TRUE 1
#define FILL_FALSE 0

//dnd:
enum {
  TARGET_APP_COLLECTION_MEMBER,
  TARGET_URI_LIST,
  TARGET_TEXT_PLAIN
};


struct _app
{
	MYSQL mysql;

	int       playing_id; //database index of the file that is currently playing, or zero if none playing.

	GtkListStore *store;
	
	GtkWidget *window;
	GtkWidget *vbox;
	GtkWidget *scroll;
	GtkWidget *view;
	GtkWidget *statusbar;
	GtkWidget *search;
	GtkWidget *context_menu;

	GtkCellRenderer*   cell1;     //sample name.
	GtkTreeViewColumn* col_pixbuf;

	GdkColor fg_colour;
	GdkColor bg_colour;
};

typedef struct _sample
{
	char        filename[256];
	int         sample_rate;
	int         length;     // milliseconds
	int         channels;

} sample;

gboolean	window_new();
void        window_on_realise(GtkWidget *win, gpointer user_data);
gboolean	filter_new();

gboolean	db_connect();
void        do_search();

gboolean    new_search(GtkWidget *widget, gpointer userdata);

void        scan_dir();

void        dnd_setup();
gint        drag_received(GtkWidget *widget, GdkDragContext *drag_context, gint x, gint y,
                          GtkSelectionData *data, guint info, guint time, gpointer user_data);

void        add_file(char *uri);
GdkPixbuf*  make_overview(sample* sample);

void        keywords_on_edited(GtkCellRendererText *cell, gchar *path_string, gchar *new_text, gpointer user_data);
void        delete_row(GtkWidget *widget, gpointer user_data);
void        update_row(GtkWidget *widget, gpointer user_data);
void        edit_row  (GtkWidget *widget, gpointer user_data);
GtkWidget*  make_context_menu();
gboolean    on_row_clicked(GtkWidget *widget, GdkEventButton *event, gpointer user_data);

