/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2017-2017 Tim Orford <tim@orford.org>                  |
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
#include "debug/debug.h"
#include "file_manager/support.h" // to_utf8()
#include "agl/ext.h"
#include "agl/utils.h"
#include "agl/actor.h"
#include "agl/fbo.h"
#include "samplecat.h"
#include "views/scrollbar.h"
#include "views/inspector.h"

#define _g_free0(var) (var = (g_free (var), NULL))

#define row_height 20

#define scrollable_height (view->cache.n_rows * row_height)
#define max_scroll_offset (scrollable_height / row_height - view->cache.n_rows_visible)

static AGl* agl = NULL;
static int instance_count = 0;
static AGlActorClass actor_class = {0, "Inspector", (AGlActorNew*)inspector_view};

static void inspector_set_scroll_position (AGlActor*, int);


AGlActorClass*
inspector_view_get_class ()
{
	return &actor_class;
}


static void
_init()
{
	static bool init_done = false;

	if(!init_done){
		agl = agl_get_instance();
		init_done = true;
	}
}


AGlActor*
inspector_view(gpointer _)
{
	instance_count++;

	_init();

	bool inspector_paint(AGlActor* actor)
	{
		InspectorView* view = (InspectorView*)actor;
		Sample* sample = view->sample;

		int row = 0;
		agl_print(0, row_height * (view->scroll_offset + row++), 0, 0xffffffff, "Inspector");

		if(!sample) return true;

		agl_enable_stencil(0, row_height * view->scroll_offset, actor->region.x2, agl_actor__height(actor));

		char* ch_str = format_channels(sample->channels);
		char* level  = gain2dbstring(sample->peaklevel);

		char fs_str[32]; samplerate_format(fs_str, sample->sample_rate); strcpy(fs_str + strlen(fs_str), " kHz");
		char length[64]; format_smpte(length, sample->length);
		char frames[32]; sprintf(frames, "%"PRIi64"", sample->frames);
		char bitrate[32]; bitrate_format(bitrate, sample->bit_rate);
		char bitdepth[32]; bitdepth_format(bitdepth, sample->bit_depth);

		char* keywords = (sample->keywords && strlen(sample->keywords)) ? sample->keywords : "<no tags>";
		char* path = to_utf8(sample->full_path);

#define PRINT_ROW(KEY, VAL) \
		agl_print( 0, row_height * (view->scroll_offset + row)  , 0, 0xffffffff, KEY); \
		agl_print(80, row_height * (view->scroll_offset + row++), 0, 0xffffffff, VAL);

		struct {
			char* name;
			char* val;
		} rows[] = {
			{"Name", sample->name},
			{"Path", path},
			{"Tags", keywords},
			{"Length", length},
			{"Frames", frames},
			{"Samplerate", fs_str},
			{"Channels", ch_str},
			{"Mimetype", sample->mimetype},
			{"Level", level},
			{"Bitrate", bitrate},
			{"Bitdepth", bitdepth},
			{"Notes", sample->notes ? sample->notes : ""}
		};

		int r = view->scroll_offset;
		for(;r<G_N_ELEMENTS(rows);r++){
			PRINT_ROW(rows[r].name, rows[r].val);
		}

		agl_disable_stencil();

		g_free(path);
		g_free(level);
		g_free(ch_str);

		return true;
	}

	void inspector_init(AGlActor* a)
	{
	}

	void inspector_set_size(AGlActor* actor)
	{
		InspectorView* view = (InspectorView*)actor;

		#define N_ROWS_VISIBLE(A) (agl_actor__height(((AGlActor*)A)) / row_height)
		view->cache.n_rows_visible = N_ROWS_VISIBLE(actor);

		inspector_set_scroll_position(actor, -1);
	}

	bool inspector_event(AGlActor* actor, GdkEvent* event, AGliPt xy)
	{
		InspectorView* view = (InspectorView*)actor;

		switch(event->type){
			case GDK_BUTTON_PRESS:
				switch(event->button.button){
					case 4:
						inspector_set_scroll_position(actor, view->scroll_offset - 1);
						agl_actor__invalidate(actor);
						break;
					case 5:
						if(scrollable_height > N_ROWS_VISIBLE(actor)){
							inspector_set_scroll_position(actor, view->scroll_offset + 1);
							agl_actor__invalidate(actor);
						}
						break;
				}
				break;
			default:
				break;
		}
		return AGL_HANDLED;
	}

	void inspector_free(AGlActor* actor)
	{
		if(!--instance_count){
		}
	}

	InspectorView* view = AGL_NEW(InspectorView,
		.actor = {
			.class = &actor_class,
			.name = actor_class.name,
			.init = inspector_init,
			.free = inspector_free,
			.paint = inspector_paint,
			.set_size = inspector_set_size,
			.on_event = inspector_event
		},
		.cache = {
			.n_rows = 13
		}
	);

	void inspector_on_selection_change(SamplecatModel* m, Sample* sample, gpointer actor)
	{
		InspectorView* inspector = actor;

		dbg(1, "sample=%s", sample->name);

		if(inspector->sample) sample_unref(inspector->sample);
		inspector->sample = sample_ref(sample);
		inspector->cache.n_rows = 13; // TODO really count the number of rows needed for this sample

		agl_actor__invalidate(actor);
	}
	g_signal_connect((gpointer)samplecat.model, "selection-changed", G_CALLBACK(inspector_on_selection_change), view);

	agl_actor__add_child((AGlActor*)view, scrollbar_view(NULL, GTK_ORIENTATION_VERTICAL));

	return (AGlActor*)view;
}


/*
 *  Use -1 to preserve existing offset
 */
static void
inspector_set_scroll_position(AGlActor* actor, int scroll_offset)
{
	InspectorView* view = (InspectorView*)actor;

	scroll_offset = MAX(0, scroll_offset < 0 ? view->scroll_offset : scroll_offset); // TODO why doesnt CLAMP do this?

	view->scroll_offset = CLAMP(scroll_offset, 0, max_scroll_offset);
	actor->scrollable.y1 = - view->scroll_offset * row_height;
	actor->scrollable.y2 = actor->scrollable.y1 + scrollable_height;
}

