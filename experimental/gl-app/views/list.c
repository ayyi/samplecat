/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2012-2019 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#define __wf_private__
#include "config.h"
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "agl/utils.h"
#include "agl/actor.h"
#include "waveform/waveform.h"
#include "waveform/peakgen.h"
#include "waveform/shader.h"
#include "waveform/actors/text.h"
#include "samplecat.h"
#include "application.h"
#include "behaviours/panel.h"
#include "views/list.h"
#include "views/context_menu.h"

extern int need_draw;
extern Menu menu;

#define _g_free0(var) (var = (g_free (var), NULL))

#define FONT "Droid Sans"

static void list_view_free (AGlActor*);

static AGl* agl = NULL;
static int instance_count = 0;
static AGlActorClass actor_class = {0, "List", (AGlActorNew*)list_view, list_view_free};


AGlActorClass*
list_view_get_class ()
{
	static bool init_done = false;

	if(!init_done){
		agl = agl_get_instance();

		agl_actor_class__add_behaviour(&actor_class, panel_get_class());

		init_done = true;
	}

	return &actor_class;
}


static void
_on_context_menu_selection (gpointer _view, gpointer _promise)
{
	AMPromise* promise = _promise;

	am_promise_unref(promise);
}


AGlActor*
list_view (gpointer _)
{
	instance_count++;

	list_view_get_class();

	bool list_paint (AGlActor* actor)
	{
		ListView* view = (ListView*)actor;

		#define row_height 20
		#define N_ROWS_VISIBLE(A) (agl_actor__height(((AGlActor*)A)) / row_height)
		int n_rows = N_ROWS_VISIBLE(actor);

		int col[] = {0, 150, 260, 360, 420};
		col[4] = MAX(col[4], actor->region.x2);

		GtkTreeIter iter;
		if(!gtk_tree_model_get_iter_first((GtkTreeModel*)samplecat.store, &iter)){ gerr ("cannot get iter."); return false; }
		int i = 0;
		for(;i<view->scroll_offset;i++){
			gtk_tree_model_iter_next((GtkTreeModel*)samplecat.store, &iter);
		}
		int row_count = 0;
		do {
			if(row_count == view->selection - view->scroll_offset){
				agl->shaders.plain->uniform.colour = STYLE.selection;
				agl_use_program((AGlShader*)agl->shaders.plain);
				agl_rect_((AGlRect){0, row_count * row_height - 2, agl_actor__width(actor), row_height});
			}

			Sample* sample = samplecat_list_store_get_sample_by_iter(&iter);
			if(sample){
				char* len[32]; format_smpte((char*)len, sample->frames);
				char* f[32]; samplerate_format((char*)f, sample->sample_rate);

				char* val[4] = {sample->name, sample->sample_dir, (char*)len, (char*)f};

				int c; for(c=0;c<G_N_ELEMENTS(val);c++){
					agl_enable_stencil(0, 0, col[c + 1] - 6, actor->region.y2);
					agl_print(col[c], row_count * row_height, 0, STYLE.text, val[c]);
				}
				sample_unref(sample);
			}
		} while (++row_count < n_rows && gtk_tree_model_iter_next((GtkTreeModel*)samplecat.store, &iter));

		agl_disable_stencil();

		return true;
	}

	void list_init(AGlActor* a)
	{
#ifdef AGL_ACTOR_RENDER_CACHE
		a->fbo = agl_fbo_new(agl_actor__width(a), agl_actor__height(a), 0, AGL_FBO_HAS_STENCIL);
		a->cache.enabled = true;
#endif
	}

	void list_set_size (AGlActor* actor)
	{
	}

	bool list_event (AGlActor* actor, GdkEvent* event, AGliPt xy)
	{
		switch(event->type){
			case GDK_BUTTON_PRESS:
				switch(event->button.button){
					case 3:;
						AMPromise* promise = am_promise_new(actor);
						am_promise_add_callback(promise, _on_context_menu_selection, promise);

						AGliPt offset = agl_actor__find_offset(actor);
						context_menu_open_new(actor->root, (AGliPt){xy.x + offset.x, xy.y + offset.y}, &menu, promise);
						break;
				}
				// falling through ...
			case GDK_BUTTON_RELEASE:
				switch(event->button.button){
					case 1:
						agl_actor__invalidate(actor);
						int row = xy.y / row_height;
						dbg(1, "y=%i row=%i", xy.y, row);
						list_view_select((ListView*)actor, row);
						break;
					default:
						break;
				}
				break;
			default:
				break;
		}
		return AGL_HANDLED;
	}

	ListView* view = AGL_NEW(ListView,
		.actor = {
			.class = &actor_class,
			.name = actor_class.name,
			.colour = 0xffff99ff,
			.init = list_init,
			.paint = list_paint,
			.set_size = list_set_size,
			.on_event = list_event,
		}
	);
	AGlActor* actor = (AGlActor*)view;

	void on_selection_change(SamplecatModel* m, Sample* sample, gpointer user_data)
	{
		PF;
	}
	g_signal_connect((gpointer)samplecat.model, "selection-changed", G_CALLBACK(on_selection_change), NULL);

	void list_on_search_filter_changed(GObject* _filter, gpointer _actor)
	{
		// update list...
		agl_actor__invalidate((AGlActor*)_actor);
	}
	g_signal_connect(samplecat.store, "content-changed", G_CALLBACK(list_on_search_filter_changed), actor);

	return actor;
}


static void
list_view_free (AGlActor* actor)
{
	if(!--instance_count){
	}

	g_free(actor);
}


void
list_view_select(ListView* list, int row)
{
	int n_rows_total = ((SamplecatListStore*)samplecat.store)->row_count;

	if(row >= 0 && row < n_rows_total){
		list->selection = row;
		if(list->selection >= list->scroll_offset + N_ROWS_VISIBLE(list)){
			dbg(0, "need to scroll down");
			list->scroll_offset++;
		}else if(list->selection < list->scroll_offset){
			list->scroll_offset--;
		}
		agl_actor__invalidate((AGlActor*)list);

		Sample* sample = samplecat_list_store_get_sample_by_row_index(list->selection);
		if(sample){
			samplecat_model_set_selection (samplecat.model, sample);
			sample_unref(sample);
		}
	}
}

