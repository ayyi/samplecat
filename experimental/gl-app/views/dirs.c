/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. https://ayyi.github.io/samplecat/    |
 | copyright (C) 2016-2025 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#include "config.h"
#include "agl/utils.h"
#include "agl/actor.h"
#include "agl/event.h"
#include "agl/behaviours/cache.h"
#include "agl/behaviours/scrollable.h"
#include "agl/behaviours/key.h"
#include "waveform/shader.h"
#include "debug/debug.h"
#include "icon/utils.h"
#include "samplecat/model.h"
#include "samplecat/dir_list.h"
#include "dh_link.h"
#include "behaviours/panel.h"

#define row_height 20
#define cache() ((CacheBehaviour*)actor->behaviours[1])
#define dir_node_at(A, I) &g_array_index(A, DirNode, I)
#define NODE(N, I) ((DirNode*)N->data)[I]

typedef struct {
   GNode*  node;
   bool    open;
   GArray* children;
   int     n_rows;
} DirNode;

#define DIR_NODE DirNode
#include "views/dirs.h"

static void dirs_free (AGlActor*);

static AGl* agl = NULL;
static int instance_count = 0;
static AGlActorClass actor_class = {0, "Dirs", (AGlActorNew*)directories_view, dirs_free};

static void     dirs_select            (DirectoriesView*, int, DirNode*);
static void     dirs_on_filter_changed (AGlActor*);
static void     dirs_open_node         (DirectoriesView*, DirNode*);
static void     dirs_close_node        (DirectoriesView*, DirNode*);
static void     node_to_array          (GNode* parent, GArray*);
static void     clear_data             (DirectoriesView*);
static DirNode* find_node_by_row       (DirectoriesView*, int row, int* depth);
static DirNode* find_node_by_path      (DirectoriesView*, const char* path, int* row);
static int      count_node_children    (DirNode*);

static ActorKeyHandler
	nav_up,
	nav_down,
	nav_left,
	nav_right;

static ActorKey keys[] = {
	{XK_Up,    nav_up},
	{XK_Down,  nav_down},
	{XK_Left,  nav_left},
	{XK_Right, nav_right},
	{0,}
};

#define SCROLLABLE(A) ((ScrollableBehaviour*)((AGlActor*)A)->behaviours[2])
#define KEYS(A) ((KeyBehaviour*)((AGlActor*)A)->behaviours[3])


AGlActorClass*
directories_view_get_class ()
{
	if (!agl) {
		agl = agl_get_instance();

		agl_actor_class__add_behaviour(&actor_class, panel_get_class());
		agl_actor_class__add_behaviour(&actor_class, cache_get_class());
		agl_actor_class__add_behaviour(&actor_class, scrollable_get_class());
		agl_actor_class__add_behaviour(&actor_class, key_get_class());
	}

	return &actor_class;
}


