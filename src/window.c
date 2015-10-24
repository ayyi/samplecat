/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2007-2015 Tim Orford <tim@orford.org>                  |
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
#include "samplecat/worker.h"
#include "gimp/gimpaction.h"
#include "gimp/gimpactiongroup.h"
#include "typedefs.h"
#include "db/db.h"
#include "sample.h"
#include "support.h"
#include "model.h"
#include "application.h"
#include "listview.h"
#include "dnd.h"
#include "inspector.h"
#include "progress_dialog.h"
#include "player_control.h"
#ifdef USE_OPENGL
#include <GL/gl.h>
#include "agl/actor.h"
#ifdef USE_LIBASS
#include "waveform/view_plus.h"
#else
#include "waveform/view.h"
#endif
#endif
#ifdef HAVE_FFTW3
#ifdef USE_OPENGL
#include "gl_spectrogram_view.h"
#else
#include "spectrogram_widget.h"
#endif
#endif
#include "colour_box.h"
#include "rotator.h"
#include "window.h"
#ifndef __APPLE__
#include "icons/samplecat.xpm"
#endif

#include "../layouts/layouts.c"

extern Filer      filer;
extern GList*     themes; 
extern SamplecatBackend backend;
 
extern void       view_details_dnd_get            (GtkWidget*, GdkDragContext*, GtkSelectionData*, guint info, guint time, gpointer data);
extern void       on_quit                         (GtkMenuItem*, gpointer);

char* categories[] = {"drums", "perc", "bass", "keys", "synth", "strings", "brass", "fx", "impulse", "breaks"};

static gboolean   window_on_destroy               (GtkWidget*, gpointer);
static void       window_on_realise               (GtkWidget*, gpointer);
static void       window_on_size_request          (GtkWidget*, GtkRequisition*, gpointer);
static void       window_on_allocate              (GtkWidget*, GtkAllocation*, gpointer);
static gboolean   window_on_configure             (GtkWidget*, GdkEventConfigure*, gpointer);
static gboolean   tag_selector_new                ();
static void       window_on_fileview_row_selected (GtkTreeView*, gpointer);
static void       delete_selected_rows            ();
static void       on_category_set_clicked         (GtkComboBox*, gpointer);
static void       menu__add_to_db                 (GtkMenuItem*, gpointer);
static void       menu__add_dir_to_db             (GtkMenuItem*, gpointer);
static void       menu__play                      (GtkMenuItem*, gpointer);
static void       make_menu_actions               (struct _accel[], int, void (*add_to_menu)(GtkAction*));
#ifndef USE_GDL
static GtkWidget* message_panel__new              ();
#endif
#ifndef USE_GDL
static void       left_pane2                      ();
#endif
static void       window_on_layout_changed        ();
static void       update_waveform_view            (Sample*);

static void       k_delete_row                    (GtkAccelGroup*, gpointer);
#ifdef USE_GDL
static void       k_show_layout_manager           (GtkAccelGroup*, gpointer);
static void       window_load_layout              ();
static void       window_save_layout              ();
#endif
static GtkWidget* make_context_menu               ();

struct _window {
   GtkWidget*     vbox;
   GtkWidget*     dock;
#ifdef USE_GDL
   GdlDockLayout* layout;
#endif
   GtkWidget*     toolbar;
   GtkWidget*     toolbar2;
   GtkWidget*     search;
   GtkWidget*     category;
   GtkWidget*     file_man;
   GtkWidget*     dir_tree;
   GtkWidget*     waveform;
   GtkWidget*     spectrogram;
#ifndef USE_GDL
   GtkWidget*     vpaned;        //vertical divider on lhs between the dir_tree and inspector
#endif
   struct {
      AGlActor*   spp;
   }              layers;

} window;

typedef enum {
   PANEL_TYPE_LIBRARY,
   PANEL_TYPE_SEARCH,
   PANEL_TYPE_FILTERS,
   PANEL_TYPE_INSPECTOR,
   PANEL_TYPE_DIRECTORIES,
   PANEL_TYPE_FILEMANAGER,
   PANEL_TYPE_PLAYER,
#ifdef USE_OPENGL
   PANEL_TYPE_WAVEFORM,
#endif
#ifdef HAVE_FFTW3
   PANEL_TYPE_SPECTROGRAM,
#endif
   PANEL_TYPE_MAX
} PanelType;

typedef GtkWidget* (NewPanelFn)();

static NewPanelFn
#ifdef USE_OPENGL
	waveform_panel_new,
#endif
	spectrogram_new, search_new, filters_new, make_fileview_pane;
extern NewPanelFn dir_panel_new;

typedef struct {
   char        name[16];
   NewPanelFn* new;
   GtkWidget*  widget;
   GtkWidget*  dock_item;

   GtkWidget*  menu_item;
   gulong      handler;
} Panel_;

Panel_ panels[] = {
   {"Library",     listview__new},
   {"Search",      search_new},
   {"Filters",     filters_new},
   {"Inspector",   inspector_new},
   {"Directories", dir_panel_new},
   {"Filemanager", make_fileview_pane},
   {"Player",      player_control_new},
#ifdef USE_OPENGL
   {"Waveform",    waveform_panel_new},
#endif
#ifdef HAVE_FFTW3
   {"Spectrogram", spectrogram_new},
#endif
};

#ifdef USE_GDL
static Panel_*    panel_lookup                    (GdlDockObject*);
static int        panel_lookup_index              (GdlDockObject*);
#endif

Accel menu_keys[] = {
	{"Add to database",NULL,        {{(char)'a',    0               },  {0, 0}}, menu__add_to_db,       GINT_TO_POINTER(0)},
	{"Play"           ,NULL,        {{(char)'p',    0               },  {0, 0}}, menu__play,            NULL              },
};

Accel window_keys[] = {
	{"Quit",           NULL,        {{(char)'q',    GDK_CONTROL_MASK},  {0, 0}}, on_quit,               GINT_TO_POINTER(0)},
	{"Close",          NULL,        {{(char)'w',    GDK_CONTROL_MASK},  {0, 0}}, on_quit,               GINT_TO_POINTER(0)},
	{"Delete",         NULL,        {{GDK_Delete,   0               },  {0, 0}}, k_delete_row,          NULL},
#ifdef USE_GDL
	{"Layout Manager", NULL,        {{(char)'l',    GDK_CONTROL_MASK},  {0, 0}}, k_show_layout_manager, NULL},
#endif
};

Accel fm_tree_keys[] = {
    {"Add to database",NULL,        {{(char)'y',    0               },  {0, 0}}, menu__add_dir_to_db,   NULL},
};

static GtkAccelGroup* accel_group = NULL;

const char* preferred_width = "preferred-width";
const char* preferred_height = "preferred-height";

#ifndef USE_GDL
#define PACK(widget, position, width, height, id, name, D) \
	if(GTK_IS_BOX(D)) \
		gtk_box_pack_start(GTK_BOX(D), widget, EXPAND_FALSE, FILL_FALSE, 0); \
	else \
		if(!gtk_paned_get_child1(GTK_PANED(D))) gtk_paned_add1(GTK_PANED(D), widget); \
		else gtk_paned_add2(GTK_PANED(D), widget);
#endif


