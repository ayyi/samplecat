/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
 | copyright (C) 2007-2022 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */
#undef ROTATOR

#include "config.h"
#include <math.h>
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <gtk/gtk.h>
#pragma GCC diagnostic warning "-Wdeprecated-declarations"
#include <gdk/gdkkeysyms.h>
#ifdef USE_GDL
#include "gdl/gdl-dock-layout.h"
#include "gdl/gdl-dock-bar.h"
#include "gdl/gdl-dock-paned.h"
#include "gdl/registry.h"
#endif
#include "debug/debug.h"
#include "gtk/menu.h"
#include "gtk/gimpactiongroup.h"
#include "file_manager.h"
#include "file_manager/menu.h"
#include "samplecat/worker.h"
#include "player/player.h"
#include "audio_analysis/waveform/waveform.h"
#include "src/typedefs.h"
#include "sample.h"
#include "support.h"
#include "model.h"
#include "application.h"
#include "library.h"
#include "dnd.h"
#include "inspector.h"
#include "progress_dialog.h"
#include "player_control.h"
#include "icon_theme.h"
#ifdef USE_OPENGL
#include "waveform/view_plus.h"
#endif
#ifdef HAVE_FFTW3
#ifndef USE_OPENGL
#include "spectrogram_widget.h"
#endif
#endif
#ifdef ROTATOR
#include "rotator.h"
#endif
#include "window.h"
#ifndef __APPLE__
#include "icons/samplecat.xpm"
#endif

#include "../layouts/layouts.c"

extern void view_details_dnd_get (GtkWidget*, GdkDragContext*, GtkSelectionData*, guint info, guint time, gpointer data);
extern void on_quit              (GtkMenuItem*, gpointer);

#define BACKEND samplecat.model->backend
#define _INSPECTOR ((Inspector*)panels[PANEL_TYPE_INSPECTOR].widget)

typedef GtkWidget* (NewPanelFn)();
typedef void       (ShowPanelFn)(bool);

typedef enum {
   PANEL_TYPE_LIBRARY,
   PANEL_TYPE_SEARCH,
   PANEL_TYPE_TAGS,
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
#ifdef ROTATOR
   PANEL_TYPE_ROTATOR,
#endif
   PANEL_TYPE_MAX
} PanelType;

static NewPanelFn
#ifdef HAVE_FFTW3
	spectrogram_new,
#endif
	filters_new,
	make_fileview_pane;

extern NewPanelFn search_new, dir_panel_new, tags_new, spectrogram_area_new,
#ifdef USE_OPENGL
	waveform_panel_new
#endif
	;

#ifdef USE_OPENGL
ShowPanelFn show_waveform;
#endif

typedef struct {
   char*          name;
   NewPanelFn*    new;
   ShowPanelFn*   show;
   GtkOrientation orientation;
   GtkWidget*     widget;
   GtkWidget*     dock_item;

   GtkWidget*     menu_item;
   gulong         handler;
} Panel_;

Panel_ panels[] = {
   {"Library",     listview__new},
   {"Search",      search_new},
   {"Tags",        tags_new},
   {"Filters",     filters_new},
   {"Inspector",   inspector_new},
   {"Directories", dir_panel_new},
   {"Filemanager", make_fileview_pane},
   {"Player",      player_control_new, player_control_on_show_hide},
#ifdef USE_OPENGL
   {"Waveform",    waveform_panel_new, show_waveform},
#endif
#ifdef HAVE_FFTW3
   {"Spectrogram", spectrogram_new, show_spectrogram, GTK_ORIENTATION_HORIZONTAL},
#endif
#ifdef ROTATOR
   {"Rotator",     _rotator_new},
#endif
};

struct _window {
   GtkWidget*     vbox;
   GtkWidget*     dock;
#ifdef USE_GDL
   GdlDockLayout* layout;
#endif
   GtkWidget*     file_man;
   GtkWidget*     dir_tree;
   GtkWidget*     spectrogram;
#ifndef USE_GDL
   GtkWidget*     vpaned;        //vertical divider on lhs between the dir_tree and inspector
#endif
} window = {0,};

static gboolean   window_on_destroy               (GtkWidget*, gpointer);
static void       window_on_realise               (GtkWidget*, gpointer);
static void       window_on_size_request          (GtkWidget*, GtkRequisition*, gpointer);
static void       window_on_allocate              (GtkWidget*, GtkAllocation*, gpointer);
static gboolean   window_on_configure             (GtkWidget*, GdkEventConfigure*, gpointer);
static void       window_on_fileview_row_selected (GtkTreeView*, gpointer);
static void       delete_selected_rows            ();

#ifdef USE_GDL
static Panel_*    panel_lookup_by_name            (const char*);
#endif

#include "menu.c"

