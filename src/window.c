/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
 | copyright (C) 2007-2023 Tim Orford <tim@orford.org>                  |
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
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "gdl/gdl-dock-layout.h"
#include "gdl/gdl-dock-bar.h"
#include "gdl/gdl-dock-paned.h"
#include "gdl/registry.h"
#include "debug/debug.h"
#include "gtk/menu.h"
#ifdef GTK4_TODO
#include "gtk/gimpactiongroup.h"
#endif
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
#include "progress_dialog.h"
#include "player.h"
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
#ifdef GTK4_TODO
#ifndef __APPLE__
#include "icons/samplecat.xpm"
#endif
#endif

#include "../layouts/layouts.c"

#include "filters.c"

#ifdef GTK4_TODO
extern void on_quit              (GtkMenuItem*, gpointer);
#endif

#define BACKEND samplecat.model->backend
#define _INSPECTOR ((Inspector*)panels[PANEL_TYPE_INSPECTOR].widget)

typedef GtkWidget* (NewPanelFn)();
typedef void       (ShowPanelFn)(bool);

extern NewPanelFn dir_panel_new, spectrogram_area_new,
	fileview_new,
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
   GType          gtype;
   DockParameter* params;
   GtkWidget*     widget;
   GtkWidget*     dock_item;

   struct {
      GAction*    action;
      GMenuItem*  item;
   }              menu;
} Panel;

Panel panels[] = {
   {"Library",     listview__new},
   {"Search",      },
   {"Tags",        },
   {"Filters",     filters_new},
   {"Inspector",   },
   {"Directories", dir_panel_new},
   {"Filemanager", fileview_new},
   {"Player",      player_control_new, player_control_on_show_hide},
#ifdef USE_OPENGL
   {"Waveform",    waveform_panel_new, show_waveform},
#endif
#ifdef HAVE_FFTW3
#ifdef USE_OPENGL
   {"Spectrogram", spectrogram_area_new, show_spectrogram, GTK_ORIENTATION_HORIZONTAL},
#else
   {"Spectrogram", spectrogram_widget_new, show_spectrogram, GTK_ORIENTATION_HORIZONTAL},
#endif
#endif
#ifdef ROTATOR
   {"Rotator",     _rotator_new},
#endif
};

#include "register.c"

struct _window {
   GtkWidget*     vbox;
   GtkWidget*     dock;
   GdlDockLayout* layout;
#ifndef USE_OPENGL
   GtkWidget*     spectrogram;
#endif
} window = {0,};

#ifdef GTK4_TODO
static gboolean   window_on_destroy               (GtkWidget*, gpointer);
#endif
static void       window_on_realise               (GtkWidget*, gpointer);
#ifdef GTK4_TODO
static void       window_on_size_request          (GtkWidget*, GtkRequisition*, gpointer);
static void       window_on_allocate              (GtkWidget*, GtkAllocation*, gpointer);
#endif
static gboolean   window_on_configure             (GtkWidget*, gpointer);

static Panel*    panel_lookup_by_name            (const char*);
static Panel*    panel_lookup_by_gtype           (GType);

#include "menu.c"

#ifdef GTK4_TODO
static void       make_menu_actions               (struct _accel[], int, void (*add_to_menu)(GtkAction*));
#endif
static void       window_on_layout_changed        ();

#ifdef GTK4_TODO
static void       k_delete_row                    (GtkAccelGroup*, gpointer);
#endif
#ifdef GTK4_TODO
static void       k_show_layout_manager           (GtkAccelGroup*, gpointer);
#endif
static void       window_load_layout              (const char*);
static void       window_save_layout              ();

#ifdef ROTATOR
GtkWidget*
_rotator_new ()
{
	return rotator_new_with_model(GTK_TREE_MODEL(samplecat.store));
}
#endif

#ifdef GTK4_TODO
Accel menu_keys[] = {
	{"Add to database",NULL,        {{(char)'a',    0               },  {0, 0}}, menu__add_to_db,       GINT_TO_POINTER(0)},
	{"Play"           ,NULL,        {{(char)'p',    0               },  {0, 0}}, menu__play,            NULL              },
};

