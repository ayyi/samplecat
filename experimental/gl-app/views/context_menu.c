/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2016-2019 Tim Orford <tim@orford.org>                  |
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
#include "debug/debug.h"
#include "agl/utils.h"
#include "agl/shader.h"
#include "waveform/promise.h"
#include "waveform/utils.h"
#include "application.h"
#include "views/context_menu.h"

#define BORDER 4

static AGl* agl = NULL;

static AGlActorClass actor_class = {0, "Context menu", (AGlActorNew*)context_menu};
static AMPromise* _promise = NULL;
static AGlWindow* popup = NULL;
static MenuItem* menu_items = NULL;
static Menu* _menu = NULL;

static float hover_opacity = 0;
static int hover_row = 0;
static WfAnimatable animatable = {
	.start_val.f = 0.0,
	.val.f       = 0.0,
	.type        = WF_FLOAT,
	.model_val.f = &hover_opacity
};


AGlWindow*
context_menu_open_new (AGlScene* parent, AGliPt xy, Menu* menu, AMPromise* promise)
{
	g_return_val_if_fail(!_promise, NULL);
	_promise = promise;
	_menu = menu;

	Display* dpy = glXGetCurrentDisplay();
	int screen = DefaultScreen(dpy);
	AGliPt size = {240, menu->len * 16 + 2 * BORDER};

	int x, y;
	Window child;
	Window window = parent->gl.glx.window;
	XWindowAttributes attr;
	XGetWindowAttributes(dpy, window, &attr);
	XTranslateCoordinates(dpy, window, attr.root, 0, 0, &x, &y, &child);
	AGliPt offset = {x - attr.x, y - attr.y};

	AGlScene* scene = (AGlRootActor*)agl_actor__new_root_(CONTEXT_TYPE_GLX);
	agl_actor__add_child((AGlActor*)scene, scene->selected = context_menu(promise));

	return popup = agl_make_window(dpy, "Popup", xy.x + offset.x, xy.y + offset.y, size.x, size.y, scene);
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


static void
context_menu_init (AGlActor* actor)
{
}


static void
context_menu_free (AGlActor* actor)
{
	if(_promise){
		am_promise_resolve(_promise, 0);
		_promise = NULL;
	}
}


static bool
context_menu_paint (AGlActor* actor)
{
	agl_set_font_string("Roboto 10");

	agl->shaders.plain->uniform.colour = 0xffffff00 + (int)(31.0 * animatable.val.f);
	agl_use_program((AGlShader*)agl->shaders.plain);
	agl_rect(-BORDER, hover_row * 16, agl_actor__width(actor), 16);

	for(int i=0; i<_menu->len; i++){
		MenuItem* item = &_menu->items[i];
		agl_print(4, i * 16, 0, app->style.text, item->title);
		if(item->key.code) agl_print(200, i * 16, 0, app->style.text, "%i", item->key.code);
	}

	return true;
}


static bool
popup_destroy ()
{
	am_promise_resolve(_promise, 0);
	_promise = NULL;

	agl_window_destroy(glXGetCurrentDisplay(), &popup);

	return G_SOURCE_REMOVE;
}


static bool
context_menu_event (AGlActor* actor, GdkEvent* event, AGliPt xy)
{
	switch(event->type){
		case GDK_BUTTON_PRESS:
			switch(event->button.button){
				case 1:;
					int row = xy.y / 16;
					if(row >= 0 && row < _menu->len){
						MenuItem* item = &_menu->items[row];
						if(item->action) item->action(NULL);
					}

					g_idle_add(popup_destroy, NULL);
					break;
			}
			break;
		case GDK_ENTER_NOTIFY:
			hover_row = xy.y / 16;
			hover_opacity = 1.0;
			agl_actor__start_transition(actor, g_list_append(NULL, &animatable), NULL, NULL);
			break;
		case GDK_LEAVE_NOTIFY:
			hover_opacity = 0.0;
			agl_actor__start_transition(actor, g_list_append(NULL, &animatable), NULL, NULL);
			break;
		case GDK_MOTION_NOTIFY:;
			int _hover_row = xy.y / 16;
			if(_hover_row != hover_row){
				hover_row = _hover_row;
				agl_actor__invalidate(actor);
			}
			break;
		case GDK_FOCUS_CHANGE:
			g_idle_add(popup_destroy, NULL);
			break;
	}
}


AGlActor*
context_menu (gpointer _)
{
	_init();

	return AGL_NEW(AGlActor,
		.class = &actor_class,
		.name = "Context menu",
		.region = {BORDER, BORDER, BORDER + 240, BORDER + _menu->len * 16},
		.init = context_menu_init,
		.free = context_menu_free,
		.paint = context_menu_paint,
		.on_event = context_menu_event
	);
}


