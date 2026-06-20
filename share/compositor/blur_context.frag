#version 130

in vec2 coord;
out vec4 fragment;
uniform sampler2D aoMap;
uniform sampler2D depthMap;
uniform sampler2D normalMap;

uniform float falloff;		// Blur falloff factor
uniform float sharpness;	// Sharpness with depth adjustment
uniform float radius;		// Blur radius
uniform vec2 axis; 			// Blur axis: (1,0) or (0,1)

uniform mat4 viewMatrix;
uniform mat4 inverseProjection;

vec3 getPosition(vec2 coord) {
    float rawDepth = texture(depthMap, coord).r * 2.f - 1.f;
    vec4 pos = vec4(inverseProjection * vec4(coord * 2.f - 1.f, rawDepth, 1.f));
    return pos.xyz / pos.w;
}

vec3 getNormal(vec2 coord) {
	vec3 worldNormal = texture(normalMap, coord).rgb * 2.0 - 1.0;
	return normalize(mat3(viewMatrix) * worldNormal);
}

void main() {
	vec2 halfRes = 0.5 / vec2(textureSize(aoMap, 0)) * axis;
	float blur = texture(aoMap, coord).r;
	vec3 norm = getNormal(coord);
	float total = 1.0;

	for(float r = -radius; r<=radius; ++r) {
		vec2 sampleCoord = coord + halfRes * r;
		float c = texture(aoMap, sampleCoord).r;
		float w = exp(-r*r*falloff);
		vec3 n = getNormal(sampleCoord);
		w *= max(0.0, dot(n, norm));

		total += w;
		blur += w * c;
	}

	fragment = vec4(blur/total);
}





