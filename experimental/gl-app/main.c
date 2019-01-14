/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2007-2018 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#define __main_c__
#include "config.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <libgen.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#define GLX_GLXEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glx.h>
#include <gtk/gtk.h>
#include <debug/debug.h>
#include "agl/actor.h"
#include "agl/ext.h"
#include "file_manager/mimetype.h"
#include "file_manager/pixmaps.h"
#include "src/typedefs.h"
#include "samplecat.h"
#include "utils/ayyi_utils.h"
#include "waveform/waveform.h"
#include "icon/utils.h"
#include "yaml_utils.h"
#include "application.h"
#include "glx.h"
#include "keys.h"
#include "layout.h"
#include "views/dock_h.h"
#include "views/dock_v.h"
#include "views/panel.h"
#include "views/list.h"
#include "views/files.h"
#include "views/files.with_wav.h"
#include "views/scrollbar.h"
#include "views/dirs.h"
#include "views/search.h"
#include "views/tabs.h"
#ifdef SHOW_FBO_DEBUG
#include "waveform/actors/debug.h"
#endif

Application* app = NULL;

struct Actors {AGlActor *bg, *hdock, *vdock1, *vdock2, *list, *files, *wave, *search, *tabs, *debug; } actors = {NULL,};

static KeyHandler
	nav_up,
	nav_down;

Key keys[] = {
	{XK_Up,   nav_up},
	{XK_Down, nav_down},
	{0,}
};

static void add_key_handlers ();

static gboolean show_directory (gpointer);
static gboolean add_content    (gpointer);


static const struct option long_options[] = {
  { "dir",              0, NULL, 'd' },
  { "help",             0, NULL, 'h' },
};

static const char* const short_options = "d:h";

static const char* const usage =
	"Usage: %s [OPTIONS]\n\n"
	"SampleCat is a program for cataloguing and auditioning audio samples.\n"
	"\n"
	"Options:\n"
	"  -d, --dir              show contents of filesystem directory (temporary view, state not saved).\n";

int
main(int argc, char* argv[])
{
	_debug_ = 0;

	app = application_new();

	gtk_init_check(&argc, &argv);

	int opt;
	while((opt = getopt_long (argc, argv, short_options, long_options, NULL)) != -1) {
		switch(opt) {
			case 'v':
				_debug_ = 1;
				break;
			case 'd':
				app->temp_view = true;
				g_strlcpy(app->config.browse_dir, optarg, PATH_MAX);
				dbg(0, "dir=%s", optarg);
				break;
			case 'h':
				printf(usage, basename(argv[0]));
				exit(EXIT_SUCCESS);
				break;
			case '?':
			default:
				printf(usage, basename(argv[0]));
				exit(EXIT_FAILURE);
		}
	}

	type_init();
	pixmaps_init();
#if 0
	icon_theme_init();
#endif

	Display* dpy = XOpenDisplay(NULL);
	if(!dpy){
		printf("Error: couldn't open display %s\n", XDisplayName(NULL));
		return -1;
	}

	app->scene = (AGlScene*)agl_actor__new_root_(CONTEXT_TYPE_GLX);

	AGliPt size = get_window_size_from_settings();
	int screen = DefaultScreen(dpy);
	AGlWindow* window = agl_make_window(dpy, "Samplecat", (XDisplayWidth(dpy, screen) - size.x) / 2, (XDisplayHeight(dpy, screen) - size.y) / 2, size.x, size.y, app->scene);

	if(app->temp_view){
		g_idle_add(show_directory, NULL);
	}else{
		g_idle_add(add_content, NULL);
	}

#ifdef USE_GLIB_LOOP
	GMainLoop* mainloop = main_loop_new(dpy, window->window);
#else
	g_main_loop_new(NULL, true);
#endif

	on_window_resize(dpy, window, size.x, size.y);

#ifdef USE_GLIB_LOOP
	g_main_loop_run(mainloop);
#else
	event_loop(dpy);
#endif

	if(!app->temp_view){
		save_settings();
	}

	agl_window_destroy(dpy, &window);
	XCloseDisplay(dpy);

	return 0;
}


					static void load_file_done(WaveformActor* a, gpointer _c)
					{
						// TODO not sure if we need to redraw here or not...
						//agl_actor__invalidate(((AGlActor*)a);
					}

				static void on_selection_change(SamplecatModel* m, Sample* sample, gpointer actor)
				{
					PF;

					Waveform* waveform = waveform_new(sample->full_path);
					wf_actor_set_waveform((WaveformActor*)actor, waveform, load_file_done, actor);
					g_object_unref(waveform);
				}

		static void on_actor_added(Application* app, AGlActor* actor, gpointer data)
		{
			AGlActorClass* c = actor->class;
			if(c == wf_actor_get_class()){
				g_signal_connect((gpointer)samplecat.model, "selection-changed", G_CALLBACK(on_selection_change), actor);
			}
		}

		static void scene_set_size(AGlActor* scene)
		{
			dbg(2, "%i", ((AGlActor*)app->scene)->region.x2);

			actors.hdock->region = (AGliRegion){20, 20, agl_actor__width(scene) - 20, agl_actor__height(scene) - 20};
			agl_actor__set_size(actors.hdock);

// not needed?
			agl_actor__set_size(actors.list); // clear cache

#ifdef SHOW_FBO_DEBUG
			actors.debug->region = (AGliRegion){scene->region.x2/2, 10, scene->region.x2 - 10, scene->region.x2/2};
#endif

			agl_actor__invalidate((AGlActor*)app->scene);
		}


