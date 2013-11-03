/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://samplecat.orford.org          |
* | copyright (C) 2007-2013 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#ifdef USE_GDL
#include "gdl/gdl-dock-layout.h"
#include "gdl/gdl-dock-bar.h"
#include "gdl/gdl-dock-paned.h"
#endif
#include "debug/debug.h"
#include "file_manager.h"
#include "file_manager/menu.h"
#include "dir_tree/view_dir_tree.h"
#include "gimp/gimpaction.h"
#include "gimp/gimpactiongroup.h"
#include "typedefs.h"
#include "db/db.h"
#include "sample.h"
#include "support.h"
#include "main.h"
#include "listview.h"
#include "dnd.h"
#include "inspector.h"
#include "progress_dialog.h"
#include "player_control.h"
#ifdef USE_OPENGL
#ifdef USE_LIBASS
#include "waveform/view_plus.h"
#else
#include "waveform/view.h"
#endif
#endif
#ifndef NO_USE_DEVHELP_DIRTREE
#include "dh_link.h"
#include "dh_tree.h"
#else
#include "dir_tree.h"
#endif
#ifdef HAVE_FFTW3
#ifdef USE_OPENGL
#include "gl_spectrogram_view.h"
#else
#include "spectrogram_widget.h"
#endif
#endif
#ifdef USE_MYSQL
#include "db/mysql.h"
#endif
#include "colour_box.h"
#include "rotator.h"
#include "window.h"
#include "auditioner.h"
#ifndef __APPLE__
#include "icons/samplecat.xpm"
#endif

extern Filer filer;
extern unsigned debug;

char* categories[] = {"drums", "perc", "bass", "keys", "synth", "strings", "brass", "fx", "impulse", "breaks"};
 
GtkWidget*        view_details_new                ();
void              view_details_dnd_get            (GtkWidget*, GdkDragContext*, GtkSelectionData*, guint info, guint time, gpointer data);

static GtkWidget* _dir_tree_new                   ();

static gboolean   window_on_destroy               (GtkWidget*, gpointer);
static void       window_on_realise               (GtkWidget*, gpointer);
static void       window_on_size_request          (GtkWidget*, GtkRequisition*, gpointer);
static void       window_on_allocate              (GtkWidget*, GtkAllocation*, gpointer);
static gboolean   window_on_configure             (GtkWidget*, GdkEventConfigure*, gpointer);
static GtkWidget* filter_new                      ();
static void       window_on_fileview_row_selected (GtkTreeView*, gpointer);
static void       on_category_set_clicked         (GtkComboBox*, gpointer);
static void       menu__add_to_db                 (GtkMenuItem*, gpointer);
static void       menu__add_dir_to_db             (GtkMenuItem*, gpointer);
static void       menu__play                      (GtkMenuItem*, gpointer);
static void       make_menu_actions               (struct _accel[], int, void (*add_to_menu)(GtkAction*));
static gboolean   on_dir_tree_link_selected       (GObject*, DhLink*, gpointer);
#ifndef USE_GDL
static GtkWidget* message_panel__new              ();
#endif
static GtkWidget* dir_panel_new                   ();
static GtkWidget* left_pane2                      ();
static void       on_layout_changed               ();
static void       update_waveform_view            (Sample*);

static void       k_delete_row                    (GtkAccelGroup*, gpointer);
static void       k_save_dock                     (GtkAccelGroup*, gpointer);

struct _window {
   GtkWidget* vbox;
   GtkWidget* dock;
   GtkWidget* toolbar;
   GtkWidget* toolbar2;
   GtkWidget* category;
   GtkWidget* view_category;
   GtkWidget* file_man;
   GtkWidget* dir_tree;
} window;

struct _accel menu_keys[] = {
	{"Add to database",NULL,        {{(char)'a',    0               },  {0, 0}}, menu__add_to_db, GINT_TO_POINTER(0)},
	{"Play"           ,NULL,        {{(char)'p',    0               },  {0, 0}}, menu__play,      NULL              },
};

struct _accel window_keys[] = {
	{"Quit",           NULL,        {{(char)'q',    GDK_CONTROL_MASK},  {0, 0}}, on_quit,         GINT_TO_POINTER(0)},
	{"Close",          NULL,        {{(char)'w',    GDK_CONTROL_MASK},  {0, 0}}, on_quit,         GINT_TO_POINTER(0)},
	{"Delete",         NULL,        {{GDK_Delete,   0               },  {0, 0}}, k_delete_row,    NULL},
	{"Save Dock",      NULL,        {{(char)'s',    GDK_CONTROL_MASK},  {0, 0}}, k_save_dock,     NULL},
};

struct _accel fm_tree_keys[] = {
    {"Add to database",NULL,        {{(char)'y',    0               },  {0, 0}}, menu__add_dir_to_db,           NULL},
};

static GtkAccelGroup* accel_group = NULL;


const char* preferred_width = "preferred-width";
const char* preferred_height = "preferred-height";

#ifdef USE_GDL
#define PACK(widget, position, width, height, id, name, D) \
	{ \
	dbg(1, "PACK %s", id); \
	GtkWidget* dock_item = gdl_dock_item_new (id, name, GDL_DOCK_ITEM_BEH_LOCKED); \
	gtk_container_add(GTK_CONTAINER(dock_item), widget); \
	g_object_set(dock_item, preferred_width, width, NULL); \
	if(height > 0) g_object_set(dock_item, preferred_height, height, NULL); \
	gdl_dock_add_item(GDL_DOCK(window.dock), GDL_DOCK_ITEM(dock_item), position); \
	}
#else
#define PACK(widget, position, width, height, id, name, D) \
	if(GTK_IS_BOX(D)) \
		gtk_box_pack_start(GTK_BOX(D), widget, EXPAND_FALSE, FILL_FALSE, 0); \
	else \
		if(!gtk_paned_get_child1(GTK_PANED(D))) gtk_paned_add1(GTK_PANED(D), widget); \
		else gtk_paned_add2(GTK_PANED(D), widget);
#endif