gboolean
window_new()
{
/* Widget hierarchy for non-gdl mode:

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

	window.vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(app->window), window.vbox);

#ifndef USE_GDL
	GtkWidget* filter = search_new();

	//alignment to give top border to main hpane.
	GtkWidget* align1 = gtk_alignment_new(0.0, 0.0, 1.0, 1.0);
	gtk_alignment_set_padding(GTK_ALIGNMENT(align1), 2, 1, 0, 0); //top, bottom, left, right.
	gtk_box_pack_start(GTK_BOX(window.vbox), align1, EXPAND_TRUE, TRUE, 0);

	//---------

	GtkWidget* main_vpaned = gtk_vpaned_new();
	// default width is too big.
	// let its size be determined by its parent.
	gtk_widget_set_size_request(main_vpaned, 20, 20);
	gtk_container_add(GTK_CONTAINER(align1), main_vpaned);

	GtkWidget* hpaned = gtk_hpaned_new();
	gtk_paned_set_position(GTK_PANED(hpaned), 210);
	gtk_paned_add1(GTK_PANED(main_vpaned), hpaned);

	GtkWidget* left = window.vpaned = gtk_vpaned_new();
	gtk_paned_add1(GTK_PANED(hpaned), left);

	GtkWidget* pcpaned = gtk_vpaned_new();
	gtk_paned_add1(GTK_PANED(window.vpaned), pcpaned);

	window.dir_tree = dir_panel_new();
	left_pane2();

	GtkWidget* rhs_vbox = gtk_vbox_new(NON_HOMOGENOUS, 0);
	gtk_paned_add2(GTK_PANED(hpaned), rhs_vbox);

	gtk_box_pack_start(GTK_BOX(rhs_vbox), message_panel__new(), EXPAND_FALSE, FILL_FALSE, 0);

	//split the rhs in two:
	GtkWidget* r_vpaned = gtk_vpaned_new();
	gtk_paned_set_position(GTK_PANED(r_vpaned), 300);
	gtk_box_pack_start(GTK_BOX(rhs_vbox), r_vpaned, EXPAND_TRUE, TRUE, 0);

	listview__new();
	if(0 && BACKEND_IS_NULL) gtk_widget_set_no_show_all(app->libraryview->widget, true); //dont show main view if no database.

#if 0
	GtkWidget* rotator = rotator_new_with_model(GTK_TREE_MODEL(app->store));
	gtk_widget_show(rotator);
	gtk_box_pack_start(GTK_BOX(rhs_vbox), rotator, EXPAND_FALSE, FALSE, 0);
	gtk_widget_set_size_request(rotator, -1, 100);
#endif

	dbg(2, "making fileview pane...");
	make_fileview_pane();

	//int library_height = MAX(100, height - default_heights.filters - default_heights.file - default_heights.waveform);
	PACK(app->libraryview->scroll, GDL_DOCK_TOP, -1, library_height, "library", "Library", r_vpaned);
	// this addition sets the dock to be wider than the left column. the height is set to be small so that the height will be determined by the requisition.
	PACK(filter, GDL_DOCK_TOP, left_column_width + 5, 10, "search", "Search", window.vbox);

	gtk_box_reorder_child((GtkBox*)window.vbox, filter, 0);

	PACK(window.file_man, GDL_DOCK_BOTTOM, 216, default_heights.file, "files", "Files", main_vpaned);

	gtk_paned_add1(GTK_PANED(pcpaned), window.dir_tree);

	if (app->auditioner && app->auditioner->position && app->auditioner->seek){
		gtk_paned_add2(GTK_PANED(pcpaned), panels[PANEL_TYPE_PLAYER].widget);
	}

#ifdef USE_OPENGL
	if(app->view_options[SHOW_WAVEFORM].value){
		show_waveform(true);
	}
#endif

#if (defined HAVE_JACK)
	/* initially hide player seek bar */
	if(app->view_options[SHOW_PLAYER].value){
		show_player(true);
	}
#endif
#endif // not USE_GDL

#ifdef USE_GDL
	GtkWidget* dock = window.dock = gdl_dock_new();

	window.layout =	gdl_dock_layout_new(GDL_DOCK(dock));

	GtkWidget* dockbar = gdl_dock_bar_new(GDL_DOCK(dock));
	gdl_dock_bar_set_style(GDL_DOCK_BAR(dockbar), GDL_DOCK_BAR_TEXT);

	gtk_box_pack_start(GTK_BOX(window.vbox), dock, EXPAND_TRUE, true, 0);
	gtk_box_pack_start(GTK_BOX(window.vbox), dockbar, EXPAND_FALSE, false, 0);

	dock->requisition.height = 197; // the size must be set directly. using gtk_widget_set_size_request has no imediate effect.
	int allocation = dock->allocation.height;
	dock->allocation.height = 197; // nasty hack
	dock->allocation.height = allocation;

	void _on_layout_changed(GObject* object, gpointer user_data)
	{
		PF;
		GList* items = gdl_dock_get_named_items((GdlDock*)window.dock);
		GList* l = items;
		for(;l;l=l->next){
			GdlDockObject* object = l->data;
			GdlDockItem* item = l->data;
			bool active = gdl_dock_item_is_active(item);
			//dbg(0, "  %s child=%p visible=%i active=%i", object->name, item->child, item->child ? gtk_widget_get_visible(item->child) : 0, active);
			if(active && !item->child){
				dbg(1, "   must load: %s", object->name);
				int i = panel_lookup_index(object);
				if(i > -1){
					Panel_* panel = &panels[i];
					if(panel->widget){ gwarn("%s: panel already loaded", panel->name); }
					else {
						gtk_container_add(GTK_CONTAINER(item), panel->widget = panel->new());
						switch(i){
							case PANEL_TYPE_SEARCH:
																											dbg(1, "setting resize... TODO");
								g_object_set(item, "resize", false, NULL); // unfortunately gdl-dock does not make any use of this property.
								break;
#ifdef HAVE_FFTW3
							case PANEL_TYPE_SPECTROGRAM:
								show_spectrogram(true);
								break;
#endif
#ifdef USE_OPENGL
							case PANEL_TYPE_WAVEFORM:
								show_waveform(true);
								break;
#endif
							case PANEL_TYPE_PLAYER:
								player_control_on_show_hide(true);
								break;
						}
						gtk_widget_show_all(panel->widget); // just testing. 
					}
				}
			}
		}
		g_list_free(items);

		g_signal_emit_by_name (app, "layout-changed");
	}
	g_signal_connect(G_OBJECT(((GdlDockObject*)window.dock)->master), "layout-changed", G_CALLBACK(_on_layout_changed), NULL);

	// create an empty panel for each panel type
	int i;for(i=0;i<PANEL_TYPE_MAX;i++){
		Panel_* panel = &panels[i];
		panels[i].dock_item = gdl_dock_item_new (panel->name, panel->name, GDL_DOCK_ITEM_BEH_LOCKED);
		gdl_dock_add_item ((GdlDock*)dock, (GdlDockItem*)panels[i].dock_item, GDL_DOCK_LEFT);
	}
#endif

#ifndef USE_GDL
#ifdef HAVE_FFTW3
	if(app->view_options[SHOW_SPECTROGRAM].value){
		show_spectrogram(true);
	}
#endif
#endif

	{
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
	}

	g_signal_connect(G_OBJECT(app->window), "realize", G_CALLBACK(window_on_realise), NULL);
	g_signal_connect(G_OBJECT(app->window), "size-request", G_CALLBACK(window_on_size_request), NULL);
	g_signal_connect(G_OBJECT(app->window), "size-allocate", G_CALLBACK(window_on_allocate), NULL);
	g_signal_connect(G_OBJECT(app->window), "configure_event", G_CALLBACK(window_on_configure), NULL);

	GtkAccelGroup* accel_group = gtk_accel_group_new();
	gboolean mnemonics = false;
	GimpActionGroup* action_group = gimp_action_group_new("Samplecat-window", "Samplecat-window", "gtk-paste", mnemonics, NULL, (GimpActionGroupUpdateFunc)NULL);
	make_accels(accel_group, action_group, window_keys, G_N_ELEMENTS(window_keys), NULL);
	gtk_window_add_accel_group(GTK_WINDOW(app->window), accel_group);

	gtk_widget_show_all(app->window);

	void window_on_selection_change(SamplecatModel* m, Sample* sample, gpointer user_data)
	{
		PF;
#ifdef HAVE_FFTW3
		if(window.spectrogram){
#ifdef USE_GDL
			if(gdl_dock_item_is_active((GdlDockItem*)panels[PANEL_TYPE_SPECTROGRAM].dock_item)){
#else
			if(true){
#endif
#ifdef USE_OPENGL
				gl_spectrogram_set_file((GlSpectrogram*)window.spectrogram, sample->full_path);
#else
				spectrogram_widget_set_file ((SpectrogramWidget*)window.spectrogram, sample->full_path);
#endif
			}
		}
#endif
	}
	g_signal_connect((gpointer)app->model, "selection-changed", G_CALLBACK(window_on_selection_change), NULL);

#ifdef USE_GDL
	void window_on_quit(Application* a, gpointer user_data)
	{
		window_save_layout();
	}
	g_signal_connect((gpointer)app, "on-quit", G_CALLBACK(window_on_quit), NULL);
#endif

	void store_content_changed(GtkListStore* store, gpointer data)
	{
		int n_results = backend.n_results;
		int row_count = ((SamplecatListStore*)store)->row_count;

		if(0 && row_count < MAX_DISPLAY_ROWS){
			statusbar_print(1, "%i samples found.", row_count);
		}else if(!row_count){
			statusbar_print(1, "no samples found. filters: dir=%s", app->model->filters.dir->value);
		}else if (n_results < 0) {
			statusbar_print(1, "showing %i sample(s)", row_count);
		}else{
			statusbar_print(1, "showing %i of %i sample(s)", row_count, n_results);
		}
	}
	g_signal_connect(G_OBJECT(app->store), "content-changed", G_CALLBACK(store_content_changed), NULL);

	dnd_setup();

	window_on_layout_changed();

	bool window_on_clicked(GtkWidget* widget, GdkEventButton* event, gpointer user_data)
	{
		if(event->button == 3){
			if(app->context_menu){
				gtk_menu_popup(GTK_MENU(app->context_menu),
				               NULL, NULL,  // parents
				               NULL, NULL,  // fn and data used to position the menu.
				               event->button, event->time);
				return HANDLED;
			}
		}
		return NOT_HANDLED;
	}
	g_signal_connect((gpointer)app->window, "button-press-event", G_CALLBACK(window_on_clicked), NULL);

	void on_worker_progress(gpointer _n)
	{
		int n = GPOINTER_TO_INT(_n);

		if(n) statusbar_print(2, "updating %i items...", n);
		else statusbar_print(2, "");
	}
	worker_register(on_worker_progress);

	return true;
}


static void
window_on_realise(GtkWidget* win, gpointer user_data)
{
}


static void
window_on_size_request(GtkWidget* widget, GtkRequisition* req, gpointer user_data)
{
	req->height = atoi(app->config.window_height);
}


static void
window_on_allocate(GtkWidget* win, GtkAllocation* allocation, gpointer user_data)
{
	#define SCROLLBAR_WIDTH_HACK 32
	static gboolean done = false;
	static gboolean library_done = false;
#ifndef USE_GDL
	if(!app->libraryview->widget->requisition.width) return;
#endif

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

	static gboolean did_set_colours = false;
	if (!did_set_colours) {
		did_set_colours = true;

		colour_get_style_bg(&app->bg_colour, GTK_STATE_NORMAL);
		colour_get_style_fg(&app->fg_colour, GTK_STATE_NORMAL);
		colour_get_style_base(&app->base_colour, GTK_STATE_NORMAL);
		colour_get_style_text(&app->text_colour, GTK_STATE_NORMAL);

		hexstring_from_gdkcolor(app->config.colour[0], &app->bg_colour);

		//make modifier colours:
		colour_get_style_bg(&app->bg_colour_mod1, GTK_STATE_NORMAL);
		app->bg_colour_mod1.red   = MIN(app->bg_colour_mod1.red   + 0x1000, 0xffff);
		app->bg_colour_mod1.green = MIN(app->bg_colour_mod1.green + 0x1000, 0xffff);
		app->bg_colour_mod1.blue  = MIN(app->bg_colour_mod1.blue  + 0x1000, 0xffff);

		//set column colours:
		dbg(3, "fg_color: %x %x %x", app->fg_colour.red, app->fg_colour.green, app->fg_colour.blue);

#if 0
		if(is_similar(&app->bg_colour_mod1, &app->fg_colour, 0xFF)) perr("colours not set properly!");
#endif
		dbg(2, "%s %s", gdkcolor_get_hexstring(&app->bg_colour_mod1), gdkcolor_get_hexstring(&app->fg_colour));
		if(app->fm_view) view_details_set_alt_colours(VIEW_DETAILS(app->fm_view), &app->bg_colour_mod1, &app->fg_colour);

		g_signal_emit_by_name (app, "theme-changed", NULL);
	}

	if(did_set_colours && !library_done && app->libraryview && app->libraryview->widget->requisition.width){
		g_object_set(app->libraryview->cells.name, "cell-background-gdk", &app->bg_colour_mod1, "cell-background-set", TRUE, NULL);
		g_object_set(app->libraryview->cells.name, "foreground-gdk", &app->fg_colour, "foreground-set", TRUE, NULL);
		library_done = true;
	}
}


static gboolean
window_on_configure(GtkWidget* widget, GdkEventConfigure* event, gpointer user_data)
{
	static gboolean window_size_set = false;
	if(!window_size_set){
		//take the window size from the config file, or failing that, from the treeview requisition.
		int width = atoi(app->config.window_width);
#ifdef USE_GDL
		// panels not yet created, so widget requisition cannot be used.
		if(!width) width = 640;
#else
		if(!width) width = app->libraryview->widget->requisition.width + SCROLLBAR_WIDTH_HACK;
#endif
		int window_height = atoi(app->config.window_height);
		if(!window_height) window_height = 480; //MIN(app->libraryview->widget->requisition.height, 900);  -- ignore the treeview height, is meaningless

		if(width && window_height){
			dbg(2, "setting size: width=%i height=%i", width, window_height);
			gtk_window_resize(GTK_WINDOW(app->window), width, window_height);
			window_size_set = true;

			//set the position of the left pane elements.
			//As the allocation is somehow bigger than its container, we just do it v approximately.
/*
			if(window.vpaned && GTK_WIDGET_REALIZED(window.vpaned)){
				//dbg(0, "height=%i %i %i", app->hpaned->allocation.height, app->statusbar->allocation.y, app->inspector->widget->allocation.height);
				guint inspector_y = height - app->hpaned->allocation.y - 210;
				gtk_paned_set_position(GTK_PANED(window.vpaned), inspector_y);
			}
*/
		}

#ifdef USE_GDL
		window_load_layout();
#else
		if(app->view_options[SHOW_PLAYER].value && panels[PANEL_TYPE_PLAYER].widget){
			show_player(true);
		}
#endif

#ifdef USE_GDL
		bool window_activate_layout(gpointer data)
		{
			PF;

			if(gdl_dock_layout_load_layout (window.layout, "__default__")){
				if(_debug_) gdl_dock_print_recursive((GdlDockMaster*)((GdlDockObject*)window.dock)->master);
			}else{
				gwarn("load layout failed");
			}

			return TIMER_STOP;
		}
		window_activate_layout(NULL);
#endif
		app->context_menu = make_context_menu();
	}

	return false;
}


static gboolean
window_on_destroy(GtkWidget* widget, gpointer user_data)
{
	return false;
}


static GtkWidget*
make_fileview_pane()
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

	const char* dir = (app->config.browse_dir && app->config.browse_dir[0] && g_file_test(app->config.browse_dir, G_FILE_TEST_IS_DIR))
		? app->config.browse_dir
		: g_get_home_dir();
	fman_left(dir);
	fman_right(dir);

	return fman_hpaned;
}


