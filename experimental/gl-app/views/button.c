/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
 | copyright (C) 2013-2023 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#include "config.h"
#include "debug/debug.h"
#include "agl/utils.h"
#include "agl/event.h"
#include "samplecat/support.h"
#include "utils.h"
#include "shader.h"
#include "application.h"
#include "button.h"

#define SIZE 24
#define BG_OPACITY(B) (button->bg_opacity)
#define RIPPLE(B) (button->ripple_radius)

static AGl* agl = NULL;

struct {
	AGliPt pt;
} GlButtonPress;

static void button_free     (AGlActor*);
static bool button_on_event (AGlActor*, AGlEvent*, AGliPt);

static AGlActorClass actor_class = {0, "Button", (AGlActorNew*)button, button_free};


AGlActorClass*
button_get_class ()
{
	return &actor_class;
}


static void
_init ()
{
	if (!agl) {
		agl = agl_get_instance();
	}
}


AGlActor*
button (int* icon, ButtonAction action, ButtonGetState get_state, gpointer user_data)
{
	_init();

	bool button_paint (AGlActor* actor)
	{
		ButtonActor* button = (ButtonActor*)actor;
		StyleBehaviour* style = &STYLE;

		bool state = button->get_state ? button->get_state(actor, button->user_data) : false;

		AGlRect r = {
			.w = agl_actor__width(actor),
			.h = agl_actor__height(actor)
		};

		// background
		if (RIPPLE(button) > 0.0) {
			PLAIN_COLOUR2 (agl->shaders.plain) = state
				? colour_mix(style->bg, style->bg_selected, RIPPLE(button) / 64.0)
				: colour_mix(style->bg_selected, style->bg, RIPPLE(button) / 64.0);
		} else if (state) {
			PLAIN_COLOUR2 (agl->shaders.plain) = style->bg_selected;
		} else {
			PLAIN_COLOUR2 (agl->shaders.plain) = style->bg;
		}

		// hover background
		if (BG_OPACITY(button) > 0.0) { // dont get events if disabled, so no need to check state (TODO turn off hover when disabling).
			float alpha = button->bg_opacity;
			uint32_t fg = colour_lighter(PLAIN_COLOUR2 (agl->shaders.plain), 16);
			PLAIN_COLOUR2 (agl->shaders.plain) = style->bg ? colour_mix(PLAIN_COLOUR2 (agl->shaders.plain), fg, alpha) : (fg & 0xffffff00) + (uint32_t)(alpha * 0xff);
		}

		if (!(RIPPLE(button) > 0.0)) {
			agl_use_program (agl->shaders.plain);
			agl_rect_(r);

		} else {
			// click ripple
			//int radius = RIPPLE(button);
#undef RIPPLE_EXPAND
#ifdef RIPPLE_EXPAND
			circle_shader.uniform.radius = radius;
			agl_use_program((AGlShader*)&circle_shader);
			AGlRect rr = r;
			rr.w = rr.h = radius * 2;
			agl_translate((AGlShader*)&circle_shader, GlButtonPress.pt.x - actor->region.x1 - radius, GlButtonPress.pt.y - radius);
			agl_rect_(rr);
			agl_translate((AGlShader*)&circle_shader, -(GlButtonPress.pt.x - actor->region.x1 - radius), -(GlButtonPress.pt.y - radius));
#else
			CIRCLE_COLOUR() = colour_lighter (PLAIN_COLOUR2 (agl->shaders.plain), 32);
			CIRCLE_BG_COLOUR() = PLAIN_COLOUR2 (agl->shaders.plain);
			circle_shader.uniform.centre = GlButtonPress.pt;
			circle_shader.uniform.radius = RIPPLE(button);
			agl_use_program((AGlShader*)&circle_shader);
			agl_rect_(r);
#endif
		}

		//icon
		agl->shaders.texture->uniform.fg_colour = 0xffffff00 + (agl_actor__is_disabled(actor) ? 0x77 : 0xff);
		agl_use_program((AGlShader*)agl->shaders.texture);

#if 0
		agl_textured_rect(bg_textures[*button->icon + (state ? 1 : 0)], r.x, r.y + r.h / 2.0 - 8.0, SIZE, SIZE, NULL);
#else
		agl_textured_rect(*button->icon, r.x, (agl_actor__height(actor) - SIZE) / 2.0, SIZE, SIZE, NULL);
#endif
		//texture_box(arrange, bg_textures[button->icon], get_style_bg_color_rgba(GTK_STATE_NORMAL), 10.0, -10.0, 32.0, 32.0);

		//texture_box(arrange, bg_textures[TEXTURE_BG], get_style_bg_color_rgba(GTK_STATE_NORMAL), 200.0, 10.0, 50.0, 50.0);
		//texture_box(arrange, bg_textures[TEXTURE_ICON], 0xffffffff, 200.0, 61.0, w, h);

		return true;
	}

	void button_set_size (AGlActor* actor)
	{
		actor->region.y2 = actor->parent->region.y2 - actor->parent->region.y1;
	}

	void button_init (AGlActor* actor)
	{
		if (!circle_shader.shader.program) agl_create_program(&circle_shader.shader);
	}

	ButtonActor* button = agl_actor__new(ButtonActor,
		.actor = {
			.class    = &actor_class,
			.init     = button_init,
			.paint    = button_paint,
			.set_size = button_set_size,
			.on_event = button_on_event,
		},
		.icon      = (guint*)icon,
		.action    = action,
		.get_state = get_state,
		.user_data = user_data,
	);

	button->animatables[0] = AGL_NEW(WfAnimatable,
		.val.f        = &button->bg_opacity,
		.start_val.f  = 0.0,
		.target_val.f = 0.0,
		.type         = WF_FLOAT
	);

	button->animatables[1] = AGL_NEW(WfAnimatable,
		.val.f        = &button->ripple_radius,
		.start_val.f  = 0.0,
		.target_val.f = 0.0,
		.type         = WF_FLOAT
	);

	return (AGlActor*)button;
}


