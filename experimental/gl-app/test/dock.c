/*
 +----------------------------------------------------------------------+
 | This file is part of the Ayyi project. https://www.ayyi.org          |
 | copyright (C) 2024-2025 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#include "config.h"
#undef USE_GTK
#include <getopt.h>
#include "agl/x11.h"
#include "agl/debug.h"
#include "agl/text/text_node.h"
#include "actors/background.h"
#include "keys.h"
#include "test/common.h"
#include "test/runner.h"
#include "views/dock_h.h"
#include "../application.h"

GHashTable* agl_actor_registry;

extern AGlMaterialClass aaline_class;

Application* app = NULL;

#define N_LINES 16

TestFn test1;

gpointer tests[] = {
	test1,
};

static KeyHandler
	zoom_in,
	zoom_out;

Key keys[] = {
	{XK_equal,       zoom_in},
	{XK_KP_Add,      zoom_in},
	{XK_minus,       zoom_out},
	{XK_KP_Subtract, zoom_out},
	{XK_Escape,      (KeyHandler*)exit},
	{XK_q,           (KeyHandler*)exit},
};


static gboolean
send_quit (gpointer window)
{
	AGlActor* dock = agl_actor__find_by_class((AGlActor*)((AGlWindow*)window)->scene, dock_h_get_class());
	agl_send_key_event (window, dock, /*XK_q*/"q", 0/*XK_Control_L*/);

	return G_SOURCE_REMOVE;
}


void
test1 ()
{
	START_TEST;

	const int width = 400, height = 200;

	agl_scene_get_class()->behaviour_classes[0] = style_get_class();

	AGlWindow* window = agl_window("Text test", 0, 0, width, height, 0);
	XMapWindow(dpy, window->window);

	if (g_getenv("NON_INTERACTIVE")) {
		g_timeout_add(3000, (gpointer)send_quit, window);
	}

	AGlActor* text = agl_actor__add_child((AGlActor*)window->scene, text_node(NULL));
	text_node_set_text((TextNode*)text, g_strdup("Dock test"));
	text->colour = 0xbbbbbbff;
	text->region = (AGlfRegion){.x2 = 80, .y2 = 30};

	AGlActor* dock = agl_actor__add_child((AGlActor*)window->scene, dock_h_view(NULL));
	dock->region = (AGlfRegion){.x2 = 360, .y2 = 150};

	for (int i=0;i<2;i++) {
		AGlActor* panel = panel_view(NULL);
		agl_actor__add_child(panel, background_actor(NULL));
		dock_h_add_panel((DockHView*)dock, panel);
	}

	add_key_handlers(keys);

	gboolean check ()
	{
		// TODO check layout
		return G_SOURCE_REMOVE;
	}
	g_idle_add(check, NULL);

	g_main_loop_run(agl_main_loop_new());

	agl_window_destroy(&window);
	XCloseDisplay(dpy);

	FINISH_TEST;
}


void
setup (char* argv[])
{
	app = application_new();

	TEST.n_tests = G_N_ELEMENTS(tests);
}


void
teardown ()
{
}


static void
zoom_in ()
{
}


static void
zoom_out ()
{
}
