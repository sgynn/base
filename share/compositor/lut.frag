#version 130

#define LUT_RESOLUTION 32.0

in vec2 coord;
out vec4 fragment;
uniform sampler2D source;
uniform sampler2D lut;
void main() {
	float maxColour = LUT_RESOLUTION - 1.0;
	float halfx = 0.5 / (LUT_RESOLUTION * LUT_RESOLUTION); // constant
	float halfy = 0.5 / LUT_RESOLUTION;	// constant
	float threshold = maxColour / LUT_RESOLUTION;
	float layer = 1.0 / LUT_RESOLUTION;

	vec4 col = texture(source, coord);
	float x = halfx + col.r * threshold * layer;
	float y = halfy + col.g * threshold;
	float z = col.b * maxColour;
	
	vec4 a = texture(lut, vec2(floor(z) * layer + x, 1.0-y));
	vec4 b = texture(lut, vec2(ceil(z) * layer + x, 1.0-y));
	fragment = mix(a, b, fract(z));
}




