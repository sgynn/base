#ifndef EMSCRIPTEN
#include "openxr.h"

#include <base/opengl.h>
#include <base/shader.h>
#include <base/material.h>
#include <base/renderer.h>

using namespace xr;

FlipTarget::FlipTarget(int w, int h, uint32 col, uint32 depth) : m_buffer(w,h,base::Texture::NONE) {
	uint32 staging[2] = {0, 0};
	glGenTextures(2, staging);
	glBindTexture(GL_TEXTURE_2D, staging[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
	glBindTexture(GL_TEXTURE_2D, staging[1]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, depth, w, h, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, nullptr);
	m_buffer.attachColour(0, base::Texture(staging[0]));
	m_buffer.attachDepth(base::Texture(staging[1]));
	GL_CHECK_ERROR;
	
	static float vx[] = { -1,-1, 3,-1, -1,3 };
	m_vx.attributes.add(base::VA_VERTEX, base::VA_FLOAT2);
	m_vx.setData(vx, 3, 8);
	m_vx.createBuffer();
	glGenVertexArrays(1, &m_geometry);
	glBindVertexArray(m_geometry);
	m_vx.bind();
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, false, m_vx.getStride(), nullptr);
	glBindVertexArray(0);
	GL_CHECK_ERROR;

	static const char* vs =
	"#version 330\nin vec2 vertex; out vec2 uv; void main() { gl_Position=vec4(vertex,0.5,1.0); uv=vertex*vec2(0.5,-0.5)+0.5; }\n";
	static const char* fs = 
	"#version 330\nin vec2 uv; out vec4 frag; uniform sampler2D col; uniform sampler2D dep;"
	"void main() { frag=texture(col, uv); gl_FragDepth=texture(dep, uv).r; }\n";

	m_material = new base::Material();
	base::Pass* pass = m_material->addPass();
	pass->setShader(base::Shader::create(vs, fs));
	pass->setTexture("col", new base::Texture(staging[0]));
	pass->setTexture("dep", new base::Texture(staging[1]));
	pass->state.depthTest = base::DEPTH_ALWAYS;
	pass->compile();
	GL_CHECK_ERROR;
	char log[2048];
	pass->getShader()->getLog(log, 2048);
	printf("%s", log);
}

void FlipTarget::execute(base::FrameBuffer* out, base::RenderState& state) {
	state.setMaterial(m_material);
	state.setTarget(out, Rect(0,0,m_buffer.getSize()));
	glBindVertexArray(m_geometry);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	GL_CHECK_ERROR;
}

FlipTarget::~FlipTarget() {
	delete m_material;
}
#endif