static void       make_menu_actions               (struct _accel[], int, void (*add_to_menu)(GtkAction*));
#ifndef USE_GDL
static GtkWidget* message_panel__new              ();
#endif
#ifndef USE_GDL
static void       left_pane2                      ();
#endif
static void       window_on_layout_changed        ();

static void       k_delete_row                    (GtkAccelGroup*, gpointer);
#ifdef USE_GDL
static void       k_show_layout_manager           (GtkAccelGroup*, gpointer);
static void       window_load_layout              (const char*);
static void       window_save_layout              ();
#endif

#ifdef ROTATOR
GtkWidget*
_rotator_new ()
{
	return rotator_new_with_model(GTK_TREE_MODEL(samplecat.store));
}
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
	if (GTK_IS_BOX(D)) \
		gtk_box_pack_start(GTK_BOX(D), widget, EXPAND_FALSE, FILL_FALSE, 0); \
	else \
		if (!gtk_paned_get_child1(GTK_PANED(D))) gtk_paned_add1(GTK_PANED(D), widget); \
		else gtk_paned_add2(GTK_PANED(D), widget);
#endif


void
window_new ()
{
	app->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(app->window), "SampleCat");
	g_signal_connect(G_OBJECT(app->window), "delete_event", G_CALLBACK(on_quit), NULL);
	g_signal_connect(app->window, "destroy", G_CALLBACK(window_on_destroy), NULL);

#ifndef __APPLE__
	gtk_window_set_icon(GTK_WINDOW(app->window), gdk_pixbuf_new_from_xpm_data(samplecat_xpm));
