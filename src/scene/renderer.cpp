#include <base/renderer.h>
#include <base/drawable.h>
#include <base/material.h>
#include <base/shader.h>
#include <base/autovariables.h>
#include <base/hardwarebuffer.h>
#include <base/framebuffer.h>
#include <base/camera.h>
#include <base/opengl.h>
#include <algorithm>

using namespace base;

Renderer::Renderer() {
	memset(m_queueMode, 0, 256*sizeof(RenderQueueMode));
}

Renderer::~Renderer() {
}

void Renderer::setQueue(unsigned char queue, RenderQueueMode mode) {
	m_queueMode[queue] = mode;
}

void Renderer::clear() {
	for(auto& queue: m_drawables) queue.clear();
}

void Renderer::add(Drawable* d, unsigned char queue) {
	m_drawables[queue].push_back(d);
}

void Renderer::remove(Drawable* d, unsigned char queue) {
	for(size_t i=0; i<m_drawables[queue].size(); ++i) {
		if(m_drawables[queue][i] == d) {
			m_drawables[queue][i] = m_drawables[queue].back();
			m_drawables[queue].pop_back();
			return;
		}
	}
}

void Renderer::render(unsigned char first, unsigned char last) {
	vec3 cp = m_state.getCamera()->getPosition();
	#define DIST(d) (cp.x*d->getTransform()[12] + cp.y*d->getTransform()[13] + cp.z*d->getTransform()[14])
	for(size_t i=first; i<=last; ++i) {
		std::vector<Drawable*>& queue = m_drawables[i];
		switch(m_queueMode[i]) {
		case QUEUE_NORMAL: break;
		case QUEUE_SORTED:         std::sort(queue.begin(), queue.end(), [cp](Drawable*a, Drawable*b) { return DIST(a) < DIST(b); }); break;
		case QUEUE_SORTED_INVERSE: std::sort(queue.begin(), queue.end(), [cp](Drawable*a, Drawable*b) { return DIST(a) > DIST(b); }); break;
		}

		for(Drawable* d : queue) {
			m_state.getVariableSource()->setModelMatrix(d->getTransform());
			m_state.getVariableSource()->setCustom(d->getCustom());
			d->draw(m_state);
		}
	}
}

void Renderer::clearScreen() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

// ============================================================= //

RenderState::RenderState() : m_camera(0), m_activePass(0), m_materialOverride(0), m_materialTechnique(0), m_viewport(0,0,0,0) {
	m_auto = new AutoVariableSource();
}

RenderState::~RenderState() {
	delete m_auto;
}

void RenderState::setCamera(Camera* c) {
	if(c && c != m_camera) m_auto->setCamera(c);
	m_camera = c;
}

void RenderState::setTarget(const FrameBuffer* f) {
	if(!f) f = &FrameBuffer::Screen;
	if(!f->isBound()) f->bind();
	m_viewport.set(0,0,f->width(),f->height());
}

void RenderState::setTarget(const FrameBuffer* f, const Rect& viewport) {
	if(!f) f = &FrameBuffer::Screen;
	if(!f->isBound() || viewport!=m_viewport) f->bind(viewport);
	m_viewport = viewport;
}

void RenderState::setMaterial(Material* m) {
	if(m_materialOverride) m = m_materialOverride;
	Pass* p = m? m->getPassByID(m_materialTechnique): 0;
	if(m && !p) p = m->getPass(0); // Fallback ?
	setMaterialPass(p);
}

void RenderState::setMaterialPass(Pass* p) {
	if(p) {
		if(p == m_activePass) p->bindVariables(m_auto, true);
		else p->bind(m_auto); // ToDo: separate blocks
	}
	else {
		// This is actually an error - perhaps use a default vertex only shader ?
		Shader::Null.bind();
	}
	m_activePass = p;
}

void RenderState::setMaterialOverride(Material* m) {
	m_materialOverride = m;
}

void RenderState::setMaterialTechnique(size_t id) {
	m_materialTechnique = id;
}

void RenderState::setMaterialTechnique(const char* t) {
	m_materialTechnique = Material::getPassID(t);
}

void RenderState::unbindVertexBuffers() {
	static HardwareIndexBuffer ix;
	static HardwareVertexBuffer vx;
	glBindVertexArray(0);
	ix.bind();
	vx.bind();
}

void RenderState::reset() {
	m_materialTechnique = 0;
	setMaterial(0);
	glDepthMask(1);
	
	static Blend blend(BLEND_ALPHA);
	static MacroState state;
	blend.bind();
	state.bind();

//	glColor4f(1,1,1,1); // can use glVertexAttrib4f(3, 1,1,1,1); to set an unbuffered attribute
	unbindVertexBuffers();
	FrameBuffer::Screen.bind();
}

