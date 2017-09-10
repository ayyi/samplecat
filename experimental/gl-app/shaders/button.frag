/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2017-2017 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/

uniform vec2 btn_size;
uniform float radius;
uniform vec4 colour;
uniform vec4 bg_colour;
uniform vec4 fill_colour;

varying vec2 fPosition;

void main()
{
	vec2 topleft     = vec2(radius + 1.0                                        );
	vec2 topright    = vec2(btn_size.x - radius - 1.0, radius + 1.0             );
	vec2 bottomleft  = vec2(radius + 1.0,              btn_size.y - radius - 1.0);

	float dist = 0.0;
	if(fPosition.y < topleft.y){
		if(fPosition.x < topleft.x){
			dist = distance(fPosition, topleft);
		}else if(fPosition.x > topright.x){
			dist = distance(fPosition, topright);
		}else{
			dist = distance(fPosition.y, topleft.y);
		}
	}else if(fPosition.y > bottomleft.y){
		vec2 bottomright = vec2(btn_size.x - radius - 1.0, btn_size.y - radius - 1.0);

		if(fPosition.x < radius){
			dist = distance(fPosition, bottomleft);
		}else if(fPosition.x > bottomright.x){
			dist = distance(fPosition, bottomright);
		}else{
			dist = distance(fPosition.y, bottomleft.y);
		}
	}else{
		if(fPosition.x < radius){
			dist = distance(fPosition.x, topleft.x);
		}else if(fPosition.x > topright.x){
			dist = distance(fPosition.x, topright.x);
		}else{
			dist = 1.0;
		}
	}

	// 1- rounded rectangle with no fill
	/*
	gl_FragColor = mix(colour, bg_colour,
		1.0 // sets default colour as bg_colour - when fully inside the circle both smoothsteps are zero, and when fully outside the circle both the smoothsteps are 1.0 so cancel out.
		+ smoothstep(radius,       radius + 1.0, dist) // change to bg_colour as move outside the circle
		- smoothstep(radius - 1.0, radius,       dist) // change to fg_colour as move from inside the circle
	);
	*/

	// 2- rounded rectangle with fill
	float inner = smoothstep(radius - 1.0, radius, dist);
	vec4 fill = mix(fill_colour, bg_colour, inner);
	vec4 outer = mix(colour, bg_colour, smoothstep(radius, radius + 1.0, dist));
	gl_FragColor = mix(fill, outer, inner);
}
