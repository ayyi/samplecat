/*
  copyright (C) 2016 Tim Orford <tim@orford.org>

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

uniform vec2 centre1;
uniform vec2 centre2;
uniform float radius;
uniform vec4 colour;
uniform vec4 bg_colour;
varying vec2 fPosition;

void main() {
	float dist = 0.0;
	if(fPosition.y < centre1.y){
		dist = distance(fPosition, centre1);
	}else if(fPosition.y > centre2.y){
		dist = distance(fPosition, centre2);
	}else{
		dist = distance(fPosition.x, centre1.x);
	}

	gl_FragColor = mix(colour, bg_colour, smoothstep(radius - 1.0, radius + 1.0, dist));
}
