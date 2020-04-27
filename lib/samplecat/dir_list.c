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
#include "config.h"
#include "debug/debug.h"
#include "samplecat.h"
#include "dh_link.h"

static void create_nodes_for_path (gchar** path);


/*
 *  Build a node list of all the directories in the database.
 */
void
dir_list_update ()
{
	g_return_if_fail(samplecat.model->backend.dir_iter_new);

	samplecat.model->backend.dir_iter_new();

	if(samplecat.model->dir_tree) g_node_destroy(samplecat.model->dir_tree);
	samplecat.model->dir_tree = g_node_new(NULL);

	DhLink* link = dh_link_new("all directories", "");
	GNode* leaf = g_node_new(link);
	g_node_insert(samplecat.model->dir_tree, -1, leaf);

	GNode* lookup_node_by_path (gchar** path)
	{
		// find a node containing the given path

		typedef struct {
			gchar** path;
			GNode*  insert_at;
		} C;

		gboolean traverse_func(GNode* node, gpointer data)
		{
			// test if the given node is a match for the closure path.

			DhLink* link = node->data;
			C* c = data;
			gchar** path = c->path;
			if(link){
				c->insert_at = node;
				gchar** v = g_strsplit(link->name, "/", 64);
				for(int i=0;v[i];i++){
					if(strlen(v[i])){
						if(strcmp(v[i], path[i])){
							c->insert_at = NULL;
							break;
						}
					}
				}
				g_strfreev(v);
			}
			return false; // continue
		}

		C* c = SC_NEW(C,
			.path = path
		);

		g_node_traverse(samplecat.model->dir_tree, G_IN_ORDER, G_TRAVERSE_ALL, 64, traverse_func, c);
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
	dbg(2, "size=%i", g_node_n_children(samplecat.model->dir_tree));
}


static void
create_nodes_for_path (gchar** path)
{
	gchar* make_uri (gchar** path, int n)
	{
		// result must be freed by caller

		gchar* a = NULL;
		int k; for(k=0;k<=n;k++){
			gchar* b = g_strjoin("/", a, path[k], NULL);
			if(a) g_free(a);
			a = b;
		}
		dbg(2, "uri=%s", a);
		return a;
	}

	GNode* node = samplecat.model->dir_tree;
	for(int i=0;path[i];i++){
		if(strlen(path[i])){
			bool found = false;
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
					DhLink* l = dh_link_new(path[i], uri);
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


