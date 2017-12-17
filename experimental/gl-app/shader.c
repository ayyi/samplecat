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
#include "debug/debug.h"
#include "agl/ext.h"
#include "waveform/utils.h"
#include "gl-app/shader.h"
#include "gl-app/shaders/shaders.c"

static void _v_scrollbar_set_uniforms();
static void _button_set_uniforms();
static void _ring_set_uniforms();


ScrollbarShader v_scrollbar_shader = {{NULL, NULL, 0, NULL, _v_scrollbar_set_uniforms, &vscrollbar_text}, {}, {END_OF_UNIFORMS}};
ButtonShader button_shader = {{NULL, NULL, 0, NULL, _button_set_uniforms, &button_text}, {}, {END_OF_UNIFORMS}};

static AGlUniformInfo ringuniforms[] = {
   {"radius",    1, GL_FLOAT, {7,  }, -1},
   {"centre",    2, GL_FLOAT, {8,8,}, -1},
   {"colour",    4, GL_FLOAT, {0,  }, -1},
   {"bg_colour", 4, GL_FLOAT, {0,0,0,1}, -1},
   END_OF_UNIFORMS
};
AGlShader ring = {NULL, NULL, 0, (AGlUniformInfo*)&ringuniforms, _ring_set_uniforms, &ring_text};


static inline void
set_uniform_f(AGlShader* shader, int u, float* prev)
{
	//AGlUniformInfo* uniform = &shader->uniforms[u];
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


static void
_button_set_uniforms()
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
_ring_set_uniforms()
{
	glUniform4fv(ring.uniforms[3].location, 1, ring.uniforms[3].value);
	glUniform4fv(ring.uniforms[2].location, 1, ring.uniforms[2].value);
}

