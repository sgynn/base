#version 330

#pragma vertex_shader

in vec4 vertex;
in vec3 normal;

uniform mat4 transform;
uniform mat4 modelview;

out vec4 vColour;
out vec3 vNormal;

void main() {
	gl_Position = transform * vertex;
	vNormal = mat3(modelview) * normal;
	vColour = vec4(1.0);
}


#pragma fragment_shader

in vec4 vColour;
in vec3 vNormal;

layout(location=0) out vec4 buf0;
layout(location=1) out vec4 buf1;

void main() {
	buf0 = vColour;
	buf1 = vec4(normalize(vNormal) * 0.5 + 0.5, 1.0);
}