#endif

	int width = atoi(app->config.window_width);
	int height = atoi(app->config.window_height);
	if (width && height) {
		gtk_window_resize(GTK_WINDOW(app->window), width, height);

		// note that the window size is also set in the configure callback.
		// -sometimes setting it here is ignored, sometimes setting it in configure is ignored.
		// -setting the size in configure is actually too late, as the window is drawn first
		//  at the wrong size.
	} else {
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

	// Split the rhs in two
	GtkWidget* r_vpaned = gtk_vpaned_new();
	gtk_paned_set_position(GTK_PANED(r_vpaned), 300);
	gtk_box_pack_start(GTK_BOX(rhs_vbox), r_vpaned, EXPAND_TRUE, TRUE, 0);

	listview__new();
	if(0 && BACKEND_IS_NULL) gtk_widget_set_no_show_all(app->libraryview->widget, true); //dont show main view if no database.

	dbg(2, "making fileview pane...");
	make_fileview_pane();

	//int library_height = MAX(100, height - default_heights.filters - default_heights.file - default_heights.waveform);
	PACK(app->libraryview->scroll, GDL_DOCK_TOP, -1, library_height, "library", "Library", r_vpaned);
	// this addition sets the dock to be wider than the left column. the height is set to be small so that the height will be determined by the requisition.
	PACK(filter, GDL_DOCK_TOP, left_column_width + 5, 10, "search", "Search", window.vbox);

	gtk_box_reorder_child((GtkBox*)window.vbox, filter, 0);

	PACK(window.file_man, GDL_DOCK_BOTTOM, 216, default_heights.file, "files", "Files", main_vpaned);

	gtk_paned_add1(GTK_PANED(pcpaned), window.dir_tree);

	if (play->auditioner && play->auditioner->position && play->auditioner->seek) {
		gtk_paned_add2(GTK_PANED(pcpaned), panels[PANEL_TYPE_PLAYER].widget);
	}

#ifdef USE_OPENGL
	if (app->view_options[SHOW_WAVEFORM].value) {
		extern void ensure_waveform (GtkWidget*);
		ensure_waveform(window.vbox);
		show_waveform(true);
	}
#endif

#if (defined HAVE_JACK)
	/* initially hide player seek bar */
	if (app->view_options[SHOW_PLAYER].value) {
		show_player(true);
	}
#endif
#endif // not USE_GDL

#ifdef USE_GDL
	registry_init();
	for (int p=0;p<G_N_ELEMENTS(panels);p++) {
		Panel_* panel = &panels[p];
		register_gtk_fn(panel->name, panel->new);
	}

	GtkWidget* dock = window.dock = gdl_dock_new();

	window.layout =	gdl_dock_layout_new(GDL_DOCK(dock));
	gchar* cwd = g_get_current_dir();
	window.layout->dirs[0] = g_strdup_printf("%s/layouts", app->configctx.dir);
	window.layout->dirs[1] = g_strdup_printf("%s/samplecat/layouts", SYSCONFDIR);
	window.layout->dirs[2] = g_strdup_printf("%s/layouts", cwd);
	g_free(cwd);

	GtkWidget* dockbar = gdl_dock_bar_new(GDL_DOCK(dock));
	gdl_dock_bar_set_style(GDL_DOCK_BAR(dockbar), GDL_DOCK_BAR_TEXT);

	gtk_box_pack_start(GTK_BOX(window.vbox), dock, EXPAND_TRUE, true, 0);
	gtk_box_pack_start(GTK_BOX(window.vbox), dockbar, EXPAND_FALSE, false, 0);

	dock->requisition.height = 197; // the size must be set directly. using gtk_widget_set_size_request has no imediate effect.
	int allocation = dock->allocation.height;
	dock->allocation.height = 197; // nasty hack
	dock->allocation.height = allocation;

	void item_added (GdlDockMaster* master, GdlDockObject* object, gpointer _)
	{
		dbg(1, "%s %s", G_OBJECT_CLASS_NAME(G_OBJECT_GET_CLASS(object)), object->name);

		Panel_* panel = panel_lookup_by_name(object->name);
		g_return_if_fail(panel);
		g_return_if_fail(!panel->dock_item);

		panel->dock_item = (GtkWidget*)object;
		panel->widget = ((GdlDockItem*)object)->child;

		if (panel->show) {
			panel->show(true);
		}

		if (panel == &panels[PANEL_TYPE_SEARCH]) {
			dbg(1, "TODO disallow resizing of search box");
			g_object_set(object, "resize", false, NULL); // unfortunately gdl-dock does not make any use of this property.
		}
	}
	g_signal_connect(((GdlDockObject*)dock)->master, "dock-item-added", (GCallback)item_added, NULL);

	void _on_layout_changed (GObject* object, gpointer user_data)
	{
		PF;

		if (panels[PANEL_TYPE_INSPECTOR].widget) {
#ifdef USE_OPENGL
			_INSPECTOR->show_waveform = !panels[PANEL_TYPE_WAVEFORM].dock_item || !gdl_dock_item_is_active((GdlDockItem*)panels[PANEL_TYPE_WAVEFORM].dock_item);
#else
			_INSPECTOR->show_waveform = true;
#endif
		}

		g_signal_emit_by_name (app, "layout-changed");
	}
	g_signal_connect(G_OBJECT(((GdlDockObject*)window.dock)->master), "layout-changed", G_CALLBACK(_on_layout_changed), NULL);
#endif

#ifndef USE_GDL
#ifdef HAVE_FFTW3
	if (app->view_options[SHOW_SPECTROGRAM].value) {
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
		if (window.spectrogram) {
#ifdef USE_GDL
			if (gdl_dock_item_is_active((GdlDockItem*)panels[PANEL_TYPE_SPECTROGRAM].dock_item)) {
#else
			if (true) {
#endif
#ifndef USE_OPENGL
				spectrogram_widget_set_file ((SpectrogramWidget*)window.spectrogram, sample->full_path);
#endif
			}
		}
#endif
	}
	g_signal_connect((gpointer)samplecat.model, "selection-changed", G_CALLBACK(window_on_selection_change), NULL);

#ifdef USE_GDL
	void window_on_quit (Application* a, gpointer user_data)
	{
		window_save_layout();

#ifdef WITH_VALGRIND
														#if 0
		g_clear_pointer(&window.waveform, gtk_widget_destroy);
														#endif
		g_clear_pointer(&window.file_man, gtk_widget_destroy);
		g_clear_pointer(&app->dir_treeview2, vdtree_free);

		gtk_widget_destroy(app->window);
#endif
	}
	g_signal_connect((gpointer)app, "on-quit", G_CALLBACK(window_on_quit), NULL);
#endif

	void store_content_changed (GtkListStore* store, gpointer data)
	{
		int n_results = BACKEND.n_results;
		int row_count = ((SamplecatListStore*)store)->row_count;

		if (0 && row_count < LIST_STORE_MAX_ROWS) {
			statusbar_print(1, "%i samples found.", row_count);
		} else if (!row_count) {
			statusbar_print(1, "no samples found.");
		} else if (n_results < 0) {
			statusbar_print(1, "showing %i sample(s)", row_count);
		} else {
			statusbar_print(1, "showing %i of %i sample(s)", row_count, n_results);
		}
	}
	g_signal_connect(G_OBJECT(samplecat.store), "content-changed", G_CALLBACK(store_content_changed), NULL);

	dnd_setup();

	window_on_layout_changed();

	bool window_on_clicked (GtkWidget* widget, GdkEventButton* event, gpointer user_data)
	{
		if (event->button == 3) {
			if (app->context_menu) {
				gtk_menu_popup(GTK_MENU(app->context_menu), NULL, NULL, NULL, NULL, event->button, event->time);
				return HANDLED;
			}
		}
		return NOT_HANDLED;
	}
	g_signal_connect((gpointer)app->window, "button-press-event", G_CALLBACK(window_on_clicked), NULL);

	void on_worker_progress (gpointer _n)
	{
		int n = GPOINTER_TO_INT(_n);

		if (n) statusbar_print(2, "updating %i items...", n);
		else statusbar_print(2, "");
	}
	worker_register(on_worker_progress);
}


static void
window_on_realise (GtkWidget* win, gpointer user_data)
{
}


static void
window_on_size_request (GtkWidget* widget, GtkRequisition* req, gpointer user_data)
{
	req->height = atoi(app->config.window_height);
}


static void
window_on_allocate (GtkWidget* win, GtkAllocation* allocation, gpointer user_data)
{
	#define SCROLLBAR_WIDTH_HACK 32
	static gboolean done = false;
	static gboolean library_done = false;
#ifndef USE_GDL
	if (!app->libraryview->widget->requisition.width) return;
#endif

	if (!done) {
		//dbg(2, "app->libraryview->widget->requisition: wid=%i height=%i", app->libraryview->widget->requisition.width, app->libraryview->widget->requisition.height);
		//setting window size is now done in window_on_configure
		done = TRUE;
	} else {
		//now reduce the minimum size to allow the user to manually make the window smaller.
		gtk_widget_set_size_request(win, 100, 100);
	}

	static bool did_set_colours = false;
	if (!did_set_colours) {
		did_set_colours = true;

		colour_get_style_bg(&app->bg_colour, GTK_STATE_NORMAL);
		colour_get_style_fg(&app->fg_colour, GTK_STATE_NORMAL);
		colour_get_style_base(&app->base_colour, GTK_STATE_NORMAL);
		colour_get_style_text(&app->text_colour, GTK_STATE_NORMAL);

		set_overview_colour(&app->text_colour, &app->base_colour, &app->bg_colour);

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
#if 0
		dbg(2, "%s %s", gdkcolor_get_hexstring(&app->bg_colour_mod1), gdkcolor_get_hexstring(&app->fg_colour));
#endif
		if (app->fm_view) view_details_set_alt_colours(VIEW_DETAILS(app->fm_view), &app->bg_colour_mod1, &app->fg_colour);

		g_signal_emit_by_name (app, "theme-changed", NULL);
	}

	if (did_set_colours && !library_done && app->libraryview && app->libraryview->widget->requisition.width) {
		g_object_set(app->libraryview->cells.name, "cell-background-gdk", &app->bg_colour_mod1, "cell-background-set", TRUE, NULL);
		g_object_set(app->libraryview->cells.name, "foreground-gdk", &app->fg_colour, "foreground-set", TRUE, NULL);
		library_done = true;
	}
}


static gboolean
window_on_configure (GtkWidget* widget, GdkEventConfigure* event, gpointer user_data)
{
	static gboolean window_size_set = false;
	if (!window_size_set) {
		//take the window size from the config file, or failing that, from the treeview requisition.
		int width = atoi(app->config.window_width);
#ifdef USE_GDL
		// panels not yet created, so widget requisition cannot be used.
		if (!width) width = 640;
#else
		if (!width) width = app->libraryview->widget->requisition.width + SCROLLBAR_WIDTH_HACK;
#endif
		int window_height = atoi(app->config.window_height);
		if (!window_height) window_height = 480; //MIN(app->libraryview->widget->requisition.height, 900);  -- ignore the treeview height, is meaningless

		if (width && window_height) {
			dbg(2, "setting size: width=%i height=%i", width, window_height);
			gtk_window_resize(GTK_WINDOW(app->window), width, window_height);
			window_size_set = true;

			//set the position of the left pane elements.
			//As the allocation is somehow bigger than its container, we just do it v approximately.
/*
			if(window.vpaned && GTK_WIDGET_REALIZED(window.vpaned)){
				//dbg(0, "height=%i %i %i", app->hpaned->allocation.height, app->statusbar->allocation.y, _INSPECTOR->widget->allocation.height);
				guint inspector_y = height - app->hpaned->allocation.y - 210;
				gtk_paned_set_position(GTK_PANED(window.vpaned), inspector_y);
			}
*/
		}

#ifdef USE_GDL
		window_load_layout(app->temp_view ? "File Manager" : app->args.layout ? app->args.layout : "__default__");
		if (_debug_) gdl_dock_print_recursive((GdlDockMaster*)((GdlDockObject*)window.dock)->master);
#else
		if (app->view_options[SHOW_PLAYER].value && panels[PANEL_TYPE_PLAYER].widget) {
			show_player(true);
		}
#endif

#ifdef USE_GDL
		void window_activate_layout()
		{
			PF;

			if (app->temp_view) {

				void select_first_audio ()
				{
					bool done = false;
					DirItem* item;
					ViewIter iter;
					AyyiFilemanager* fm = file_manager__get();
					view_get_iter(fm->view, &iter, 0);
					while (!done && (item = iter.next(&iter))) {
						MIME_type* mime_type = item->mime_type;
						if (mime_type) {
							char* mime_string = g_strdup_printf("%s/%s", mime_type->media_type, mime_type->subtype);
							if (!mimetype_is_unsupported(item->mime_type, mime_string)) {
								gtk_widget_grab_focus(app->fm_view);
								view_cursor_to_iter((ViewIface*)app->fm_view, &iter);
								done = true;
							}
							g_free(mime_string);
						}
					}
				}

				gboolean check_ready (gpointer user_data)
				{
					return file_manager__get()->directory->have_scanned
						? (select_first_audio(), G_SOURCE_REMOVE)
						: G_SOURCE_CONTINUE;
				}

				if (app->temp_view) g_timeout_add(100, check_ready, NULL);
			}
		}
		window_activate_layout();
#endif
		app->context_menu = make_context_menu();
	}

	return false;
}


static gboolean
window_on_destroy (GtkWidget* widget, gpointer user_data)
{
	return false;
}


static GtkWidget*
make_fileview_pane ()
{
	GtkWidget* fman_hpaned = window.file_man = gtk_hpaned_new();
	gtk_paned_set_position(GTK_PANED(fman_hpaned), 210);

	void fman_left (const char* initial_folder)
	{
		void dir_on_select (ViewDirTree* vdt, const gchar* path, gpointer data)
		{
			PF;
			fm__change_to(file_manager__get(), path, NULL);
		}

		gint expand = TRUE;
		ViewDirTree* dir_list = app->dir_treeview2 = vdtree_new(initial_folder, expand); 
		vdtree_set_select_func(dir_list, dir_on_select, NULL); //callback
		gtk_paned_add1(GTK_PANED(fman_hpaned), dir_list->widget);

		void icon_theme_changed (Application* a, char* theme, gpointer _dir_tree)
		{
			vdtree_on_icon_theme_changed((ViewDirTree*)app->dir_treeview2);
		}
		g_signal_connect((gpointer)app, "icon-theme", G_CALLBACK(icon_theme_changed), dir_list);

		make_menu_actions(fm_tree_keys, G_N_ELEMENTS(fm_tree_keys), vdtree_add_menu_item);
	}

	void fman_right (const char* initial_folder)
	{
		GtkWidget* file_view = app->fm_view = file_manager__new_window(initial_folder);
		AyyiFilemanager* fm = file_manager__get();
		gtk_paned_add2(GTK_PANED(fman_hpaned), file_view);
		g_signal_connect(G_OBJECT(fm->view), "cursor-changed", G_CALLBACK(window_on_fileview_row_selected), NULL);

		void window_on_dir_changed (GtkWidget* widget, gpointer data)
		{
			PF;
		}
		g_signal_connect(G_OBJECT(file_manager__get()), "dir_changed", G_CALLBACK(window_on_dir_changed), NULL);

		void icon_theme_changed (Application* a, char* theme, gpointer _dir_tree)
		{
			file_manager__update_all();
		}
		g_signal_connect((gpointer)app, "icon-theme", G_CALLBACK(icon_theme_changed), file_view);

		make_menu_actions(menu_keys, G_N_ELEMENTS(menu_keys), fm__add_menu_item);

		//set up fileview as dnd source:
		gtk_drag_source_set(file_view, GDK_BUTTON1_MASK | GDK_BUTTON2_MASK, dnd_file_drag_types, dnd_file_drag_types_count, GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_ASK);
		g_signal_connect(G_OBJECT(file_view), "drag_data_get", G_CALLBACK(view_details_dnd_get), NULL);
	}

	const char* dir = (app->config.browse_dir[0] && g_file_test(app->config.browse_dir, G_FILE_TEST_IS_DIR))
		? app->config.browse_dir
		: g_get_home_dir();
	fman_left(dir);
	fman_right(dir);

	return fman_hpaned;
}


#ifndef USE_GDL
static void
left_pane2 ()
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
	if (!app->view_options[SHOW_PLAYER].value)
		gtk_widget_set_no_show_all(panels[PANEL_TYPE_PLAYER].widget, true);

	inspector_new();
	gtk_paned_add2(GTK_PANED(window.vpaned), (GtkWidget*)app->inspector);
	//g_signal_connect(app->inspector->widget, "size-allocate", (gpointer)on_inspector_allocate, NULL);

	void on_vpaned_allocate (GtkWidget* widget, GtkAllocation* vp_allocation, gpointer user_data)
	{
		static int previous_height = 0;
		if (!app->inspector || vp_allocation->height == previous_height) return;

		int inspector_requisition = 240;//app->inspector->vbox->requisition.height; //FIXME
		if (!inspector_requisition) return;

		if (!app->inspector->user_height) {
			//user has not specified a height so we have free reign

			int tot_height = vp_allocation->height;
			dbg(1, "req=%i tot_allocation=%i %i", inspector_requisition, tot_height, ((GtkWidget*)app->inspector)->allocation.height);

			//small window - dont allow the inspector to take up more than half the space.
			if (vp_allocation->height < inspector_requisition) {
				gtk_paned_set_position(GTK_PANED(window.vpaned), MAX(tot_height / 2, tot_height - inspector_requisition));
			}

			//large window - dont allow the inspector to be too big
			int current_insp_height = tot_height - gtk_paned_get_position(GTK_PANED(window.vpaned));
			if (current_insp_height > inspector_requisition) {
				gtk_paned_set_position(GTK_PANED(window.vpaned), tot_height - inspector_requisition);
			}
		}
		previous_height = vp_allocation->height;
	}

	g_signal_connect(window.vpaned, "size-allocate", (gpointer)on_vpaned_allocate, NULL);
}
#endif