Accel window_keys[] = {
	{"Quit",           NULL,        {{(char)'q',    GDK_CONTROL_MASK},  {0, 0}}, on_quit,               GINT_TO_POINTER(0)},
	{"Close",          NULL,        {{(char)'w',    GDK_CONTROL_MASK},  {0, 0}}, on_quit,               GINT_TO_POINTER(0)},
	{"Delete",         NULL,        {{GDK_Delete,   0               },  {0, 0}}, k_delete_row,          NULL},
	{"Layout Manager", NULL,        {{(char)'l',    GDK_CONTROL_MASK},  {0, 0}}, k_show_layout_manager, NULL},
};

Accel fm_tree_keys[] = {
    {"Add to database",NULL,        {{(char)'y',    0               },  {0, 0}}, menu__add_dir_to_db,   NULL},
};

static GtkAccelGroup* accel_group = NULL;
#endif

const char* preferred_width = "preferred-width";
const char* preferred_height = "preferred-height";


GtkWidget*
window_new (GtkApplication* gtk, gpointer user_data)
{
	GtkWindow* win = (GtkWindow*)gtk_application_window_new (gtk);
	gtk_window_set_title(GTK_WINDOW(win), "SampleCat");
#ifdef GTK4_TODO
	g_signal_connect(G_OBJECT(win), "delete_event", G_CALLBACK(on_quit), NULL);
	g_signal_connect(window, "destroy", G_CALLBACK(window_on_destroy), NULL);

#ifndef __APPLE__
	gtk_window_set_icon(GTK_WINDOW(win), gdk_pixbuf_new_from_xpm_data(samplecat_xpm));
#endif
#endif

	int width = atoi(app->config.window_width);
	int height = atoi(app->config.window_height);
	if (width && height) {
		gtk_window_set_default_size (GTK_WINDOW (win), width, height);

		// note that the window size is also set in the configure callback.
		// -sometimes setting it here is ignored, sometimes setting it in configure is ignored.
		// -setting the size in configure is actually too late, as the window is drawn first
		//  at the wrong size.
	} else {
		// we need an approx size in the case where there is no config file. relative panel sizes will be slightly out.
		width = 450;
		height = 600;
	}

	window.vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_window_set_child(GTK_WINDOW(win), window.vbox);

	register_panels();

	GtkWidget* dock = window.dock = gdl_dock_new();

	window.layout =	gdl_dock_layout_new(gdl_dock_object_get_master(GDL_DOCK_OBJECT(dock)));
	g_autofree gchar* cwd = g_get_current_dir();
	window.layout->dirs[0] = g_strdup_printf("%s/layouts", app->configctx.dir);
	window.layout->dirs[1] = g_strdup_printf("%s/samplecat/layouts", SYSCONFDIR);
	window.layout->dirs[2] = g_strdup_printf("%s/layouts", cwd);

	GtkWidget* dockbar = gdl_dock_bar_new(gdl_dock_object_get_master(GDL_DOCK_OBJECT(dock)));
	gdl_dock_bar_set_style(GDL_DOCK_BAR(dockbar), GDL_DOCK_BAR_TEXT);

	gtk_box_append(GTK_BOX(window.vbox), dock);
	gtk_box_append(GTK_BOX(window.vbox), dockbar);

#ifdef GTK4_TODO
	dock->requisition.height = 197; // the size must be set directly. using gtk_widget_set_size_request has no imediate effect.
	int allocation = dock->allocation.height;
	dock->allocation.height = 197; // nasty hack
	dock->allocation.height = allocation;
#endif

	void item_added (GdlDockMaster* master, GdlDockObject* object, gpointer _)
	{
		dbg(1, "%s %s", G_OBJECT_CLASS_NAME(G_OBJECT_GET_CLASS(object)), gdl_dock_object_get_name(object));

		const char* name = gdl_dock_object_get_name(object);
		Panel* panel = name ? panel_lookup_by_name(name) : panel_lookup_by_gtype(G_OBJECT_TYPE(object));
		g_return_if_fail(panel);
		g_return_if_fail(!panel->dock_item);

		panel->dock_item = (GtkWidget*)object;
		panel->widget = gdl_dock_item_get_child((GdlDockItem*)object);

		if (panel->show) {
			panel->show(true);
		}
	}
	g_signal_connect(gdl_dock_object_get_master((GdlDockObject*)dock), "dock-item-added", (GCallback)item_added, NULL);

	void _on_layout_changed (GObject* object, gpointer user_data)
	{
		PF;

#ifdef GTK4_TODO
		if (panels[PANEL_TYPE_INSPECTOR].widget) {
#ifdef USE_OPENGL
			_INSPECTOR->show_waveform = !panels[PANEL_TYPE_WAVEFORM].dock_item || !gdl_dock_item_is_active((GdlDockItem*)panels[PANEL_TYPE_WAVEFORM].dock_item);
#else
			_INSPECTOR->show_waveform = true;
#endif
		}
#endif

		g_signal_emit_by_name (app, "layout-changed");
	}
	g_signal_connect(G_OBJECT(gdl_dock_object_get_master((GdlDockObject*)window.dock)), "layout-changed", G_CALLBACK(_on_layout_changed), NULL);

	{
		GtkWidget* hbox_statusbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
		gtk_box_append(GTK_BOX(window.vbox), hbox_statusbar);

		GtkWidget* statusbar = ((Application*)app)->statusbar = gtk_statusbar_new();
		//gtk_statusbar_set_has_resize_grip(GTK_STATUSBAR(statusbar), TRUE);	//why does give a warning??????
#ifdef GTK4_TODO
		gtk_statusbar_set_has_resize_grip(GTK_STATUSBAR(statusbar), FALSE);
		gtk_container_set_border_width(GTK_CONTAINER(statusbar), 5);
#endif
		gtk_box_append(GTK_BOX(hbox_statusbar), statusbar);

		GtkWidget* statusbar2 = ((Application*)app)->statusbar2 = gtk_statusbar_new();
#ifdef GTK4_TODO
		gtk_statusbar_set_has_resize_grip(GTK_STATUSBAR(statusbar2), FALSE);
		gtk_container_set_border_width(GTK_CONTAINER(statusbar2), 5);
#endif
		gtk_box_append(GTK_BOX(hbox_statusbar), statusbar2);
	}

	g_signal_connect(G_OBJECT(win), "realize", G_CALLBACK(window_on_realise), NULL);
#ifdef GTK4_TODO
	g_signal_connect(G_OBJECT(win), "size-request", G_CALLBACK(window_on_size_request), NULL);
	g_signal_connect(G_OBJECT(win), "size-allocate", G_CALLBACK(window_on_allocate), NULL);
#endif
	g_signal_connect(G_OBJECT(win), "map", G_CALLBACK(window_on_configure), NULL);

#ifdef GTK4_TODO
	GtkAccelGroup* accel_group = gtk_accel_group_new();
	gboolean mnemonics = false;
	GimpActionGroup* action_group = gimp_action_group_new("Samplecat-window", "Samplecat-window", "gtk-paste", mnemonics, NULL, (GimpActionGroupUpdateFunc)NULL);
	make_accels(accel_group, action_group, window_keys, G_N_ELEMENTS(window_keys), NULL);
	gtk_window_add_accel_group(GTK_WINDOW(app->window), accel_group);
#endif

	gtk_widget_show((GtkWidget*)win);

#ifndef USE_OPENGL
	void window_on_selection_change (SamplecatModel* m, Sample* sample, gpointer user_data)
	{
		PF;
#ifdef HAVE_FFTW3
		if (window.spectrogram) {
			if (gdl_dock_item_is_active((GdlDockItem*)panels[PANEL_TYPE_SPECTROGRAM].dock_item)) {
				spectrogram_widget_set_file ((SpectrogramWidget*)window.spectrogram, sample->full_path);
#endif
			}
		}
	}
	g_signal_connect((gpointer)samplecat.model, "selection-changed", G_CALLBACK(window_on_selection_change), NULL);
#endif

	void window_on_quit (Application* a, gpointer user_data)
	{
		window_save_layout();

#ifdef WITH_VALGRIND
#ifdef GTK4_TODO
		g_clear_pointer(&window.waveform, gtk_widget_destroy);
		g_clear_pointer(&window.file_man, gtk_widget_unparent);
#endif
		g_clear_pointer(&((Application*)app)->dir_treeview2, vdtree_free);

		gtk_widget_unparent(GTK_WIDGET(gtk_application_get_active_window(GTK_APPLICATION(app))));
#endif
	}
	g_signal_connect((gpointer)app, "on-quit", G_CALLBACK(window_on_quit), NULL);

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

	window_on_layout_changed();

	void click_gesture_pressed (GtkGestureClick *gesture, int n_press, double widget_x, double widget_y, gpointer)
	{
		if (((Application*)app)->context_menu) {
			gtk_popover_popup(GTK_POPOVER(((Application*)app)->context_menu));
		}
	}
	GtkGesture* click = gtk_gesture_click_new ();
	g_signal_connect (click, "pressed", G_CALLBACK (click_gesture_pressed), NULL);
	gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (click), 3);
	gtk_widget_add_controller (GTK_WIDGET (win), GTK_EVENT_CONTROLLER (click));

	void on_worker_progress (gpointer _n)
	{
		int n = GPOINTER_TO_INT(_n);

		if (n) statusbar_print(2, "updating %i items...", n);
		else statusbar_print(2, "");
	}
	worker_register(on_worker_progress);

	return GTK_WIDGET(win);
}


