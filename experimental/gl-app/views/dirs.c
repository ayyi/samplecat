/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. https://ayyi.github.io/samplecat/    |
 | copyright (C) 2016-2024 Tim Orford <tim@orford.org>                  |
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
#include "waveform/shader.h"
#include "debug/debug.h"
#include "samplecat/model.h"
#include "samplecat/dir_list.h"
#include "dh_link.h"
#include "behaviours/panel.h"
#include "views/dirs.h"

#define row_height 20
#define cache() ((CacheBehaviour*)actor->behaviours[1])

static void dirs_free (AGlActor*);

static AGl* agl = NULL;
static int instance_count = 0;
static AGlActorClass actor_class = {0, "Dirs", (AGlActorNew*)directories_view, dirs_free};

static void dirs_select            (DirectoriesView*, int);
static void dirs_on_filter_changed (AGlActor*);


AGlActorClass*
directories_view_get_class ()
{
	if (!agl) {
		agl = agl_get_instance();

		agl_actor_class__add_behaviour(&actor_class, panel_get_class());
		agl_actor_class__add_behaviour(&actor_class, cache_get_class());
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

#ifndef AGL_ACTOR_RENDER_CACHE
		agl_enable_stencil(0, 0, actor->region.x2, actor->region.y2);
#endif

		int i = 0;
		GNode* node = g_node_first_child (samplecat.model->dir_tree);
		for (; node && i < view->cache.n_rows; node = g_node_next_sibling(node)) {
			DhLink* link = (DhLink*)node->data;

			if (i == view->selection/* - view->scroll_offset*/) {
				PLAIN_COLOUR2 (agl->shaders.plain) = 0x6677ff77;
				agl_use_program((AGlShader*)agl->shaders.plain);
				agl_rect_((AGlRect){0, i * row_height - 2, agl_actor__width(actor), row_height});
			}

			agl_print(0, i * row_height, 0, 0xffffffff, strlen(link->uri) ? link->uri : link->name);

			bool node_open = false;
			if (node_open) {
				// recurse children
			}
			i++;
		}

#ifndef AGL_ACTOR_RENDER_CACHE
		agl_disable_stencil();
#endif

		return true;
	}

	void dirs_init (AGlActor* actor)
	{
		dir_list_update(); // because this is slow, it is not done until a consumer needs it.

		cache()->on_invalidate = dirs_on_filter_changed;
		cache_behaviour_add_dependency(cache(), actor, samplecat.model->filters2.dir);
	}

	void dirs_set_size (AGlActor* actor)
	{
		DirectoriesView* view = (DirectoriesView*)actor;

		#define N_ROWS_VISIBLE(A) (agl_actor__height(((AGlActor*)A)) / row_height)
		view->cache.n_rows_visible = N_ROWS_VISIBLE(actor);

		if(!view->cache.n_rows){
			for(GNode* node = g_node_first_child (samplecat.model->dir_tree); node; node = g_node_next_sibling(node)){
				view->cache.n_rows++;
			}
			agl_actor__invalidate(actor);
		}

		actor->region.y2 = MIN(actor->region.y2, actor->region.y1 + (view->cache.n_rows + 1) * row_height);
	}

	bool dirs_event (AGlActor* actor, AGlEvent* event, AGliPt xy)
	{
		switch (event->type) {
			case AGL_BUTTON_RELEASE:
				if (event->button.button == 1) {
					int row = xy.y / row_height;
					dbg(1, "y=%i row=%i", xy.y, row);
					dirs_select((DirectoriesView*)actor, row);
				}
			default:
				break;
		}
		return AGL_HANDLED;
	}

	DirectoriesView* view = agl_actor__new(DirectoriesView,
		.actor = {
			.class = &actor_class,
			.colour = 0xaaff33ff,
			.init = dirs_init,
			.paint = dirs_paint,
			.set_size = dirs_set_size,
			.on_event = dirs_event
		},
		.selection = -1
	);

	return (AGlActor*)view;
}


static void
dirs_free (AGlActor* actor)
{
	if (!--instance_count) {
	}

	g_free(actor);
}


static void
dirs_select (DirectoriesView* dirs, int row)
{
	int n_rows_total = dirs->cache.n_rows;

	if(row >= 0 && row < n_rows_total){
		dirs->selection = row;
		/*
		if(dirs->selection >= dirs->scroll_offset + N_ROWS_VISIBLE(dirs)){
			dbg(0, "need to scroll down");
			dirs->scroll_offset++;
		}else if(dirs->selection < dirs->scroll_offset){
			dirs->scroll_offset--;
		}
		*/
		agl_actor__invalidate((AGlActor*)dirs);

		GNode* node = g_node_nth_child (samplecat.model->dir_tree, row);
		if (node) {
			DhLink* link = (DhLink*)node->data;
			observable_string_set(samplecat.model->filters2.dir, g_strdup(link->uri));
		}
	}
}


static void
dirs_on_filter_changed (AGlActor* _view)
{
	DirectoriesView* view = (DirectoriesView*)_view;

	if(samplecat.model->filters2.dir->value.c[0] == '\0'){
		view->selection = -1;
	}else{
		gchar** a = g_strsplit(samplecat.model->filters2.dir->value.c, "/", 100);
		if (a[0] && a[1]) {
			char* target = a[1];

			int i = 0;
			GNode* node = g_node_first_child (samplecat.model->dir_tree);
			for (; node; node = g_node_next_sibling(node), i++) {
				DhLink* link = (DhLink*)node->data;
				gchar** split = g_strsplit(link->uri, "/", 100);
				if(split[0] && split[1]){
					if(!strcmp(split[1], target)){
						view->selection = i;
						break;
					}
				}
				g_strfreev(split);
			}
		}
		g_strfreev(a);
	}
	agl_actor__invalidate((AGlActor*)view);
}
