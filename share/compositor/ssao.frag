#version 130

in vec2 coord;
out vec4 fragment;

uniform sampler2D noise;
uniform sampler2D depthMap;
uniform sampler2D normalMap;

uniform float ssao_radius;// = 0.1;
uniform float ssao_strength;// = 0.4;
const float offset   = 18.0;
const float falloff  = 0.00001;

uniform mat4 viewMatrix;
uniform mat4 inverseProjection;

#define SAMPLES 16
const float invSamples = 0.0625; // 1.0 / SAMPLES;


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
	// Random vectors in unit sphere
	vec3 pSphere[16] = vec3[](	vec3( 0.53812504,   0.18565957,   -0.43192),
								vec3( 0.13790712,   0.24864247,    0.44301823),
								vec3( 0.33715037,   0.56794053,   -0.005789503),
								vec3(-0.6999805,   -0.04511441,   -0.0019965635),
								vec3( 0.06896307,  -0.15983082,   -0.85477847),
								vec3( 0.056099437,  0.006954967,  -0.1843352),
								vec3(-0.014653638,  0.14027752,    0.0762037),
								vec3( 0.010019933, -0.1924225,    -0.034443386),
								vec3(-0.35775623,  -0.5301969,    -0.43581226),
								vec3(-0.3169221,    0.106360726,   0.015860917),
								vec3( 0.010350345, -0.58698344,    0.0046293875),
								vec3(-0.08972908,  -0.49408212,    0.3287904),
								vec3( 0.7119986,   -0.0154690035, -0.09183723),
								vec3(-0.053382345,  0.059675813,  -0.5411899),
								vec3( 0.035267662, -0.063188605,   0.54602677),
								vec3(-0.47761092,   0.2847911,    -0.0271716));

	// Normal for reflecting sample rays
	//vec2 noiseScale = vec2(textureSize(depthMap,0)) / vec2(textureSize(noise,0));
	vec3 randomNormal = normalize(texture(noise, coord*offset).xyz * 2.0 - 1.0);
	vec3 normal = getNormal(coord);
	vec3 viewPos = getPosition(coord);
	//normal.z = max(normal.z, 0.1);

	float bl = 0.0;
	float radD = ssao_radius / viewPos.z;

	// Loop variables
	float occluderDepth, depthDifference;
	for(int i=0; i<SAMPLES; ++i) {
		// Get a ray to test
		vec3 ray = radD * reflect(pSphere[i], randomNormal);
		// Get occluder fragment
		//vec2 pos = coord.xy + sign(dot(ray, normal)-0.1) * ray.xy;
		vec2 pos = coord.xy + sign(dot(ray, normal)) * ray.xy + (normal*0.01*radD).xy;
		pos = clamp(pos, 0.0, 1.0);
		vec3 occluderNormal = getNormal(pos);
		vec3 occluderPos = getPosition(pos);

		float h = 1.0 - step(dot(normal, occluderPos - viewPos)-0.02, 0.0);

		// Falloff equation
		depthDifference = viewPos.z - occluderPos.z;
		//bl += h * step(falloff, depthDifference) * (1.0 - dot(occluderNormal, normal)) * (1.0 - smoothstep(falloff, ssao_strength, depthDifference));
		h *= 1.0 - dot(occluderNormal, normal);
		h *= 1.0 - smoothstep(falloff, ssao_strength, -depthDifference);

		bl += h;
	}

	// Result
	float ao = 1.0 - bl * invSamples * ssao_strength;
	fragment = vec4(ao, ao, ao, 1.0);
}

