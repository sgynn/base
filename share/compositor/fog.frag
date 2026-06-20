#version 130

in vec2 coord;
out vec4 fragment;

uniform sampler2D bufferD;

uniform mat4 inverseView;
uniform mat4 proj;
uniform vec4 data; // top, bottom, densityTop, densityBottom
uniform vec3 lightColour;



vec3 constructViewPosition(vec3 coord, mat4 proj) {
	vec3 p = coord * 2.0 - 1.0;
	float A = proj[2].z;
	float B = proj[3].z;
	float z = -B / (A + p.z);
	vec2 div = vec2(proj[0].x, proj[1].y);
	vec2 skew = proj[2].xy;
	return vec3( (-z*p.xy + skew*z) / div, z);
}


void main() {
	float depth = texture(bufferD, coord.xy).x;
	vec3 position = constructViewPosition( vec3(coord, depth), proj);
	position = (inverseView * vec4(position,1.0)).xyz;

	vec3 start = (inverseView * vec4(0.0, 0.0, 0.0, 1.0)).xyz;

	// Three parts: above, middle, below
	vec3 ray = position - start;
	float len = length(ray);
	ray /= len;
	
	float top = (data.x - start.y) / ray.y;
	float bottom = (data.y - start.y) / ray.y;

	float above = max(0.0, ray.y<0.0? len-top: top);
	float below = max(0.0, ray.y<0.0? bottom: len-bottom);
	
	// meh - Super basic placeholder fog
	if(len > 1000.0) len = 0.0;
	fragment = vec4(max(lightColour, vec3(0,0.05,0.2)) * 0.8, len / 200.0);
}