#ifdef USE_GDL
#define PACK2(widget, parent, position, id, name, D) \
	{ \
	dbg(1, "PACK2 %s", id); \
	GtkWidget* dock_item = gdl_dock_item_new (id, name, GDL_DOCK_ITEM_BEH_LOCKED); \
	gtk_container_add(GTK_CONTAINER(dock_item), widget); \
	g_object_set((GdlDockItem*)parent, "preferred-width", 400, NULL); \
	g_object_set(dock_item, preferred_width, 210, NULL); \
	gdl_dock_item_dock_to(GDL_DOCK_ITEM(dock_item), parent, position, -1); \
	dbg(1, "PACK2: done"); \
	}
#else
#define PACK2(widget, parent, position, id, name, D) \
	if(GTK_IS_BOX(D)) \
		gtk_box_pack_start(GTK_BOX(D), widget, EXPAND_FALSE, FILL_FALSE, 0); \
	else \
		if(!gtk_paned_get_child1(GTK_PANED(D))) gtk_paned_add1(GTK_PANED(D), widget); \
		else gtk_paned_add2(GTK_PANED(D), widget);
#endif


gboolean
window_new()
{
/*
GtkWindow
+--GtkVbox                        window.vbox
   +--GtkVBox
   |  +--GtkHBox search box
   |  |  +--label
   |  |  +--text entry
   |  |
   |  +--GtkHBox edit metadata
   |
   +--GtkAlignment                align1
   |  +--GtkVPaned main_vpaned
   |     +--GtkHPaned hpaned
   |     |  +--GTkVPaned vpaned (main left pane)
   |     |  |  +--GTkVPaned
   |     |  |  |   +--directory tree
   |     |  |  |   +--player control
   |     |  |  +--inspector
   |     |  | 
   |     |  +--GtkVBox rhs_vbox (main right pane)
   |     |     +--GtkLabel message box
   |     |     +--GtkScrolledWindow scrollwin
   |     |        +--GtkTreeView app->libraryview->widget  (main sample list)
   |     |
   |     +--GtkHPaned fman_hpaned
   |        +--fs_tree
   |        +--scrollwin
   |           +--file view
   |
   +--waveform
   |
   +--spectrogram
   |
   +--statusbar hbox
      +--statusbar
      +--statusbar2

*/
	memset(&window, 0, sizeof(struct _window));

	app->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(app->window), "SampleCat");
	g_signal_connect(G_OBJECT(app->window), "delete_event", G_CALLBACK(on_quit), NULL);
	g_signal_connect(app->window, "destroy", G_CALLBACK(window_on_destroy), NULL);

#ifndef __APPLE__
	gtk_window_set_icon(GTK_WINDOW(app->window), gdk_pixbuf_new_from_xpm_data(samplecat_xpm));
#endif

	window.vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(app->window), window.vbox);

#ifdef USE_GDL
	GtkWidget* dock = window.dock = gdl_dock_new();

	/*GdlDockLayout* layout = */gdl_dock_layout_new(GDL_DOCK(dock));

	GtkWidget* dockbar = gdl_dock_bar_new(GDL_DOCK(dock));
	gdl_dock_bar_set_style(GDL_DOCK_BAR(dockbar), GDL_DOCK_BAR_TEXT);

	gtk_box_pack_start(GTK_BOX(window.vbox), dock, EXPAND_TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(window.vbox), dockbar, EXPAND_FALSE, FALSE, 0);
#endif

	GtkWidget* filter = filter_new();

	//alignment to give top border to main hpane.
	GtkWidget* align1 = gtk_alignment_new(0.0, 0.0, 1.0, 1.0);
	gtk_alignment_set_padding(GTK_ALIGNMENT(align1), 2, 1, 0, 0); //top, bottom, left, right.
#ifndef USE_GDL
	gtk_box_pack_start(GTK_BOX(window.vbox), align1, EXPAND_TRUE, TRUE, 0);
#endif

	//---------

	GtkWidget* main_vpaned = gtk_vpaned_new();
	// default width is too big.
	// let its size be determined by its parent.
	gtk_widget_set_size_request(main_vpaned, 20, 20);
	gtk_container_add(GTK_CONTAINER(align1), main_vpaned);

#ifndef USE_GDL
	GtkWidget* hpaned = gtk_hpaned_new();
	gtk_paned_set_position(GTK_PANED(hpaned), 210);
	gtk_paned_add1(GTK_PANED(main_vpaned), hpaned);

	GtkWidget* left = app->vpaned = gtk_vpaned_new();
	gtk_paned_add1(GTK_PANED(hpaned), left);

	GtkWidget* pcpaned = gtk_vpaned_new();
	gtk_paned_add1(GTK_PANED(app->vpaned), pcpaned);
#endif

	window.dir_tree = dir_panel_new();
	left_pane2();

#ifndef USE_GDL
	GtkWidget* rhs_vbox = gtk_vbox_new(NON_HOMOGENOUS, 0);
	gtk_paned_add2(GTK_PANED(hpaned), rhs_vbox);

	gtk_box_pack_start(GTK_BOX(rhs_vbox), message_panel__new(), EXPAND_FALSE, FILL_FALSE, 0);

	//split the rhs in two:
	GtkWidget* r_vpaned = gtk_vpaned_new();
	gtk_paned_set_position(GTK_PANED(r_vpaned), 300);
	gtk_box_pack_start(GTK_BOX(rhs_vbox), r_vpaned, EXPAND_TRUE, TRUE, 0);
#endif

	listview__new();
	if(0 && BACKEND_IS_NULL) gtk_widget_set_no_show_all(app->libraryview->widget, true); //dont show main view if no database.
	gtk_container_add(GTK_CONTAINER(app->libraryview->scroll), app->libraryview->widget);

#if 0
	GtkWidget* rotator = rotator_new_with_model(GTK_TREE_MODEL(app->store));
	gtk_widget_show(rotator);
	gtk_box_pack_start(GTK_BOX(rhs_vbox), rotator, EXPAND_FALSE, FALSE, 0);
	gtk_widget_set_size_request(rotator, -1, 100);
