/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2007-2020 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/

static void menu__add_to_db      (GtkMenuItem*, gpointer);
static void menu__add_dir_to_db  (GtkMenuItem*, gpointer);
static void menu__play           (GtkMenuItem*, gpointer);


static GtkWidget*
make_context_menu ()
{
	void menu_delete_row (GtkMenuItem* widget, gpointer user_data)
	{
		delete_selected_rows();
	}

	/** sync the selected catalogue row with the filesystem. */
	void
	menu_update_rows (GtkWidget* widget, gpointer user_data)
	{
		PF;
		gboolean force_update = true; //(GPOINTER_TO_INT(user_data)==2) ? true : false; // NOTE - linked to order in menu_def[]

		GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(app->libraryview->widget));
		GList* selectionlist = gtk_tree_selection_get_selected_rows(selection, NULL);
		if(!selectionlist) return;
		dbg(2, "%i rows selected.", g_list_length(selectionlist));

		int i;
		for(i=0;i<g_list_length(selectionlist);i++){
			GtkTreePath* treepath = g_list_nth_data(selectionlist, i);
			Sample* sample = samplecat_list_store_get_sample_by_path(treepath);
			if(do_progress(0, 0)) break; // TODO: set progress title to "updating"
			samplecat_model_refresh_sample (samplecat.model, sample, force_update);
			statusbar_print(1, "online status updated (%s)", sample->online ? "online" : "not online");
			sample_unref(sample);
		}
		hide_progress();
		g_list_free(selectionlist);
	}

	void menu_play_all (GtkWidget* widget, gpointer user_data)
	{
		application_play_all();
	}

	void menu_play_stop (GtkWidget* widget, gpointer user_data)
	{
		player_stop();
	}

	void
	toggle_loop_playback (GtkMenuItem* widget, gpointer user_data)
	{
		PF;
		play->config.loop = !play->config.loop;
	}

	void toggle_recursive_add (GtkMenuItem* widget, gpointer user_data)
	{
		PF;
		app->config.add_recursive = !app->config.add_recursive;
	}

	MenuDef menu_def[] = {
		{"Delete",         G_CALLBACK(menu_delete_row),         GTK_STOCK_DELETE},
		{"Update",         G_CALLBACK(menu_update_rows),        GTK_STOCK_REFRESH},
	#if 0 // force is now the default update. Is there a use case for 2 choices?
		{"Force Update",   G_CALLBACK(update_rows),             GTK_STOCK_REFRESH},
	#endif
		{"Reset Colours",  G_CALLBACK(listview__reset_colours), GTK_STOCK_OK},
		{"Edit tags",      G_CALLBACK(listview__edit_row),      GTK_STOCK_EDIT},
#if 0
		{"Open",           G_CALLBACK(listview__edit_row),      GTK_STOCK_OPEN},
		{"Open Directory", G_CALLBACK(NULL),                    GTK_STOCK_OPEN},
#endif
		{"-",                                                                       },
		{"Play All",       G_CALLBACK(menu_play_all),           GTK_STOCK_MEDIA_PLAY},
		{"Stop Playback",  G_CALLBACK(menu_play_stop),          GTK_STOCK_MEDIA_STOP},
		{"-",                                                                       },
		{"View",           G_CALLBACK(NULL),                    GTK_STOCK_PREFERENCES},
	#ifdef USE_GDL
		{"Layouts",        G_CALLBACK(NULL),                    GTK_STOCK_PROPERTIES},
	#endif
		{"Prefs",          G_CALLBACK(NULL),                    GTK_STOCK_PREFERENCES},
	};

	static struct
	{
		GtkWidget* play_all;
		GtkWidget* stop_playback;
		GtkWidget* loop_playback;
	} widgets;

	GtkWidget* menu = gtk_menu_new();

	add_menu_items_from_defn(menu, G_N_ELEMENTS(menu_def), menu_def, NULL);

	GList* menu_items = gtk_container_get_children((GtkContainer*)menu);
	GList* last = g_list_last(menu_items);

	widgets.play_all = g_list_nth_data(menu_items, 5);
	widgets.stop_playback = g_list_nth_data(menu_items, 6);
	gtk_widget_set_sensitive(widgets.play_all, false);
	gtk_widget_set_sensitive(widgets.stop_playback, false);

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
		void set_view_state (GtkMenuItem* item, Panel_* panel, bool visible)
		{
			if(panel->handler){
				g_signal_handler_block(item, panel->handler);
				gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), visible);
				g_signal_handler_unblock(item, panel->handler);
			}
			dbg(2, "value=%i", visible);
		}

		void toggle_view (GtkMenuItem* menu_item, gpointer _panel_type)
		{
			PF;
			Panel_* panel = &panels[GPOINTER_TO_INT(_panel_type)];
			GdlDockItem* item = (GdlDockItem*)panel->dock_item;
			if(item){
				if(gdl_dock_item_is_active(item))
					gdl_dock_item_hide_item(item);
				else
					gdl_dock_item_show_item(item);
			} else {
				panel->widget = panel->new();
				GtkWidget* dock_item = gdl_dock_item_new (panel->name, panel->name, GDL_DOCK_ITEM_BEH_LOCKED);
				gtk_container_add(GTK_CONTAINER(dock_item), panel->widget);

				GdlDockObject* parent = (GdlDockObject*)window.dock;

				GtkAllocation allocation;
				gtk_widget_get_allocation((GtkWidget*)parent, &allocation);
				GdlDockPlacement placement = panel->orientation == GTK_ORIENTATION_HORIZONTAL
					? GDL_DOCK_BOTTOM
					: allocation.width > allocation.height ? GDL_DOCK_RIGHT : GDL_DOCK_BOTTOM;

				GDL_DOCK_OBJECT_GET_CLASS(parent)->dock(parent, (GdlDockObject*)dock_item, placement, NULL);

				gtk_widget_show_all(panel->widget);
				g_signal_emit_by_name(((GdlDockObject*)window.dock)->master, "dock-item-added", dock_item);
			}
		}