#ifndef USE_GDL
static void
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
		gtk_widget_set_no_show_all(panels[PANEL_TYPE_PLAYER].widget, true);

	inspector_new();
	gtk_paned_add2(GTK_PANED(window.vpaned), app->inspector->widget);
	//g_signal_connect(app->inspector->widget, "size-allocate", (gpointer)on_inspector_allocate, NULL);

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
				gtk_paned_set_position(GTK_PANED(window.vpaned), MAX(tot_height / 2, tot_height - inspector_requisition));
			}

			//large window - dont allow the inspector to be too big
			int current_insp_height = tot_height - gtk_paned_get_position(GTK_PANED(window.vpaned));
			if(current_insp_height > inspector_requisition){
				gtk_paned_set_position(GTK_PANED(window.vpaned), tot_height - inspector_requisition);
			}
		}
		previous_height = vp_allocation->height;
	}

	g_signal_connect(window.vpaned, "size-allocate", (gpointer)on_vpaned_allocate, NULL);
}
#endif


static GtkWidget*
search_new()
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
		const gchar* text = gtk_entry_get_text(GTK_ENTRY(window.search));
		if(!app->model->filters.search->value || strcmp(text, app->model->filters.search->value)){
			samplecat_filter_set_value(app->model->filters.search, g_strdup(text));
		}
		return NOT_HANDLED;
	}

	SamplecatFilter* filter = app->model->filters.search;
	GtkWidget* entry = window.search = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(entry), 64);
	if(filter->value) gtk_entry_set_text(GTK_ENTRY(entry), filter->value);
	gtk_box_pack_start(GTK_BOX(row1), entry, EXPAND_TRUE, TRUE, 0);
	g_signal_connect(G_OBJECT(entry), "focus-out-event", G_CALLBACK(on_focus_out), NULL);
	gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
	g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(on_focus_out), NULL);

	void on_search_filter_changed(GObject* _filter, gpointer _entry)
	{
		gtk_entry_set_text(GTK_ENTRY(_entry), ((SamplecatFilter*)_filter)->value);
	}
	g_signal_connect(filter, "changed", G_CALLBACK(on_search_filter_changed), entry);

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