#endif

	dbg(2, "making fileview pane...");
	void make_fileview_pane()
	{
		GtkWidget* fman_hpaned = window.file_man = gtk_hpaned_new();
		gtk_paned_set_position(GTK_PANED(fman_hpaned), 210);

		void fman_left(const char* initial_folder)
		{
			void dir_on_select(ViewDirTree* vdt, const gchar* path, gpointer data)
			{
				PF;
				filer_change_to(&filer, path, NULL);
			}

			gint expand = TRUE;
			ViewDirTree* dir_list = app->dir_treeview2 = vdtree_new(initial_folder, expand); 
			vdtree_set_select_func(dir_list, dir_on_select, NULL); //callback
			GtkWidget* fs_tree = dir_list->widget;
			gtk_paned_add1(GTK_PANED(fman_hpaned), fs_tree);

			void icon_theme_changed(Application* a, char* theme, gpointer _dir_tree)
			{
				vdtree_on_icon_theme_changed((ViewDirTree*)app->dir_treeview2);
			}
			g_signal_connect((gpointer)app, "icon-theme", G_CALLBACK(icon_theme_changed), dir_list);

			make_menu_actions(fm_tree_keys, G_N_ELEMENTS(fm_tree_keys), vdtree_add_menu_item);
		}

		void fman_right(const char* initial_folder)
		{
			GtkWidget* file_view = app->fm_view = file_manager__new_window(initial_folder);
			gtk_paned_add2(GTK_PANED(fman_hpaned), VIEW_DETAILS(file_view)->scroll_win);
			g_signal_connect(G_OBJECT(file_view), "cursor-changed", G_CALLBACK(window_on_fileview_row_selected), NULL);

			void window_on_dir_changed(GtkWidget* widget, gpointer data)
			{
				PF;
			}
			g_signal_connect(G_OBJECT(file_manager__get_signaller()), "dir_changed", G_CALLBACK(window_on_dir_changed), NULL);

			void icon_theme_changed(Application* a, char* theme, gpointer _dir_tree)
			{
				file_manager__update_all();
			}
			g_signal_connect((gpointer)app, "icon-theme", G_CALLBACK(icon_theme_changed), file_view);

			make_menu_actions(menu_keys, G_N_ELEMENTS(menu_keys), fm__add_menu_item);

			//set up fileview as dnd source:
			gtk_drag_source_set(file_view, GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
					dnd_file_drag_types, dnd_file_drag_types_count,
					GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_ASK);
			g_signal_connect(G_OBJECT(file_view), "drag_data_get", G_CALLBACK(view_details_dnd_get), NULL);
		}

		const char* dir = (app->config.browse_dir && app->config.browse_dir[0] && g_file_test(app->config.browse_dir, G_FILE_TEST_IS_DIR)) ? app->config.browse_dir : g_get_home_dir();
		fman_left(dir);
		fman_right(dir);
	}
	make_fileview_pane();

	int width = atoi(app->config.window_width);
	int height = atoi(app->config.window_height);
	if(width && height) gtk_window_resize(GTK_WINDOW(app->window), width, height);
	// note that the window size is also set in the configure callback.
	// -sometimes setting it here is ignored, sometimes setting it in configure is ignored.
	// -setting the size in configure is actually too late, as the window is drawn first
	//  at the wrong size.
	else{
		// we need an approx size in the case where there is no config file. relative panel sizes will be slightly out.
		width = 450;
		height = 600;
	}

#ifdef USE_GDL
	typedef struct {
		int filters;
		int file;
		int waveform;
	} DefaultHeights;
	DefaultHeights default_heights = {
		60,
		150,
		app->view_options[SHOW_WAVEFORM].value ? 96 : 0,
	};
	int left_column_width = 210;

	int library_height = MAX(100, height - default_heights.filters - default_heights.file - default_heights.waveform);
	dock->requisition.height = 197; // the size must be set directly. using gtk_widget_set_size_request has no imediate effect.
	int allocation = dock->allocation.height;
	dock->allocation.height = 197; // nasty hack
#endif
	PACK(app->libraryview->scroll, GDL_DOCK_TOP, -1, library_height, "library", "Library", r_vpaned);
	// this addition sets the dock to be wider than the left column. the height is set to be small so that the height will be determined by the requisition.
	PACK(filter, GDL_DOCK_TOP, left_column_width + 5, 10, "filters", "Filters", window.vbox);
#ifdef USE_GDL
	dock->allocation.height = allocation;
#else
	gtk_box_reorder_child((GtkBox*)window.vbox, filter, 0);
#endif
	PACK(window.file_man, GDL_DOCK_BOTTOM, 216, default_heights.file, "files", "Files", main_vpaned);

	PACK2(app->inspector->widget, GDL_DOCK_ITEM(gtk_widget_get_parent(app->libraryview->scroll)), GDL_DOCK_LEFT, "inspector", "Inspector", app->vpaned);
	PACK2(window.dir_tree, GDL_DOCK_ITEM(gtk_widget_get_parent(app->inspector->widget)), GDL_DOCK_TOP, "directories", "Directories", pcpaned);

	if (app->auditioner->status && app->auditioner->seek){
		PACK2(app->playercontrol->widget, GDL_DOCK_ITEM(gtk_widget_get_parent(app->inspector->widget)), GDL_DOCK_BOTTOM, "player", "Player", pcpaned);
	}

#ifdef USE_OPENGL
	if(app->view_options[SHOW_WAVEFORM].value){
		show_waveform(true);
	}
#endif

#ifdef HAVE_FFTW3
	if(app->view_options[SHOW_SPECTROGRAM].value){
		show_spectrogram(true);

		//gtk_widget_set_no_show_all(app->spectrogram, true);
	}
#endif

	GtkWidget* hbox_statusbar = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_end(GTK_BOX(window.vbox), hbox_statusbar, EXPAND_FALSE, FALSE, 0);

	GtkWidget* statusbar = app->statusbar = gtk_statusbar_new();
	//gtk_statusbar_set_has_resize_grip(GTK_STATUSBAR(statusbar), TRUE);	//why does give a warning??????
	gtk_statusbar_set_has_resize_grip(GTK_STATUSBAR(statusbar), FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(statusbar), 5);
	gtk_box_pack_start(GTK_BOX(hbox_statusbar), statusbar, EXPAND_TRUE, FILL_TRUE, 0);

	GtkWidget* statusbar2 = app->statusbar2 = gtk_statusbar_new();
	gtk_statusbar_set_has_resize_grip(GTK_STATUSBAR(statusbar2), FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(statusbar2), 5);
	gtk_box_pack_start(GTK_BOX(hbox_statusbar), statusbar2, EXPAND_TRUE, FILL_TRUE, 0);

	g_signal_connect(G_OBJECT(app->window), "realize", G_CALLBACK(window_on_realise), NULL);
	g_signal_connect(G_OBJECT(app->window), "size-request", G_CALLBACK(window_on_size_request), NULL);
	g_signal_connect(G_OBJECT(app->window), "size-allocate", G_CALLBACK(window_on_allocate), NULL);
	g_signal_connect(G_OBJECT(app->window), "configure_event", G_CALLBACK(window_on_configure), NULL);

	GtkAccelGroup* accel_group = gtk_accel_group_new();
	gboolean mnemonics = FALSE;
	GimpActionGroupUpdateFunc update_func = NULL;
	GimpActionGroup* action_group = gimp_action_group_new("Samplecat-window", "Samplecat-window", "gtk-paste", mnemonics, NULL, update_func);
	make_accels(accel_group, action_group, window_keys, G_N_ELEMENTS(window_keys), NULL);
	gtk_window_add_accel_group(GTK_WINDOW(app->window), accel_group);

	gtk_widget_show_all(app->window);

