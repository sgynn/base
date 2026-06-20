#version 130

// Source: https://www.gamedev.net/articles/programming/graphics/a-simple-and-practical-approach-to-ssao-r2753/

uniform sampler2D normalMap;
uniform sampler2D depthMap;
uniform sampler2D noise;

uniform mat4 viewMatrix;
uniform mat4 inverseProjection;

uniform float random_size; // size of noise texture could use textureSize(noise,0).x
uniform float g_sample_rad; //the sampling radius.
uniform float g_intensity;
uniform float g_scale; //scales distance between occluders and occludee.
uniform float g_bias;  //controls the width of the occlusion cone considered by the occludee.


in vec2 coord;
out vec4 fragment;

vec3 getPosition(vec2 coord) {
    float rawDepth = texture(depthMap, coord).r * 2.f - 1.f;
    vec4 pos = vec4(inverseProjection * vec4(coord * 2.f - 1.f, rawDepth, 1.f));
    return pos.xyz / pos.w;
}

vec3 getNormal(vec2 coord) {
	vec3 worldNormal = texture(normalMap, coord).rgb * 2.0 - 1.0;
	return normalize(mat3(viewMatrix) * worldNormal);
}

vec2 getRandom(vec2 coord) {
	vec2 s = vec2(textureSize(depthMap, 0));
	return normalize(texture(noise, s * coord / random_size).xy * 2.0f - 1.0f);
}

float doAmbientOcclusion(vec2 tcoord, vec2 uv, vec3 p, vec3 cnorm, vec4 bounds) {
	vec3 diff = getPosition(clamp(tcoord + uv, bounds.xy, bounds.zw)) - p;
 	vec3 v = normalize(diff);
	float d = length(diff);
	if(d > 2.0) return 0.0;
	return max(0.0, dot(cnorm, v) - g_bias) * (1.0 / (1.0 + d * g_scale)) * g_intensity * 4.0;
}

void main() {
	vec2 vec[4] = vec2[]( vec2(1.0, 0.0), vec2(-1.0, 0.0), vec2(0.0, 1.0), vec2(0.0, -1.0));
	vec3 p = getPosition(coord);
	vec3 n = getNormal(coord);
	vec2 rand = getRandom(coord);
	float ao = 0.0f;
	float rad = g_sample_rad / p.z;

	vec2 pixel = 0.5 / vec2(textureSize(depthMap, 0)); // Clamp to edge
	vec4 bounds = vec4(pixel, 1.0 - pixel);

	int iterations = 4;
	for(int j = 0; j < iterations; ++j) {
		vec2 coord1 = reflect(vec[j], rand) * rad;
		vec2 coord2 = vec2(coord1.x*0.707 - coord1.y*0.707, coord1.x*0.707 + coord1.y*0.707);

		ao += doAmbientOcclusion(coord, coord1 * 0.25, p, n, bounds);
		ao += doAmbientOcclusion(coord, coord2 * 0.5, p, n, bounds);
		ao += doAmbientOcclusion(coord, coord1 * 0.75, p, n, bounds);
		ao += doAmbientOcclusion(coord, coord2, p, n, bounds);
	}

	ao /= float(iterations) * 4.0;
	ao = 1.0 - ao;
	fragment = vec4(ao, ao, ao, 1.0);
}

