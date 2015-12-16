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
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
# define GLX_GLXEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glx.h>
#include "gtk/gtk.h"
#include <debug/debug.h>
#include "src/typedefs.h"
#include "samplecat.h"
#include "db/db.h"
#include "utils/ayyi_utils.h"
#include "agl/actor.h"
#include "agl/ext.h"
#include "waveform/waveform.h"
#include "src/list_store.h"
#include "glx.h"
#include "keys.h"
#include "views/list.h"

extern GLboolean need_draw; // TODO use scene->invalidate instead

SamplecatBackend backend = {0,};

typedef struct _Application {
   ConfigContext  config_ctx;
   Config         config;
} Application;
Application _app;
Application* app = &_app;

AGlRootActor* scene = NULL;
struct Actors {AGlActor *bg, *list, *wave; } actors = {NULL,};

static KeyHandler
	nav_up,
	nav_down;

Key keys[] = {
	{XK_Up,   nav_up},
	{XK_Down, nav_down},
};

static void add_key_handlers();
//#define MAX_DISPLAY_ROWS 20


gint
mainX (gint argc, gchar** argv)
{
	gtk_init (&argc, &argv);

	return 0;
}


#include "file_manager/mimetype.h"
#include "src/icon_theme.h"
int
main(int argc, char* argv[])
{
	_debug_ = 0;

	samplecat_init();

	gtk_init_check(&argc, &argv);
	type_init();
	pixmaps_init();
	icon_theme_init();

	Window win;
	GLXContext ctx;
	GLboolean fullscreen = GL_FALSE;
	static int width = 400, height = 300;

	int i; for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-verbose") == 0) {
			_debug_ = 1;
		}
#if 0
		else if (strcmp(argv[i], "-swap") == 0 && i + 1 < argc) {
			swap_interval = atoi( argv[i+1] );
			do_swap_interval = GL_TRUE;
			i++;
		}
		else if (strcmp(argv[i], "-forcegetrate") == 0) {
			/* This option was put in because some DRI drivers don't support the
			 * full GLX_OML_sync_control extension, but they do support
			 * glXGetMscRateOML.
			 */
			force_get_rate = GL_TRUE;
		}