#if (defined HAVE_JACK)
	/* initially hide player seek bar */
	if(app->view_options[SHOW_PLAYER].value){
		show_player(true);
	}
#endif

	void window_on_selection_change(Application* a, Sample* sample, gpointer user_data)
	{
		PF;
#ifdef HAVE_FFTW3
		if(app->spectrogram){
#ifdef USE_OPENGL
			gl_spectrogram_set_file((GlSpectrogram*)app->spectrogram, sample->full_path);
#else
			spectrogram_widget_set_file ((SpectrogramWidget*)app->spectrogram, sample->full_path);
#endif
		}
#endif
#ifdef USE_OPENGL
		if(app->waveform) update_waveform_view(sample);
#endif
	}
	g_signal_connect((gpointer)app, "selection-changed", G_CALLBACK(window_on_selection_change), NULL);

	dnd_setup();

	on_layout_changed();

	return TRUE;
}


static void
window_on_realise(GtkWidget* win, gpointer user_data)
{
	gtk_tree_view_column_set_resizable(app->libraryview->col_name, TRUE);
	gtk_tree_view_column_set_resizable(app->libraryview->col_path, TRUE);
	//gtk_tree_view_column_set_sizing(col1, GTK_TREE_VIEW_COLUMN_FIXED);
}


static void
window_on_size_request(GtkWidget* widget, GtkRequisition* req, gpointer user_data)
{
	req->height = atoi(app->config.window_height);
}


static void
window_on_allocate(GtkWidget *win, GtkAllocation *allocation, gpointer user_data)
{
	#define SCROLLBAR_WIDTH_HACK 32
	static gboolean done = FALSE;
	if(!app->libraryview->widget->requisition.width) return;

	if(!done){
		//dbg(2, "app->libraryview->widget->requisition: wid=%i height=%i", app->libraryview->widget->requisition.width, app->libraryview->widget->requisition.height);
		//setting window size is now done in window_on_configure
		done = TRUE;
	}else{
		//now reduce the minimum size to allow the user to manually make the window smaller.
		gtk_widget_set_size_request(win, 100, 100);
	}

#if 0 
	/* XXX do those really need to be repeated? - style changes ? */
	colour_get_style_bg(&app->bg_colour, GTK_STATE_NORMAL);
	colour_get_style_fg(&app->fg_colour, GTK_STATE_NORMAL);
	colour_get_style_base(&app->base_colour, GTK_STATE_NORMAL);
	colour_get_style_text(&app->text_colour, GTK_STATE_NORMAL);
#endif

	static gboolean did_set_colours = FALSE;
	if (!did_set_colours) {
		did_set_colours=TRUE;

		colour_get_style_bg(&app->bg_colour, GTK_STATE_NORMAL);
		colour_get_style_fg(&app->fg_colour, GTK_STATE_NORMAL);
		colour_get_style_base(&app->base_colour, GTK_STATE_NORMAL);
		colour_get_style_text(&app->text_colour, GTK_STATE_NORMAL);

		hexstring_from_gdkcolor(app->config.colour[0], &app->bg_colour);

		if(app->colourbox_dirty){
			colour_box_colourise();
			app->colourbox_dirty = FALSE;
		}

		//make modifier colours:
		colour_get_style_bg(&app->bg_colour_mod1, GTK_STATE_NORMAL);
		app->bg_colour_mod1.red   = MIN(app->bg_colour_mod1.red   + 0x1000, 0xffff);
		app->bg_colour_mod1.green = MIN(app->bg_colour_mod1.green + 0x1000, 0xffff);
		app->bg_colour_mod1.blue  = MIN(app->bg_colour_mod1.blue  + 0x1000, 0xffff);

		//set column colours:
		dbg(3, "fg_color: %x %x %x", app->fg_colour.red, app->fg_colour.green, app->fg_colour.blue);

		g_object_set(app->libraryview->cells.name, "cell-background-gdk", &app->bg_colour_mod1, "cell-background-set", TRUE, NULL);
		g_object_set(app->libraryview->cells.name, "foreground-gdk", &app->fg_colour, "foreground-set", TRUE, NULL);
#if 0
		if(is_similar(&app->bg_colour_mod1, &app->fg_colour, 0xFF)) perr("colours not set properly!");
#endif
		dbg(2, "%s %s", gdkcolor_get_hexstring(&app->bg_colour_mod1), gdkcolor_get_hexstring(&app->fg_colour));
		if(app->fm_view) view_details_set_alt_colours(VIEW_DETAILS(app->fm_view), &app->bg_colour_mod1, &app->fg_colour);

		colour_box_update();
	}
}


static gboolean
window_on_configure(GtkWidget *widget, GdkEventConfigure *event, gpointer user_data)
{
	static gboolean window_size_set = FALSE;
	if(!window_size_set){
		//take the window size from the config file, or failing that, from the treeview requisition.
		int width = atoi(app->config.window_width);
		if(!width) width = app->libraryview->widget->requisition.width + SCROLLBAR_WIDTH_HACK;
		int window_height = atoi(app->config.window_height);
		if(!window_height) window_height = 480; //MIN(app->libraryview->widget->requisition.height, 900);  -- ignore the treeview height, is meaningless

		if(width && window_height){
			dbg(2, "setting size: width=%i height=%i", width, window_height);
			gtk_window_resize(GTK_WINDOW(app->window), width, window_height);
			window_size_set = TRUE;

			//set the position of the left pane elements.
			//As the allocation is somehow bigger than its container, we just do it v approximately.
/*
			if(app->vpaned && GTK_WIDGET_REALIZED(app->vpaned)){
				//dbg(0, "height=%i %i %i", app->hpaned->allocation.height, app->statusbar->allocation.y, app->inspector->widget->allocation.height);
				guint inspector_y = height - app->hpaned->allocation.y - 210;
				gtk_paned_set_position(GTK_PANED(app->vpaned), inspector_y);
			}
*/
		}
	}
	return FALSE;
}


