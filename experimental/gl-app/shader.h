/**
 +----------------------------------------------------------------------+
 | This file is part of the Ayyi project. http://www.ayyi.org           |
 | copyright (C) 2013-2021 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 */

#pragma once

#include "agl/utils.h"
#include "agl/shader.h"

typedef struct {
	AGlShader      shader;
	struct {
		uint32_t   colour;
		uint32_t   bg_colour;
		uint32_t   fill_colour;
		AGlfPt     btn_size;
		float      radius;
	}              uniform;
} ButtonShader;

enum {
    RING_RADIUS = 0,
    RING_CENTRE,
    RING_COLOUR,
    RING_BG_COLOUR,
};

typedef struct {
	AGlShader      shader;
	struct {
		AGliPt     centre;
		float      radius;
	}              uniform;
} CircleShader;

extern ButtonShader button_shader;
extern AGlShader ring;
extern CircleShader circle_shader;

#define CIRCLE_COLOUR() \
	(((AGlUniformUnion*)&circle_shader.shader.uniforms[0])->value.i[0])
#define CIRCLE_BG_COLOUR() \
	(((AGlUniformUnion*)&circle_shader.shader.uniforms[1])->value.i[0])
