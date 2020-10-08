/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2020-2020 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/

#include "config.h"
#include <getopt.h>
#include "gdl/gdl-dock-item.h"
#include "gdl/gdl-dock-master.h"
#include "gdk/gdkkeysyms.h"
#include "debug/debug.h"
#include "icon_theme.h"
#include "file_manager/pixmaps.h"
#include "test/runner.h"
#include "support.h"
#include "panels/library.h"
#include "application.h"
#include "window.h"

static bool search_pending = false;

Application* app = NULL;

TestFn test_1, test_2;

gpointer tests[] = {
	test_1,
	test_2,
};

int n_tests = G_N_ELEMENTS(tests);


int
application_main (int argc, char** argv)
{
	g_log_set_default_handler(log_handler, NULL);

	gtk_init_check(&argc, &argv);

	app = application_new();
	SamplecatModel* model = samplecat.model;

#define ADD_PLAYER(A) app->players = g_list_append(app->players, A)

#ifdef HAVE_JACK
	ADD_PLAYER("jack");
#endif
#ifdef HAVE_AYYIDBUS
	ADD_PLAYER("ayyi");
#endif
#ifdef HAVE_GPLAYER
	ADD_PLAYER("cli");
#endif
	ADD_PLAYER("null");

	GBytes* gtkrc = g_resources_lookup_data ("/samplecat/resources/gtkrc", 0, NULL);
	gtk_rc_parse_string (g_bytes_get_data(gtkrc, 0));
	g_bytes_unref (gtkrc);

	bool player_opt = false;

	type_init();

	if(config_load(&app->configctx, &app->config)){
		g_signal_emit_by_name (app, "config-loaded");
	}

	// Pick a valid icon theme
	// Preferably from the config file, otherwise from the hardcoded list
	const char* themes[] = {NULL, "oxygen", "breeze", NULL};
	themes[0] = g_value_get_string(&app->configctx.options[CONFIG_ICON_THEME]->val);
	const char* theme = find_icon_theme(themes[0] ? &themes[0] : &themes[1]);
	icon_theme_set_theme(theme);

	db_init(
#ifdef USE_MYSQL
		&app->config.mysql
#else
		NULL
#endif
	);

	if (app->config.database_backend && can_use(model->backends, app->config.database_backend)) {
		g_clear_pointer(&model->backends, g_list_free);
		samplecat_model_add_backend(app->config.database_backend);
	}

	if (!player_opt && app->config.auditioner) {
		if(can_use(app->players, app->config.auditioner)){
			g_clear_pointer(&app->players, g_list_free);
			ADD_PLAYER(app->config.auditioner);
		}
	}

	if(player_opt) g_strlcpy(app->config.auditioner, app->players->data, 8);

#ifdef __APPLE__
	GtkOSXApplication* osxApp = (GtkOSXApplication*)
	g_object_new(GTK_TYPE_OSX_APPLICATION, NULL);
#endif
	app->gui_thread = pthread_self();

	icon_theme_init();
	pixmaps_init();

	if (!db_connect()) {
		g_warning("cannot connect to any database.");
#ifdef QUIT_WITHOUT_DB
		on_quit(NULL, GINT_TO_POINTER(EXIT_FAILURE));
#endif
	}

#ifndef DEBUG_NO_THREADS
	worker_thread_init();
#endif

	if(!app->no_gui) window_new();

	if(!samplecat.model->backend.pending){
		application_search();
		search_pending = false;
	}else{
		search_pending = true;
	}

#ifdef USE_AYYI
	ayyi_client_init();
	g_idle_add((GSourceFunc)ayyi_connect, NULL);
#endif

	application_set_ready();

	statusbar_print(2, PACKAGE_NAME". Version "PACKAGE_VERSION);

	return 0;
}