static gboolean
window_on_destroy(GtkWidget* widget, gpointer user_data)
{
	return FALSE;
}


static GtkWidget*
dir_panel_new()
{
	GtkWidget* widget = NULL;

	if(!BACKEND_IS_NULL){
#ifndef NO_USE_DEVHELP_DIRTREE
		GtkWidget* tree = _dir_tree_new();
		widget = window.dir_tree = scrolled_window_new();
		gtk_container_add((GtkContainer*)window.dir_tree, tree);
		g_signal_connect(tree, "link-selected", G_CALLBACK(on_dir_tree_link_selected), NULL);
#else
		GtkWidget* tree = _dir_tree_new();
		widget = scrolled_window_new();
		gtk_container_add((GtkContainer*)widget, tree);
#endif
	}

	//alternative dir tree:
#ifdef USE_NICE_GQVIEW_CLIST_TREE
	if(false){
		ViewDirList* dir_list = vdlist_new(app->home_dir);
		GtkWidget* tree = dir_list->widget;
	}
	gint expand = TRUE;
	ViewDirTree* dir_list = vdtree_new(app->home_dir, expand);
	widget = dir_list->widget;
#endif

	return widget;
}


static GtkWidget*
left_pane2()
{
	/*
	void on_inspector_allocate(GtkWidget* widget, GtkAllocation* allocation, gpointer user_data)
	{
		dbg(0, "req=%i allocation=%i", widget->requisition.height, allocation->height);
		int tot_height = app->vpaned->allocation.height;
		if(allocation->height > widget->requisition.height){
			gtk_paned_set_position(GTK_PANED(app->vpaned), tot_height - widget->requisition.height);
		}
		//increase size:
		if(allocation->height < widget->requisition.height){
			if(allocation->height < tot_height / 2){
				gtk_paned_set_position(GTK_PANED(app->vpaned), MAX(tot_height / 2, tot_height - widget->requisition.height));
			}
		}
	}

	*/

	player_control_new();
	if(!app->view_options[SHOW_PLAYER].value)
		gtk_widget_set_no_show_all(app->playercontrol->widget, true);

	inspector_new();
	//g_signal_connect(app->inspector->widget, "size-allocate", (gpointer)on_inspector_allocate, NULL);

#ifndef USE_GDL
	void on_vpaned_allocate(GtkWidget* widget, GtkAllocation* vp_allocation, gpointer user_data)
	{
		static int previous_height = 0;
		if(!app->inspector || vp_allocation->height == previous_height) return;

		int inspector_requisition = 240;//app->inspector->vbox->requisition.height; //FIXME
		//dbg(0, "req=%i", inspector_requisition);
		if(!inspector_requisition) return;

		if(!app->inspector->user_height){
			//user has not specified a height so we have free reign

			int tot_height = vp_allocation->height;
			dbg(1, "req=%i tot_allocation=%i %i", inspector_requisition, tot_height, app->inspector->widget->allocation.height);

			//small window - dont allow the inspector to take up more than half the space.
			if(vp_allocation->height < inspector_requisition){
				gtk_paned_set_position(GTK_PANED(app->vpaned), MAX(tot_height / 2, tot_height - inspector_requisition));
			}

			//large window - dont allow the inspector to be too big
			int current_insp_height = tot_height - gtk_paned_get_position(GTK_PANED(app->vpaned));
			if(current_insp_height > inspector_requisition){
				gtk_paned_set_position(GTK_PANED(app->vpaned), tot_height - inspector_requisition);
			}
		}
		previous_height = vp_allocation->height;
	}

	g_signal_connect(app->vpaned, "size-allocate", (gpointer)on_vpaned_allocate, NULL);
#endif
	return NULL;
}


static GtkWidget*
filter_new()
{
	//search box and tagging
	PF;

	g_return_val_if_fail(app->window, FALSE);

	GtkWidget* filter_vbox = gtk_vbox_new(NON_HOMOGENOUS, 0);

	//----------------------------------------------------------------------

	// first row

	GtkWidget* row1 = window.toolbar = gtk_hbox_new(NON_HOMOGENOUS, 0);
	gtk_box_pack_start(GTK_BOX(filter_vbox), row1, EXPAND_FALSE, FILL_FALSE, 0);

	GtkWidget* label = gtk_label_new("Search");
	gtk_misc_set_padding(GTK_MISC(label), 5, 5);
	gtk_box_pack_start(GTK_BOX(row1), label, FALSE, FALSE, 0);

	gboolean on_focus_out(GtkWidget* widget, GdkEventFocus* event, gpointer user_data)
	{
		PF;
		const gchar* text = gtk_entry_get_text(GTK_ENTRY(app->search));
		if(strcmp(text, app->model->filters.phrase)){
			strncpy(app->model->filters.phrase, text, 255);
			do_search(app->model->filters.phrase, app->model->filters.dir);
		}
		return NOT_HANDLED;
	}

	GtkWidget* entry = app->search = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(entry), 64);
	gtk_entry_set_text(GTK_ENTRY(entry), app->model->filters.phrase);
	gtk_box_pack_start(GTK_BOX(row1), entry, EXPAND_TRUE, TRUE, 0);
	g_signal_connect(G_OBJECT(entry), "focus-out-event", G_CALLBACK(on_focus_out), NULL);
	gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
	g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(on_focus_out), NULL);

	//----------------------------------------------------------------------

	//second row (metadata edit)

	GtkWidget* hbox_edit = window.toolbar2 = gtk_hbox_new(NON_HOMOGENOUS, 0);
	gtk_box_pack_start(GTK_BOX(filter_vbox), hbox_edit, EXPAND_FALSE, FILL_FALSE, 0);

	//left align the label:
	GtkWidget* align1 = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
	gtk_box_pack_start(GTK_BOX(hbox_edit), align1, EXPAND_FALSE, FILL_FALSE, 0);

	GtkWidget* label2 = gtk_label_new("Tag");
	gtk_misc_set_padding(GTK_MISC(label2), 5, 5);
	gtk_container_add(GTK_CONTAINER(align1), label2);

	//make the two lhs labels the same width:
	GtkSizeGroup* size_group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	gtk_size_group_add_widget(size_group, label);
	gtk_size_group_add_widget(size_group, align1);
	
	colour_box_new(hbox_edit);

	tagshow_selector_new();
	tag_selector_new();

	return filter_vbox;
}