static GtkWidget*
filters_new()
{
	static GHashTable* buttons; buttons = g_hash_table_new(NULL, NULL);

	GtkWidget* hbox = gtk_vbox_new(FALSE, 2);

	char* label_text(SamplecatFilter* filter)
	{
		int len = 22 - strlen(filter->name);
		char value[20] = {0,};
		g_strlcpy(value, filter->value ? filter->value : "", len);
		return g_strdup_printf("%s: %s%s", filter->name, value, filter->value && strlen(filter->value) > len ? "..." : "");
	}

	GList* l = app->model->filters_;
	for(;l;l=l->next){
		SamplecatFilter* filter = l->data;
		dbg(2, "  %s %s", filter->name, filter->value);

		char* text = label_text(filter);
		GtkWidget* button = gtk_button_new_with_label(text);
		g_free(text);
		gtk_box_pack_start(GTK_BOX(hbox), button, EXPAND_FALSE, FALSE, 0);
		gtk_button_set_alignment ((GtkButton*)button, 0.0, 0.5);
		gtk_widget_set_no_show_all(button, !(filter->value && strlen(filter->value)));

		GtkWidget* icon = gtk_image_new_from_stock(GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU);
		gtk_misc_set_padding((GtkMisc*)icon, 4, 0);
		gtk_button_set_image(GTK_BUTTON(button), icon);
		gtk_widget_set(button, "image-position", GTK_POS_LEFT, NULL);

		g_hash_table_insert(buttons, filter, button);

		void on_filter_button_clicked(GtkButton* button, gpointer _filter)
		{
			samplecat_filter_set_value((SamplecatFilter*)_filter, "");
		}

		g_signal_connect(button, "clicked", G_CALLBACK(on_filter_button_clicked), filter);

		void set_label(SamplecatFilter* filter, GtkWidget* button)
		{
			if(button){
				char* text = label_text(filter);
				gtk_button_set_label((GtkButton*)button, text);
				g_free(text);
				show_widget_if(button, filter->value && strlen(filter->value));
			}
		}

		void on_filter_changed(GObject* _filter, gpointer user_data)
		{
			SamplecatFilter* filter = (SamplecatFilter*)_filter;
			dbg(1, "filter=%s value=%s", filter->name, filter->value);
			set_label(filter, g_hash_table_lookup(buttons, filter));
		}

		g_signal_connect(filter, "changed", G_CALLBACK(on_filter_changed), NULL);
	}
	return hbox;
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


static gboolean
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

	GtkWidget* combo = gtk_combo_box_new_text();
	GtkComboBox* combo_ = GTK_COMBO_BOX(combo);
	gtk_combo_box_append_text(combo_, ALL_CATEGORIES);
	int i; for(i=0;i<G_N_ELEMENTS(categories);i++){
		gtk_combo_box_append_text(combo_, categories[i]);
	}
	gtk_combo_box_set_active(combo_, 0);
	gtk_box_pack_start(GTK_BOX(window.toolbar), combo, EXPAND_FALSE, FALSE, 0);

	void
	on_view_category_changed(GtkComboBox* widget, gpointer user_data)
	{
		//update the sample list with the new view-category.
		PF;

		char* category = gtk_combo_box_get_active_text(GTK_COMBO_BOX(widget));
		if (!strcmp(category, ALL_CATEGORIES)) g_free0(category);
		samplecat_filter_set_value(app->model->filters.category, category);
	}
	g_signal_connect(combo, "changed", G_CALLBACK(on_view_category_changed), NULL);

	void on_category_filter_changed(GObject* _filter, gpointer user_data)
	{
		GtkComboBox* combo = user_data;
		SamplecatFilter* filter = (SamplecatFilter*)_filter;
		if(filter->value){
			if(!strlen(filter->value)){
				gtk_combo_box_set_active(combo, 0);
			}
		}
	}

	GList* l = app->model->filters_;
	for(;l;l=l->next){
		SamplecatFilter* filter = l->data;
		if(!strcmp(filter->name, "category")){
			g_signal_connect(filter, "changed", G_CALLBACK(on_category_filter_changed), combo);
		}
	}

	return TRUE;
}