AGlActor*
directories_view (gpointer _)
{
	instance_count++;

	directories_view_get_class();

	bool dirs_paint (AGlActor* actor)
	{
		DirectoriesView* view = (DirectoriesView*)actor;

		void paint_node (DirectoriesView* view, int* row, int depth, DirNode* dirnode)
		{
			DhLink* link = (DhLink*)dirnode->node->data;
			float x = 20 * depth;
			float y = -((AGlActor*)view)->cache.offset.y + *row * row_height;

			if (*row) {
				guint t = get_icon_texture_by_name (dirnode->open ? "pan-down-symbolic" : "pan-end-symbolic", 16);

				agl->shaders.texture->uniform.fg_colour = 0xffffffff;
				agl_use_program ((AGlShader*)agl->shaders.texture);
				agl_textured_rect (t, x, y, 16, 16, NULL);
			}

			if (*row == view->selection) {
				PLAIN_COLOUR2 (agl->shaders.plain) = 0x6677ff77;
				agl_use_program((AGlShader*)agl->shaders.plain);
				agl_rect_((AGlRect){0, y - 2, agl_actor__width(actor), row_height});
			}

			agl_print(x + 20, y, 0, 0xffffffff, link->name);

			// children
			if (dirnode->open) {
				for (int j = 0; j < dirnode->children->len; j++) {
					(*row)++;
					paint_node(view, row, depth + 1, dir_node_at(dirnode->children, j));
				}
			}
		}

		int row = 0;
		for (int i = 0; i < view->nodes->len; i++, row++) {
			paint_node(view, &row, 0, dir_node_at(view->nodes, i));
		}

		return true;
	}

	void dirs_init (AGlActor* actor)
	{
		DirectoriesView* view = (DirectoriesView*)actor;

		void dirs_on_list_changed (GObject* _model, void* tree, gpointer _view)
		{
			DirectoriesView* view = _view;

			clear_data(view);

			g_array_set_size(view->nodes, g_node_n_children(samplecat.model->dir_tree));
			node_to_array(samplecat.model->dir_tree, view->nodes);
			view->root = (DirNode){
				.node = samplecat.model->dir_tree,
				.children = view->nodes,
				.n_rows = 1,
				.open = true,
			};

			agl_actor__set_size((AGlActor*)view);
		}

		dir_list_register(dirs_on_list_changed, view);

		cache()->on_invalidate = dirs_on_filter_changed;
		cache_behaviour_add_dependency(cache(), actor, samplecat.model->filters2.dir);
	}

	void dirs_layout (AGlActor* actor)
	{
		DirectoriesView* view = (DirectoriesView*)actor;

		#define N_ROWS_VISIBLE(A) (agl_actor__height(((AGlActor*)A)) / row_height)
		view->cache.n_rows_visible = N_ROWS_VISIBLE(actor);

		view->cache.n_rows = count_node_children(&view->root) + 1;

		actor->scrollable.x2 = 100;
		actor->scrollable.y2 = actor->scrollable.y1 + view->cache.n_rows * row_height;

		// unfortunately the scrollable behaviour layout gets called before the actor, so it is called a 2nd time here to use the correct size
		AGlBehaviour* b = actor->behaviours[2];
		actor->class->behaviour_classes[2]->layout(b, actor);

#if 0
		// no, doesn't work when content changes because we lose the original allocation
		actor->region.y2 = MIN(actor->region.y2, actor->region.y1 + (view->cache.n_rows + 1) * row_height);
#endif
	}

	bool dirs_event (AGlActor* actor, AGlEvent* event, AGliPt xy)
	{
		DirectoriesView* view = (DirectoriesView*)actor;

		switch (event->type) {
			case AGL_BUTTON_RELEASE:
				if (event->button.button == 1) {
					int row = (xy.y - actor->scrollable.y1) / row_height;
					dbg(1, "y=%i row=%i", xy.y, row);

					int depth;
					DirNode* dirnode = find_node_by_row(view, row, &depth);
					if (dirnode) {
						if (xy.x > (depth + 1) * 20) {
							dirs_select(view, row, dirnode);
						} else {
							dirnode->open ? dirs_close_node(view, dirnode) : dirs_open_node(view, dirnode);
						}
					}
				}
			default:
				break;
		}
		return AGL_HANDLED;
	}

	void dirs_on_selection (gpointer user_data)
	{
		DirectoriesView* view = user_data;

		int depth;
		DirNode* dirnode = find_node_by_row(view, view->selection, &depth);

		observable_string_set(samplecat.model->filters2.dir, g_strdup(((DhLink*)dirnode->node->data)->uri));
	}

	DirectoriesView* view = agl_actor__new(DirectoriesView,
		.actor = {
			.class = &actor_class,
			.colour = 0xaaff33ff,
			.init = dirs_init,
			.paint = dirs_paint,
			.set_size = dirs_layout,
			.on_event = dirs_event
		},
		.selection = -1,
		.nodes = g_array_new (FALSE, FALSE, sizeof(DirNode)),
		.change = {
			.fn = dirs_on_selection,
			.user_data = a,
		}
	);

	KEYS(view)->keys = &keys;

	return (AGlActor*)view;
}


static void
dirs_free (AGlActor* actor)
{
	DirectoriesView* view = (DirectoriesView*)actor;

	dir_list_unregister(actor);

	clear_data(view);
	g_array_free(view->nodes, false);
	view->nodes = NULL;

	throttle_clear(&view->change);

	if (!--instance_count) {
	}

	g_free(actor);
}


static void
dirs_select (DirectoriesView* view, int row, DirNode* dirnode)
{
	AGlActor* actor = (AGlActor*)view;

	g_return_if_fail(dirnode);

	view->selection = row;

	int position = row * row_height;
	int bottom = (int)agl_actor__height(actor) - actor->scrollable.y1;
	AGlObservable* scroll_value = SCROLLABLE(view)->scroll;
	if (position > bottom - 30) {
		agl_observable_set_int (scroll_value, (row + 2) * row_height - agl_actor__height(actor));
	} else if (position < - actor->scrollable.y1 + 20) {
		agl_observable_set_int (scroll_value, (row - 1) * row_height);
	}

	throttle_queue(&view->change);

	agl_actor__invalidate(actor);
}


static void
dirs_on_filter_changed (AGlActor* _view)
{
	DirectoriesView* view = (DirectoriesView*)_view;

	view->selection = -1;
	if (samplecat.model->filters2.dir->value.c[0] != '\0') {
		int row;
		find_node_by_path (view, samplecat.model->filters2.dir->value.c, &row);
		view->selection = row;
	}
	agl_actor__invalidate((AGlActor*)view);
}


