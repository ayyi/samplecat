#ifndef __samplecat_main_h__
#define __samplecat_main_h__

#include <gtk/gtk.h>
#include <pthread.h>
#include "typedefs.h"
#include "types.h"
#include "utils/mime_type.h"
#include "dir_tree/view_dir_tree.h"
#include "model.h"
#include "application.h"
#ifdef USE_MYSQL
#include "db/mysql.h"
#endif

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
#ifdef USE_MYSQL
	struct _mysql_config mysql;
#endif
	char      auditioner[16];
	char      show_dir[PATH_MAX];
	char      window_width[8];
	char      window_height[8];
	char      colour[PALETTE_SIZE][8];
	//gboolean  add_recursive; ///< TODO save w/ config ?
	//gboolean  loop_playback; ///< TODO save w/ config ?
	char      column_widths[4][8];
	char      browse_dir[PATH_MAX];
	char      show_player[8];
	char      show_waveform[8];
	char      show_spectrogram[8];
	char      jack_autoconnect[1024];
	char      jack_midiconnect[1024];
};


#ifndef __main_c__
extern struct _backend backend;
#endif

enum {
	SHOW_PLAYER = 0,
	SHOW_FILEMANAGER,
	SHOW_WAVEFORM,
	SHOW_SPECTROGRAM,
	MAX_VIEW_OPTIONS
};

struct _app
{
	gboolean       loaded;

	char*          config_filename;
	struct _config config;
	SamplecatModel*model;
	pthread_t      gui_thread;
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
		void             (*on_toggle)(gboolean);
		gboolean         value;
		GtkWidget*       menu_item;
	} view_options[MAX_VIEW_OPTIONS];

	GKeyFile*      key_file;   //config file data.

	GList*         backends;
	GList*         players;

	int            playing_id; ///< database index of the file that is currently playing, or zero if none playing, -1 if playing an external file.

	GtkListStore*  store;
	Inspector*     inspector;
	PlayCtrl*      playercontrol;
	
	GtkWidget*     window;
	GtkWidget*     view;
	GtkWidget*     msg_panel;
	GtkWidget*     statusbar;
	GtkWidget*     statusbar2;
	GtkWidget*     search;
	GtkWidget*     context_menu;
	GtkWidget*     waveform;
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
extern Application* application;
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

gboolean    keyword_is_dupe(char* new, char* existing);

void        on_quit(GtkMenuItem*, gpointer user_data);

void        set_auditioner();

#endif
