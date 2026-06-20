#version 130
// Resolve gbuffer

#pragma fragment_shader

in vec2 coord;
out vec4 fragment;

uniform sampler2D bufferD;
uniform sampler2D buffer0;
uniform sampler2D buffer1;

uniform mat4 inverseView;
uniform mat4 proj;

uniform vec3 lightDirection;
uniform vec3 lightColour;
uniform vec3 ambient;

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
	vec4  buf0 = texture(buffer0, coord.xy);
	vec4  buf1 = texture(buffer1, coord.xy);

	if(depth == 1.0) discard;

	// Fragment position in world coordinates - not actually used
	vec3 position = constructViewPosition( vec3(coord, depth), proj);
	position = (inverseView * vec4(position,1.0)).xyz;

	vec3 albedo = buf0.rgb;
	vec3 normal = buf1.rgb * 2.0 - 1.0;
	float rough = buf0.a;


	// Basic direcional light
	float l = dot( normalize(normal), normalize(lightDirection));
	float s = (l+1.0)/1.3*0.2+0.1; // dark side lighting
	float light = max(s, l);

	// Ambient light
	float a = (normalize(normal).y + 1.0) / 1.3 * 0.2 + 0.1;

	fragment = vec4(albedo * light * lightColour + ambient * albedo * a, 1.0);

	// Glow
	fragment.rgb *= 1.0 + buf1.a * 8.0;


	//fragment = vec4(albedo * ambient, 1);
	gl_FragDepth = depth;
}


