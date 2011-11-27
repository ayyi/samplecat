#ifndef __SAMPLECAT_MAIN_H_
#define __SAMPLECAT_MAIN_H_

#include <gtk/gtk.h>
#include <pthread.h>
#include "typedefs.h"
#include "types.h"
#include "utils/mime_type.h"

#define EXPAND_TRUE 1
#define EXPAND_FALSE 0
#define FILL_TRUE 1
#define FILL_FALSE 0
#define NON_HOMOGENOUS 0
#define START_EDITING 1
#define TIMER_STOP FALSE

#define PALETTE_SIZE 17

#define MAX_DISPLAY_ROWS 1000


struct _config
{
	char      database_backend[64];
	char      database_host[64];
	char      database_user[64];
	char      database_pass[64];
	char      database_name[64];
	char      auditioner[16];
	char      show_dir[PATH_MAX];
	char      window_width[8];
	char      window_height[8];
	char      colour[PALETTE_SIZE][8];
	//gboolean  add_recursive; ///< TODO save w/ config ?
	//gboolean  loop_playback; ///< TODO save w/ config ?
	char      column_widths[4][8];
	char      browse_dir[PATH_MAX];
	char      jack_autoconnect[1024];
	char      jack_midiconnect[1024];
};


struct _backend
{
	gboolean         pending;

	gboolean         (*search_iter_new)  (char* search, char* dir, const char* category, int* n_results);
	Sample*          (*search_iter_next) (unsigned long**);
	void             (*search_iter_free) ();

	void             (*dir_iter_new)     ();
	char*            (*dir_iter_next)    ();
	void             (*dir_iter_free)    ();

	int              (*insert)           (Sample*);
	gboolean         (*remove)           (int);
	gboolean         (*file_exists)      (const char*, int *);
	GList *          (*filter_by_audio)  (Sample*);

	gboolean         (*update_string)    (int, const char*, const char*);
	gboolean         (*update_int)       (int, const char*, const long int);
	gboolean         (*update_float)     (int, const char*, const float);
	gboolean         (*update_blob)      (int, const char*, const guint8*, const guint);

	void             (*disconnect)       ();
};
#ifdef __main_c__
struct _backend backend = {false, 
	NULL, NULL, NULL, 
	NULL, NULL, NULL, 
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL};
#else
extern struct _backend backend;
#endif

#define BACKEND_IS_NULL (backend.search_iter_new == NULL)

enum {
	SHOW_FILEMANAGER = 0,
	SHOW_SPECTROGRAM,
	MAX_VIEW_OPTIONS
};

struct _app
{
	gboolean       loaded;

	char*          config_filename;
	const char*    cache_dir;
	struct _config config;
	pthread_t      gui_thread;
	char           search_phrase[256]; // XXX TODO increase to PATH_MAX
	char*          search_dir;
	gchar*         search_category;
	gboolean       add_recursive;
	gboolean       loop_playback;
	Auditioner const* auditioner;
#if (defined HAVE_JACK)
	gboolean       enable_effect;
	gboolean       effect_enabled; // read-only set by jack_player.c
	float          effect_param[3];
	float          playback_speed;
	gboolean       link_speed_pitch;
#endif
	gboolean       no_gui;
	struct _args {
		char*      search;
		char*      add;
	}              args;

	struct _view_option {
		char*            name;
		gboolean         value;
		GtkWidget*       menu_item;
		void             (*on_toggle)(GtkMenuItem*, gpointer);
	} view_options[MAX_VIEW_OPTIONS];

	GKeyFile*      key_file;   //config file data.

	GList*         backends;
	GList*         players;

	int            playing_id; ///< database index of the file that is currently playing, or zero if none playing, -1 if playing an external file.

	GtkListStore*  store;
	Inspector*     inspector;
	PlayCtrl*      playercontrol;
	
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
	GtkWidget*     spectrogram;

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

	GtkWidget*           fm_view;

	GdkColor             fg_colour;
	GdkColor             bg_colour;
	GdkColor             bg_colour_mod1;
	GdkColor             base_colour;
	GdkColor             text_colour;

	GAsyncQueue*         msg_queue;

	//nasty!
	gint       mouse_x;
	gint       mouse_y;
};
#ifndef __main_c__
extern struct _app app;
#endif

struct _palette {
	guint red[8];
	guint grn[8];
	guint blu[8];
};

void        do_search(char* search, char* dir);

gboolean    new_search(GtkWidget*, gpointer userdata);

gboolean    add_file(char* path);
void        add_dir(const char* path, int* added_count);
void        delete_selected_rows();

gboolean    on_overview_done(gpointer sample);
gboolean    on_peaklevel_done(gpointer sample);
gboolean    on_ebur128_done(gpointer _sample);

gboolean    mimestring_is_unsupported(char*);
gboolean    mimetype_is_unsupported(MIME_type*, char* mime_string);

void        update_dir_node_list();

gint        get_mouseover_row();

void        update_search_dir(gchar* uri);

gboolean    keyword_is_dupe(char* new, char* existing);

void        on_quit(GtkMenuItem*, gpointer user_data);

void        set_backend(BackendType);

void        set_auditioner();

void        observer__item_selected(Sample*);
#endif
