/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2019-2019 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#include "config.h"
#undef USE_GTK
#include "glib.h"
#include "debug/debug.h"
#include "agl/actor.h"
#include "style.h"

static void style_init (AGlBehaviour*, AGlActor*);
static void style_free (AGlBehaviour*);

typedef struct
{
    AGlBehaviourClass class;
} StyleBehaviourClass;

static StyleBehaviourClass klass = {
	.class = {
		.new = style,
		.free = style_free,
		.init = style_init
	}
};


AGlBehaviourClass*
style_get_class ()
{
	return (AGlBehaviourClass*)&klass;
}


AGlBehaviour*
style ()
{
	StyleBehaviour* behaviour = AGL_NEW(StyleBehaviour,
		.behaviour = {
			.klass = (AGlBehaviourClass*)&klass
		}
	);

	return (AGlBehaviour*)behaviour;
}


static void
style_free (AGlBehaviour* behaviour)
{
	StyleBehaviour* style = (StyleBehaviour*)behaviour;
	g_free(style);
}


static void
style_init (AGlBehaviour* behaviour, AGlActor* actor)
{
	StyleBehaviour* style = (StyleBehaviour*)behaviour;

	style->bg = 0x000000ff;
	style->bg_alt = 0x181818ff;
	style->bg_selected = 0x777777ff;
	style->fg = 0x66aaffff;
	style->text = 0xbbbbbbff;
	style->selection = 0x6677ff77;
	style->font = "Roboto";

	char* font = g_strdup_printf("%s 10", style->font);
	agl_set_font_string(font); // initialise the pango context
	g_free(font);

}
