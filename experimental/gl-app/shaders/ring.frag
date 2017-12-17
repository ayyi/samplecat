/*
  copyright (C) 2017 Tim Orford <tim@orford.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License version 3
  as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

uniform vec2 centre;
uniform float radius;
uniform vec4 colour;
uniform vec4 bg_colour;
varying vec2 fPosition;

void main() {
	float dist = distance(fPosition, centre);

	float inside = smoothstep(radius - 2.5, radius - 0.5, dist);

	vec4 colour2 = vec4(0.0, 0.0, 0.0, 1.0);
	vec4 cfill = mix(colour2, bg_colour, inside);
	vec4 couter = mix(colour, bg_colour, smoothstep(radius, radius + 2.0, dist));

	gl_FragColor = mix(cfill, couter, inside);
}