void
on_quit ()
{
	app->temp_view = true;

	if(app->loaded && !app->temp_view){
		config_save(&app->configctx);
		application_quit(app); // emit signal
	}

	if(play->auditioner){
#ifdef HAVE_AYYIDBUS
		extern Auditioner a_ayyidbus;
		if(play->auditioner != & a_ayyidbus){
#else
		if(true){
#endif
			play->auditioner->stop();
		}
		play->auditioner->disconnect();
	}

	if(samplecat.model->backend.disconnect) samplecat.model->backend.disconnect();

#ifdef WITH_VALGRIND
	application_free(app);
	mime_type_clear();
#if 0
	application_free(app);
#endif
	mime_type_clear();
#endif

	dbg (1, "done");
}


void
setup ()
{
	application_main (0, NULL);
}


void
send_key (GdkWindow* window, int keyval, GdkModifierType modifiers)
{
	assert(gdk_test_simulate_key (window, -1, -1, keyval, modifiers, GDK_KEY_PRESS), "%i", keyval);
	assert(gdk_test_simulate_key (window, -1, -1, keyval, modifiers, GDK_KEY_RELEASE), "%i", keyval);
}


void
teardown ()
{
	dbg(1, "sending CTL-Q ...");

	send_key(app->window->window, GDK_KEY_q, GDK_CONTROL_MASK);
}


void
test_1 ()
{
	START_TEST;

	void do_search (gpointer _)
	{
		assert(gtk_widget_get_realized(app->window), "window not realized");

		gboolean _do_search (gpointer _)
		{
#if 0
			extern void print_widget_tree (GtkWidget* widget);
			print_widget_tree(app->window);
#endif

			GtkWidget* search = find_widget_by_name(app->window, "search-entry");

			int n_rows = gtk_tree_model_iter_n_children(gtk_tree_view_get_model((GtkTreeView*)app->libraryview->widget), NULL);
			assert_and_stop(n_rows > 0, "no library items");
#if 0
			send_key(search->window, GDK_KEY_H, 0);
			send_key(search->window, GDK_KEY_E, 0);
			send_key(search->window, GDK_KEY_Return, 0);
#else
			gtk_test_text_set(search, "Hello");
#endif
			gtk_widget_activate(search);

			bool is_empty ()
			{
				return gtk_tree_model_iter_n_children(gtk_tree_view_get_model((GtkTreeView*)app->libraryview->widget), NULL) == 0;
			}

			void then (gpointer _)
			{
				FINISH_TEST;
			}

			wait_for(is_empty, then, NULL);

			return G_SOURCE_REMOVE;
		}
		g_timeout_add(1000, _do_search, NULL);
	}

	bool window_is_open ()
	{
		return gtk_widget_get_realized(app->window);
	}

	wait_for(window_is_open, do_search, NULL);
}


static GdlDockItem*
find_dock_item (const char* name)
{
	typedef struct
	{
		const char*  name;
		GdlDockItem* item;
	} C;

	GtkWidget* get_first_child (GtkWidget* widget)
	{
		GList* items = gtk_container_get_children((GtkContainer*)widget);
		GtkWidget* item = items->data;
		g_list_free(items);
		return item;
	}

	void gdl_dock_layout_foreach_object (GdlDockObject* object, gpointer user_data)
	{
		g_return_if_fail (object && GDL_IS_DOCK_OBJECT (object));

		C* c = user_data;
		char* name  = object->name;

		char* properties = NULL;
		if(!name){
			name = (char*)G_OBJECT_CLASS_NAME(G_OBJECT_GET_CLASS(object));
		}
		if(!strcmp(name, c->name)){
			c->item = (GdlDockItem*)object;
			return;
		}

		if (gdl_dock_object_is_compound (object)) {
			gtk_container_foreach(GTK_CONTAINER(object), (GtkCallback)gdl_dock_layout_foreach_object, c);
		}

		if(properties) g_free(properties);
	}

	GtkWidget* window = app->window;
	GtkWidget* vbox = get_first_child(window);
	GtkWidget* dock = get_first_child(vbox);

	C c = {name};
	gdl_dock_master_foreach_toplevel((GdlDockMaster*)GDL_DOCK_OBJECT(dock)->master, TRUE, (GFunc) gdl_dock_layout_foreach_object, &c);

	return c.item;
}


/*
 *  Hide the Library view via the View menu.
 */
void
test_2 ()
{
	START_TEST;

	guint button = 3;
	if(!gdk_test_simulate_button (app->window->window, 400, 400, button, 0, GDK_BUTTON_PRESS)){
		dbg(0, "click failed");
	}

	bool menu_is_visible (gpointer _)
	{
		return gtk_widget_get_visible(app->context_menu);
	}

	GtkWidget* get_view_menu ()
	{
		GList* menu_items = gtk_container_get_children((GtkContainer*)app->context_menu);
		GtkWidget* item = g_list_nth_data(menu_items, 8);
		assert_null(GTK_IS_MENU_ITEM(item), "not menu item");
		g_list_free(menu_items);

		GList* items = gtk_container_get_children((GtkContainer*)item);
		for(GList* l=items;l;l=l->next){
			assert_null(!strcmp(gtk_label_get_text(l->data), "View"), "Expected View menu");
		}
		g_list_free(items);

		return item;
	}

	void then (gpointer _)
	{
		// context menu is now open

		GtkWidget* item = get_view_menu();

		GtkWidget* submenu = gtk_menu_item_get_submenu((GtkMenuItem*)item);
		assert(!gtk_widget_get_visible(submenu), "submenu unexpectedly visible");

		void click_on_item (GtkWidget* item)
		{
			guint button = 1;
			if(!gdk_test_simulate_button (item->window, item->allocation.x + 10, item->allocation.y + 5, button, 0, GDK_BUTTON_PRESS)){
				dbg(0, "click failed");
			}
		}

		// open the View submenu
		click_on_item(item);

		bool submenu_is_visible (gpointer _)
		{
			GtkWidget* item = get_view_menu();
			GtkWidget* submenu = gtk_menu_item_get_submenu((GtkMenuItem*)item);
			return gtk_widget_get_visible(submenu);
		}

		void on_submenu_visible (gpointer _)
		{
			GtkWidget* item = get_view_menu();
			GtkWidget* submenu = gtk_menu_item_get_submenu((GtkMenuItem*)item);
			assert(gtk_widget_get_visible(submenu), "submenu not visible");

			// library
			GtkWidget* library_item = NULL;
			GList* items = gtk_container_get_children((GtkContainer*)submenu);
			for(GList* l=items;l;l=l->next){
				GtkWidget* panel_item = l->data;
				assert(GTK_IS_WIDGET(panel_item), "expected GtkWidget");
				assert((GTK_IS_CHECK_MENU_ITEM(panel_item)), "expected GtkCheckMenuItem");
				assert(gtk_check_menu_item_get_active((GtkCheckMenuItem*)panel_item), "expected GtkCheckMenuItem");
				library_item = panel_item;
				break;
			}
			g_list_free(items);


			bool library_is_visible (gpointer _)
			{
				GdlDockItem* item = find_dock_item("Library");
				return item && gtk_widget_get_visible((GtkWidget*)item);
			}

			bool library_not_visible (gpointer _)
			{
				GdlDockItem* item = find_dock_item("Library");
				return !item || !gtk_widget_get_visible((GtkWidget*)item);
			}

			void on_library_hide (gpointer _)
			{

				FINISH_TEST;
			}

			assert(library_is_visible(NULL), "expected library visible");

			// TODO why menu not closed ?
			click_on_item(library_item);
			gtk_menu_item_activate((GtkMenuItem*)library_item);
			wait_for(library_not_visible, on_library_hide, NULL);
		}

		wait_for(submenu_is_visible, on_submenu_visible, NULL);
	}

	wait_for(menu_is_visible, then, NULL);
}

