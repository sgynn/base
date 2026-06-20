#version 130

in vec2 coord;
out vec4 fragment;
uniform sampler2D source;
void main() {
	gl_FragDepth = texture(source, coord).r;
	fragment = vec4(0,0,0,0);
}


