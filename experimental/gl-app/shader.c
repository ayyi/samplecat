/**
* +----------------------------------------------------------------------+
* | This file is part of the Ayyi project. http://www.ayyi.org           |
* | copyright (C) 2015-2021 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#include "config.h"
#undef USE_GTK
#include <GL/gl.h>
#include <glib.h>
#include "debug/debug.h"
#include "agl/ext.h"
#include "waveform/utils.h"
#include "gl-app/shader.h"
#include "gl-app/shaders/shaders.c"

static void _button_set_uniforms      ();
static void _ring_set_uniforms        ();
static void _circle_set_uniforms      ();


ButtonShader button_shader = {{
	NULL, NULL, 0, NULL,
	_button_set_uniforms,
	&button_text
}};

CircleShader circle_shader = {{
	.uniforms = (AGlUniformInfo[]) {
	   {"colour", 4, GL_COLOR_ARRAY, -1,},
	   {"bg_colour", 4, GL_COLOR_ARRAY, -1,},
	   END_OF_UNIFORMS
	},
	_circle_set_uniforms,
	&circle_text
}};


AGlShader ring = {
	NULL, NULL, 0,
	(AGlUniformInfo[]) {
	   {"radius",    1, GL_FLOAT, -1, {7,  }},
	   {"centre",    2, GL_FLOAT, -1, {8,8,}},
	   {"colour",    4, GL_FLOAT, -1, {0,  }},
	   {"bg_colour", 4, GL_FLOAT, -1, {0,0,0,1}},
	   END_OF_UNIFORMS
	},
	_ring_set_uniforms,
	&ring_text
};


static inline void
set_uniform_f (AGlShader* shader, int u, float* prev)
{
	if(shader->uniforms[u].value[0] != *prev){
		glUniform1f(shader->uniforms[u].location, shader->uniforms[u].value[0]);
		*prev = shader->uniforms[u].value[0];
	}
}


#define SET_COLOUR(SHADER) \
	float colour[4] = {0.0, 0.0, 0.0, ((float)((SHADER)->uniform.colour & 0xff)) / 0x100}; \
	agl_rgba_to_float((SHADER)->uniform.colour, &colour[0], &colour[1], &colour[2]); \
	glUniform4fv(glGetUniformLocation((SHADER)->shader.program, "colour"), 1, colour);

static void
_button_set_uniforms ()
{
	float btn_size[2] = {button_shader.uniform.btn_size.x, button_shader.uniform.btn_size.y};
	glUniform2fv(glGetUniformLocation(button_shader.shader.program, "btn_size"), 1, btn_size);

	glUniform1f(glGetUniformLocation(button_shader.shader.program, "radius"), button_shader.uniform.radius);

	SET_COLOUR(&button_shader);

	GLint location = 0;
	if(!location){
		location = glGetUniformLocation(button_shader.shader.program, "bg_colour");
	}
	float bg_colour[4] = {0.0, 0.0, 0.0, ((float)(button_shader.uniform.bg_colour & 0xff)) / 0x100};
	agl_rgba_to_float(button_shader.uniform.bg_colour, &bg_colour[0], &bg_colour[1], &bg_colour[2]);
	glUniform4fv(location, 1, bg_colour);

	location = 0;
	if(!location){
		location = glGetUniformLocation(button_shader.shader.program, "fill_colour");
	}
	float fill_colour[4] = {0.0, 0.0, 0.0, ((float)(button_shader.uniform.fill_colour & 0xff)) / 0x100};
	agl_rgba_to_float(button_shader.uniform.fill_colour, &fill_colour[0], &fill_colour[1], &fill_colour[2]);
	glUniform4fv(location, 1, fill_colour);
}


static void
_ring_set_uniforms ()
{
	glUniform4fv(ring.uniforms[3].location, 1, ring.uniforms[3].value);
	glUniform4fv(ring.uniforms[2].location, 1, ring.uniforms[2].value);
}


static void
_circle_set_uniforms ()
{
	float centre[2] = {circle_shader.uniform.centre.x, circle_shader.uniform.centre.y};
	glUniform2fv(glGetUniformLocation(circle_shader.shader.program, "centre"), 1, centre);
	glUniform1f(glGetUniformLocation(circle_shader.shader.program, "radius"), circle_shader.uniform.radius);

	agl_set_uniforms ((AGlShader*)&circle_shader);
}
