/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. https://ayyi.github.io/samplecat/    |
 | copyright (C) 2017-2024 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#include "config.h"
#include <gdk/gdkkeysyms.h>
#include <GL/gl.h>
#include "debug/debug.h"
#include "file_manager/support.h" // to_utf8()
#include "agl/behaviours/scrollable.h"
#include "agl/fbo.h"
#include "utils/behaviour_subject.h"
#include "samplecat/support.h"
#include "application.h"
#include "views/inspector.h"

#define _g_free0(var) (var = (g_free (var), NULL))

#define row_height LINE_HEIGHT

#define agl_actor__scrollable_height(A) (A->scrollable.y2 - A->scrollable.y1)
#define scrollable_height (view->cache.n_rows * row_height)

static void inspector_free (AGlActor*);

static AGl* agl = NULL;
static int instance_count = 0;
static AGlActorClass actor_class = {0, "Inspector", (AGlActorNew*)inspector_view, inspector_free};


AGlActorClass*
inspector_view_get_class ()
{
	return &actor_class;
}


static void
_init ()
{
	if (!agl) {
		agl = agl_get_instance();
		agl_actor_class__add_behaviour (&actor_class, scrollable_get_class());
	}
}


AGlActor*
inspector_view (gpointer _)
{
	instance_count++;

	_init();

	bool inspector_paint (AGlActor* actor)
	{
		InspectorView* view = (InspectorView*)actor;
		Sample* sample = view->sample;

		int row = 0;

		if (!sample) return true;

#ifndef INSPECTOR_RENDER_CACHE
		agl_enable_stencil(0, row_height * view->scroll_offset, actor->region.x2, agl_actor__height(actor));
#endif

		char* ch_str = format_channels(sample->channels);
		char* level  = gain2dbstring(sample->peaklevel);

		char fs_str[32]; samplerate_format(fs_str, sample->sample_rate); strcpy(fs_str + strlen(fs_str), " kHz");
		char length[64]; format_smpte(length, sample->length);
		char frames[32]; sprintf(frames, "%"PRIi64"", sample->frames);
		char bitrate[32]; bitrate_format(bitrate, sample->bit_rate);
		char bitdepth[32]; bitdepth_format(bitdepth, sample->bit_depth);

		char* keywords = (sample->keywords && strlen(sample->keywords)) ? sample->keywords : "<no tags>";
		char* path = to_utf8(sample->full_path);

#ifdef INSPECTOR_RENDER_CACHE
#define PRINT_ROW(KEY, VAL) \
		agl_print( 0, -actor->scrollable.y1 + row_height * (row)  , 0, 0xffffff99, KEY); \
		agl_print(80, -actor->scrollable.y1 + row_height * (row++), 0, STYLE.text, VAL);
#else
#define PRINT_ROW(KEY, VAL) \
		agl_print( 0, row_height * (view->scroll_offset + row)  , 0, 0xffffff99, KEY); \
		agl_print(80, row_height * (view->scroll_offset + row++), 0, STYLE.text, VAL);
#endif

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

#ifdef INSPECTOR_RENDER_CACHE
		int r = 0;
#else
		int r = view->scroll_offset;
#endif
		for (;r<G_N_ELEMENTS(rows);r++) {
			PRINT_ROW(rows[r].name, rows[r].val);
		}

#ifndef INSPECTOR_RENDER_CACHE
		agl_disable_stencil();
#endif

		g_free(path);
		g_free(level);
		g_free(ch_str);

		return true;
	}

	void inspector_init (AGlActor* a)
	{
		a->colour = 0xffaa33ff; // panel gets colour from its child.
	}

	void inspector_set_size (AGlActor* actor)
	{
		InspectorView* view = (InspectorView*)actor;

		#define N_ROWS_VISIBLE(A) (agl_actor__height(((AGlActor*)A)) / row_height)
		view->cache.n_rows_visible = N_ROWS_VISIBLE(actor);
	}

	bool inspector_event (AGlActor* actor, AGlEvent* event, AGliPt xy)
	{
		return AGL_NOT_HANDLED;
	}

	InspectorView* view = agl_actor__new (InspectorView,
		.actor = {
			.class = &actor_class,
			.init = inspector_init,
			.paint = inspector_paint,
			.set_size = inspector_set_size,
			.on_event = inspector_event,
			.scrollable = {
				.y2 = 13 * row_height
			}
		},
		.cache = {
			.n_rows = 13
		}
	);

	void inspector_on_selection_change (SamplecatModel* m, GParamSpec* pspec, gpointer actor)
	{
		InspectorView* inspector = actor;
		Sample* sample = m->selection;

		dbg(1, "sample=%s", sample->name);
		if (inspector->sample) sample_unref(inspector->sample);
		inspector->sample = sample ? sample_ref(sample) : NULL;
		inspector->cache.n_rows = 13; // TODO really count the number of rows needed for this sample

		agl_actor__invalidate(actor);
	}
	behaviour_subject_connect ((GObject*)samplecat.model, "selection", (void*)inspector_on_selection_change, view);

	return (AGlActor*)view;
}


static void
inspector_free (AGlActor* actor)
{
	if (!--instance_count) {
	}

	g_free(actor);
}