static void
dirs_open_node (DirectoriesView* view, DirNode* dirnode)
{
	int size = g_node_n_children(dirnode->node);

	dirnode->open = true;

	if (!dirnode->children) {
		dirnode->children = g_array_sized_new(FALSE, FALSE, sizeof(DirNode), size);
		g_array_set_size(dirnode->children, size);
		node_to_array(dirnode->node, dirnode->children);
	}
	dirnode->n_rows += size;
	view->cache.n_rows += size;

	agl_actor__set_size((AGlActor*)view);
}


static void
dirs_close_node (DirectoriesView* view, DirNode* dirnode)
{
	int size = dirnode->children->len;

	dirnode->open = false;

	dirnode->n_rows -= size;
	view->cache.n_rows -= size;

	agl_actor__set_size((AGlActor*)view);
}


static void
node_to_array (GNode* parent, GArray* array)
{
	int i = 0;
	for (GNode* node = g_node_first_child (parent); node; node = g_node_next_sibling(node), i++) {
		*dir_node_at(array, i) = (DirNode){ node, .n_rows = 1 };
	}
}


static void
clear_data (DirectoriesView* view)
{
	void clear_node (DirNode* node)
	{
		for (int i = 0; i < node->children->len; i++) {
			DirNode* dirnode = dir_node_at(node->children, i);

			clear_node(dirnode);

			g_array_free(dirnode->children, false);
			dirnode->children = NULL;
		}
	}

	for (int i = 0; i < view->nodes->len; i++) {
		clear_node(dir_node_at(view->nodes, i));
	}
}


static int
count_node_children (DirNode* node)
{
	int n = 0;

	if (node->open) {
		for (int i = 0; i < node->children->len; i++) {
			n += count_node_children(dir_node_at(node->children, i)) + 1;
		}
	}

	return n;
}


static DirNode*
find_node_by_row (DirectoriesView* view, int row, int* depth)
{
	DirNode* find_node (GArray* parent, int r, int row, int* depth)
	{
		for (int i = 0; i < parent->len; i++) {
			DirNode* node = &((DirNode*)parent->data)[i];
			int n_rows = count_node_children(node) + 1;
			if (r == row) {
				return node;
			}
			if (r + n_rows > row) {
				*depth += 1;
				return find_node(node->children, r + 1, row, depth);
			}
			r += n_rows;
		}
		return NULL;
	}

	*depth = 0;
	return find_node(view->nodes, 0, row, depth);
}


static DirNode*
find_node_by_path (DirectoriesView* view, const char* path, int* row)
{
	DirNode* find_node (GArray* parent, const char* path, int* row)
	{
		for (int i = 0; i < parent->len; i++, (*row)++) {
			DirNode* dirnode = dir_node_at(parent, i);
			DhLink* link = (DhLink*)dirnode->node->data;

			if (!strcmp(link->uri, path)) {
				dbg(1, "✅ %i %s", *row, link->uri);
				return dirnode;
			}

			// if selected path is not open, selection will be -1.
			if (dirnode->open) {
				(*row)++;
				DirNode* n = find_node(dirnode->children, path, row);
				if (n) {
					return n;
				}
				(*row)--;
			}
		}
		return NULL;
	}
	return find_node(view->nodes, path, row);
}


static bool
nav (AGlActor* actor, int value)
{
	PF;

	DirectoriesView* view = (DirectoriesView*)actor;

	int row = MAX(0, view->selection + value);

	int depth;
	DirNode* dirnode = find_node_by_row(view, row, &depth);
	if (dirnode) {
		dirs_select (view, row, dirnode);
		return AGL_HANDLED;
	}

	return AGL_NOT_HANDLED;
}


static bool
nav_up (AGlActor* actor, AGlModifierType modifier)
{
	return nav(actor, -1);
}


static bool
nav_down (AGlActor* actor, AGlModifierType modifier)
{
	return nav(actor, 1);
}


static bool
nav_left (AGlActor* actor, AGlModifierType modifier)
{
	PF;

	DirectoriesView* view = (DirectoriesView*)actor;

	int depth;
	DirNode* dirnode = find_node_by_row(view, view->selection, &depth);
	if (dirnode->open) {
		dirs_close_node(view, dirnode);
		return AGL_HANDLED;
	}
	return AGL_NOT_HANDLED;
}


static bool
nav_right (AGlActor* actor, AGlModifierType modifier)
{
	PF;

	DirectoriesView* view = (DirectoriesView*)actor;

	int depth;
	DirNode* dirnode = find_node_by_row(view, view->selection, &depth);
	if (!dirnode->open) {
		dirs_open_node(view, dirnode);
		return AGL_HANDLED;
	}
	return AGL_NOT_HANDLED;
}
