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
#include "config.h"
#undef USE_GTK
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "agl/ext.h"
#include "agl/utils.h"
#include "agl/pango_render.h"
#include "debug/debug.h"
#include "waveform/shader.h"
#include "samplecat/typedefs.h"
#include "keys.h"
#include "shader.h"
#include "materials/icon_ring.h"
#include "application.h"
#include "views/dock_v.h"
#include "views/panel.h"

extern AGlShader ring;

#define _g_object_unref0(var) ((var == NULL) ? NULL : (var = (g_object_unref (var), NULL)))

static AGl* agl = NULL;
static AGlActorClass actor_class = {0, "Panel", (AGlActorNew*)panel_view};
static int instance_count = 0;
static AGliPt origin = {0,};
static AGliPt mouse = {0,};
static char* font = NULL;

extern AGlMaterialClass ring_material_class;
static AGlMaterial* ring_material = NULL;


AGlActorClass*
panel_view_get_class ()
{
	return &actor_class;
}


static void
_init()
{
	static bool init_done = false;

	if(!init_done){
		agl = agl_get_instance();

		font = g_strdup_printf("%s 10", app->style.font);
		agl_set_font_string(font); // initialise the pango context

		ring_material = ring_new();

		init_done = true;
	}
}


AGlActor*
panel_view(gpointer _)
{
	instance_count++;

	_init();

	bool panel_paint(AGlActor* actor)
	{
		PanelView* panel = (PanelView*)actor;

#ifdef SHOW_PANEL_BACKGROUND
		agl->shaders.plain->uniform.colour = 0xffff0033;
		agl_use_program((AGlShader*)agl->shaders.plain);
		agl_rect_((AGlRect){0, 0, agl_actor__width(actor), agl_actor__height(actor)});
#endif

		if(panel->title){
			IconMaterial* icon = (IconMaterial*)ring_material;

			icon->chr = panel->title[0];
			icon->colour = ((AGlActor*)actor->children->data)->colour;
			icon->layout = panel->layout;

			ring_material_class.render(ring_material);

			agl_set_font_string(font);
			agl_print(24, 0, 0, 0x777777ff, panel->title);
		}

		if(actor_context.grabbed == actor){
			AGliPt offset = {mouse.x - origin.x, mouse.y - origin.y};
			if(ABS(offset.x) > 1 || ABS(offset.y) > 1){
				agl->shaders.plain->uniform.colour = 0x6677ff77;
				agl_use_program((AGlShader*)agl->shaders.plain);
				agl_box(1, offset.x, offset.y, agl_actor__width(actor), agl_actor__height(actor));
			}else{
				agl->shaders.plain->uniform.colour = 0x6677ff33;
				agl_use_program((AGlShader*)agl->shaders.plain);
				agl_rect(0, 0, agl_actor__width(actor), PANEL_DRAG_HANDLE_HEIGHT);
			}
		}

		return true;
	}

	void panel_init(AGlActor* actor)
	{
		PanelView* panel = (PanelView*)actor;

		if(panel->title){
			PangoGlRendererClass* PGRC = g_type_class_peek(PANGO_TYPE_GL_RENDERER);
			panel->layout = pango_layout_new (PGRC->context);
			char text[2] = {panel->title[0], 0};
			pango_layout_set_text(panel->layout, text, -1);

			PangoFontDescription* font_desc = pango_font_description_new();
			pango_font_description_set_family(font_desc, "Sans");

			pango_font_description_set_size(font_desc, 7 * PANGO_SCALE);
			pango_font_description_set_weight(font_desc, PANGO_WEIGHT_BOLD);
			pango_layout_set_font_description(panel->layout, font_desc);

			pango_font_description_free (font_desc);
		}
	}

	void panel_set_size(AGlActor* actor)
	{
		PanelView* panel = (PanelView*)actor;

		// single child takes all space of panel
		if(g_list_length(actor->children) == 1){
			AGlActor* child = actor->children->data;
			child->region = (AGlfRegion){0, panel->title ? PANEL_DRAG_HANDLE_HEIGHT : 0, agl_actor__width(actor), agl_actor__height(actor)};
			agl_actor__set_size(child);
		}
	}

	bool panel_event(AGlActor* actor, GdkEvent* event, AGliPt xy)
	{
		PanelView* panel = (PanelView*)actor;

		switch(event->type){
			case GDK_BUTTON_PRESS:
				dbg(1, "PRESS %i", xy.y);
				if(xy.y < PANEL_DRAG_HANDLE_HEIGHT){
					actor_context.grabbed = actor;
					origin = mouse = xy;
					agl_actor__invalidate(actor);
				}
				break;
			case GDK_BUTTON_RELEASE:
				agl_actor__invalidate(actor);
				dbg(1, "RELEASE y=%i", xy.y);
				if(actor_context.grabbed){
					dock_v_move_panel_to_y((DockVView*)actor->parent, actor, xy.y);
					actor_context.grabbed = NULL;
				}
				break;
			case GDK_MOTION_NOTIFY:
				if(actor_context.grabbed == actor){
					mouse = xy;
					agl_actor__invalidate(actor);
				}
				break;
			case GDK_KEY_RELEASE:;
				GdkEventKey* e = (GdkEventKey*)event;
				int keyval = e->keyval;
				KeyHandler* handler = g_hash_table_lookup(panel->actions.actions, &keyval);
				if(handler) handler();
				break;
			default:
				break;
		}
		return AGL_HANDLED;
	}

	void panel_free(AGlActor* actor)
	{
		PanelView* panel = (PanelView*)actor;

		_g_object_unref0(panel->layout);

		if(!--instance_count){
		}
	}

	PanelView* view = AGL_NEW(PanelView,
		.actor = {
			.class = &actor_class,
			.name = "Panel",
			.init = panel_init,
			.free = panel_free,
			.paint = panel_paint,
			.set_size = panel_set_size,
			.on_event = panel_event,
		},
		.size_req = {
			.min = {-1, -1},
			.preferred = {-1, -1},
			.max = {-1, -1}
		}
	);

	return (AGlActor*)view;
}


