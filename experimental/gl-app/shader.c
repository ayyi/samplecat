/**
* +----------------------------------------------------------------------+
* | This file is part of the Ayyi project. http://www.ayyi.org           |
* | copyright (C) 2015-2017 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#include <GL/gl.h>
#include <glib.h>
#include "agl/ext.h"
#include "waveform/utils.h"
#include "gl-app/shader.h"
#include "gl-app/shaders/shaders.c"

static void _v_scrollbar_set_uniforms();


ScrollbarShader v_scrollbar_shader = {{NULL, NULL, 0, NULL, _v_scrollbar_set_uniforms, &vscrollbar_text}, {}, {END_OF_UNIFORMS}};


static inline void
set_uniform_f(AGlShader* shader, int u, float* prev)
{
	AGlUniformInfo* uniform = &shader->uniforms[u];
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
_scrollbar_set_uniforms(ScrollbarShader* scrollbar_shader)
{
	float centre1[2] = {scrollbar_shader->uniform.centre1.x, scrollbar_shader->uniform.centre1.y};
	float centre2[2] = {scrollbar_shader->uniform.centre2.x, scrollbar_shader->uniform.centre2.y};
	glUniform2fv(glGetUniformLocation(scrollbar_shader->shader.program, "centre1"), 1, centre1);
	glUniform2fv(glGetUniformLocation(scrollbar_shader->shader.program, "centre2"), 1, centre2);
	glUniform1f(glGetUniformLocation(scrollbar_shader->shader.program, "radius"), scrollbar_shader->uniform.radius);

	SET_COLOUR(scrollbar_shader);

	GLint location = 0;
	if(!location){
		location = glGetUniformLocation(scrollbar_shader->shader.program, "bg_colour");
	}
	float bg_colour[4] = {0.0, 0.0, 0.0, ((float)(scrollbar_shader->uniform.bg_colour & 0xff)) / 0x100};
	agl_rgba_to_float(scrollbar_shader->uniform.bg_colour, &bg_colour[0], &bg_colour[1], &bg_colour[2]);
	glUniform4fv(location, 1, bg_colour);
}


static void
_v_scrollbar_set_uniforms()
{
	_scrollbar_set_uniforms(&v_scrollbar_shader);
}


