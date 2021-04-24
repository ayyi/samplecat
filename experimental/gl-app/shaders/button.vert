uniform vec2 modelview;
uniform vec2 translate;

attribute vec2 vertex;

varying vec2 fPosition;

void main () {
	fPosition = vertex;
	gl_Position = vec4(vec2(1., -1.) * (vertex + translate) / modelview - vec2(1., -1.), 1., 1.);
}