static void
button_free (AGlActor* actor)
{
	g_free(((ButtonActor*)actor)->animatables[0]);
	g_free(((ButtonActor*)actor)->animatables[1]);
}


static bool
button_on_event (AGlActor* actor, AGlEvent* event, AGliPt xy)
{
	ButtonActor* button = (ButtonActor*)actor;

	if (button->disabled) return AGL_NOT_HANDLED;

	void animation_done (WfAnimation* animation, gpointer user_data)
	{
	}

	void ripple_done (WfAnimation* animation, gpointer user_data)
	{
		ButtonActor* button = user_data;
		RIPPLE(button) = 0.0;
	}

	switch (event->type) {
		case AGL_ENTER_NOTIFY:
			dbg (1, "ENTER_NOTIFY");
			//set_cursor(actor->root->widget->window, CURSOR_H_DOUBLE_ARROW);

			button->animatables[0]->target_val.f = 1.0;
			agl_actor__start_transition(actor, g_list_append(NULL, button->animatables[0]), animation_done, NULL);
			return AGL_HANDLED;
		case AGL_LEAVE_NOTIFY:
			dbg (1, "LEAVE_NOTIFY");
			//set_cursor(actor->root->widget->window, CURSOR_NORMAL);

			button->animatables[0]->target_val.f = 0.0;
			agl_actor__start_transition(actor, g_list_append(NULL, button->animatables[0]), animation_done, NULL);
			return AGL_HANDLED;
		case AGL_BUTTON_PRESS:
			return AGL_HANDLED;
		case AGL_BUTTON_RELEASE:
			dbg(1, "BUTTON_RELEASE");
			if (event->button.button == 1) {
				call(button->action, actor, button->user_data);

				GlButtonPress.pt = xy;
				button->ripple_radius = 0.001;
				button->animatables[1]->target_val.f = 64.0;
				agl_actor__start_transition(actor, g_list_append(NULL, button->animatables[1]), ripple_done, button);
				return AGL_HANDLED;
			}
			break;
		default:
			break;
	}
	return AGL_NOT_HANDLED;
}


void
button_set_sensitive (AGlActor* actor, bool sensitive)
{
#ifdef AGL_DEBUG_ACTOR
	dbg(0, "%s: %i", actor->name, sensitive);
#endif
	((ButtonActor*)actor)->disabled = !sensitive;
}
