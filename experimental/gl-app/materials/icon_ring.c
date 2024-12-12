/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. https://ayyi.github.io/samplecat/    |
 | copyright (C) 2016-2024 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */

#include "config.h"
#include "shader.h"
#include "agl/text/pango.h"
#include "behaviours/style.h"
#include "materials/icon_ring.h"

extern AGlShader ring;

#define SIZE (2. * FONT_SIZE / 1.4275 + 4.)

static void ring_init   ();
static void ring_render (AGlMaterial*);
static void ring_free   (AGlMaterial*);

AGlMaterialClass ring_material_class = {
	.init = ring_init,
	.free = ring_free,
	.render = ring_render,
	.shader = &ring,
};


AGlMaterial*
ring_new ()
{
	if (!ring.program) {
		agl_create_program(&ring);
	}

	AGlMaterial* ring_material = (AGlMaterial*)AGL_NEW(IconMaterial,
		.material = {
			.material_class = &ring_material_class
		}
	);
	IconMaterial* icon = (IconMaterial*)ring_material;

	icon->layout = pango_layout_new (agl_pango_get_context());

	return ring_material;
}


static void
ring_init ()
{
}


static void
ring_free (AGlMaterial* material)
{
	IconMaterial* icon = (IconMaterial*)material;

	g_object_unref(icon->layout);
}


static void
ring_render (AGlMaterial* material)
{
	IconMaterial* icon = (IconMaterial*)material;

	{
		char text[2] = {icon->chr, 0};
		pango_layout_set_text(icon->layout, text, -1);

		PangoFontDescription* font_desc = pango_font_description_new();
		pango_font_description_set_family(font_desc, "Sans");

		pango_font_description_set_size(font_desc, 7 * PANGO_SCALE);
		pango_font_description_set_weight(font_desc, PANGO_WEIGHT_BOLD);
		pango_layout_set_font_description(icon->layout, font_desc);

		pango_font_description_free (font_desc);
	}

	float* c = ring.uniforms[RING_COLOUR].value;
	c[3] = ((float)(icon->colour & 0xff)) / 0x100;
	agl_rgba_to_float(icon->colour, &c[0], &c[1], &c[2]);

	c = ring.uniforms[RING_BG_COLOUR].value;
	c[3] = 0;
	agl_rgba_to_float(((IconMaterial*)material)->bg, &c[0], &c[1], &c[2]);

	agl_use_material(material);
	agl_rect_((AGlRect){0, 0, SIZE, SIZE});

	agl_set_font("Roboto", 7, PANGO_WEIGHT_BOLD);
	PangoRectangle logical_rect;
	pango_layout_get_pixel_extents(icon->layout, NULL, &logical_rect);
	int width = logical_rect.width;
	agl_print_layout(8 - width / 2, 3, 0, icon->colour, icon->layout);

	agl_set_font("Roboto", FONT_SIZE, PANGO_WEIGHT_NORMAL);
}