static GtkWidget*
filters_new ()
{
	static GHashTable* buttons; buttons = g_hash_table_new(NULL, NULL);

	GtkWidget* hbox = gtk_vbox_new(FALSE, 2);

	char* label_text (Observable* filter)
	{
		int len = 22 - strlen(((NamedObservable*)filter)->name);
		char value[20] = {0,};
		g_strlcpy(value, filter->value.c ? filter->value.c : "", len);
		return g_strdup_printf("%s: %s%s", ((NamedObservable*)filter)->name, value, filter->value.c && strlen(filter->value.c) > len ? "..." : "");
	}

	for (int i = 0; i < N_FILTERS; i++) {
		Observable* filter = samplecat.model->filters3[i];
		dbg(2, "  %s", filter->value.c);

		char* text = label_text(filter);
		GtkWidget* button = gtk_button_new_with_label(text);
		g_free(text);
		gtk_box_pack_start(GTK_BOX(hbox), button, EXPAND_FALSE, FALSE, 0);
		gtk_button_set_alignment ((GtkButton*)button, 0.0, 0.5);
		gtk_widget_set_no_show_all(button, !(filter->value.c && strlen(filter->value.c)));

		GtkWidget* icon = gtk_image_new_from_stock(GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU);
		gtk_misc_set_padding((GtkMisc*)icon, 4, 0);
		gtk_button_set_image(GTK_BUTTON(button), icon);
		gtk_widget_set(button, "image-position", GTK_POS_LEFT, NULL);

		g_hash_table_insert(buttons, filter, button);

		void on_filter_button_clicked (GtkButton* button, gpointer _filter)
		{
			observable_string_set((Observable*)_filter, g_strdup(""));
		}

		g_signal_connect(button, "clicked", G_CALLBACK(on_filter_button_clicked), filter);

		void set_label (Observable* filter, GtkWidget* button)
		{
			if (button) {
				char* text = label_text(filter);
				gtk_button_set_label((GtkButton*)button, text);
				g_free(text);
				show_widget_if(button, filter->value.c && strlen(filter->value.c));
			}
		}

		void on_filter_changed (Observable* filter, AGlVal value, gpointer user_data)
		{
			dbg(1, "value=%s", value);
			set_label(filter, g_hash_table_lookup(buttons, filter));
		}

		agl_observable_subscribe (filter, on_filter_changed, NULL);
	}
	return hbox;
}


