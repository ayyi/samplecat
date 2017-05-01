/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2016-2017 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#define __wf_private__
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <GL/gl.h>
#include "agl/ext.h"
#include "agl/utils.h"
#include "agl/actor.h"
#include "waveform/waveform.h"
#include "samplecat.h"
#include "dh_link.h"
#include "views/dirs.h"

#define _g_free0(var) (var = (g_free (var), NULL))

#define row_height 20


static AGl* agl = NULL;
static int instance_count = 0;


static void
_init()
{
	static bool init_done = false;

	if(!init_done){
		agl = agl_get_instance();

		dir_list_update(); // because this is slow, it is not done until a consumer needs it.

		init_done = true;
	}
}


AGlActor*
directories_view(WaveformActor* _)
{
	instance_count++;

	_init();

	bool dirs_paint(AGlActor* actor)
	{
		DirectoriesView* view = (DirectoriesView*)actor;

		agl_enable_stencil(0, 0, actor->region.x2, actor->region.y2);

		agl_print(0, 0, 0, 0xffffffff, "Directories");

		int i = 0;
		GNode* node = g_node_first_child (samplecat.model->dir_tree);
		for(; node && i < view->cache.n_rows; node = g_node_next_sibling(node)){
			DhLink* link = (DhLink*)node->data;
			agl_print(0, (i + 1) * row_height, 0, 0xffffffff, strlen(link->uri) ? link->uri : link->name);

			gboolean node_open = false;
			if(node_open){
				// recurse children
			}
			i++;
		}

		agl_disable_stencil();

		return true;
	}

	void dirs_init(AGlActor* a)
	{
#ifdef AGL_ACTOR_RENDER_CACHE
		a->fbo = agl_fbo_new(agl_actor__width(a), agl_actor__height(a), 0, AGL_FBO_HAS_STENCIL);
		a->cache.enabled = true;
#endif
	}

	void dirs_set_size(AGlActor* actor)
	{
		DirectoriesView* view = (DirectoriesView*)actor;

		#define N_ROWS_VISIBLE(A) (agl_actor__height(((AGlActor*)A)) / row_height)
		view->cache.n_rows_visible = N_ROWS_VISIBLE(actor);

		if(!view->cache.n_rows){
			GNode* node = g_node_first_child (samplecat.model->dir_tree);
			for(; node; node = g_node_next_sibling(node)){
				view->cache.n_rows++;
			}
		}

		actor->region.y2 = MIN(actor->region.y2, actor->region.y1 + (view->cache.n_rows + 1) * row_height);
	}

	bool dirs_event(AGlActor* actor, GdkEvent* event, AGliPt xy)
	{
		/*
		switch(event->type){
			case GDK_BUTTON_PRESS:
			case GDK_BUTTON_RELEASE:
				agl_actor__invalidate(actor);
				dbg(0, "y=%i", xy.y - actor->region.y1);
				break;
			default:
				break;
		}
		*/
		return AGL_HANDLED;
	}

	void dirs_free(AGlActor* actor)
	{
		if(!--instance_count){
		}
	}

	DirectoriesView* view = WF_NEW(DirectoriesView,
		.actor = {
#ifdef AGL_DEBUG_ACTOR
			.name = "Directories",
#endif
			.init = dirs_init,
			.free = dirs_free,
			.paint = dirs_paint,
			.set_size = dirs_set_size,
			.on_event = dirs_event
		}
	);

	return view;
}


