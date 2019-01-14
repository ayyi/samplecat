varying vec2 fPosition;

void main() {
	fPosition = gl_Vertex.xy;
	gl_Position = ftransform();
}