GtkWidget*
message_panel__add_msg (const gchar* msg, const gchar* stock_id)
{
	//TODO expire old messages. Limit to 5 and add close button?

	GtkWidget* hbox = gtk_hbox_new(FALSE, 2);

	if (stock_id) {
		//const gchar* stock_id = GTK_STOCK_DIALOG_WARNING;
		GtkWidget* icon = gtk_image_new_from_stock(stock_id, GTK_ICON_SIZE_MENU);
		gtk_widget_set_size_request(icon, 16, -1);
		gtk_box_pack_start((GtkBox*)hbox, icon, FALSE, FALSE, 2);
	}

	GtkWidget* label = gtk_label_new(msg);
	gtk_box_pack_start((GtkBox*)hbox, label, FALSE, FALSE, 2);

	gtk_box_pack_start((GtkBox*)app->msg_panel, hbox, FALSE, FALSE, 2);
	return hbox;
}


#ifndef USE_GDL
static GtkWidget*
message_panel__new ()
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

	if (!BACKEND_IS_NULL) gtk_widget_set_no_show_all(app->msg_panel, true); //initially hidden.
	return vbox;
}
#endif


static void
delete_selected_rows ()
{
	GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(app->libraryview->widget));

	GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(app->libraryview->widget));
	GList* selectionlist = gtk_tree_selection_get_selected_rows(selection, &(model));
	if (!selectionlist) { perr("no files selected?\n"); return; }
	dbg(1, "%i rows selected.", g_list_length(selectionlist));

	statusbar_print(1, "deleting %i files...", g_list_length(selectionlist));

	GList* selected_row_refs = NULL;

	//get row refs for each selected row before the list is modified:
	GList* l = selectionlist;
	for (;l;l=l->next) {
		GtkTreePath* treepath_selection = l->data;

		GtkTreeRowReference* row_ref = gtk_tree_row_reference_new(GTK_TREE_MODEL(samplecat.store), treepath_selection);
		selected_row_refs = g_list_prepend(selected_row_refs, row_ref);
	}
	g_list_free(selectionlist);

	int n = 0;
	GtkTreePath* path;
	GtkTreeIter iter;
	l = selected_row_refs;
	for (;l;l=l->next) {
		GtkTreeRowReference* row_ref = l->data;
		if ((path = gtk_tree_row_reference_get_path(row_ref))) {

			if (gtk_tree_model_get_iter(model, &iter, path)) {
				gchar* fname;
				int id;
				gtk_tree_model_get(model, &iter, COL_NAME, &fname, COL_IDX, &id, -1);

				if (!samplecat_model_remove(samplecat.model, id)) return;

				gtk_list_store_remove(samplecat.store, &iter);
				n++;

			} else perr("bad iter!\n");
			gtk_tree_path_free(path);
		} else perr("cannot get path from row_ref!\n");
	}
	g_list_free(selected_row_refs); //FIXME free the row_refs?

	statusbar_print(1, "%i files deleted", n);
}


