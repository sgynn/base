#version 130
// quad.vert

in vec2 vertex;
in vec2 texCoord;
out vec2 coord;
void main() {
	gl_Position = vec4(vertex, 0.0, 1.0);
	coord = texCoord;
}

