#version 130

// Resolve gbuffer

#pragma vertex_shader

in vec4 vertex;
out vec3 centre;
out float radius;
uniform mat4 transform;
uniform mat4 modelview;
void main() {
	gl_Position = transform * vertex;
	centre = (modelview * vec4(0.0,0.0,0.0,1.0)).xyz;
	radius = length((modelview * vec4(1.0,0.0,0.0,1.0)).xyz - centre);
}


#pragma fragment_shader


in vec3 centre;
in float radius;
out vec4 fragment;

uniform sampler2D bufferD;
uniform sampler2D buffer0;
uniform sampler2D buffer1;

uniform vec4 viewport;
uniform mat4 inverseView;
uniform mat4 proj;

uniform vec4 colour; // RGB

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
	vec2 coord = gl_FragCoord.xy * viewport.zw;
	float depth = texture(bufferD, coord.xy).x;
	vec4  buf0 = texture(buffer0, coord.xy);
	vec4  buf1 = texture(buffer1, coord.xy);

	if(depth == 1.0) discard;

	vec3 position = constructViewPosition( vec3(coord, depth), proj);
	position = (inverseView * vec4(position,1)).xyz;

	vec3 albedo = buf0.rgb;
	vec3 normal = buf1.rgb * 2.0 - 1.0;
	float gloss = buf0.a;
	vec3  dir = centre - position;

	vec4 cam = inverseView * vec4(0.0,0.0,0.0,1.0);

	// Basic lighting
	float dist = length(dir);
	//float att = pow( max(0, 1-dist/lightData.w), 2);	// Attenuation
	//float att = (lightData.w / (dist*dist) - 1.0 / lightData.w) * 0.4; // inverse square intersecting 0 at lightdata.w
	//float att = pow(radius/4,3) / pow(dist,2) - pow(radius/4,3)/pow(radius,2);
	
	float att = 1.0 / pow(dist,2.0) - 1.0/pow(radius,2.0);


	float light = max(0.0, dot(normal, dir)) * att;

	vec3 result = albedo * colour.rgb * light;

	// Specular (blinn-phong)
	if(light > 0.0) {
		vec3 l = normalize(dir);
		vec3 e = normalize(cam.xyz - centre);
		vec3 h = normalize(l + e);
		result += pow( max( dot(h, normal), 0.0), 12.0) * att;
	}
	fragment = vec4( max(result,0.0), 1.0);
}