static void
window_on_fileview_row_selected (GtkTreeView* treeview, gpointer user_data)
{
	//a filesystem file has been clicked on.
	PF;

	AyyiFilemanager* fm = file_manager__get();

	gchar* full_path = NULL;
	DirItem* item;
	ViewIter iter;
	view_get_iter(fm->view, &iter, 0);
	while ((item = iter.next(&iter))) {
		if (view_get_selected(fm->view, &iter)) {
			full_path = g_build_filename(fm->real_path, item->leafname, NULL);
			break;
		}
	}
	if (!full_path) return;

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
	if (s) {
		s->online = true;
		samplecat_model_set_selection (samplecat.model, s);
		sample_unref(s);
	}
}


static void
make_menu_actions (Accel keys[], int count, void (*add_to_menu)(GtkAction*))
{
	// take the raw definitions and create actions and (optionally) menu items for them.

	GtkActionGroup* group = gtk_action_group_new("File Manager");
	accel_group = gtk_accel_group_new();

	for (int k=0;k<count;k++) {
		Accel* key = &keys[k];

		GtkAction* action = gtk_action_new(key->name, key->name, "Tooltip", key->stock_item ? key->stock_item->stock_id : "gtk-file");
		gtk_action_group_add_action(GTK_ACTION_GROUP(group), action);

		GClosure* closure = g_cclosure_new(G_CALLBACK(key->callback), key->user_data, NULL);
		g_signal_connect_closure(G_OBJECT(action), "activate", closure, FALSE);
		gchar path[64]; sprintf(path, "<%s>/Categ/%s", gtk_action_group_get_name(GTK_ACTION_GROUP(group)), key->name);
#if 0
		gtk_accel_group_connect(accel_group, key->key[0].code, key->key[0].mask, GTK_ACCEL_MASK, closure);
#else
		gtk_accel_group_connect_by_path(accel_group, path, closure);
#endif
		gtk_accel_map_add_entry(path, key->key[0].code, key->key[0].mask);
		gtk_action_set_accel_path(action, path);
		gtk_action_set_accel_group(action, accel_group);

		if(add_to_menu) add_to_menu(action);
	}
}


