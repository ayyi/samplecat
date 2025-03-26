/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. https://ayyi.github.io/samplecat/    |
 | copyright (C) 2007-2025 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

static void prefs_menu  (GtkWidget*, GMenuModel*, GSimpleActionGroup*);
static void layout_menu (GtkWidget*, GMenuModel*, GSimpleActionGroup*);


static GtkWidget*
make_context_menu (GtkWidget* widget)
{
	GMenuModel* model = (GMenuModel*)g_menu_new ();

	GtkWidget* menu = popover_menu_new_from_model (model);
	gtk_widget_set_parent (menu, widget);
	gtk_popover_set_has_arrow (GTK_POPOVER(menu), false);
	gtk_popover_set_position (GTK_POPOVER(menu), GTK_POS_LEFT);
	gtk_popover_set_mnemonics_visible (GTK_POPOVER(menu), true);

	GSimpleActionGroup* group = g_simple_action_group_new ();
	gtk_widget_insert_action_group (menu, "context-menu", G_ACTION_GROUP(group));

	MenuDef menu_def[] = {
		{"Delete",         "library.delete-rows",   "edit-delete-symbolic"},
		{"Update",         "library.update-rows",   "view-refresh-symbolic"},
		{"Reset Colours",  "library.reset-colours", ""},
		{"Edit tags",      "library.edit-row",      "text-editor-symbolic"},
#if 0
		{"Open",           G_CALLBACK(listview__edit_row),      GTK_STOCK_OPEN},
		{"Open Directory", G_CALLBACK(NULL),                    GTK_STOCK_OPEN},
#endif
		{"-",                                                              },
		{"Play All",       "app.play-all",           "media-playback-start"},
		{"Stop Playback",  "app.player-stop",        "media-playback-stop"},
	};

	add_menu_items_from_defn(menu, model, G_N_ELEMENTS(menu_def), menu_def, NULL);

	//
	// View menu
	//
	{
		GMenuModel* view_model = (GMenuModel*)g_menu_new ();
		g_menu_append_submenu (G_MENU(model), "View", view_model);

		void toggle_view (GSimpleAction* action, GVariant* parameter, gpointer _panel_type)
		{
			PF;
			Panel* panel = &panels[GPOINTER_TO_INT(_panel_type)];
			GdlDockItem* item = (GdlDockItem*)panel->dock_item;
			if (item) {
				if (gtk_widget_is_visible(panel->dock_item))
					gdl_dock_item_hide_item(item);
				else
					gdl_dock_item_show_item(item);
			} else {
				panel->widget = panel->gtype
					? g_object_new(panel->gtype, NULL)
					: panel->new();
				GtkWidget* dock_item = gdl_dock_item_new (panel->name, panel->name, GDL_DOCK_ITEM_BEH_LOCKED);
				gdl_dock_object_add_child(GDL_DOCK_OBJECT(dock_item), panel->widget);

				GdlDockObject* parent = (GdlDockObject*)window.dock;

				int width = gtk_widget_get_width((GtkWidget*)parent);
				int height = gtk_widget_get_height((GtkWidget*)parent);
				GdlDockPlacement placement = panel->orientation == GTK_ORIENTATION_HORIZONTAL
					? GDL_DOCK_BOTTOM
					: width > height ? GDL_DOCK_RIGHT : GDL_DOCK_BOTTOM;

				GDL_DOCK_OBJECT_GET_CLASS(parent)->dock(parent, (GdlDockObject*)dock_item, placement, NULL);

				g_signal_emit_by_name(((GdlDockObject*)window.dock)->master, "dock-item-added", dock_item);
			}
		}

		for (int i=0;i<PANEL_TYPE_MAX;i++) {
			Panel* option = &panels[i];

			char* name = g_strdup(option->name);
			name[0] = g_ascii_tolower(name[0]);
			option->menu.action = (GAction*)g_simple_action_new_stateful(name, NULL, g_variant_new_boolean(false));
			g_action_map_add_action (G_ACTION_MAP (group), G_ACTION (option->menu.action));
			g_signal_connect(G_OBJECT(option->menu.action), "activate", G_CALLBACK(toggle_view), GINT_TO_POINTER(i));

			char* id = g_strdup_printf("context-menu.%s", name);
			option->menu.item = g_menu_item_new(option->name, id);
			g_menu_append_item(G_MENU(view_model), option->menu.item);
			g_object_unref(option->menu.item);
		}

		void _view_menu_on_layout_changed (GObject*, gpointer user_data)
		{
			for (int i=0;i<PANEL_TYPE_MAX;i++) {
				Panel* panel = &panels[i];
				g_action_change_state(panel->menu.action, g_variant_new_boolean(panel->dock_item && gtk_widget_is_visible(panel->dock_item)));
			}
		}
		Idle* idle = idle_new(_view_menu_on_layout_changed, NULL);
		g_signal_connect(G_OBJECT(gdl_dock_object_get_master((GdlDockObject*)window.dock)), "layout-changed", (GCallback)idle->run, idle);
		_view_menu_on_layout_changed(NULL, NULL);
	}

	layout_menu(menu, model, group);
	prefs_menu(menu, model, group);

#ifdef GTK4_TODO
	if (themes) {
		GtkWidget* theme_menu = gtk_menu_item_new_with_label("Icon Themes");
		gtk_container_add(GTK_CONTAINER(sub), theme_menu);

		GtkWidget* sub_menu = themes->data;
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(theme_menu), sub_menu);
	}