static void
window_on_realise (GtkWidget* win, gpointer user_data)
{
}


#ifdef GTK4_TODO
static void
window_on_size_request (GtkWidget* widget, GtkRequisition* req, gpointer user_data)
{
	req->height = atoi(app->config.window_height);
}
#endif


#ifdef GTK4_TODO
static void
window_on_allocate (GtkWidget* win, GtkAllocation* allocation, gpointer user_data)
{
	#define SCROLLBAR_WIDTH_HACK 32
	static gboolean done = false;
	static gboolean library_done = false;

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

#ifdef GTK4_TODO
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
#endif

		g_signal_emit_by_name (app, "theme-changed", NULL);
	}

#ifdef GTK4_TODO
	if (did_set_colours && !library_done && app->libraryview && app->libraryview->widget->requisition.width) {
#else
	if (did_set_colours && !library_done && app->libraryview) {
#endif
#ifdef GTK4_TODO
		g_object_set(app->libraryview->cells.name, "cell-background-gdk", &app->bg_colour_mod1, "cell-background-set", TRUE, NULL);
		g_object_set(app->libraryview->cells.name, "foreground-gdk", &app->fg_colour, "foreground-set", TRUE, NULL);
#endif
		library_done = true;
	}
}
#endif


static gboolean
window_on_configure (GtkWidget* widget, gpointer user_data)
{
	static gboolean window_size_set = false;
	if (!window_size_set) {
		//take the window size from the config file, or failing that, from the treeview requisition.
		int width = atoi(app->config.window_width);

		// panels not yet created, so widget requisition cannot be used.
		if (!width) width = 640;
		int window_height = atoi(app->config.window_height);
		if (!window_height) window_height = 480; //MIN(app->libraryview->widget->requisition.height, 900);  -- ignore the treeview height, is meaningless

		if (width && window_height) {
			dbg(2, "setting size: width=%i height=%i", width, window_height);
#ifdef GTK4_TODO
			gtk_window_resize(GTK_WINDOW(app->window), width, window_height);
#endif
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

		window_load_layout(app->temp_view ? "File Manager" : app->args.layout ? app->args.layout : "__default__");
#ifdef GTK4_TODO
		if (_debug_) gdl_dock_print((GdlDockMaster*)((GdlDockObject*)window.dock)->master);
#endif

		void window_activate_layout ()
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
#ifdef GTK4_TODO
								gtk_widget_grab_focus(app->fm_view);
								view_cursor_to_iter((ViewIface*)app->fm_view, &iter);
#endif
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

		((Application*)app)->context_menu = make_context_menu(widget);
	}

	return false;
}
#ifdef GTK4_TODO
#endif


#ifdef GTK4_TODO
static gboolean
window_on_destroy (GtkWidget* widget, gpointer user_data)
{
	return false;
}
#endif


#if 0
GtkWidget*
message_panel__add_msg (const gchar* msg, const gchar* stock_id)
{
	//TODO expire old messages. Limit to 5 and add close button?

	GtkWidget* hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);

	if (stock_id) {
#ifdef GTK4_TODO
		//const gchar* stock_id = GTK_STOCK_DIALOG_WARNING;
		GtkWidget* icon = gtk_image_new_from_stock(stock_id, GTK_ICON_SIZE_MENU);
		gtk_widget_set_size_request(icon, 16, -1);
		gtk_box_pack_start((GtkBox*)hbox, icon, FALSE, FALSE, 2);
#endif
	}

	GtkWidget* label = gtk_label_new(msg);
	gtk_box_append((GtkBox*)hbox, label);

	gtk_box_append((GtkBox*)((Application*)app)->msg_panel, hbox);

	return hbox;
}
#endif


