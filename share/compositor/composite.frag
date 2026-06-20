
#version 130

in vec2 coord;
out vec4 fragment;
uniform sampler2D hdr;
uniform sampler2D adapt;
uniform sampler2D bloom;

//uniform float exposureKey = 0.5;
//uniform float bloomMagnitude = 0.1;

// Geometric mean
vec3 toneMap(vec3 colour, float luminance, float threshold, out float exposure, float exposureKey) {
	float linear = exposureKey / luminance;
	linear = max(linear, 0.001);
	exposure = log2(linear) - threshold;
	return exp2(exposure) * colour;
}

void main() {
	float exposureKey = 0.5;

	vec3 source = texture(hdr, coord).rgb;
	float lum = exp( texture(adapt, vec2(0.5,0.5)).r );

	float exposure = 0.0;
	vec3 colour = toneMap(source, lum, 0.0, exposure, exposureKey);

//	vec3 bloomValue = texture2D(bloom, coord).rgb * bloomMagnitude;
//	colour += bloomValue;

	// gamma
	colour = pow(colour, vec3(1.0/2.2));

	fragment = vec4(colour, 1.0);
}


