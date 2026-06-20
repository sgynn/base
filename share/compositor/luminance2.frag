#version 130

in vec2 coord;
out vec4 fragment;
uniform sampler2D source;

float lum(vec3 colour) {
	//if(colour == vec3(0.0)) return 0.3;
	return max( dot(colour, vec3(0.299, 0.587, 0.114)), 0.001);
}

void main() {
	ivec2 size = textureSize(source, 0);
	vec2 texelSize = 1.0 / vec2(size);

	vec4 value = vec4(0.0, 0.0, 0.0, 0.0);
	value += lum( texture(source, coord + texelSize * vec2(-0.5,-0.5)).rgb );
	value += lum( texture(source, coord + texelSize * vec2(-0.5, 0.5)).rgb );
	value += lum( texture(source, coord + texelSize * vec2( 0.5,-0.5)).rgb );
	value += lum( texture(source, coord + texelSize * vec2( 0.5, 0.5)).rgb );
	fragment = value * 0.25;
}

