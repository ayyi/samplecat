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
#include <gtk/gtk.h>
#include <string.h>
#include "debug/debug.h"
#include "dir_tree/view_dir_tree.h"
#ifndef NO_USE_DEVHELP_DIRTREE
#include "dh_link.h"
#include "dh_tree.h"
#else
#include "dir_tree.h"
#endif
#include "typedefs.h"
#include "support.h"
#include "db/db.h"
#include "application.h"
#include "model.h"
#include "directories.h"

static void update_dir_node_list      ();
static bool on_dir_tree_link_selected (GObject*, DhLink*, gpointer);
static void create_nodes_for_path     (gchar** path);


GtkWidget*
dir_panel_new()
{
	GtkWidget* widget = NULL;

	GtkWidget* _dir_tree_new()
	{
		// data:
		update_dir_node_list();

		// view:
	#ifndef NO_USE_DEVHELP_DIRTREE
		app->dir_treeview = dh_book_tree_new(&app->dir_tree);
	#else
		app->dir_treeview = dir_tree_new(&app->dir_tree);
	#endif

		return app->dir_treeview;
	}

#if 0
	if(!BACKEND_IS_NULL){ // why ?
#endif
#ifndef NO_USE_DEVHELP_DIRTREE
		GtkWidget* tree = _dir_tree_new();
		widget = scrolled_window_new();
		gtk_container_add((GtkContainer*)widget, tree);
		g_signal_connect(tree, "link-selected", G_CALLBACK(on_dir_tree_link_selected), NULL);

		/* The dir tree shows ALL directories and is not affected by the current filter setting.
		void on_dir_filter_changed(GObject* _filter, gpointer _entry)
		{
			SamplecatFilter* filter = (SamplecatFilter*)_filter;
		}
		g_signal_connect(samplecat.model->filters.dir, "changed", G_CALLBACK(on_dir_filter_changed), NULL);
		*/
#else
		GtkWidget* tree = _dir_tree_new();
		widget = scrolled_window_new();
		gtk_container_add((GtkContainer*)widget, tree);
#endif
#if 0
	}
#endif

	//alternative dir tree:
#ifdef USE_NICE_GQVIEW_CLIST_TREE
	if(false){
		ViewDirList* dir_list = vdlist_new(app->home_dir);
		GtkWidget* tree = dir_list->widget;
	}
	gint expand = true;
	ViewDirTree* dir_list = vdtree_new(app->home_dir, expand);
	widget = dir_list->widget;
#endif

	void on_dir_list_changed(GObject* _model, gpointer user_data)
	{
		/*
		 * refresh the directory tree. Called from an idle.
		 *
		 * TODO make updates more efficient so the whole node tree doesnt have to be recreated
		 */
		update_dir_node_list();
		dh_book_tree_reload((DhBookTree*)app->dir_treeview);
	}
	g_signal_connect(samplecat.model, "dir-list-changed", G_CALLBACK(on_dir_list_changed), NULL);

#ifndef NO_USE_DEVHELP_DIRTREE
	void dir_on_theme_changed(Application* a, char* theme, gpointer _tree)
	{
		GtkWidget* container = gtk_widget_get_parent(app->dir_treeview);
		gtk_widget_destroy(app->dir_treeview);
		app->dir_treeview = _dir_tree_new();
		gtk_container_add((GtkContainer*)container, app->dir_treeview);
		gtk_widget_show(app->dir_treeview);
	}
	g_signal_connect(app, "icon-theme", G_CALLBACK(dir_on_theme_changed), &tree);
#endif

	return widget;
}


static gboolean
on_dir_tree_link_selected(GObject* _, DhLink* link, gpointer data)
{
	g_return_val_if_fail(link, false);

	dbg(1, "dir=%s", link->uri);

	samplecat_filter_set_value(samplecat.model->filters.dir, g_strdup(link->uri));

	return false;
}