#endif
		else if (strcmp(argv[i], "-fullscreen") == 0) {
			fullscreen = GL_TRUE;
		}
		else if (strcmp(argv[i], "-help") == 0) {
			printf("Usage:\n");
			printf("  glx [options]\n");
			printf("Options:\n");
			printf("  -help                   Print this information\n");
			printf("  -verbose                Output info to stdout\n");
			printf("  -swap N                 Swap no more than once per N vertical refreshes\n");
			printf("  -forcegetrate           Try to use glXGetMscRateOML function\n");
			printf("  -fullscreen             Full-screen window\n");
			return 0;
		}
	}

	Display* dpy = XOpenDisplay(NULL);
	if(!dpy){
		printf("Error: couldn't open display %s\n", XDisplayName(NULL));
		return -1;
	}

	int screen = DefaultScreen(dpy);
	make_window(dpy, "Samplcecat", (XDisplayWidth(dpy, screen) - width) / 2, (XDisplayHeight(dpy, screen) - height) / 2, width, height, fullscreen, &win, &ctx);

	agl_get_extensions();

	glx_init(dpy);

	g_main_loop_new(NULL, true);

	scene = (AGlRootActor*)agl_actor__new_root_(CONTEXT_TYPE_GLX);

	void scene_needs_redraw(AGlScene* scene, gpointer _){ need_draw = true; }
	scene->draw = scene_needs_redraw;

	gboolean add_content(gpointer _)
	{ 
		app->config_ctx.filename = g_strdup_printf("%s/.config/" PACKAGE "/" PACKAGE, g_get_home_dir());
		config_load(&app->config_ctx, &app->config);

#ifdef USE_MYSQL
		mysql__init(&app->config.mysql);
		samplecat_set_backend(BACKEND_MYSQL);
#endif

		samplecat_list_store_do_search((SamplecatListStore*)samplecat.store);

		Waveform* w = NULL;
		/*
		if(((SamplecatListStore*)samplecat.store)->row_count){
			GtkTreeIter iter;
			if(!gtk_tree_model_get_iter_first((GtkTreeModel*)samplecat.store, &iter)){ gerr ("cannot get iter."); return G_SOURCE_REMOVE; }
			Sample* sample = samplecat_list_store_get_sample_by_iter(&iter);
			if(sample){
				dbg(0, " * %s", sample->name);
				w = waveform_load_new(sample->full_path);
				sample_unref(sample);
			}
		} else dbg(0, "no results");
		*/

		WaveformCanvas* wfc = wf_canvas_new(scene);

		agl_actor__add_child((AGlActor*)scene, actors.bg = background_actor(NULL));
		actors.bg->region.x2 = 1;
		actors.bg->region.y2 = 1;

		agl_actor__add_child((AGlActor*)scene, actors.list = list_view(NULL));

		agl_actor__add_child((AGlActor*)scene, actors.wave = (AGlActor*)wf_canvas_add_new_actor(wfc, w));

		void scene_set_size(AGlActor* scene)
		{
			actors.list->region = (AGliRegion){20, 20, scene->region.x2 - 20, scene->region.y2 / 2};
			agl_actor__set_size(actors.list); // clear cache

			actors.wave->region = (AGliRegion){
				20,
				scene->region.y2 / 2,
				scene->region.x2 - 20,
				scene->region.y2 - 20
			};
			wf_actor_set_rect((WaveformActor*)actors.wave, &(WfRectangle){
				0.0,
				0.0,
				agl_actor__width(actors.wave),
				agl_actor__height(actors.wave)
			});

			need_draw = true;
		}
		((AGlActor*)scene)->set_size = scene_set_size;

		if(w) wf_actor_set_region((WaveformActor*)actors.wave, &(WfSampleRegion){0, waveform_get_n_frames(w)});

		scene_set_size((AGlActor*)scene);

		list_view_select((ListView*)actors.list, 0);

		add_key_handlers();

		return G_SOURCE_REMOVE;
	}

	g_idle_add(add_content, NULL);

	void on_selection_change(SamplecatModel* m, Sample* sample, gpointer user_data)
	{
		PF0;

		void waveform_view_plus_load_file_done(WaveformActor* a, gpointer _c)
		{
			PF0;
			// TODO not sure if we need to redraw here or not...
			//agl_actor__invalidate(((AGlActor*)a);
		}

		Waveform* waveform = waveform_new(sample->full_path);
		wf_actor_set_waveform((WaveformActor*)actors.wave, waveform, waveform_view_plus_load_file_done, NULL);
		g_object_unref(waveform);

	}
	g_signal_connect((gpointer)samplecat.model, "selection-changed", G_CALLBACK(on_selection_change), NULL);

	on_window_resize(width, height);

	event_loop(dpy, win);

	glXDestroyContext(dpy, ctx);
	XDestroyWindow(dpy, win);
	XCloseDisplay(dpy);

	return 0;
}


static void
nav_up()
{
	PF0;
	ListView* list = (ListView*)actors.list;
	list_view_select(list, list->selection - 1);
}


static void
nav_down()
{
	PF0;
	ListView* list = (ListView*)actors.list;
	list_view_select(list, list->selection + 1);
}


GHashTable* key_handlers = NULL;
static void
add_key_handlers()
{
	if(!key_handlers){
		key_handlers = g_hash_table_new(g_int_hash, g_int_equal);

		int i = 0; while(true){
			Key* key = &keys[i];
			if(i > 100 || !key->key) break;
			g_hash_table_insert(key_handlers, &key->key, key->handler);
			i++;
		}
	}
}


// temporary
void
application_emit_icon_theme_changed (Application* app, const gchar* _)
{
}


