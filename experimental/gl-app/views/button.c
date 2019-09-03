/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2013-2019 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#include "config.h"
#include "debug/debug.h"
#include "agl/utils.h"
#include "samplecat.h"
#include "utils.h"
#include "shader.h"
#include "application.h"
#include "button.h"

#define SIZE 24
#define BG_OPACITY(B) (button->animatables[0]->val.f)
#define RIPPLE(B) (button->animatables[1]->val.f)

static AGl* agl = NULL;

struct {
	AGliPt pt;
} GlButtonPress;

static AGlActorClass actor_class = {0, "Button", (AGlActorNew*)button};

static bool button_on_event (AGlActor*, GdkEvent*, AGliPt);


AGlActorClass*
button_get_class ()
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
button(int* icon, ButtonAction action, ButtonGetState get_state, gpointer user_data)
{
	_init();

	bool gl_button_paint(AGlActor* actor)
	{
		ButtonActor* button = (ButtonActor*)actor;
		Style* style = &app->style;

		bool state = button->get_state ? button->get_state(actor, button->user_data) : false;

		AGlRect r = {
			.w = agl_actor__width(actor),
			.h = agl_actor__height(actor)
		};

		// background
		if(RIPPLE(button) > 0.0){
			agl->shaders.plain->uniform.colour = state
				? colour_mix(style->bg, style->bg_selected, RIPPLE(button) / 64.0)
				: colour_mix(style->bg_selected, style->bg, RIPPLE(button) / 64.0);
		}else if(state){
			agl->shaders.plain->uniform.colour = style->bg_selected;
		}else{
			agl->shaders.plain->uniform.colour = style->bg;
		}

		// hover background
		if(BG_OPACITY(button) > 0.0){ // dont get events if disabled, so no need to check state (TODO turn off hover when disabling).
			float alpha = button->animatables[0]->val.f;
			uint32_t fg = colour_lighter(agl->shaders.plain->uniform.colour, 16);
			agl->shaders.plain->uniform.colour = style->bg ? colour_mix(agl->shaders.plain->uniform.colour, fg, alpha) : (fg & 0xffffff00) + (uint32_t)(alpha * 0xff);
		}

		if(!(RIPPLE(button) > 0.0)){
			agl_use_program((AGlShader*)agl->shaders.plain);
			agl_rect_(r);

		}else{
			// click ripple
			//int radius = RIPPLE(button);
#undef RIPPLE_EXPAND
#ifdef RIPPLE_EXPAND
			circle_shader.uniform.radius = radius;
			agl_use_program((AGlShader*)&circle_shader);
			AGlRect rr = r;
			rr.w = rr.h = radius * 2;
			glTranslatef(GlButtonPress.pt.x - actor->region.x1 - radius, GlButtonPress.pt.y - radius, 0.0);
			agl_rect_(rr);
			glTranslatef(-(GlButtonPress.pt.x - actor->region.x1 - radius), -(GlButtonPress.pt.y - radius), -0.0);
#else
			circle_shader.uniform.colour = colour_lighter(agl->shaders.plain->uniform.colour, 32);
			circle_shader.uniform.bg_colour = agl->shaders.plain->uniform.colour;
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

	bool gl_button_paint_gl1(AGlActor* actor)
	{
		ButtonActor* button = (ButtonActor*)actor;

		bool state = button->get_state ? button->get_state(actor, button->user_data) : false;

		AGlRect r = {
			.w = agl_actor__width(actor),
			.h = agl_actor__height(actor->parent)
		};

		agl_enable(0/* !AGL_ENABLE_TEXTURE_2D*/);
		if(state){
			agl_colour_rbga(app->style.bg_selected);
			agl_rect_(r);
		}
		if(!button->disabled && agl_actor__is_hovered(actor)){
			glColor4f(1.0, 1.0, 1.0, 0.1);
			agl_rect_(r);
		}

		agl_enable(AGL_ENABLE_TEXTURE_2D);
		glColor4f(1.0, 1.0, 1.0, 1.0);
#if 0
		agl_textured_rect(*button->icon + (state ? 1 : 0)], r.x, r.y + r.h / 2.0 - 8.0, SIZE, SIZE, NULL);
#else
		agl_textured_rect(*button->icon, r.x, r.y + r.h / 2.0 - 8.0, SIZE, SIZE, NULL);
#endif

		return true;
	}

	void button_set_size(AGlActor* actor)
	{
		actor->region.y2 = actor->parent->region.y2 - actor->parent->region.y1;
	}

	void button_init(AGlActor* actor)
	{
		if(!circle_shader.shader.program) agl_create_program(&circle_shader.shader);

		actor->paint = agl->use_shaders ? gl_button_paint : gl_button_paint_gl1;
	}

	void button_free(AGlActor* actor)
	{
		g_free(((ButtonActor*)actor)->animatables[0]);
		g_free(((ButtonActor*)actor)->animatables[1]);
	}

	ButtonActor* button = AGL_NEW(ButtonActor,
		.actor = {
			.class    = &actor_class,
			.name     = "Button",
			.init     = button_init,
			.free     = button_free,
			.paint    = agl_actor__null_painter,
			.set_size = button_set_size,
			.on_event = button_on_event,
		},
		.icon      = (guint*)icon,
		.action    = action,
		.get_state = get_state,
		.user_data = user_data,
	);

	button->animatables[0] = SC_NEW(WfAnimatable,
		.model_val.f = &button->bg_opacity,
		.start_val.f = 0.0,
		.val.f       = 0.0,
		.type        = WF_FLOAT
	);

	button->animatables[1] = SC_NEW(WfAnimatable,
		.model_val.f = &button->ripple_radius,
		.start_val.f = 0.0,
		.val.f       = 0.0,
		.type        = WF_FLOAT
	);

	return (AGlActor*)button;
}


static bool
button_on_event(AGlActor* actor, GdkEvent* event, AGliPt xy)
{
	ButtonActor* button = (ButtonActor*)actor;

	if(button->disabled) return AGL_NOT_HANDLED;

	void animation_done (WfAnimation* animation, gpointer user_data)
	{
	}

	void ripple_done (WfAnimation* animation, gpointer user_data)
	{
		ButtonActor* button = user_data;
		RIPPLE(button) = 0.0;
	}

	switch (event->type){
		case GDK_ENTER_NOTIFY:
			dbg (1, "ENTER_NOTIFY");
			//set_cursor(actor->root->widget->window, CURSOR_H_DOUBLE_ARROW);

			button->bg_opacity = 1.0;
			agl_actor__start_transition(actor, g_list_append(NULL, button->animatables[0]), animation_done, NULL);
			return AGL_HANDLED;
		case GDK_LEAVE_NOTIFY:
			dbg (1, "LEAVE_NOTIFY");
			//set_cursor(actor->root->widget->window, CURSOR_NORMAL);

			button->bg_opacity = 0.0;
			agl_actor__start_transition(actor, g_list_append(NULL, button->animatables[0]), animation_done, NULL);
			return AGL_HANDLED;
		case GDK_BUTTON_PRESS:
			return AGL_HANDLED;
		case GDK_BUTTON_RELEASE:
			dbg(1, "BUTTON_RELEASE");
			if(event->button.button == 1){
				call(button->action, actor, button->user_data);

				GlButtonPress.pt = xy;
				button->animatables[1]->val.f = 0.001;
				button->ripple_radius = 64.0;
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
button_set_sensitive(AGlActor* actor, bool sensitive)
{
#ifdef AGL_DEBUG_ACTOR
	dbg(0, "%s: %i", actor->name, sensitive);
#endif
	((ButtonActor*)actor)->disabled = !sensitive;
}