#endif

	return menu;
}


GtkWidget*
make_panel_context_menu (GtkWidget* widget, int size, MenuDef defn[size])
{
	GMenuModel* model = (GMenuModel*)g_menu_new ();

	GtkWidget* menu = popover_menu_new_from_model (model);
	gtk_widget_set_parent (menu, widget);
	gtk_popover_set_has_arrow (GTK_POPOVER(menu), false);
	gtk_popover_set_position (GTK_POPOVER(menu), GTK_POS_LEFT);
	gtk_popover_set_mnemonics_visible (GTK_POPOVER(menu), true);

	GSimpleActionGroup* group = g_simple_action_group_new ();
	gtk_widget_insert_action_group (menu, "context-menu", G_ACTION_GROUP(group));

	add_menu_items_from_defn(menu, model, size, defn, NULL);

	layout_menu(menu, model, group);
	prefs_menu(menu, model, group);

	return menu;
}


//
//  Layout menu
//
static void
layout_menu (GtkWidget* menu, GMenuModel* model, GSimpleActionGroup* group)
{
	GMenuModel* layouts_model = (GMenuModel*)g_menu_new ();
	g_menu_append_submenu (G_MENU(model), "Layouts", layouts_model);

	{
		static const char* current_layout = "__default__";

		// Load
		{
			// action
			{
				void load_layout (GSimpleAction* action, GVariant* parameter, gpointer _)
				{
					GtkWindow* root = gtk_application_get_active_window(GTK_APPLICATION(app));
					gtk_window_set_focus(root, NULL);

					const char* target = g_variant_get_string(parameter, NULL);
					gdl_dock_layout_load_layout(window.layout, current_layout = target);

#ifdef GTK4_TODO
					GList* menu_items = gtk_container_get_children((GtkContainer*)sub);
					gtk_widget_set_sensitive(g_list_last(menu_items)->data, true); // enable the Save menu
					g_list_free(menu_items);
#endif

					statusbar_print(1, "Layout %s loaded", current_layout);
				}

				g_action_map_add_action (G_ACTION_MAP (group), G_ACTION (({
					GSimpleAction* action = g_simple_action_new ("layout-load", g_variant_type_new("s"));
					g_signal_connect (action, "activate", G_CALLBACK (load_layout), NULL);
					action;
				})));
			}

			// menu
			{
				void add_layout_item (const char* item, gpointer model)
				{
					g_autoptr(GMenuItem) mi;
					g_menu_append_item (G_MENU(model), ({
						mi = g_menu_item_new (item, "dummy");
						g_menu_item_set_action_and_target (mi, "context-menu.layout-load", "s", item);
						mi;
					}));
				}
				gdl_dock_layout_get_yaml_layouts (window.layout, add_layout_item, layouts_model);
			}
		}

		// Save
		{
			GMenuModel* section = (GMenuModel*)g_menu_new ();
			g_menu_append_section (G_MENU(layouts_model), NULL, section);

			{
				void layout_save (GSimpleAction* action, GVariant* parameter, gpointer _panel_type)
				{
					window_save_layout();
					statusbar_print(1, "Layout %s saved", current_layout);
				}

				GSimpleAction* action = g_simple_action_new ("layout-save", NULL);
				g_action_map_add_action (G_ACTION_MAP (group), G_ACTION (action));
				g_signal_connect (action, "activate", G_CALLBACK (layout_save), NULL);
				add_menu_items_from_defn(menu, section, 1, (MenuDef[]){{"Save", "context-menu.layout-save", "document-save"}}, NULL);

#ifdef GTK4_TODO
				GList* children = gtk_container_get_children((GtkContainer*)sub);
				gtk_widget_set_sensitive(g_list_last(children)->data, false);
				g_list_free(children);
#endif
			}
		}
	}
}


//
// Prefs menu
//
static void
prefs_menu (GtkWidget* menu, GMenuModel* model, GSimpleActionGroup* group)
{
	GMenuModel* prefs_model = (GMenuModel*)g_menu_new ();
	g_menu_append_submenu (G_MENU(model), "Prefs", prefs_model);

	{
		void toggle_loop_playback (GSimpleAction* action, GVariant* value, gpointer)
		{
			play->config.loop = !play->config.loop;
			g_simple_action_set_state (action, value);
		}

		GSimpleAction* action = g_simple_action_new_stateful ("toggle-loop", NULL, g_variant_new_boolean (play->config.loop));
		g_action_map_add_action (G_ACTION_MAP (group), G_ACTION (action));
		g_signal_connect (action, "change-state", G_CALLBACK (toggle_loop_playback), NULL);
	}

	{
		void toggle_recursive_add (GSimpleAction* action, GVariant* value, gpointer)
		{
			app->config.add_recursive = !app->config.add_recursive;
			g_simple_action_set_state (action, value);
		}

		GSimpleAction* action = g_simple_action_new_stateful ("toggle-recursive", NULL, g_variant_new_boolean (app->config.add_recursive));
		g_action_map_add_action (G_ACTION_MAP (group), G_ACTION (action));
		g_signal_connect (action, "change-state", G_CALLBACK (toggle_recursive_add), NULL);
	}

	add_menu_items_from_defn(menu, prefs_model, 2,
		(MenuDef[]){
			{"Add Recursively", "context-menu.toggle-recursive"},
			{"Loop Playback", "context-menu.toggle-loop"}
		},
		NULL
	);
}
