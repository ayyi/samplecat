/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. https://ayyi.github.io/samplecat/    |
 | copyright (C) 2016-2023 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#include "config.h"
#include "agl/ext.h"
#include "agl/utils.h"
#include "agl/event.h"
#include "agl/text/pango.h"
#include "debug/debug.h"
#include "waveform/shader.h"
#include "samplecat/typedefs.h"
#include "keys.h"
#include "shader.h"
#include "materials/icon_ring.h"
#include "application.h"
#include "views/dock_v.h"
#include "views/dock_h.h"
#include "views/overlay.h"
#include "views/panel.h"

extern AGlShader ring;

static void panel_free (AGlActor*);

static AGl* agl = NULL;
static AGlActorClass actor_class = {0, "Panel", (AGlActorNew*)panel_view, panel_free};
static int instance_count = 0;
static AGliPt origin = {0,};
static AGliPt mouse = {0,};
static char* font = NULL;

extern AGlMaterialClass ring_material_class;
static AGlMaterial* ring_material = NULL;

static void get_drop_location (AGlActor* actor, AGlActor* picked, AGlActor** dock, AGliPt* dock_position, AGlActor** insert_at);


AGlActorClass*
panel_view_get_class ()
{
	return &actor_class;
}


static void
_init ()
{
	if (!agl) {
		agl = agl_get_instance();

		font = g_strdup_printf("%s 10", APP_STYLE.font);
		agl_set_font_string(font); // initialise the pango context

		ring_material = ring_new();
	}
}


typedef struct {
	AGliPt mouse;
} Abs;