GtkWidget*
message_panel__add_msg(const gchar* msg, const gchar* stock_id)
{
	//TODO expire old messages. Limit to 5 and add close button?

	GtkWidget* hbox = gtk_hbox_new(FALSE, 2);

	if(stock_id){
		//const gchar* stock_id = GTK_STOCK_DIALOG_WARNING;
		GtkWidget* icon = gtk_image_new_from_stock(stock_id, GTK_ICON_SIZE_MENU);
		gtk_widget_set_size_request(icon, 16, -1);
		//gtk_image_set_from_stock(GTK_IMAGE(icon), stock_id, GTK_ICON_SIZE_MENU);
		gtk_box_pack_start((GtkBox*)hbox, icon, FALSE, FALSE, 2);
	}

	GtkWidget* label = gtk_label_new(msg);
	gtk_box_pack_start((GtkBox*)hbox, label, FALSE, FALSE, 2);

	gtk_box_pack_start((GtkBox*)app->msg_panel, hbox, FALSE, FALSE, 2);
	return hbox;
}


#ifndef USE_GDL
static GtkWidget*
message_panel__new()
{
	PF;
	GtkWidget* vbox = app->msg_panel = gtk_vbox_new(FALSE, 2);

#if 0
#ifndef USE_TRACKER
	char* msg = db__is_connected() ? "" : "no database available";
#else
	char* msg = "";
#endif
	GtkWidget* hbox = message_panel__add_msg(msg, GTK_STOCK_INFO);
	gtk_box_pack_start((GtkBox*)vbox, hbox, FALSE, FALSE, 2);
#endif

	if(!BACKEND_IS_NULL) gtk_widget_set_no_show_all(app->msg_panel, true); //initially hidden.
	return vbox;
}
#endif


static GtkWidget*
_dir_tree_new()
{
	//data:
	update_dir_node_list();

	//view:
#ifndef NO_USE_DEVHELP_DIRTREE
	app->dir_treeview = dh_book_tree_new(&app->dir_tree);
#else
	app->dir_treeview = dir_tree_new(&app->dir_tree);
#endif

	return app->dir_treeview;
}


gboolean
tag_selector_new()
{
	//the tag _edit_ selector

	GtkWidget* combo2 = window.category = gtk_combo_box_entry_new_text();
	GtkComboBox* combo_ = GTK_COMBO_BOX(combo2);
	gtk_combo_box_append_text(combo_, "no categories");
	int i; for (i=0;i<G_N_ELEMENTS(categories);i++) {
		gtk_combo_box_append_text(combo_, categories[i]);
	}
	gtk_box_pack_start(GTK_BOX(window.toolbar2), combo2, EXPAND_FALSE, FALSE, 0);

	//"set" button:
	GtkWidget* set = gtk_button_new_with_label("Set Tag");
	gtk_box_pack_start(GTK_BOX(window.toolbar2), set, EXPAND_FALSE, FALSE, 0);
	g_signal_connect(set, "clicked", G_CALLBACK(on_category_set_clicked), NULL);

	return TRUE;
}


gboolean
tagshow_selector_new()
{
	//the view-filter tag-select.

	#define ALL_CATEGORIES "all categories"

	GtkWidget* combo = window.view_category = gtk_combo_box_new_text();
	GtkComboBox* combo_ = GTK_COMBO_BOX(combo);
	gtk_combo_box_append_text(combo_, ALL_CATEGORIES);
	int i; for(i=0;i<G_N_ELEMENTS(categories);i++){
		gtk_combo_box_append_text(combo_, categories[i]);
	}
	gtk_combo_box_set_active(combo_, 0);
	gtk_box_pack_start(GTK_BOX(window.toolbar), combo, EXPAND_FALSE, FALSE, 0);

	void
	on_view_category_changed(GtkComboBox *widget, gpointer user_data)
	{
		//update the sample list with the new view-category.
		PF;

		if (app->model->filters.category){ g_free(app->model->filters.category); app->model->filters.category = NULL; }
		char* category = gtk_combo_box_get_active_text(GTK_COMBO_BOX(window.view_category));
		if (strcmp(category, ALL_CATEGORIES)){
			app->model->filters.category = category;
		}
		else g_free(category);

		do_search(app->model->filters.phrase, app->model->filters.dir);
	}
	g_signal_connect(combo, "changed", G_CALLBACK(on_view_category_changed), NULL);

	return TRUE;
}


static void
window_on_fileview_row_selected(GtkTreeView* treeview, gpointer user_data)
{
	//a filesystem file has been clicked on.
	PF;

	Filer* filer = file_manager__get();

	gchar* full_path = NULL;
	DirItem* item;
	ViewIter iter;
	view_get_iter(filer->view, &iter, 0);
	while((item = iter.next(&iter))){
		if(view_get_selected(filer->view, &iter)){
			full_path = g_build_filename(filer->real_path, item->leafname, NULL);
			break;
		}
	}
	if(!full_path) return;

	dbg(1, "%s", full_path);

	/* TODO: do nothing if directory selected 
	 * 
	 * this happens when a dir is selected in the left tree-browser
	 * while some file was previously selected in the right file-list
	 * -> we get the new dir + old filename
	 *
	 * event-handling in window.c should use 
	 *   gtk_tree_selection_set_select_function()
	 * or block file-list events during updates after the
	 * dir-tree brower selection changed.
	 */

	Sample* s = sample_new_from_filename(full_path, true);
	if(s){
		g_signal_emit_by_name (app, "selection-changed", s, NULL);
		sample_unref(s);
	}
}


static void
menu__add_to_db(GtkMenuItem* menuitem, gpointer user_data)
{
	PF;

	DirItem* item;
	ViewIter iter;
	view_get_iter(filer.view, &iter, 0);
	while((item = iter.next(&iter))){
		if(view_get_selected(filer.view, &iter)){
			gchar* filepath = g_build_filename(filer.real_path, item->leafname, NULL);
			if(do_progress(0, 0)) break;
			add_file(filepath);
			g_free(filepath);
		}
	}
	hide_progress();
}