#else
		void set_view_toggle_state (GtkMenuItem* item, ViewOption* option)
		{
			option->value = !option->value;
			gulong sig_id = g_signal_handler_find(item, G_SIGNAL_MATCH_FUNC, 0, 0, 0, option->on_toggle, NULL);
			if(sig_id){
				g_signal_handler_block(item, sig_id);
				gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), option->value);
				g_signal_handler_unblock(item, sig_id);
			}
		}

		void toggle_view (GtkMenuItem* item, gpointer _option)
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
		void add_item (GtkMenuShell* parent, const char* name, GCallback callback, gpointer user_data)
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

			void layout_activate (GtkMenuItem* item, gpointer _)
			{
				gdl_dock_layout_load_layout(window.layout, current_layout = gtk_menu_item_get_label(item));

				GList* menu_items = gtk_container_get_children((GtkContainer*)sub);
				gtk_widget_set_sensitive(g_list_last(menu_items)->data, true); // enable the Save menu
				g_list_free(menu_items);

				statusbar_print(1, "Layout %s loaded", current_layout);
			}

			void layout_save (GtkMenuItem* item, gpointer _)
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

			add_menu_items_from_defn(sub, 1, (MenuDef[]){{"Save", (GCallback)layout_save, GTK_STOCK_SAVE}}, NULL);
			GList* children = gtk_container_get_children((GtkContainer*)sub);
			gtk_widget_set_sensitive(g_list_last(children)->data, false);
			g_list_free(children);
		}

		void _view_menu_on_layout_changed (GObject* object, gpointer user_data)
		{
#if 1
			for(int i=0;i<PANEL_TYPE_MAX;i++){
				Panel_* panel = &panels[i];
				GdlDockItem* item = g_hash_table_lookup(((GdlDockMaster*)((GdlDockObject*)window.dock)->master)->dock_objects, panel->name);
				if(item){
					set_view_state((GtkMenuItem*)panel->menu_item, panel, gdl_dock_item_is_active(item));
				}
			}
#else
			GList* items = gdl_dock_get_named_items((GdlDock*)window.dock);
			for(GList* l=items;l;l=l->next){
				GdlDockItem* item = l->data;
				Panel_* panel = NULL;
				if((panel = panel_lookup((GdlDockObject*)item))){
					set_view_state((GtkMenuItem*)panel->menu_item, panel, gdl_dock_item_is_active(item));
				}
			}
#endif
		}
		Idle* idle = idle_new(_view_menu_on_layout_changed, NULL);
		g_signal_connect(G_OBJECT(((GdlDockObject*)window.dock)->master), "layout-changed", (GCallback)idle->run, idle);
#endif
	}

	GtkWidget* menu_item = gtk_check_menu_item_new_with_mnemonic("Add Recursively");
	gtk_menu_shell_append(GTK_MENU_SHELL(sub), menu_item);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item), app->config.add_recursive);
	g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(toggle_recursive_add), NULL);

	widgets.loop_playback = menu_item = gtk_check_menu_item_new_with_mnemonic("Loop Playback");
	gtk_menu_shell_append(GTK_MENU_SHELL(sub), menu_item);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item), play->config.loop);
	g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(toggle_loop_playback), NULL);
	gtk_widget_set_no_show_all(menu_item, true);

	if(themes){
		GtkWidget* theme_menu = gtk_menu_item_new_with_label("Icon Themes");
		gtk_container_add(GTK_CONTAINER(sub), theme_menu);

		GtkWidget* sub_menu = themes->data;
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(theme_menu), sub_menu);
	}

	void menu_on_audio_ready(gpointer _, gpointer __)
	{
		gtk_widget_set_sensitive(widgets.play_all, true);
		gtk_widget_set_sensitive(widgets.stop_playback, true);
		if (play->auditioner->seek) {
			gtk_widget_show(widgets.loop_playback);
		}
	}
	am_promise_add_callback(play->ready, menu_on_audio_ready, NULL);

	gtk_widget_show_all(menu);
	g_list_free(menu_items);
	return menu;
}


static void
menu__add_to_db (GtkMenuItem* menuitem, gpointer user_data)
{
	PF;
	AyyiFilemanager* fm = file_manager__get();

	DirItem* item;
	ViewIter iter;
	view_get_iter(fm->view, &iter, 0);
	while((item = iter.next(&iter))){
		if(view_get_selected(fm->view, &iter)){
			gchar* filepath = g_build_filename(fm->real_path, item->leafname, NULL);
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
menu__add_dir_to_db (GtkMenuItem* menuitem, gpointer user_data)
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


static void
menu__play (GtkMenuItem* menuitem, gpointer user_data)
{
	PF;
	AyyiFilemanager* fm = file_manager__get();

	GList* selected = fm__selected_items(fm);
	GList* l = selected;
	for(;l;l=l->next){
		char* item = l->data;
		dbg(1, "%s", item);
		application_play_path(item);
		g_free(item);
	}
	g_list_free(selected);
}


