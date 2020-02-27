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
#define __main_c__
#include "config.h"
#include <getopt.h>
#include <libgen.h>
#include <X11/keysym.h>
#include <debug/debug.h>
#include "file_manager/mimetype.h"
#include "file_manager/pixmaps.h"
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

struct Actors {AGlActor *files, *debug; } actors = {NULL,};

#if 0
static Key keys[] = {
	{0,}
};

static void add_key_handlers ();
#endif

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
main (int argc, char* argv[])
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

	g_log_set_default_handler(log_handler, NULL);

	const char* themes[] = {"breeze", NULL};
	const char* theme = find_icon_theme(themes);
	if(theme)
		set_icon_theme(theme);

	Display* dpy = XOpenDisplay(NULL);
	if(!dpy){
		printf("Error: couldn't open display %s\n", XDisplayName(NULL));
		return -1;
	}

	agl_scene_get_class()->behaviour_classes[0] = style_get_class();

	bool on_event (AGlActor* actor, GdkEvent* event, AGliPt xy)
	{
		switch(event->type){
			case GDK_KEY_PRESS:
			case GDK_KEY_RELEASE:;
				AGlActor* list = agl_actor__find_by_name(actor, "List");
				if(list){
					return list->on_event(list, event, xy);
				}
				break;
			default:
				break;
		}
		return AGL_NOT_HANDLED;
	}

	AGliPt size = get_window_size_from_settings();
	int screen = DefaultScreen(dpy);
	AGlWindow* window = agl_make_window(dpy, "Samplecat", (XDisplayWidth(dpy, screen) - size.x) / 2, (XDisplayHeight(dpy, screen) - size.y) / 2, size.x, size.y);
	app->scene = window->scene;
	((AGlActor*)app->scene)->on_event = on_event;

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


					static void load_file_done (WaveformActor* a, gpointer _c)
					{
						// TODO not sure if we need to redraw here or not...
						//agl_actor__invalidate(((AGlActor*)a);
					}

				static void on_selection_change (SamplecatModel* m, Sample* sample, gpointer actor)
				{
					PF;

					Waveform* waveform = waveform_new(sample->full_path);
					wf_actor_set_waveform((WaveformActor*)actor, waveform, load_file_done, actor);
					g_object_unref(waveform);
				}

		static void on_actor_added (Application* app, AGlActor* actor, gpointer data)
		{
			AGlActorClass* c = actor->class;
			if(c == wf_actor_get_class()){
				g_signal_connect((gpointer)samplecat.model, "selection-changed", G_CALLBACK(on_selection_change), actor);
			}
		}

		static void scene_set_size (AGlActor* scene)
		{
			dbg(2, "%i", ((AGlActor*)app->scene)->region.x2);

			AGlActor* container = ((AGlActor*)app->scene)->children->data;
			container->region = (AGlfRegion){20, 20, agl_actor__width(scene) - 20, agl_actor__height(scene) - 20};
			agl_actor__set_size(container);

#ifdef SHOW_FBO_DEBUG
			actors.debug->region = (AGlfRegion){scene->region.x2/2, 10, scene->region.x2 - 10, scene->region.x2/2};
#endif

			agl_actor__invalidate((AGlActor*)app->scene);
		}


static gboolean
add_content (gpointer _)
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
		g_warning("cannot connect to any database.");
		return EXIT_FAILURE;
	}

	samplecat_list_store_do_search((SamplecatListStore*)samplecat.store);

	application_set_auditioner();

	app->wfc = wf_context_new((AGlActor*)app->scene);

#if 0
	AGlActor* bg = agl_actor__add_child((AGlActor*)scene, background_actor(NULL));
	bg->region.x2 = 1;
	bg->region.y2 = 1;
#endif
	g_signal_connect(app, "actor-added", G_CALLBACK(on_actor_added), NULL);

	if(load_settings()){
		dbg(1, "window setting loaded ok");
		actors.files = agl_actor__find_by_name((AGlActor*)app->scene, "Files");

		application_menu_init();

		Sample* sample = samplecat_list_store_get_sample_by_row_index(0);
		if(sample){
			samplecat_model_set_selection(samplecat.model, sample);
			sample_unref(sample);
		}

#ifdef DEBUG
		if(_debug_ > 2) agl_actor__print_tree((AGlActor*)app->scene);
#endif

#ifdef SHOW_FBO_DEBUG
		agl_actor__add_child((AGlActor*)scene, actors.debug = wf_debug_actor(NULL));
		wf_debug_actor_set_actor((DebugActor*)actors.debug, actors.list);
#endif

		AGlActor* tabs = agl_actor__find_by_name((AGlActor*)app->scene, "Tabs");
		if(tabs){
			AGlActor* selected = g_list_nth_data(tabs->children, ((TabsView*)tabs)->active);
			app->scene->selected = selected;
		}
	}

	if(actors.files){
		files_view_set_path((FilesView*)actors.files, app->config.browse_dir);
	}

	((AGlActor*)app->scene)->set_size = scene_set_size;

	return G_SOURCE_REMOVE;
}


		static void scene_set_size2(AGlActor* scene)
		{
			dbg(2, "%i", ((AGlActor*)app->scene)->region.x2);

			actors.files->region = (AGlfRegion){20, 20, agl_actor__width(scene) - 10, agl_actor__height(scene) - 20};

#ifdef SHOW_FBO_DEBUG
			actors.debug->region = (AGlfRegion){scene->region.x2/2, 10, scene->region.x2 - 10, scene->region.x2/2};
#endif

			agl_actor__invalidate((AGlActor*)app->scene);
		}

static gboolean
show_directory (gpointer _)
{
	app->wfc = wf_context_new((AGlActor*)app->scene);
	((AGlActor*)app->scene)->set_size = scene_set_size2;

	AGlActor* view = actors.files = files_with_wav(NULL);
	agl_actor__add_child((AGlActor*)app->scene, view);

	((FilesWithWav*)view)->wfc = app->wfc;
	files_with_wav_set_path((FilesWithWav*)view, app->config.browse_dir);

	agl_actor__set_size((AGlActor*)app->scene);

	app->scene->selected = actors.files;

	return G_SOURCE_REMOVE;
}


#if 0
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
#endif


// temporary
void
application_emit_icon_theme_changed (Application* app, const gchar* _)
{
}