static void
menu__add_dir_to_db(GtkMenuItem* menuitem, gpointer user_data)
{
	PF;
	const char* path = vdtree_get_selected(app->dir_treeview2);
	dbg(1, "path=%s", path);
	if(path){
		int n_added = 0;
		add_dir(path, &n_added);
		hide_progress();
	}
}


void
menu__play(GtkMenuItem* menuitem, gpointer user_data)
{
	PF;
	GList* selected = fm_selected_items(&filer);
	GList* l = selected;
	for(;l;l=l->next){
		char* item = l->data;
		dbg(1, "%s", item);
		app->auditioner->play_path(item);
		g_free(item);
	}
	g_list_free(selected);
}


static void
make_menu_actions(struct _accel keys[], int count, void (*add_to_menu)(GtkAction*))
{
	// take the raw definitions and create actions and (optionally) menu items for them.

	GtkActionGroup* group = gtk_action_group_new("File Manager");
	accel_group = gtk_accel_group_new();

	int k;
	for(k=0;k<count;k++){
		struct _accel* key = &keys[k];

    	GtkAction* action = gtk_action_new(key->name, key->name, "Tooltip", key->stock_item? key->stock_item->stock_id : "gtk-file");
  		gtk_action_group_add_action(GTK_ACTION_GROUP(group), action);

    	GClosure* closure = g_cclosure_new(G_CALLBACK(key->callback), key->user_data, NULL);
		g_signal_connect_closure(G_OBJECT(action), "activate", closure, FALSE);
		//dbg(0, "callback=%p", closure->callback);
		gchar path[64]; sprintf(path, "<%s>/Categ/%s", gtk_action_group_get_name(GTK_ACTION_GROUP(group)), key->name);
		//gtk_accel_group_connect(accel_group, key->key[0].code, key->key[0].mask, GTK_ACCEL_MASK, closure);
		gtk_accel_group_connect_by_path(accel_group, path, closure);
		gtk_accel_map_add_entry(path, key->key[0].code, key->key[0].mask);
		gtk_action_set_accel_path(action, path);
		gtk_action_set_accel_group(action, accel_group);

		if(add_to_menu) add_to_menu(action);
	}
}


static gboolean
on_dir_tree_link_selected(GObject *ignored, DhLink *link, gpointer data)
{
	g_return_val_if_fail(link, false);

	dbg(1, "dir=%s", link->uri);

	samplecat_model_set_search_dir (app->model, link->uri);

	const gchar* text = app->search ? gtk_entry_get_text(GTK_ENTRY(app->search)) : "";
	do_search((gchar*)text, link->uri);

	return FALSE;
}


static void
on_category_set_clicked(GtkComboBox *widget, gpointer user_data)
{
	// add selected category to selected samples.

	PF;
	//selected category?
	gchar* category = gtk_combo_box_get_active_text(GTK_COMBO_BOX(window.category));

	GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(app->libraryview->widget));
	GList* selectionlist = gtk_tree_selection_get_selected_rows(selection, NULL);
	if(!selectionlist){ dbg(0, "no files selected."); statusbar_print(1, "no files selected."); return; }

	int i;
	GtkTreeIter iter;
	for(i=0;i<g_list_length(selectionlist);i++){
		GtkTreePath *treepath_selection = g_list_nth_data(selectionlist, i);

		if(gtk_tree_model_get_iter(GTK_TREE_MODEL(app->store), &iter, treepath_selection)){
			gchar* fname; gchar* tags;
			int id;
			gtk_tree_model_get(GTK_TREE_MODEL(app->store), &iter, COL_NAME, &fname, COL_KEYWORDS, &tags, COL_IDX, &id, -1);
			dbg(1, "id=%i name=%s", id, fname);

			if(!strcmp(category, "no categories"))
				listmodel__update_by_tree_iter(&iter, COL_KEYWORDS, "");
			else{

				if(!keyword_is_dupe(category, tags)){
					char tags_new[1024];
					snprintf(tags_new, 1024, "%s %s", tags ? tags : "", category);
					g_strstrip(tags_new);//trim

					listmodel__update_by_tree_iter(&iter, COL_KEYWORDS, (void*)tags_new);
				}else{
					dbg(1, "keyword is a dupe - not applying.");
					statusbar_print(1, "ignoring duplicate keyword.");
				}
			}

		} else perr("bad iter! i=%i (<%i)\n", i, g_list_length(selectionlist));
	}
	g_list_foreach(selectionlist, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(selectionlist);

	g_free(category);
}


#ifdef USE_OPENGL
void
show_waveform(gboolean enable)
{
	if(enable && !app->waveform){
		extern GdkGLContext* window_get_gl_context();
#ifdef USE_LIBASS
		waveform_view_plus_set_gl(window_get_gl_context());
		app->waveform = (GtkWidget*)waveform_view_plus_new(NULL);
#else
		waveform_view_set_gl(window_get_gl_context());
		app->waveform = (GtkWidget*)waveform_view_new(NULL);
#endif
		//gtk_box_pack_start(GTK_BOX(window.vbox), app->waveform, EXPAND_FALSE, FILL_TRUE, 0);
		//TODO this used FILL_TRUE but PACK2 uses FILL_FALSE.
		PACK2(app->waveform, GDL_DOCK_ITEM(gtk_widget_get_parent(window.file_man)), GDL_DOCK_BOTTOM, "waveform", "Waveform", window.vbox);
		gtk_widget_set_size_request(app->waveform, 100, 96);
	}

	if(app->waveform){
#ifdef USE_GDL
		show_widget_if((GtkWidget*)gdl_dock_get_item_by_name(GDL_DOCK(window.dock), "waveform"), enable);
#else
		show_widget_if(app->waveform, enable);
#endif
		app->inspector->show_waveform = !enable;
		if(enable){
			bool show_wave()
			{
				Sample* s;
				if((s = listview__get_first_selected_sample())){
					update_waveform_view(s);
					sample_unref(s);
				}
				return IDLE_STOP;
			}
			g_idle_add(show_wave, NULL);
		}
	}
}
#endif