static void
delete_selected_rows()
{
	GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(app->libraryview->widget));

	GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(app->libraryview->widget));
	GList* selectionlist = gtk_tree_selection_get_selected_rows(selection, &(model));
	if(!selectionlist){ perr("no files selected?\n"); return; }
	dbg(1, "%i rows selected.", g_list_length(selectionlist));

	GList* selected_row_refs = NULL;

	//get row refs for each selected row before the list is modified:
	GList* l = selectionlist;
	for(;l;l=l->next){
		GtkTreePath* treepath_selection = l->data;

		GtkTreeRowReference* row_ref = gtk_tree_row_reference_new(GTK_TREE_MODEL(app->store), treepath_selection);
		selected_row_refs = g_list_prepend(selected_row_refs, row_ref);
	}
	g_list_free(selectionlist);

	int n = 0;
	GtkTreePath* path;
	GtkTreeIter iter;
	l = selected_row_refs;
	for(;l;l=l->next){
		GtkTreeRowReference* row_ref = l->data;
		if((path = gtk_tree_row_reference_get_path(row_ref))){

			if(gtk_tree_model_get_iter(model, &iter, path)){
				gchar* fname;
				int id;
				gtk_tree_model_get(model, &iter, COL_NAME, &fname, COL_IDX, &id, -1);

				if(!samplecat_model_remove(app->model, id)) return;

				gtk_list_store_remove(app->store, &iter);
				n++;

			} else perr("bad iter!\n");
			gtk_tree_path_free(path);
		} else perr("cannot get path from row_ref!\n");
	}
	g_list_free(selected_row_refs); //FIXME free the row_refs?

	statusbar_print(1, "%i rows deleted", n);
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
		s->online = true;
		samplecat_model_set_selection (app->model, s);
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
			ScanResults results = {0,};
			application_add_file(filepath, &results);
    		if(results.n_added) statusbar_print(1, "file added");
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
		ScanResults results = {0,};
		application_scan(path, &results);
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
		application_play_path(item);
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