#ifdef HAVE_FFTW3
static GtkWidget*
spectrogram_new ()
{
	return
#ifdef USE_OPENGL
		(GtkWidget*)spectrogram_area_new();
#else
        (GtkWidget*)spectrogram_widget_new();
#endif
}


void
show_spectrogram (bool enable)
{
	if (enable && !window.spectrogram) {
#ifdef USE_GDL
		window.spectrogram = panels[PANEL_TYPE_SPECTROGRAM].widget;
#else
		extern void gl_spectrogram_set_gl_context (GdkGLContext*);

		gl_spectrogram_set_gl_context(agl_get_gl_context());
		window.spectrogram = panels[PANEL_TYPE_SPECTROGRAM].new();
#ifdef USE_OPENGL
		gtk_widget_set_size_request(window.spectrogram, 100, 100);
#endif
		gtk_box_pack_start(GTK_BOX(window.vbox), window.spectrogram, EXPAND_TRUE, FILL_TRUE, 0);
#endif

		Sample* selection = samplecat.model->selection;
		if (selection) {
			dbg(1, "selection=%s", selection->full_path);
#ifndef USE_OPENGL
			spectrogram_widget_set_file((SpectrogramWidget*)window.spectrogram, selection->full_path);
#endif
		}
	}

	if (window.spectrogram) {
		show_widget_if(window.spectrogram, enable);
	}

	window_on_layout_changed();
}
#endif


void
show_filemanager (bool enable)
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
show_player (bool enable)
{
#ifdef USE_GDL
	show_widget_if(panels[PANEL_TYPE_PLAYER].widget, enable);
#else
	show_widget_if(panels[PANEL_TYPE_PLAYER].widget, enable);
#endif

	player_control_on_show_hide(enable);
}


