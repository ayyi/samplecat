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

	GtkListStore *store;
	
	GtkWidget *window;
	GtkWidget *vbox;
	GtkWidget *statusbar;
	GtkWidget *search;
};

typedef struct _sample
{
	char*       filename;
	int         sample_rate;

} sample;

gboolean	window_new();
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
