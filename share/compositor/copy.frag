#version 130

#ifndef SWIZZLE
#define SWIZZLE rgba
#endif

in vec2 coord;
out vec4 fragment;
uniform sampler2D source;
void main() {
	fragment = texture(source, coord).SWIZZLE;
}