static void
on_category_set_clicked(GtkComboBox* widget, gpointer user_data)
{
	// add selected category to selected samples.

	PF;
	//selected category?
	gchar* category = gtk_combo_box_get_active_text(GTK_COMBO_BOX(window.category));

	GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(app->libraryview->widget));
	GList* selectionlist = gtk_tree_selection_get_selected_rows(selection, NULL);
	if(!selectionlist){ statusbar_print(1, "no files selected."); return; }

	int i;
	GtkTreeIter iter;
	for(i=0;i<g_list_length(selectionlist);i++){
		GtkTreePath* treepath_selection = g_list_nth_data(selectionlist, i);

		if(gtk_tree_model_get_iter(GTK_TREE_MODEL(app->store), &iter, treepath_selection)){
			gchar* fname; gchar* tags;
			int id;
			Sample* sample;
			gtk_tree_model_get(GTK_TREE_MODEL(app->store), &iter, COL_SAMPLEPTR, &sample, COL_NAME, &fname, COL_KEYWORDS, &tags, COL_IDX, &id, -1);
			dbg(1, "id=%i name=%s", id, fname);

			if(!strcmp(category, "no categories")){
				if(samplecat_model_update_sample(app->model, sample, COL_KEYWORDS, "")){
				}
			}else{
				if(!keyword_is_dupe(category, tags)){
					char tags_new[1024];
					snprintf(tags_new, 1024, "%s %s", tags ? tags : "", category);
					g_strstrip(tags_new); // trim

					if(samplecat_model_update_sample(app->model, sample, COL_KEYWORDS, (void*)tags_new)){
						statusbar_print(1, "category set");
					}

				}else{
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
static GtkWidget*
waveform_panel_new()
{
#ifdef USE_LIBASS
	waveform_view_plus_set_gl(agl_get_gl_context());
	WaveformViewPlus* view = waveform_view_plus_new(NULL);

	AGlActor* text_layer = waveform_view_plus_add_layer(view, text_actor(NULL), 3);
	text_actor_set_colour((TextActor*)text_layer, 0x000000bb, 0xffffffbb);

	window.layers.spp = waveform_view_plus_add_layer(view, spp_actor(waveform_view_plus_get_actor(view)), 0);

	//waveform_view_plus_add_layer(view, grid_actor(waveform_view_plus_get_actor(view)), 0);
#if 0
	waveform_view_plus_set_show_grid(view, true);
#endif
	return (GtkWidget*)view;

#else
	waveform_view_set_gl(agl_get_gl_context());
	return (GtkWidget*)waveform_view_new(NULL);
#endif
}


void
show_waveform(gboolean enable)
{
	typedef struct {
		gulong selection_handler;
	} C;

	void on_waveform_view_finalize(gpointer _c, GObject* was)
	{
		// it is not expected for the waveform to be finalised, but just in case

		dbg(0, "!");
		C* c = _c;

		g_signal_handler_disconnect((gpointer)app, c->selection_handler);
		g_free(c);
		window.waveform = NULL;
	}

	void on_waveform_view_realise(GtkWidget* widget, gpointer user_data)
	{
		// The widget becomes unrealised after layout changes and it needs to be restored afterwards
		// TODO the widget now supports being unrealised so this may no longer be needed.
		PF;
#ifdef USE_LIBASS
		WaveformViewPlus* view = (WaveformViewPlus*)widget;
#else
		WaveformView* view = (WaveformView*)widget;
#endif
		static guint timer = 0;

		bool redisplay(gpointer _view)
		{

#ifdef USE_GDL
			if(gdl_dock_item_is_active((GdlDockItem*)panels[PANEL_TYPE_WAVEFORM].dock_item)){
#else
#ifdef USE_LIBASS
			WaveformViewPlus* view = (WaveformViewPlus*)_view;
#else
			WaveformView* view = (WaveformView*)_view;
#endif
			if(!view->waveform && app->view_options[SHOW_WAVEFORM].value){
#endif
				show_waveform(true);
			}
			timer = 0;
			return TIMER_STOP;
		}
		if(timer) g_source_remove(timer);
		timer = g_timeout_add(500, redisplay, view);
	}

	void _waveform_on_selection_change(SamplecatModel* m, Sample* sample, gpointer _c)
	{
		PF;
		g_return_if_fail(window.waveform);
		update_waveform_view(sample);
	}

#ifdef USE_LIBASS
	bool waveform_is_playing()
	{
		if(!app->play.sample) return false;
		if(!((WaveformViewPlus*)window.waveform)->waveform) return false;

		char* path = ((WaveformViewPlus*)window.waveform)->waveform->filename;
		return !strcmp(path, app->play.sample->full_path);
	}

	void waveform_on_position(GObject* _app, gpointer _)
	{
		g_return_if_fail(app->play.sample);
		if(!((WaveformViewPlus*)window.waveform)->waveform) return;

		if(window.layers.spp){
			spp_actor_set_time((SppActor*)window.layers.spp, waveform_is_playing() ? app->play.position : UINT32_MAX);
		}
	}

	void waveform_on_play(GObject* _app, gpointer _)
	{
		waveform_on_position(_app, _);
	}

	void waveform_on_stop(GObject* _app, gpointer _)
	{
#if 0
		waveform_view_plus_set_time((WaveformViewPlus*)window.waveform, UINT32_MAX);
#else
		AGlActor* spp = waveform_view_plus_get_layer((WaveformViewPlus*)window.waveform, 5);
		if(spp){
			spp_actor_set_time((SppActor*)spp, UINT32_MAX);
		}
#endif
	}
#endif

	if(enable && !window.waveform){
		C* c = g_new0(C, 1);

#ifdef USE_GDL
		window.waveform = panels[PANEL_TYPE_WAVEFORM].widget;
#else
		window.waveform = waveform_panel_new();
#endif
		g_object_weak_ref((GObject*)window.waveform, on_waveform_view_finalize, c);

#ifndef USE_GDL
		gtk_box_pack_start(GTK_BOX(window.vbox), window.waveform, EXPAND_FALSE, FILL_TRUE, 0);
		gtk_widget_set_size_request(window.waveform, 100, 96);
#endif

		c->selection_handler = g_signal_connect((gpointer)app->model, "selection-changed", G_CALLBACK(_waveform_on_selection_change), c);
		g_signal_connect((gpointer)window.waveform, "realize", G_CALLBACK(on_waveform_view_realise), NULL);

		void waveform_on_audio_ready(GObject* _app, gpointer _)
		{
#ifdef USE_LIBASS
			g_signal_connect(app, "play-start", (GCallback)waveform_on_play, NULL);
			g_signal_connect(app, "play-stop", (GCallback)waveform_on_stop, NULL);
			g_signal_connect(app, "play-position", (GCallback)waveform_on_position, NULL);
#endif
		}
		g_signal_connect(app, "audio-ready", (GCallback)waveform_on_audio_ready, NULL);
	}

	if(window.waveform){
#ifdef USE_GDL
		show_widget_if((GtkWidget*)gdl_dock_get_item_by_name(GDL_DOCK(window.dock), panels[PANEL_TYPE_WAVEFORM].name), enable);
#else
		show_widget_if(window.waveform, enable);
#endif
		if(app->inspector) app->inspector->show_waveform = !enable;
		if(enable){
			bool show_wave()
			{
				Sample* s;
				if((s = app->model->selection)){
#ifdef USE_LIBASS
					WaveformViewPlus* view = (WaveformViewPlus*)window.waveform;
					Waveform* w = view->waveform;
					if(!w || strcmp(w->filename, s->full_path)){
						update_waveform_view(s);
					}
#else
					update_waveform_view(s);
#endif
				}
				return G_SOURCE_REMOVE;
			}
			g_idle_add(show_wave, NULL);
		}
	}
}
#endif


#ifdef HAVE_FFTW3
static GtkWidget*
spectrogram_new()
{
#ifdef USE_OPENGL
	gl_spectrogram_set_gl_context(agl_get_gl_context());
#endif

	return
#ifdef USE_OPENGL
		(GtkWidget*)gl_spectrogram_new();
#else
        (GtkWidget*)spectrogram_widget_new();
#endif
}


void
show_spectrogram(gboolean enable)
{
	if(enable && !window.spectrogram){
#ifdef USE_GDL
		window.spectrogram = panels[PANEL_TYPE_SPECTROGRAM].widget;
#else
		gl_spectrogram_set_gl_context(agl_get_gl_context());
		window.spectrogram = panels[PANEL_TYPE_SPECTROGRAM].new();
#ifdef USE_OPENGL
		gtk_widget_set_size_request(window.spectrogram, 100, 100);
#endif
		gtk_box_pack_start(GTK_BOX(window.vbox), window.spectrogram, EXPAND_TRUE, FILL_TRUE, 0);
#endif

		gchar* filename = listview__get_first_selected_filepath();
		if(filename){
			dbg(1, "file=%s", filename);
#ifdef USE_OPENGL
			gl_spectrogram_set_file((GlSpectrogram*)window.spectrogram, filename);
#else
			spectrogram_widget_set_file((SpectrogramWidget*)window.spectrogram, filename);
#endif
			g_free(filename);
		}
	}

	if(window.spectrogram){
		show_widget_if(window.spectrogram, enable);
	}

	window_on_layout_changed();
}
#endif


void
show_filemanager(gboolean enable)
{
#ifdef USE_GDL
#if 1
	// using gtk_widget_show() appears to work, but the correct way would be to use gdl_dock_item_hide_item() and gdl_dock_item_show_item()
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
	show_widget_if(panels[PANEL_TYPE_PLAYER].widget, enable);
#else
	show_widget_if(panels[PANEL_TYPE_PLAYER].widget, enable);
#endif

	player_control_on_show_hide(enable);
}


#ifdef USE_OPENGL
static void
update_waveform_view(Sample* sample)
{
	g_return_if_fail(window.waveform);
	if(!gtk_widget_get_realized(window.waveform)) return; // may be hidden by the layout manager.

#ifdef USE_LIBASS
	WaveformViewPlus* view = (WaveformViewPlus*)window.waveform;

	waveform_view_plus_load_file(view, sample->online ? sample->full_path : NULL, NULL, NULL);

#if 0
	void on_waveform_finalize(gpointer _c, GObject* was)
	{
		dbg(0, "!");
	}
	g_object_weak_ref((GObject*)((WaveformViewPlus*)window.waveform)->waveform, on_waveform_finalize, NULL);
#endif

	dbg(1, "name=%s", sample->name);

	AGlActor* text_layer = waveform_view_plus_get_layer(view, 3);
	if(text_layer){
		char* text = NULL;
		if(sample->channels){
			char* ch_str = format_channels(sample->channels);
			char* level  = gain2dbstring(sample->peaklevel);
			char length[64]; format_smpte(length, sample->length);
			char fs_str[32]; samplerate_format(fs_str, sample->sample_rate); strcpy(fs_str + strlen(fs_str), " kHz");

			text = g_strdup_printf("%s  %s  %s  %s", length, ch_str, fs_str, level);

			g_free(ch_str);
			g_free(level);
		}

		text_actor_set_text(((TextActor*)text_layer), g_strdup(sample->name), text);
		//text_actor_set_colour((TextActor*)text_layer, 0x33aaffff, 0xffff00ff);
	}

#else
	waveform_view_load_file((WaveformView*)window.waveform, sample->online ? sample->full_path : NULL);
#endif
}
#endif


static void
window_on_layout_changed()
{
#ifndef USE_GDL
	//what is the height of the inspector?

	if(app->inspector){
		GtkWidget* widget;
		if((widget = app->inspector->widget)){
			int tot_height = window.vpaned->allocation.height;
			int max_auto_height = tot_height / 2;
			dbg(1, "inspector_height=%i tot=%i", widget->allocation.height, tot_height);
			if(widget->allocation.height < app->inspector->preferred_height
					&& widget->allocation.height < max_auto_height){
				int inspector_height = MIN(max_auto_height, app->inspector->preferred_height);
				dbg(1, "setting height : %i/%i", tot_height - inspector_height, inspector_height);
				gtk_paned_set_position(GTK_PANED(window.vpaned), tot_height - inspector_height);
			}
		}
	}
#endif

	// scroll to current dir in the directory list
	if(app->dir_treeview2) vdtree_set_path(app->dir_treeview2, app->dir_treeview2->path);
}


static void
k_delete_row(GtkAccelGroup* _, gpointer user_data)
{
	delete_selected_rows();
}


#ifdef USE_GDL
static void
k_show_layout_manager(GtkAccelGroup* _, gpointer user_data)
{
	PF;
	gdl_dock_layout_run_manager(window.layout);
}


static void
window_load_layout()
{
	// Try to load xml file from the users config dir, or fallback to using the built in default xml

	PF;
	bool have_layout = false;

	bool _load_layout(const char* name)
	{
		if (gdl_dock_layout_load_layout(window.layout, name)) // name must match file contents
			return true;
		else
			g_warning ("Loading layout failed");
		return false;
	}

	bool _load_layout_from_file(const char* name)
	{
		bool ok = false;
		char* filename = g_strdup_printf("%s/layouts/%s.xml", app->config_dir, name);
		if(gdl_dock_layout_load_from_file(window.layout, filename)){
			if(!strcmp(name, "__default__")){ // only activate one layout
				_load_layout(name);
				ok = true;
			}
		}
		else gwarn("failed to load %s", filename);
		g_free(filename);
		return ok;
	}

	char* path = g_strdup_printf("%s/layouts/", app->config_dir);
	GError* error = NULL;
	GDir* dir = g_dir_open(path, 0, &error);
	if(!error) {
		const gchar* filename;
		while((filename = g_dir_read_name(dir))){
			//if(g_strrstr(filename, ".xml")){
			if(g_str_has_suffix(filename, ".xml")){
				gchar* name = g_strndup(filename, strlen(filename) - 4);
				dbg(1, "loading layout: %s", name);
				if(_load_layout_from_file(name)) have_layout = true;
				g_free(name);
			}
		}
		g_dir_close(dir);
	}else{
		if(_debug_) gwarn("dir='%s' failed. %s", path, error->message);
		g_error_free0(error);
	}

	if(!have_layout){
		dbg(1, "using default_layout");
		if(gdl_dock_layout_load_from_string (window.layout, default_layout)){
			_load_layout("__default__");
		}
	}

	g_free(path);
}


static void
window_save_layout()
{
	PF;
	g_return_if_fail(window.layout);

	char* dir = g_build_filename(app->config_dir, "layouts", NULL);
	if(!g_mkdir_with_parents(dir, 488)){

		char* filename = g_build_filename(app->config_dir, "layouts", "__default__.xml", NULL);
		if(gdl_dock_layout_save_to_file(window.layout, filename)){
		}
		g_free(filename);

	} else gwarn("failed to create layout directory: %s", dir);
	g_free(dir);
}
#endif


static GtkWidget*
make_context_menu()
{
	void menu_delete_row(GtkMenuItem* widget, gpointer user_data)
	{
		delete_selected_rows();
	}

	/** sync the selected catalogue row with the filesystem. */
	void
	menu_update_rows(GtkWidget* widget, gpointer user_data)
	{
		PF;
		gboolean force_update = true; //(GPOINTER_TO_INT(user_data)==2) ? true : false; // NOTE - linked to order in _menu_def[]

		GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(app->libraryview->widget));
		GList* selectionlist = gtk_tree_selection_get_selected_rows(selection, NULL);
		if(!selectionlist) return;
		dbg(2, "%i rows selected.", g_list_length(selectionlist));

		int i;
		for(i=0;i<g_list_length(selectionlist);i++){
			GtkTreePath* treepath = g_list_nth_data(selectionlist, i);
			Sample* sample = sample_get_from_model(treepath);
			if(do_progress(0, 0)) break; // TODO: set progress title to "updating"
			samplecat_model_refresh_sample (app->model, sample, force_update);
			statusbar_print(1, "online status updated (%s)", sample->online ? "online" : "not online");
			sample_unref(sample);
		}
		hide_progress();
		g_list_free(selectionlist);
	}

	void menu_play_all(GtkWidget* widget, gpointer user_data)
	{
		application_play_all();
	}

	void menu_play_stop(GtkWidget* widget, gpointer user_data)
	{
		application_stop();
	}

	void
	toggle_loop_playback(GtkMenuItem* widget, gpointer user_data)
	{
		PF;
		if(app->loop_playback) app->loop_playback = false; else app->loop_playback = true;
	}

	void toggle_recursive_add(GtkMenuItem* widget, gpointer user_data)
	{
		PF;
		if(app->add_recursive) app->add_recursive = false; else app->add_recursive = true;
	}

	MenuDef _menu_def[] = {
		{"Delete",         G_CALLBACK(menu_delete_row),         GTK_STOCK_DELETE,      true},
		{"Update",         G_CALLBACK(menu_update_rows),        GTK_STOCK_REFRESH,     true},
	#if 0 // force is now the default update. Is there a use case for 2 choices?
		{"Force Update",   G_CALLBACK(update_rows),             GTK_STOCK_REFRESH,     true},
	#endif
		{"Reset Colours",  G_CALLBACK(listview__reset_colours), GTK_STOCK_OK,          true},
		{"Edit tags",      G_CALLBACK(listview__edit_row),      GTK_STOCK_EDIT,        true},
		{"Open",           G_CALLBACK(listview__edit_row),      GTK_STOCK_OPEN,       false},
		{"Open Directory", G_CALLBACK(NULL),                    GTK_STOCK_OPEN,        true},
		{"",                                                                               },
		{"Play All",       G_CALLBACK(menu_play_all),           GTK_STOCK_MEDIA_PLAY, false},
		{"Stop Playback",  G_CALLBACK(menu_play_stop),          GTK_STOCK_MEDIA_STOP, false},
		{"",                                                                               },
		{"View",           G_CALLBACK(NULL),                    GTK_STOCK_PREFERENCES, true},
	#ifdef USE_GDL
		{"Layouts",        G_CALLBACK(NULL),                    GTK_STOCK_PROPERTIES,  true},
	#endif
		{"Prefs",          G_CALLBACK(NULL),                    GTK_STOCK_PREFERENCES, true},
	};

	static struct
	{
		GtkWidget* play_all;
		GtkWidget* stop_playback;
		GtkWidget* loop_playback;
	} widgets;

	GtkWidget* menu = gtk_menu_new();

	add_menu_items_from_defn(menu, _menu_def, G_N_ELEMENTS(_menu_def));

	GList* menu_items = gtk_container_get_children((GtkContainer*)menu);
	GList* last = g_list_last(menu_items);

	widgets.play_all = g_list_nth_data(menu_items, 7);
	widgets.stop_playback = g_list_nth_data(menu_items, 8);

	GtkWidget* sub = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(last->data), sub);

	{
#ifdef USE_GDL
		GtkWidget* view = last->prev->prev->data;
#else
		GtkWidget* view = last->prev->data;
#endif

		GtkWidget* sub = gtk_menu_new();
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(view), sub);

#ifdef USE_GDL
		void set_view_state(GtkMenuItem* item, Panel_* panel, bool visible)
		{
			if(panel->handler){
				g_signal_handler_block(item, panel->handler);
				gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), visible);
				g_signal_handler_unblock(item, panel->handler);
			}
			dbg(2, "value=%i", visible);
		}

		void toggle_view(GtkMenuItem* menu_item, gpointer _panel_type)
		{
			PF;
			Panel_* panel = &panels[GPOINTER_TO_INT(_panel_type)];
			GdlDockItem* item = (GdlDockItem*)panel->dock_item;
			if(gdl_dock_item_is_active(item)) gdl_dock_item_hide_item(item); else gdl_dock_item_show_item(item);
		}
#else
		void set_view_toggle_state(GtkMenuItem* item, ViewOption* option)
		{
			option->value = !option->value;
			gulong sig_id = g_signal_handler_find(item, G_SIGNAL_MATCH_FUNC, 0, 0, 0, option->on_toggle, NULL);
			if(sig_id){
				g_signal_handler_block(item, sig_id);
				gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), option->value);
				g_signal_handler_unblock(item, sig_id);
			}
		}

		void toggle_view(GtkMenuItem* item, gpointer _option)
		{
			PF;
			ViewOption* option = (ViewOption*)_option;
			option->on_toggle(option->value = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(item)));
		}