static gboolean
add_content(gpointer _)
{
	config_load(&app->config_ctx, &app->config);

	if (app->config.database_backend && can_use(samplecat.model->backends, app->config.database_backend)) {
		#define list_clear(L) g_list_free(L); L = NULL;
		list_clear(samplecat.model->backends);
		samplecat_model_add_backend(app->config.database_backend);
	}

	db_init(
#ifdef USE_MYSQL
		&app->config.mysql
#else
		NULL
#endif
	);
	if (!db_connect()) {
		g_warning("cannot connect to any database.\n");
		return EXIT_FAILURE;
	}

	samplecat_list_store_do_search((SamplecatListStore*)samplecat.store);

	application_set_auditioner();

	Waveform* w = NULL;

	app->wfc = wf_context_new(app->scene);

#if 0
	agl_actor__add_child((AGlActor*)scene, actors.bg = background_actor(NULL));
	actors.bg->region.x2 = 1;
	actors.bg->region.y2 = 1;
#endif
	g_signal_connect(app, "actor-added", G_CALLBACK(on_actor_added), NULL);

	if(load_settings()){
		dbg(1, "window setting loaded ok");
		actors.hdock = agl_actor__find_by_name((AGlActor*)app->scene, "Dock H");
		actors.list = agl_actor__find_by_name((AGlActor*)app->scene, "List");
		actors.files = agl_actor__find_by_name((AGlActor*)app->scene, "Files");

		Sample* sample = samplecat_list_store_get_sample_by_row_index(0);
		if(sample){
			samplecat_model_set_selection(samplecat.model, sample);
			sample_unref(sample);
		}

#ifdef DEBUG
		if(_debug_ > 2) agl_actor__print_tree((AGlActor*)app->scene);
#endif
	}

#ifdef SHOW_FBO_DEBUG
	agl_actor__add_child((AGlActor*)scene, actors.debug = wf_debug_actor(NULL));
	wf_debug_actor_set_actor((DebugActor*)actors.debug, actors.list);
#endif

	files_view_set_path((FilesView*)actors.files, app->config.browse_dir);

	((AGlActor*)app->scene)->set_size = scene_set_size;

	if(w) wf_actor_set_region((WaveformActor*)actors.wave, &(WfSampleRegion){0, waveform_get_n_frames(w)});

	//scene_set_size((AGlActor*)app->scene);

	// TODO how do these handlers interact with individual view key handlers?
	add_key_handlers();

	return G_SOURCE_REMOVE;
}


		static void scene_set_size2(AGlActor* scene)
		{
			dbg(2, "%i", ((AGlActor*)app->scene)->region.x2);

			actors.files->region = (AGliRegion){20, 20, agl_actor__width(scene) - 10, agl_actor__height(scene) - 20};

#ifdef SHOW_FBO_DEBUG
			actors.debug->region = (AGliRegion){scene->region.x2/2, 10, scene->region.x2 - 10, scene->region.x2/2};
#endif

			agl_actor__invalidate((AGlActor*)app->scene);
		}

static gboolean
show_directory(gpointer _)
{
	AGlActor* root = (AGlActor*)app->scene;

	app->wfc = wf_context_new(app->scene);
	((AGlActor*)app->scene)->set_size = scene_set_size2;

	AGlActor* view = actors.files = files_with_wav(NULL);
	agl_actor__add_child((AGlActor*)app->scene, view);

	((FilesWithWav*)view)->wfc = app->wfc;
	files_with_wav_set_path((FilesWithWav*)view, app->config.browse_dir);

	agl_actor__set_size((AGlActor*)app->scene);

	app->scene->selected = actors.files;
	add_key_handlers();

	return G_SOURCE_REMOVE;
}


static void
nav_up()
{
	PF0;
	if(actors.list){
		ListView* list = (ListView*)actors.list;
		list_view_select(list, list->selection - 1);
	}

	if(app->scene->selected){
		if(app->scene->selected == actors.files){

			FilesView* files = (FilesView*)actors.files;
			files_with_wav_select((FilesWithWav*)files, files->view->selection - 1);
		}
	}
}


static void
nav_down()
{
	PF0;
	if(actors.list){
		ListView* list = (ListView*)actors.list;
		list_view_select(list, list->selection + 1);
	}else{
		if(app->scene->selected){
			if(app->scene->selected == actors.files){

				FilesView* files = (FilesView*)actors.files;
				files_with_wav_select((FilesWithWav*)files, files->view->selection + 1);
			}
		}
	}
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


