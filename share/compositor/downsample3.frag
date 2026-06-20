#version 130

in vec2 coord;
out vec4 fragment;
uniform sampler2D source;

void main() {
	ivec2 size = textureSize(source, 0);
	vec2 texelSize = 1.0 / vec2(size);

	vec4 value = vec4(0.0, 0.0, 0.0, 0.0);
	value += texture(source, coord + texelSize * vec2(-1.0,-1.0));
	value += texture(source, coord + texelSize * vec2(-1.0, 0.0));
	value += texture(source, coord + texelSize * vec2(-1.0, 1.0));
	value += texture(source, coord + texelSize * vec2( 0.0,-1.0));
	value += texture(source, coord + texelSize * vec2( 0.0, 0.0));
	value += texture(source, coord + texelSize * vec2( 0.0, 1.0));
	value += texture(source, coord + texelSize * vec2( 1.0,-1.0));
	value += texture(source, coord + texelSize * vec2( 1.0, 0.0));
	value += texture(source, coord + texelSize * vec2( 1.0, 1.0));
	fragment = value / 9.0;
}


