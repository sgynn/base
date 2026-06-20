#version 130

in vec2 coord;
out vec4 fragment;

uniform sampler2D screen;
uniform sampler2D depthMap;
uniform sampler2D normalMap;
uniform sampler2D colourMap;

uniform mat4 viewMatrix;
uniform mat4 projection;
uniform mat4 inverseProjection;

// Could be uniforms
const float maxDistance = 8.0;
const float resolution = 0.2;
const float thickness = 0.2;

int steps = 5;

#line 22
vec3 constructViewPosition(vec3 coord, mat4 proj) {
	vec3 p = coord * 2.0 - 1.0;
	float A = proj[2].z;
	float B = proj[3].z;
	float z = -B / (A + p.z);
	vec2 div = vec2(proj[0].x, proj[1].y);
	vec2 skew = proj[2].xy;
	return vec3( (-z*p.xy + skew*z) / div, z);
}


vec3 getPosition(vec2 coord) {
    float depth = texture(depthMap, coord).r;
	return constructViewPosition(vec3(coord, depth), projection);
}

vec3 getNormal(vec2 coord) {
	vec3 worldNormal = texture(normalMap, coord).rgb * 2.0 - 1.0;
	return normalize(mat3(viewMatrix) * worldNormal);
}

vec2 project(vec3 view) {
	vec4 p = projection * vec4(view, 1.0);
	p /= p.w;
	return p.xy * 0.5 + 0.5;
}


void main() {
	vec2 size = vec2(textureSize(normalMap, 0));
	vec3 from = getPosition(coord);
	vec3 normal = getNormal(coord);
	vec3 pivot = normalize(reflect(normalize(from), normal));

	vec3 end = from + pivot * maxDistance;

	// Get start and end points in screen coordinates
	vec2 fragStart = project(from);
	vec2 fragEnd = project(end);

	// Quick fix for end pos off screen
	if(abs(fragEnd-0.5).y > 0.5) {
		end = from + pivot * 3.0;
		fragEnd = project(end);
	}


	vec2 deltaValue = fragEnd - fragStart;
	vec2 absDelta = abs(deltaValue) * size;
	float useX = absDelta.x < absDelta.y? 0.0: 1.0;
	float delta = mix(absDelta.y, absDelta.x, useX) * clamp(resolution, 0.0, 1.0);
	vec2 increment = deltaValue / max(delta, 0.001);

	delta = min(delta, 500.0); // iteration limit
	float roughness = texture(colourMap, coord).a;
	//if(roughness==0) delta = 0;

	float search0 = 0.0;
	float search1 = 0.0;
	int hit0 = 0;
	int hit1 = 0;

	float dist = from.z;
	float depth = thickness;
	vec2 frag = fragStart;
	vec3 pos = from;
	for(int i=0; i<int(delta); ++i) {
		frag += increment;
		pos = getPosition(frag);
		vec2 search = (frag - fragStart) / deltaValue;
		search1 = clamp(mix(search.y, search.x, useX), 0.0, 1.0);
		dist = (from.z*end.z) / mix(end.z, from.z, search1);
		depth = pos.z - dist;

		if(depth>0.0 && depth<thickness) { hit0=1; break; }
		else search0 = search1;
	}

	search1 = (search0 + search1) * 0.5;
	steps *= hit0;

	for(int i=0; i<steps; ++i) {
		frag = mix(fragStart, fragEnd, search1);
		pos = getPosition(frag);
		dist = (from.z*end.z) / mix(end.z, from.z, search1);
		depth = pos.z - dist;

		float halfStep = (search1 - search0) / 2.0;
		if(depth>0.0 && depth<thickness) { hit1=1; search1 -= halfStep; }
		else { search0 = search1;  search1 += halfStep; }
	}

	float vis = float(hit0)
		* (1.0-max( dot(-normalize(from), pivot), 0.0))
		* (1.0-clamp(depth/thickness, 0.0, 1.0))
		* (1.0-clamp(length(pos-from)/maxDistance, 0.0,1.0))
		* (frag.x<0.0 || frag.x>1.0? 0.0: 1.0)
		* (frag.y<0.0 || frag.y>1.0? 0.0: 1.0);
	
	vis = clamp(vis, 0.0, 1.0);

	// Fade screen edges
	vec2 edge = max(abs(frag - 0.5) - 0.4, 0.0) * 10.0;
	vis *= 1.0 - max(edge.x, edge.y);
	vis *= 1.0 - roughness;

	// Some output
	vec4 base = texture(screen, coord);
	vec4 ref  = texture(screen, frag);

	fragment = mix(base, ref, vis*0.8);
}




