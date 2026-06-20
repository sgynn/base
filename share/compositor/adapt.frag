#version 130

in vec2 coord;
out vec4 fragment;

uniform sampler2D luminance;
uniform sampler2D last;

uniform vec2 adaptRange;
uniform float adaptRate;
uniform float frameTime;

void main() {
	float previous = exp( texture(last, vec2(0.5,0.5)).r );
	float current  = texture(luminance, vec2(0.5,0.5)).r;

	// Adapt using Pattanik's technique
	float adapted = mix(previous, current, 1.0 - exp(-frameTime * adaptRate));

	adapted = clamp(adapted, adaptRange.x, adaptRange.y);
	fragment = vec4( log(adapted), 0.0, 0.0, 0.0);
}