static void
window_on_layout_changed ()
{
#ifndef USE_GDL
	//what is the height of the inspector?

	if (app->inspector) {
		GtkWidget* widget;
		if ((widget = (GtkWidget*)app->inspector)) {
			int tot_height = window.vpaned->allocation.height;
			int max_auto_height = tot_height / 2;
			dbg(1, "inspector_height=%i tot=%i", widget->allocation.height, tot_height);
			if (widget->allocation.height < app->inspector->preferred_height
					&& widget->allocation.height < max_auto_height) {
				int inspector_height = MIN(max_auto_height, app->inspector->preferred_height);
				dbg(1, "setting height : %i/%i", tot_height - inspector_height, inspector_height);
				gtk_paned_set_position(GTK_PANED(window.vpaned), tot_height - inspector_height);
			}
		}
	}
#endif

	// scroll to current dir in the directory list
	if (app->dir_treeview2) vdtree_set_path(app->dir_treeview2, app->dir_treeview2->path);
}


static void
k_delete_row (GtkAccelGroup* _, gpointer user_data)
{
	delete_selected_rows();
}


#ifdef USE_GDL
static void
k_show_layout_manager (GtkAccelGroup* _, gpointer user_data)
{
	PF;
	gdl_dock_layout_run_manager(window.layout);
}


static void
window_load_layout (const char* layout_name)
{
	// Try to load xml file from the users config dir, or fallback to using the built in default xml

	PF;
	bool have_layout = false;

#ifdef GDL_DOCK_XML_FALLBACK
	bool _load_xml_layout (const char* name)
	{
		if (gdl_dock_layout_load_layout(window.layout, name)) // name must match file contents
			return true;
		else
			g_warning ("Loading layout failed");
		return false;
	}
#endif

#if 0
	bool _load_layout_from_file (const char* path, const char* name)
	{
		bool ok = false;
		if(gdl_dock_layout_load_from_yaml_file(window.layout, path)){
			ok = true;
		}
		else gwarn("failed to load %s", path);
		return ok;
	}
#endif

	GError* error = NULL;
	for (int i=0;i<N_LAYOUT_DIRS;i++) {
		const char* dir = window.layout->dirs[i];
		char* path = g_strdup_printf("%s/%s.yaml", dir, layout_name);
		if (g_file_test(path, G_FILE_TEST_EXISTS)) {
			if ((have_layout = gdl_dock_layout_load_from_yaml_file(window.layout, path))) {
				g_free(path);
				break;
			}
		}
		g_free(path);
	}

#ifdef GDL_DOCK_XML_FALLBACK
	if (!have_layout) {
		// fallback to using legacy xml file

		GDir* dir = g_dir_open(window.layout->dirs[0], 0, &error);
		if (!error) {
			const gchar* filename;
			while ((filename = g_dir_read_name(dir)) && !have_layout) {
				if (g_str_has_suffix(filename, ".xml")) {
					gchar* name = layout_name && strlen(layout_name)
						? g_strdup(layout_name)
						: g_strndup(filename, strlen(filename) - 4);
					char* path = g_strdup_printf("%s/layouts/%s.xml", app->configctx.dir, name);
					if (gdl_dock_layout_load_from_xml_file(window.layout, path)) {
						if (!strcmp(name, "__default__")) { // only activate one layout
							have_layout = _load_xml_layout(name);
						}
					}
					g_free(path);
				}
			}
		}
	}
#endif

	if (!have_layout) {
		if (app->temp_view) {
			perr("unable to find File Manger layout");
			exit(1);
		}

		dbg(1, "using default_layout");
		if (gdl_dock_layout_load_from_string (window.layout, default_layout)) {
		}
	}
}


static void
window_save_layout ()
{
	PF;
	g_return_if_fail(window.layout);

	char* dir = g_build_filename(app->configctx.dir, "layouts", NULL);
	if (!g_mkdir_with_parents(dir, 488)) {

#ifdef GDL_DOCK_YAML
		char* filename = g_build_filename(app->configctx.dir, "layouts", "__default__.yaml", NULL);
#else
		char* filename = g_build_filename(app->configctx.dir, "layouts", "__default__.xml", NULL);
#endif
		if (gdl_dock_layout_save_to_file(window.layout, filename)) {
		}
		g_free(filename);

	} else gwarn("failed to create layout directory: %s", dir);

	g_free(dir);
}
#endif


#ifdef USE_GDL
static Panel_*
panel_lookup_by_name (const char* name)
{
	for (int i=0;i<PANEL_TYPE_MAX;i++) {
		Panel_* panel = &panels[i];
		if (!strcmp(panel->name, name)) {
			return panel;
		}
	}
	return NULL;
}
#endif
