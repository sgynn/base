#version 330

in vec2 coord;
out vec4 fragment;

uniform sampler2D normalMap;
uniform sampler2D depthMap;

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
	vec2 one = 1.0 / vec2(textureSize(normalMap, 0));
	vec3 normal = getNormal(coord);
	vec3 pos = getPosition(coord);
	
	float d1 = dot(normal, getPosition(coord - one) - pos);
	float d2 = dot(normal, getPosition(coord + one) - pos);
	float d3 = dot(normal, getPosition(coord + vec2(0.0, -one.y)) - pos);
	float d4 = dot(normal, getPosition(coord + vec2(one.x, -one.y)) - pos);
	float d5 = dot(normal, getPosition(coord + vec2(-one.x, 0.0)) - pos);
	float d6 = dot(normal, getPosition(coord + vec2(one.x, 0.0)) - pos);
	float d7 = dot(normal, getPosition(coord + vec2(one.x, one.y)) - pos);
	float d8 = dot(normal, getPosition(coord + vec2(one.x, 0.0)) - pos);
	
	float t = d1 + d2 + d3 + d4 + d5 + d6 + d7 + d8;

	fragment = vec4(1.0 - t*4.0);
}