#endif

#ifdef USE_GDL
		int i; for(i=0;i<PANEL_TYPE_MAX;i++){
			Panel_* option = &panels[i];
#else
		int i; for(i=0;i<MAX_VIEW_OPTIONS;i++){
			ViewOption* option = &app->view_options[i];
#endif
			GtkWidget* menu_item = gtk_check_menu_item_new_with_mnemonic(option->name);
			gtk_menu_shell_append(GTK_MENU_SHELL(sub), menu_item);
#ifdef USE_GDL
			option->menu_item = menu_item;
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item), false); // TODO can leave til later
			option->handler = g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(toggle_view), GINT_TO_POINTER(i));
#else
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item), option->value);
			option->value = !option->value; //toggle before it gets toggled back.
			set_view_toggle_state((GtkMenuItem*)menu_item, option);
			g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(toggle_view), option);
#endif
		}

#ifdef USE_GDL
		void add_item(GtkMenuShell* parent, const char* name, GCallback callback, gpointer user_data)
		{
			GtkWidget* item = gtk_menu_item_new_with_label(name);
			gtk_container_add(GTK_CONTAINER(parent), item);
			if(callback) g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(callback), user_data);
		}

		{
			static const char* current_layout = "__default__";

			GtkWidget* layouts_menu = last->prev->data;
			static GtkWidget* sub; sub = gtk_menu_new();
			gtk_menu_item_set_submenu(GTK_MENU_ITEM(layouts_menu), sub);

			void layout_activate(GtkMenuItem* item, gpointer _)
			{
				gdl_dock_layout_load_layout(window.layout, current_layout = gtk_menu_item_get_label(item));

				GList* menu_items = gtk_container_get_children((GtkContainer*)sub);
				gtk_widget_set_sensitive(g_list_last(menu_items)->data, true); // enable the Save menu
				g_list_free(menu_items);

				statusbar_print(1, "Layout %s loaded", current_layout);
			}

			void layout_save(GtkMenuItem* item, gpointer _)
			{
				// will not be saved to disk until quit.
				gdl_dock_layout_save_layout (window.layout, current_layout);
				statusbar_print(1, "Layout %s saved", current_layout);
			}

			GList* layouts = gdl_dock_layout_get_layouts (window.layout, false);
			GList* l = layouts;
			for(;l;l=l->next){
				add_item(GTK_MENU_SHELL(sub), l->data, (GCallback)layout_activate, NULL);
				g_free(l->data);
			}
			g_list_free(layouts);

			gtk_menu_shell_append (GTK_MENU_SHELL(sub), gtk_separator_menu_item_new());

			add_menu_items_from_defn(sub, (MenuDef[]){{"Save", (GCallback)layout_save, GTK_STOCK_SAVE, false}}, 1);
		}

		void _window_on_layout_changed(GObject* object, gpointer user_data)
		{
			GList* items = gdl_dock_get_named_items((GdlDock*)window.dock);
			GList* l = items;
			for(;l;l=l->next){
				GdlDockItem* item = l->data;
				Panel_* panel = NULL;
				if((panel = panel_lookup((GdlDockObject*)item))){
					set_view_state((GtkMenuItem*)panel->menu_item, panel, gdl_dock_item_is_active(item));
				}
			}
		}
		Idle* idle = idle_new(_window_on_layout_changed, NULL);
		g_signal_connect(G_OBJECT(((GdlDockObject*)window.dock)->master), "layout-changed", (GCallback)idle->run, idle);
