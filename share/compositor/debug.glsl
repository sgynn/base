#version 130

#pragma vertex_shader

in vec4 vertex;
in vec3 normal;
in vec4 colour;

uniform mat4 transform;
uniform mat4 modelview;

out vec4 vColour;

void main() {
	gl_Position = transform * vertex;
	vColour = colour;
}


#pragma fragment_shader

in vec4 vColour;
out vec4 fragment;

void main() {
	fragment = vColour;
}