#ifdef HAVE_FFTW3
void
show_spectrogram(gboolean enable)
{
	if(enable && !app->spectrogram){
#ifdef USE_OPENGL
		app->spectrogram = (GtkWidget*)gl_spectrogram_new();
		gtk_widget_set_size_request(app->spectrogram, 100, 100);
#else
		app->spectrogram = (GtkWidget*)spectrogram_widget_new();
#endif
		gtk_box_pack_start(GTK_BOX(window.vbox), app->spectrogram, EXPAND_TRUE, FILL_TRUE, 0);

		gchar* filename = listview__get_first_selected_filepath();
		if(filename){
			dbg(1, "file=%s", filename);
#ifdef USE_OPENGL
			gl_spectrogram_set_file((GlSpectrogram*)app->spectrogram, filename);
#else
			spectrogram_widget_set_file((SpectrogramWidget*)app->spectrogram, filename);
#endif
			g_free(filename);
		}
	}

	if(app->spectrogram){
		show_widget_if(app->spectrogram, enable);
	}

	on_layout_changed();
}
#endif


void
show_filemanager(gboolean enable)
{
#ifdef USE_GDL
	// It is not clear if GDlDock fully supports hiding of dock items, but it appears to work...

#if 1
	show_widget_if((GtkWidget*)gdl_dock_get_item_by_name(GDL_DOCK(window.dock), "files"), enable);
#else
	GdlDockItem* get_dock_parent(GdlDockItem* item)
	{
		GType parent_type;
		GtkWidget* parent = (GtkWidget*)item;
		do {
			parent = gtk_widget_get_parent(parent);
			parent_type = G_OBJECT_CLASS_TYPE(G_OBJECT_GET_CLASS(parent));
			dbg(0, "  type=%s", G_OBJECT_CLASS_NAME(G_OBJECT_GET_CLASS(parent)));
		} while(parent && (parent_type == GTK_TYPE_VPANED || parent_type == GTK_TYPE_HPANED/* || parent_type == GDL_TYPE_DOCK_PANED*/));

		return (GdlDockItem*)parent;
	}

	static GdlDockItem* dock_item = NULL;
	static GdlDockItem* parent = NULL;
	if(enable){
		if(parent){
			gdl_dock_item_dock_to(dock_item, parent, GDL_DOCK_BOTTOM, -1);
		}else{
			gdl_dock_add_item(GDL_DOCK(window.dock), dock_item, GDL_DOCK_BOTTOM);
		}
	}else{
		dock_item = gdl_dock_get_item_by_name(GDL_DOCK(window.dock), "files");
		parent = get_dock_parent(dock_item);
		gdl_dock_object_detach(GDL_DOCK_OBJECT(dock_item), false);
	}
#endif

#else
	show_widget_if(app->fm_view, enable);
	show_widget_if(app->dir_treeview2->widget, enable);
#endif
}


void
show_player(gboolean enable)
{
#ifdef USE_GDL
	show_widget_if((GtkWidget*)gdl_dock_get_item_by_name(GDL_DOCK(window.dock), "player"), enable);
#else
	show_widget_if(app->playercontrol->widget, enable);
#endif

	player_control_on_show_hide(enable);
}


static void update_waveform_view(Sample* sample)
{
#ifdef USE_LIBASS
	waveform_view_plus_load_file((WaveformViewPlus*)app->waveform, sample->online ? sample->full_path : NULL);
	dbg(1, "name=%s", sample->sample_name);

	char* ch_str = channels_format(sample->channels);
	char* level  = gain2dbstring(sample->peaklevel);
	char length[64]; smpte_format(length, sample->length);
	char fs_str[32]; samplerate_format(fs_str, sample->sample_rate); strcpy(fs_str + strlen(fs_str), " kHz");

	char text[128];
	snprintf(text, 128, "%s  %s  %s  %s", length, ch_str, fs_str, level);
	text[127] = '\0';

	waveform_view_plus_set_colour((WaveformViewPlus*)app->waveform, 0xaaaaaaff, 0xf00000ff, 0x000000bb, 0xffffffbb);
	waveform_view_plus_set_title((WaveformViewPlus*)app->waveform, sample->sample_name);
	waveform_view_plus_set_text((WaveformViewPlus*)app->waveform, text);

	g_free(ch_str);
	g_free(level);
#else
	waveform_view_load_file((WaveformView*)app->waveform, sample->online ? sample->full_path : NULL);
#endif
}


static void
on_layout_changed()
{
	//what is the height of the inspector?

#ifndef USE_GDL
	if(app->inspector){
		GtkWidget* widget;
		if((widget = app->inspector->widget)){
			int tot_height = app->vpaned->allocation.height;
			int max_auto_height = tot_height / 2;
			dbg(1, "inspector_height=%i tot=%i", widget->allocation.height, tot_height);
			if(widget->allocation.height < app->inspector->min_height
					&& widget->allocation.height < max_auto_height){
				int inspector_height = MIN(max_auto_height, app->inspector->min_height);
				dbg(1, "setting height : %i/%i", tot_height - inspector_height, inspector_height);
				gtk_paned_set_position(GTK_PANED(app->vpaned), tot_height - inspector_height);
			}
		}
	}
#endif

	/* scroll to current dir in tree-view bottom left */
	if(app->dir_treeview2) vdtree_set_path(app->dir_treeview2, app->dir_treeview2->path);
}


static void
k_delete_row(GtkAccelGroup* _, gpointer user_data)
{
	delete_selected_rows();
}


static void
k_save_dock(GtkAccelGroup* _, gpointer user_data)
{
	PF0;
}


#ifdef USE_OPENGL
/*   Returns a global GdkGLContext that can be used to share
 *   OpenGL display lists between multiple drawables with
 *   dynamic lifetimes.
 */
#include <gtk/gtkgl.h>
GdkGLContext*
window_get_gl_context()
{
	static GdkGLContext* share_list = NULL;

	if(!share_list){
		GdkGLConfig* const config = gdk_gl_config_new_by_mode(GDK_GL_MODE_RGBA | GDK_GL_MODE_DOUBLE | GDK_GL_MODE_DEPTH);
		GdkPixmap* const pixmap = gdk_pixmap_new(0, 8, 8, gdk_gl_config_get_depth(config));
		gdk_pixmap_set_gl_capability(pixmap, config, 0);
		share_list = gdk_gl_context_new(gdk_pixmap_get_gl_drawable(pixmap), 0, true, GDK_GL_RGBA_TYPE);
	}

	return share_list;
}
#endif