static void
create_nodes_for_path(gchar** path)
{
	gchar* make_uri(gchar** path, int n)
	{
		//result must be freed by caller

		gchar* a = NULL;
		int k; for(k=0;k<=n;k++){
			gchar* b = g_strjoin("/", a, path[k], NULL);
			if(a) g_free(a);
			a = b;
		}
		dbg(2, "uri=%s", a);
		return a;
	}

	GNode* node = app->dir_tree;
	int i; for(i=0;path[i];i++){
		if(strlen(path[i])){
			//dbg(0, "  testing %s ...", path[i]);
			gboolean found = false;
			GNode* n = g_node_first_child(node);
			if(n) do {
				DhLink* link = n->data;
				if(link){
					//dbg(0, "  %s / %s", path[i], link->name);
					if(!strcmp(link->name, path[i])){
						//dbg(0, "    found. node=%p", n);
						node = n;
						found = true;
						break;
					}
				}
			} while ((n = g_node_next_sibling(n)));

			if(!found){
				dbg(2, "  not found. inserting... (%s)", path[i]);
				gchar* uri = make_uri(path, i);
				if(strcmp(uri, g_get_home_dir())){
					DhLink* l = dh_link_new(DH_LINK_TYPE_PAGE, path[i], uri);
					GNode* leaf = g_node_new(l);

					gint position = -1; //end
					g_node_insert(node, position, leaf);
					node = leaf;
				}
				if (uri) g_free(uri);
			}
		}
	}
}


static void
update_dir_node_list()
{
	/*
	builds a node list of directories listed in the latest database result.
	*/

	if(BACKEND_IS_NULL) return;

	samplecat.model->backend.dir_iter_new();

	if(app->dir_tree) g_node_destroy(app->dir_tree);
	app->dir_tree = g_node_new(NULL);

	DhLink* link = dh_link_new(DH_LINK_TYPE_PAGE, "all directories", "");
	GNode* leaf = g_node_new(link);
	g_node_insert(app->dir_tree, -1, leaf);

	GNode* lookup_node_by_path(gchar** path)
	{
		//find a node containing the given path

		//dbg(0, "path=%s", path[0]);
		struct _closure {
			gchar** path;
			GNode*  insert_at;
		};
		gboolean traverse_func(GNode *node, gpointer data)
		{
			//test if the given node is a match for the closure path.

			DhLink* link = node->data;
			struct _closure* c = data;
			gchar** path = c->path;
			//dbg(0, "%s", link ? link->name : NULL);
			if(link){
				c->insert_at = node;
				//dbg(0, "%s 0=%s", link->name, path[0]);
				gchar** v = g_strsplit(link->name, "/", 64);
				int i; for(i=0;v[i];i++){
					if(strlen(v[i])){
						if(!strcmp(v[i], path[i])){
							//dbg(0, "found! %s at depth %i", v[i], i);
							//dbg(0, " %s", v[i]);
						}else{
							//dbg(0, "no match. failed at %i: %s", i, path[i]);
							c->insert_at = NULL;
							break;
						}
					}
				}
				g_strfreev(v);
			}
			return false; //continue
		}

		struct _closure* c = g_new0(struct _closure, 1);
		c->path = path;

		g_node_traverse(app->dir_tree, G_IN_ORDER, G_TRAVERSE_ALL, 64, traverse_func, c);
		GNode* ret = c->insert_at;
		g_free(c);
		return ret;
	}

	char* u;
	while((u = samplecat.model->backend.dir_iter_next())){

		dbg(2, "%s", u);
		gchar** v = g_strsplit(u, "/", 64);
		int i = 0;
		while(v[i]){
			//dbg(0, "%s", v[i]);
			i++;
		}
		GNode* node = lookup_node_by_path(v);
		if(node){
			dbg(3, "node found. not adding.");
		}else{
			create_nodes_for_path(v);
		}
		g_strfreev(v);
	}

	samplecat.model->backend.dir_iter_free();
	dbg(2, "size=%i", g_node_n_children(app->dir_tree));
}