#ifdef GTK4_TODO
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
#endif


#ifdef HAVE_FFTW3
void
show_spectrogram (bool enable)
{
	if (enable && !panels[PANEL_TYPE_SPECTROGRAM].widget) {
		Sample* selection = samplecat.model->selection;
		if (selection) {
			dbg(1, "selection=%s", selection->full_path);
#ifndef USE_OPENGL
			spectrogram_widget_set_file((SpectrogramWidget*)window.spectrogram, selection->full_path);
#endif
		}
	}

	if (panels[PANEL_TYPE_SPECTROGRAM].widget) {
		show_widget_if(panels[PANEL_TYPE_SPECTROGRAM].widget, enable);
	}

	window_on_layout_changed();
}
#endif


void
show_filemanager (bool enable)
{
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
}


void
show_player (bool enable)
{
#ifdef GTK4_TODO
	show_widget_if(panels[PANEL_TYPE_PLAYER].widget, enable);

	player_control_on_show_hide(enable);
#endif
}


static void
window_on_layout_changed ()
{
	// scroll to current dir in the directory list
	if (((Application*)app)->dir_treeview2) vdtree_set_path(((Application*)app)->dir_treeview2, ((Application*)app)->dir_treeview2->path);
}


#ifdef GTK4_TODO
static void
k_delete_row (GtkAccelGroup* _, gpointer user_data)
{
	listview__delete_selected();
}
#endif


#ifdef GTK4_TODO
static void
k_show_layout_manager (GtkAccelGroup* _, gpointer user_data)
{
	PF;
	gdl_dock_layout_run_manager(window.layout);
}
#endif


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
		else pwarn("failed to load %s", path);
		return ok;
	}
#endif

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

		GError* error = NULL;
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
			perr("unable to find File Manager layout");
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

	} else pwarn("failed to create layout directory: %s", dir);

	g_free(dir);
}


static Panel*
panel_lookup_by_name (const char* name)
{
	for (int i=0;i<PANEL_TYPE_MAX;i++) {
		Panel* panel = &panels[i];
		if (!strcmp(panel->name, name)) {
			return panel;
		}
	}
	return NULL;
}


static Panel*
panel_lookup_by_gtype (GType type)
{
	for (int i=0;i<PANEL_TYPE_MAX;i++) {
		Panel* panel = &panels[i];
		if (panel->gtype == type) {
			return panel;
		}
	}
	return NULL;
}
