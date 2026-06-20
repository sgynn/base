#version 130

in vec2 coord;
out vec4 fragment;
uniform sampler2D aoMap;
uniform sampler2D depthMap;

uniform float falloff;		// Blur falloff factor
uniform float sharpness;	// Sharpness with depth adjustment
uniform float radius;		// Blur radius
uniform vec2 axis; 			// Blur axis: (1,0) or (0,1)

void main() {
	vec2 halfRes = 0.5 / vec2(textureSize(aoMap, 0)) * axis;
	float blur = texture(aoMap, coord).r;
	float depth = texture(depthMap, coord).r;
	float total = 1.0;

	for(float r = -radius; r<=radius; ++r) {
		vec2 sampleCoord = coord + halfRes * r;
		float c = texture(aoMap, sampleCoord).r;
		float d = texture(depthMap, sampleCoord).r - depth;
		float w = exp(-r*r*falloff - d*d*sharpness);
		total += w;
		blur += w * c;
	}

	fragment = vec4(blur/total);
}





