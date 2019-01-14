/**
* +----------------------------------------------------------------------+
* | This file is part of the Ayyi project. http://www.ayyi.org           |
* | copyright (C) 2013-2017 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#ifndef __arrange_shader_h__
#define __arrange_shader_h__
#include "agl/utils.h"
#include "agl/shader.h"

typedef struct {
	AGlShader      shader;
	struct {
		uint32_t   colour;
		uint32_t   bg_colour;
		AGliPt     centre1;
		AGliPt     centre2;
		float      radius;
	}              uniform;
	AGlUniformInfo uniforms[1];
} ScrollbarShader;

typedef struct {
	AGlShader      shader;
	struct {
		uint32_t   colour;
		uint32_t   bg_colour;
		uint32_t   fill_colour;
		AGliPt     btn_size;
		float      radius;
	}              uniform;
	AGlUniformInfo uniforms[1];
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
		uint32_t   colour;
		uint32_t   bg_colour;
		AGliPt     centre;
		float      radius;
	}              uniform;
	AGlUniformInfo uniforms[1];
} CircleShader;

extern ScrollbarShader v_scrollbar_shader;
extern ButtonShader button_shader;
extern AGlShader ring;
extern CircleShader circle_shader;

#endif