#endif
	}

	GtkWidget* menu_item = gtk_check_menu_item_new_with_mnemonic("Add Recursively");
	gtk_menu_shell_append(GTK_MENU_SHELL(sub), menu_item);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item), app->add_recursive);
	g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(toggle_recursive_add), NULL);

	widgets.loop_playback = menu_item = gtk_check_menu_item_new_with_mnemonic("Loop Playback");
	gtk_menu_shell_append(GTK_MENU_SHELL(sub), menu_item);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item), app->loop_playback);
	g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(toggle_loop_playback), NULL);
	gtk_widget_set_no_show_all(menu_item, true);

	if(themes){
		GtkWidget* theme_menu = gtk_menu_item_new_with_label("Icon Themes");
		gtk_container_add(GTK_CONTAINER(sub), theme_menu);

		GtkWidget* sub_menu = themes->data;
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(theme_menu), sub_menu);
	}

	void menu_on_audio_ready(GObject* _app, gpointer _)
	{
		gtk_widget_set_sensitive(widgets.play_all, true);
		gtk_widget_set_sensitive(widgets.stop_playback, true);
		if (app->auditioner->seek) {
			gtk_widget_show(widgets.loop_playback);
		}
	}
	g_signal_connect(app, "audio-ready", (GCallback)menu_on_audio_ready, NULL);

	gtk_widget_show_all(menu);
	g_list_free(menu_items);
	return menu;
}


#ifdef USE_GDL
static Panel_*
panel_lookup(GdlDockObject* dock_object)
{
	int i;for(i=0;i<PANEL_TYPE_MAX;i++){
		Panel_* panel = &panels[i];
		if(dock_object == (GdlDockObject*)panel->dock_item){
			return panel;
		}
	}
	return NULL;
}


static int
panel_lookup_index(GdlDockObject* dock_object)
{
	int i;for(i=0;i<PANEL_TYPE_MAX;i++){
		Panel_* panel = &panels[i];
		if(dock_object == (GdlDockObject*)panel->dock_item){
			return i;
		}
	}
	return -1;
}
#endif