AGlActor*
panel_view (gpointer _)
{
	instance_count++;

	_init();

	bool panel_paint (AGlActor* actor)
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
				PLAIN_COLOUR2 (agl->shaders.plain) = 0x6677ff77;
				agl_use_program (agl->shaders.plain);
				agl_box (1, offset.x, offset.y, agl_actor__width(actor), agl_actor__height(actor));

				// show drop point
				AGliPt position2 = {(int)actor->region.x1 + offset.x, (int)actor->region.y1 + offset.y};
				if(position2.y > -1){
					AGlActor* picked = agl_actor__pick(actor, mouse);

					if(picked){
						OverlayView* overlay = (OverlayView*)agl_actor__find_by_class((AGlActor*)actor->root, overlay_view_get_class());
						if(!overlay)
							overlay = (OverlayView*)overlay_view (actor->root);

						AGlActor* dock = NULL;
						AGlActor* insert_at = NULL;
						AGliPt dock_offset = {-1, -1};
						get_drop_location(actor, picked, &dock, &dock_offset, &insert_at);
						if(dock && insert_at){
							overlay_set_insert_pos(overlay, (AGliRegion){
								dock_offset.x + insert_at->region.x1,
								dock_offset.y + insert_at->region.y1,
								dock_offset.x + insert_at->region.x1 + agl_actor__width(insert_at),
								dock_offset.y + insert_at->region.y1 + agl_actor__height(insert_at)
							});
						}else{
							overlay_set_insert_pos(overlay, (AGliRegion){-1000, -1000, -1000, -1000});
						}
					}
				}
			}else{
				PLAIN_COLOUR2 (agl->shaders.plain) = 0x6677ff33;
				agl_use_program (agl->shaders.plain);
				agl_rect (0, 0, agl_actor__width(actor), PANEL_DRAG_HANDLE_HEIGHT);
			}
		}

		return true;
	}

	void panel_init (AGlActor* actor)
	{
		PanelView* panel = (PanelView*)actor;

		if(panel->title){
			panel->layout = pango_layout_new (agl_pango_get_context());
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

	void panel_set_size (AGlActor* actor)
	{
		PanelView* panel = (PanelView*)actor;

		// first child takes all space of panel
		// (2nd child could be scrollbar)
		if (actor->children) {
			AGlActor* child = actor->children->data;
			child->region = (AGlfRegion){0, panel->title ? PANEL_DRAG_HANDLE_HEIGHT : 0, agl_actor__width(actor), agl_actor__height(actor)};
			agl_actor__set_size (child);
		}
	}

	bool panel_event (AGlActor* actor, AGlEvent* event, AGliPt xy)
	{
		PanelView* panel = (PanelView*)actor;

		switch (event->type) {
			case AGL_BUTTON_PRESS:
				dbg(1, "PRESS %i", xy.y);
				if (xy.y < PANEL_DRAG_HANDLE_HEIGHT) {
					actor_context.grabbed = actor;
					origin = mouse = xy;
					agl_actor__invalidate(actor);
					return AGL_HANDLED;
				}
				break;
			case AGL_BUTTON_RELEASE:
				agl_actor__invalidate(actor);
				dbg(1, "RELEASE y=%i", xy.y);
				if (actor_context.grabbed) {
					if (actor->parent->class == dock_v_get_class()) {

						AGlActor* picked = agl_actor__pick(actor, mouse);

						if (picked) {
							AGlActor* dock = NULL;
							AGlActor* insert_at = NULL;
							AGliPt dock_offset = {-1, -1};
							get_drop_location(actor, picked, &dock, &dock_offset, &insert_at);
							if (dock && insert_at) {
								// TODO use dock and insert at to handle inter-dock dragging
								dock_v_move_panel_to_y((DockVView*)actor->parent, actor, (int)actor->region.y1 + xy.y);
							}
						}
					}
					actor_context.grabbed = NULL;
					return AGL_HANDLED;
				}
				break;
			case AGL_MOTION_NOTIFY:
				if (actor_context.grabbed == actor) {
					mouse = xy;
					agl_actor__invalidate(actor);
					return AGL_HANDLED;
				}
				break;
			case AGL_KEY_RELEASE:;
				AGlEventKey* e = (AGlEventKey*)event;
				int keyval = e->keyval;
				if (panel->actions.actions) {
					KeyHandler* handler = g_hash_table_lookup(panel->actions.actions, &keyval);
					if (handler) handler();
					return AGL_HANDLED;
				}
				break;
			default:
				break;
		}
		return AGL_NOT_HANDLED;
	}

	PanelView* view = agl_actor__new(PanelView,
		.actor = {
			.class = &actor_class,
			.init = panel_init,
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


static void
panel_free (AGlActor* actor)
{
	PanelView* panel = (PanelView*)actor;

	g_clear_pointer(&panel->layout, g_object_unref);

	if (!--instance_count) {
	}

	g_free(actor);
}


static AGlActor*
find_dock (AGlActor* parent)
{
	do {
		if (parent->class == dock_v_get_class() || parent->class == dock_h_get_class()) {
			return parent;
		}
	} while ((parent = parent->parent));

	return NULL;
}


static void
get_drop_location (AGlActor* actor, AGlActor* picked, AGlActor** dock, AGliPt* dock_position, AGlActor** insert_at)
{
	*dock = find_dock(picked);
	picked = *dock;

	if (picked) {
		// TODO maybe pick could return the offset
		AGliPt dock_offset = agl_actor__find_offset(picked);
		AGliPt offset2 = agl_actor__find_offset(actor);
		if ((*dock)->class == dock_v_get_class()) {
			Abs abs = {
				.mouse = {offset2.x + mouse.x, offset2.y + mouse.y}
			};
			int position_in_dock = abs.mouse.y - dock_offset.y;
			for (GList* l = ((DockVView*)*dock)->panels; l; l = l->next) {
				AGlActor* a = l->data;
				if (position_in_dock < (int)(a->region.y1 + agl_actor__height(a)) - MIN(40, (int)(agl_actor__height(a) / 2))) {
					if (a != actor){
						*dock_position = dock_offset;
						*insert_at = a;
					}
					return;
				}
			}
		} else if((*dock)->class == dock_h_get_class()) {
			// TODO
		}
	}
}
