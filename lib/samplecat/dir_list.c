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

#include "config.h"
#include "debug/debug.h"
#include "samplecat.h"
#include "dh_link.h"

static void create_nodes_for_path (gchar** path);
static void dir_list_update ();

typedef struct {
	void (*callback)(GObject*, gpointer, gpointer);
	gpointer user_data;
} Subscriber;

static GList* subscribers;


/*
 *  Because updating the directory list is a slow operation, it is initialised
 *  by individual consumers, not by the application.
 */
void
dir_list_register (void (*callback)(GObject*, gpointer, gpointer), gpointer user_data)
{
	static gulong connection;

	subscribers = g_list_append(subscribers, SC_NEW(Subscriber, .callback = callback, .user_data = user_data ));

	void on_dir_list_changed (GObject* model, void* tree, gpointer _)
	{
		if (subscribers) {
			dir_list_update();

			for (GList* l = subscribers; l; l = l->next) {
				Subscriber* s = l->data;
				s->callback(model, samplecat.model->dir_tree, s->user_data);
			}
		}
	}
	if (!connection)
		connection = g_signal_connect(samplecat.model, "dir-list-changed", G_CALLBACK(on_dir_list_changed), subscribers);
	g_signal_emit_by_name (samplecat.model, "dir-list-changed");
}


void
dir_list_unregister (gpointer key)
{
	for (GList* l = subscribers; l; l = l->next) {
		Subscriber* s = l->data;
		if (s->user_data == key) {
			subscribers = g_list_remove(subscribers, s);
			g_free(s);
			return;
		}
	}

	pwarn("not found");
}


/*
 *  Build a node list of all the directories in the database.
 */
static void
dir_list_update ()
{
	g_return_if_fail(samplecat.model->backend.dir_iter_new);

	samplecat.model->backend.dir_iter_new();

	DhLink* root_link = dh_link_new("all directories", "/");

	if (samplecat.model->dir_tree) g_node_destroy(samplecat.model->dir_tree);
	samplecat.model->dir_tree = g_node_new(root_link);

	g_node_insert(samplecat.model->dir_tree, -1, g_node_new(root_link));

	GNode* lookup_node_by_path (gchar** path)
	{
		// find a node containing the given path

		typedef struct {
			gchar** path;
			GNode*  insert_at;
		} C;

		gboolean traverse_func (GNode* node, gpointer data)
		{
			// test if the given node is a match for the closure path.

			DhLink* link = node->data;
			C* c = data;
			gchar** path = c->path;
			if (link) {
				c->insert_at = node;
				gchar** v = g_strsplit(link->name, "/", 64);
				for (int i=0;v[i];i++) {
					if (strlen(v[i])) {
						if (strcmp(v[i], path[i])) {
							c->insert_at = NULL;
							break;
						}
					}
				}
				g_strfreev(v);
			}
			return false; // continue
		}

		C c = { .path = path };
		g_node_traverse(samplecat.model->dir_tree, G_IN_ORDER, G_TRAVERSE_ALL, 64, traverse_func, &c);

		return c.insert_at;
	}

	char* u;
	while ((u = samplecat.model->backend.dir_iter_next())) {

		dbg(2, "%s", u);
		gchar** v = g_strsplit(u, "/", 64);
		int i = 0;
		while (v[i]) {
			i++;
		}
		GNode* node = lookup_node_by_path(v);
		if (node) {
			dbg(3, "node found. not adding.");
		} else {
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
		for (int k=0;k<=n;k++) {
			gchar* b = g_strjoin("/", a, path[k], NULL);
			if (a) g_free(a);
			a = b;
		}
		dbg(2, "uri=%s", a);
		return a;
	}

	GNode* node = samplecat.model->dir_tree;
	for (int i=0;path[i];i++) {
		if (strlen(path[i])) {
			bool found = false;
			GNode* n = g_node_first_child(node);
			if (n) do {
				DhLink* link = n->data;
				if (link) {
					if (!strcmp(link->name, path[i])) {
						node = n;
						found = true;
						break;
					}
				}
			} while ((n = g_node_next_sibling(n)));

			if (!found) {
				dbg(2, "  not found. inserting... (%s)", path[i]);
				gchar* uri = make_uri(path, i);
				if (strcmp(uri, g_get_home_dir())) {
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
