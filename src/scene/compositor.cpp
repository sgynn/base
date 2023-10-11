#include <base/compositor.h>
#include <base/shader.h>
#include <base/scene.h>
#include <base/material.h>
#include <base/renderer.h>
#include <base/framebuffer.h>
#include <base/hardwarebuffer.h>
#include <base/opengl.h>
#include <base/hashmap.h>
#include <cstring>
#include <map>
#include <set>

using namespace base;

Compositor* Compositor::Output = new Compositor();

CompositorPassClear::CompositorPassClear(char bits, uint c, float d) : mDepth(d), mBits(0) {
	mColour[2] = (c&0xff)/255.f;
	mColour[1] = ((c>>8)&0xff)/255.f;
	mColour[0] = ((c>>16)&0xff)/255.f;
	mColour[3] = ((c>>24)&0xff)/255.f;
	if(bits & CLEAR_COLOUR)  mBits |= GL_COLOR_BUFFER_BIT;
	if(bits & CLEAR_DEPTH)   mBits |= GL_DEPTH_BUFFER_BIT;
	if(bits & CLEAR_STENCIL) mBits |= GL_STENCIL_BUFFER_BIT;
}
CompositorPassClear::CompositorPassClear(char bits, const float* c, float d) : mDepth(d), mBits(0) {
	if(c) memcpy(mColour, c, 4*sizeof(float));
	else memset(mColour, 0, 4*sizeof(float));
	if(bits & CLEAR_COLOUR)  mBits |= GL_COLOR_BUFFER_BIT;
	if(bits & CLEAR_DEPTH)   mBits |= GL_DEPTH_BUFFER_BIT;
	if(bits & CLEAR_STENCIL) mBits |= GL_STENCIL_BUFFER_BIT;
}
void CompositorPassClear::execute(const FrameBuffer* target, const Rect& view, Renderer* r, Camera*, Scene*) const {
	bool full = view.x==0 && view.y==0 && view.width==target->width() && view.height==target->height();
	r->getState().setTarget(target, view);
	if(!full) {
		// Scissor test to clear part of viewport. glClear is not affected by glViewport
		glEnable(GL_SCISSOR_TEST);
		glScissor(view.x, view.y, view.width, view.height);
	}
	glDepthMask(1); // If the previous material turned these off, glClear will not work
	glClearColor(mColour[0], mColour[1], mColour[2], mColour[3]);
	glClear(mBits);
	if(!full) glDisable(GL_SCISSOR_TEST);
}

// ================================================================================ //

CompositorPassQuad::CompositorPassQuad(const char* mat, const char* tech) : mMaterial(0), mInstancedMaterial(false) {
	mMaterialName = strdup(mat);
	mTechnique = Material::getPassID(tech);
}
CompositorPassQuad::CompositorPassQuad(Material* mat, const char* tech) : mMaterialName(0), mInstancedMaterial(false) {
	mMaterial = mat;
	mTechnique = Material::getPassID(tech);
}
CompositorPassQuad::~CompositorPassQuad() {
	free(mMaterialName);
	for(auto& i: mTextures) free(i.value.source);
	if(mInstancedMaterial) delete mMaterial;
}
void CompositorPassQuad::setMaterial(Material* m, const char* tech, bool isInstance) {
	if(tech) mTechnique = Material::getPassID(tech);
	else mTechnique = 0;
	mInstancedMaterial = isInstance;
	mMaterial = m;

}
Pass* CompositorPassQuad::getMaterialPass() {
	if(!mMaterial) return 0;
	Pass* p = mMaterial->getPass(mTechnique);
	if(!p) p = mMaterial->getPass(0);
	return p;
}
void CompositorPassQuad::setTexture(const char* name, const char* tex) {
	mTextures[name] = TextureOverride { strdup(tex), 0 };
}
void CompositorPassQuad::setTexture(const char* name, const Texture* tex, bool compile) {
	if(!tex) return;
	auto it = mTextures.find(name);
	if(it != mTextures.end()) {
		free(it->value.source);
		it->value = TextureOverride{0,tex};
	}
	else mTextures[name] = TextureOverride{0,tex};
	setupInstancedMaterial();
	Pass* p = getMaterialPass();
	if(p) {
		p->setTexture(name, tex);
		if(compile) p->compile();
	}
}
void CompositorPassQuad::setupInstancedMaterial() {
	if(mMaterial && !mInstancedMaterial) {
		mMaterial = mMaterial->clone();
		mInstancedMaterial = true;
	}
}
void CompositorPassQuad::resolveExternals(MaterialResolver* r) {
	if(mMaterialName && !mMaterial) mMaterial = r->resolveMaterial(mMaterialName);
	if(!mTextures.empty()) {
		setupInstancedMaterial();
		Pass* pass = getMaterialPass();
		for(auto& t: mTextures) {
			if(!t.value.texture) {
				t.value.texture = r->resolveTexture(t.value.source);
				if(pass && t.value.texture) pass->setTexture(t.key, t.value.texture);
			}
		}
		if(pass) pass->compile();
	}
}

void CompositorPassQuad::execute(const FrameBuffer* target, const Rect& view, Renderer* r, Camera*, Scene*) const {
	createGeometry();

	RenderState& state = r->getState();
	state.setMaterialTechnique(mTechnique);
	state.setMaterial(mMaterial);
	if(!Shader::current().isCompiled() || !sGeometry) 
		return; // Error: no shader
	state.setTarget(target, view);

	glBindVertexArray(sGeometry);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

unsigned CompositorPassQuad::sGeometry = 0;
void CompositorPassQuad::createGeometry() {
	if(sGeometry) return;
	static float vx[] = { -1,-1,0,0,   1,-1,1,0,   -1,1,0,1,   1,1,1,1 };
	static HardwareVertexBuffer buf;
	glGenVertexArrays(1, &sGeometry);
	glBindVertexArray(sGeometry);

	buf.setData(vx, 4, 16);
	buf.attributes.add(VA_VERTEX, VA_FLOAT2);
	buf.attributes.add(VA_TEXCOORD, VA_FLOAT2);
	buf.createBuffer();
	buf.bind();

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(0, 2, GL_FLOAT, false, 16, (void*)0);
	glVertexAttribPointer(3, 2, GL_FLOAT, false, 16, (void*)8);

	glBindVertexArray(0);
}

// ================================================================================ //

CompositorPassScene::CompositorPassScene(uint8 q, const char* tech) : mFirst(q), mLast(q), mMaterialName(0), mTechnique(0), mCameraName(0), mMaterial(0) {
	mTechnique = Material::getPassID(tech);
}
CompositorPassScene::CompositorPassScene(uint8 a, uint8 b, const char* tech) : mFirst(a), mLast(b), mMaterialName(0), mTechnique(0), mCameraName(0), mMaterial(0) {
	mTechnique = Material::getPassID(tech);
}

CompositorPassScene::CompositorPassScene(uint8 a, uint8 b, const char* material, const char* tech) : mFirst(a), mLast(b), mMaterialName(0), mTechnique(0), mCameraName(0), mMaterial(0) {
	mMaterialName = material? strdup(material): 0;
	mTechnique = Material::getPassID(tech);
}

CompositorPassScene::CompositorPassScene(uint8 a, uint8 b, Material* material, const char* tech) : mFirst(a), mLast(b), mMaterialName(0), mTechnique(0), mCameraName(0), mMaterial(material) {
	mMaterial = material;
	mTechnique = Material::getPassID(tech);
}

CompositorPassScene::~CompositorPassScene() {
	free(mMaterialName);
	free(mCameraName);
}

void CompositorPassScene::setCamera(const char* camera) {
	if(mCameraName) free(mCameraName);
	mCameraName = camera? strdup(camera): 0;
}

void CompositorPassScene::resolveExternals(MaterialResolver* r) {
	if(mMaterialName && !mMaterial) mMaterial = r->resolveMaterial(mMaterialName);
}

void CompositorPassScene::execute(const FrameBuffer* target, const Rect& view, Renderer* renderer, Camera* camera, Scene* scene) const {
	RenderState& state = renderer->getState();
	state.setTarget(target, view);
	state.setMaterialTechnique(mTechnique);
	state.setCamera(camera);

	if(scene) {
		renderer->clear();
		scene->collect(renderer, state.getCamera(), mFirst, mLast);
	}
	renderer->render(mFirst, mLast);
}

// ================================================================================ //

CompositorPassCopy::CompositorPassCopy(const char* src, int index) : mSource(0), mSourceIndex(index) {
}
void CompositorPassCopy::execute(const FrameBuffer* target, const Rect& view, Renderer* r, Camera*, Scene*) const {
	const Texture* src = CompositorTextures::getInstance()->get(mSource, mSourceIndex);
	if(!src) return;
	if(target->texture().getFormat() != src->getFormat()) return;
	// ??
	//glCopyTexImage2D( tgt.unit(), 0, Texture::getInternalFormat(tgt.getFormat(), 0,0,tgt.width(), tgt.height(), 0);
}


// ================================================================================ //

CompositorTextures* CompositorTextures::getInstance() {
	static CompositorTextures* t = new CompositorTextures;
	return t;
}
CompositorTextures::~CompositorTextures() {
	reset();
	for(CTexture* t: m_textures) {
		while(t) {
			CTexture* c = t;
			t = t->next;
			delete c->texture;
			delete c;
		}
	}
}
size_t CompositorTextures::getId(const char* name) {
	return m_names.get(name);
}
const Texture* CompositorTextures::get(const char* name, int index) {
	return get( getId(name), index);
}
const Texture* CompositorTextures::get(size_t id, int index) {
	if(m_textures.size() <= id) m_textures.resize(id+1, 0);
	for(CTexture* t = m_textures[id]; t; t=t->next) {
		if(t->index == index) return t->texture;
	}
	// New one
	CTexture* t = new CTexture;
	t->index = index;
	t->texture = new Texture;
	t->next = m_textures[id];
	m_textures[id] = t;
	return t->texture;
}
void CompositorTextures::expose(size_t id, FrameBuffer* buffer) {
	if(id < m_textures.size()) {
		for(CTexture* t = m_textures[id]; t; t=t->next) {
			if(t->index<0) *t->texture = buffer->depthTexture();
			else *t->texture = buffer->texture(t->index);
			if(!t->texture->unit()) {
				printf("Error: Compositor exposed texture not part of framebuffer %s:%c\n", m_names.name(id), t->index<0? 'd': t->index+'0');
			}
		}
	}
}
void CompositorTextures::expose(size_t id, const Texture* texture) {
	if(id < m_textures.size()) {
		for(CTexture* t = m_textures[id]; t; t=t->next) {
			if(t->index==0) *t->texture = *texture;
		}
	}
}
void CompositorTextures::conceal(size_t id) {
	static const Texture base;
	if(id < m_textures.size()) {
		for(CTexture* t = m_textures[id]; t; t=t->next) *t->texture = base;
	}
}
void CompositorTextures::reset() {
	Texture base;
	for(CTexture* t : m_textures) {
		for(;t;t=t->next) *t->texture = base;
	}
}


// ================================================================================ //

Compositor::Compositor(const char* name) {
	if(name) strcpy(m_name, name);
	else m_name[0] = 0;
}
Compositor::~Compositor() {
	for(Connector& i: m_inputs) {
		free(i.name);
		if(i.name!=i.buffer) free(i.buffer);
	}
	for(Connector& i: m_outputs) {
		free(i.name);
		if(i.name!=i.buffer) free(i.buffer);
	}
	for(Buffer* b: m_buffers) {
		delete [] b->depth.input;
		for(BufferAttachment& a: b->colour) delete [] a.input;
		delete b;
	}
	for(Pass& p : m_passes) {
		free(p.target);
		delete p.pass;
	}
}

void Compositor::addPass(const char* target, CompositorPass* pass) {
	Pass cp;
	cp.pass = pass;
	cp.target = strdup(target);
	m_passes.push_back(cp);
}

Compositor::Buffer* Compositor::addBuffer(const char* name, int w, int h, Format f1, Format f2, Format f3, Format f4, Format dfmt, bool unique) {
	for(Buffer* b: m_buffers) {
		if(strcmp(name, b->name)==0) printf("Error: Compositor %s already has a buffer named %s\n", m_name, name);
	}
	Buffer* b = new Buffer;
	strncpy(b->name, name, 63);
	b->width = w;
	b->height = h;
	b->relativeWidth = 0;
	b->relativeHeight = 0;
	b->colour[0].format = f1;
	b->colour[1].format = f2;
	b->colour[2].format = f3;
	b->colour[3].format = f4;
	b->depth.format = dfmt;
	b->unique = unique;
	b->texture = nullptr;
	m_buffers.push_back(b);
	return b;
}
Compositor::Buffer* Compositor::addBuffer(const char* name, int w, int h, Format f1, Format dfmt, bool unique)                 { return addBuffer(name,w,h,f1,Format::NONE,Format::NONE,Format::NONE,dfmt,unique); }
Compositor::Buffer* Compositor::addBuffer(const char* name, int w, int h, Format f1, Format f2, Format dfmt, bool unique)         { return addBuffer(name,w,h,f1,f2,Format::NONE,Format::NONE,dfmt,unique); }
Compositor::Buffer* Compositor::addBuffer(const char* name, int w, int h, Format f1, Format f2, Format f3, Format dfmt, bool unique) { return addBuffer(name,w,h,f1,f2,f3,Format::NONE,dfmt,unique); }
Compositor::Buffer* Compositor::addBuffer(const char* name, float w, float h, Format f1, Format dfmt, bool unique)                 { return addBuffer(name,w,h,f1,Format::NONE,Format::NONE,Format::NONE,dfmt,unique); }
Compositor::Buffer* Compositor::addBuffer(const char* name, float w, float h, Format f1, Format f2, Format dfmt, bool unique)         { return addBuffer(name,w,h,f1,f2,Format::NONE,Format::NONE,dfmt,unique); }
Compositor::Buffer* Compositor::addBuffer(const char* name, float w, float h, Format f1, Format f2, Format f3, Format dfmt, bool unique) { return addBuffer(name,w,h,f1,f2,f3,Format::NONE,dfmt,unique); }
Compositor::Buffer* Compositor::addBuffer(const char* name, float w, float h, Format f1, Format f2, Format f3, Format f4, Format dfmt, bool unique) {
	Buffer* b = addBuffer(name, 0,0,f1,f2,f3,f4,dfmt,unique);
	b->relativeHeight = h;
	b->relativeWidth = w;
	return b;
}

Compositor::Buffer* Compositor::addTexture(const char* name, Texture* texture) {
	Format O = Format::NONE;
	Buffer* buf = addBuffer(name, texture->width(), texture->height(), O,O,O,O,O,true);
	buf->texture = texture;
	return buf;
}
void Compositor::addInput(const char* name, const char* buffer) { addConnector(m_inputs, name, buffer); }
void Compositor::addOutput(const char* name, const char* buffer) { addConnector(m_outputs, name, buffer); }
void Compositor::addConnector(std::vector<Connector>& list, const char* name, const char* buffer) {
	for(Connector& c: list) {
		if(strcmp(c.name, name)==0) {
			printf("Warning: Compositor '%s' already has connector '%s'\n", m_name, name);
			return;
		}
	}
	Connector c;
	c.part = 0;
	c.name = strdup(name);
	if(name==buffer || !buffer) c.buffer = c.name;
	else {
		const char* split = strchr(buffer, ':');
		if(!split) c.buffer = strdup(buffer);
		else {
			c.buffer = strdup(buffer);
			c.buffer[split-buffer] = 0;
			if(strcmp(split, ":depth")==0) c.part = 1;
			else if(split[1]>='0' && split[1]<='9') c.part = atoi(split+1) + 2;
			if(c.part==0 || c.part>5) printf("Error: Invalid connector subscript '%s'\n", split+1);
		}
	}
	list.push_back(c);
}

int Compositor::getOutput(const char* name) const {
	for(size_t i=0; i<m_outputs.size(); ++i) if(strcmp(name, m_outputs[i].name)==0) return (int)i;
	return -1;
}
int Compositor::getInput(const char* name) const {
	for(size_t i=0; i<m_inputs.size(); ++i) if(strcmp(name, m_inputs[i].name)==0) return (int)i;
	return -1;
}
Compositor::Buffer* Compositor::getBuffer(const char* name) const {
	for(Buffer* b : m_buffers) if(strcmp(name, b->name)==0) return b;
	return 0;
}

// ----------------------------------------------- //

const char* Compositor::getInputName(uint i) const { return i<m_inputs.size()? m_inputs[i].name: 0; }
const char* Compositor::getOutputName(uint i) const { return i<m_outputs.size()? m_outputs[i].name: 0; }
const char* Compositor::getPassTarget(uint i) const { return i<m_passes.size()? m_passes[i].target: 0; }
CompositorPass* Compositor::getPass(uint i) { return i<m_passes.size()? m_passes[i].pass: 0; }
Compositor::Buffer* Compositor::getBuffer(uint i) { return i<m_buffers.size()? m_buffers[i]: 0; }
void Compositor::removeInput(const char* name) { removeConnector(m_inputs, getInput(name)); }
void Compositor::removeOutput(const char* name) { removeConnector(m_outputs, getOutput(name)); }
void Compositor::removeConnector(std::vector<Connector>& list, int index) {
	if(index<0) return;
	free(list[index].name);
	if(list[index].name != list[index].buffer) free(list[index].buffer);
	list.erase(list.begin() + index);
}
void Compositor::removeBuffer(const char* name) {
	Buffer* b = getBuffer(name);
	for(size_t i=0; i<m_buffers.size(); ++i) {
		if(m_buffers[i]==b) {
			m_buffers.erase(m_buffers.begin()+i);
			delete b;
			break;
		}
	}
}
void Compositor::removePass(uint index) {
	if(index>=m_passes.size()) return;
	delete m_passes[index].pass;
	free(m_passes[index].target);
	m_passes.erase(m_passes.begin()+index);
}

// ================================================================================ //

CompositorGraph::CompositorGraph() {
}
CompositorGraph::~CompositorGraph() {
}
void CompositorGraph::link(Compositor* a, Compositor* b) { link(a,b,0,0); }
void CompositorGraph::link(Compositor* a, Compositor* b, const char* key) { link(a,b,key,key); }
void CompositorGraph::link(Compositor* a, Compositor* b, const char* ka, const char* kb) {
	const size_t nope = -1;
	size_t ia = find(a);
	size_t ib = find(b);
	if(ia==nope) ia = add(a);
	if(ib==nope) ib = add(b);
	link(ia, ib, ka, kb);
}

// ----- //

Compositor* CompositorGraph::getCompositor(size_t key) const {
	return m_compositors[key];
}
size_t CompositorGraph::add(Compositor* c) {
	m_compositors.push_back(c);
	return m_compositors.size()-1;
}
size_t CompositorGraph::find(Compositor* c, int n) const {
	for(size_t i=0; i<m_compositors.size(); ++i) {
		if(m_compositors[i]==c && --n<0) return i;
	}
	return -1;
}
void CompositorGraph::link(size_t a, size_t b) { link(a,b,0,0); }
void CompositorGraph::link(size_t a, size_t b, const char* k) { link(a,b,k,k); }
void CompositorGraph::link(size_t a, size_t b, const char* ka, const char* kb) {
	int ia = ka? m_compositors[a]->getOutput(ka): -1;
	int ib = kb? m_compositors[b]->getInput(kb): -1;
	if(ka&&ia<0) printf("Link: No compositor output '%s'\n", ka);
	if(kb&&ib<0) printf("Link: No compositor input '%s'\n", kb);
	m_links.push_back( Link{ a,b,ia,ib } );
}

void CompositorGraph::resolveExternals(MaterialResolver* r) {
	for(Compositor* c: m_compositors) {
		for(Compositor::Pass& p: c->m_passes) p.pass->resolveExternals(r);
	}
}

size_t CompositorGraph::size() const {
	return m_compositors.size();
}

bool CompositorGraph::requiresTargetSize() const {
	for(Compositor* c: m_compositors) {
		for(Compositor::Buffer* b: c->m_buffers) {
			if(b->relativeWidth!=0 && b->relativeHeight!=0) return true;
		}
	}
	return false;
}

// ================================================================================ //

Workspace::Workspace(const CompositorGraph* c) : m_graph(c), m_compiled(false) {
}
Workspace::~Workspace() {
	destroy();
}
void Workspace::destroy() {
	for(FrameBuffer* b : m_buffers) delete b;
	m_buffers.clear();
	m_passes.clear();
	m_compiled = false;
}
bool Workspace::isCompiled() const {
	return m_compiled;
}
void Workspace::execute(Scene* scene, Renderer* renderer) {
	execute(&FrameBuffer::Screen, scene, renderer);
}
void Workspace::execute(const FrameBuffer* output, Scene* scene, Renderer* renderer) {
	execute(output, Rect(0,0,output->width(), output->height()), scene, renderer);
}
void Workspace::execute(const FrameBuffer* output, const Rect& view, Scene* scene, Renderer* renderer) {
	CompositorTextures* tex = CompositorTextures::getInstance();
	for(const Pass& pass: m_passes) {
		// Expose buffers
		for(size_t e: pass.conceal) tex->conceal(e);
		for(const Expose& e: pass.expose) {
			if(e.buffer) tex->expose(e.id, e.buffer);
			else if(e.texture) tex->expose(e.id, e.texture);
		}
		Camera* camera = m_cameras[pass.camera].camera;
		const FrameBuffer* target = pass.target? pass.target: output;
		Rect targetRect = pass.target? Rect(0, 0, target->width(), target->height()): view;

		// Run
		pass.pass->execute(target, targetRect, renderer, camera, scene);
	}
}

void Workspace::setCamera(Camera* cam) { setCamera("default", cam); }
void Workspace::setCamera(const char* name, Camera* cam) {
	m_cameras[ getCameraIndex(name) ].camera = cam;
}
void Workspace::copyCameras(const Workspace* from) {
	for(auto& i: from->m_cameras) setCamera(i.name, i.camera);
}
size_t Workspace::getCameraIndex(const char* name) {
	for(size_t i=0; i<m_cameras.size(); ++i) {
		if(strcmp(m_cameras[i].name, name)==0) return i;
	}
	m_cameras.push_back( CameraRef() );
	m_cameras.back().camera = 0;
	strcpy(m_cameras.back().name, name);
	return m_cameras.size() - 1;
}

Camera* Workspace::getCamera(const char* name) const {
	for(size_t i=0; i<m_cameras.size(); ++i) {
		if(strcmp(m_cameras[i].name, name)==0) return m_cameras[i].camera;
	}
	return 0;
}

const FrameBuffer* Workspace::getBuffer(const char* compositor, const char* name) {
	if(!m_compiled) return 0;
	for(Compositor* c: m_graph->m_compositors) {
		if(strcmp(c->m_name, compositor)==0) {
			for(const Compositor::Pass& pass: c->m_passes) {
				if(strcmp(name, pass.target)==0) {
					for(Pass& p: m_passes) {
						if(p.pass == pass.pass) return p.target;
					}
					break;
				}
			}
			break;
		}
	}
	return 0;
}


bool Workspace::compile(int w, int h) {
	if(!m_graph) {
		printf("Error: No compositor data\n");
		return false;
	}
	if(m_compiled) destroy();
	if(Compositor::Output->m_inputs.empty()) Compositor::Output->addInput("");
	const CompositorGraph* chain = m_graph;
	m_width = w;
	m_height = h;
	
	// Build some helper structures
	struct LinkBuffer {
		Compositor::Buffer* buffer = 0;
		FrameBuffer* frameBuffer = 0;
	};
	struct CLink {
		size_t from; int out;		// Compositor index and output index this link came from
		LinkBuffer* data = 0;		// Data being passed over link
		char part = 0; 				// 0=framebuffer, 1=depth, 2-5=colour
		bool used = true;			// Is this link actually used
		bool processed = false;		// Processing flag
	};
	struct CInfo {
		Compositor* compositor;				// Compositor data
		std::vector<int> inputs, outputs;	// CLink data for each connection - index in links array
		int order;							// Processing order
		bool used;							// Is this compositor actually used
	};
	std::vector<CInfo> compositors;
	std::vector<CLink> links;
	int orderIndex = 0;
	// Initialise Compositor info structures
	compositors.reserve(chain->m_compositors.size());
	for(Compositor* c: chain->m_compositors) {
		CInfo info;
		info.compositor = c;
		info.inputs.resize(c->m_inputs.size(), -1);	// Only one link per input. Can have multiple on outputs.
		info.order = 0;
		info.used = true;
		compositors.push_back(info);
	}
	// Initialise link info structures
	for(auto& link : chain->m_links) {
		// Compositor order tie breaker from defined link order
		if(compositors[link.a].order==0) compositors[link.a].order = ++orderIndex;
		if(compositors[link.b].order==0) compositors[link.b].order = ++orderIndex;

		// Link named
		if(link.out>=0 && link.in>=0) {
			compositors[link.a].outputs.push_back(links.size());
			compositors[link.b].inputs[link.in] = links.size();
			links.push_back( CLink{link.a, link.out} );
		}
		// Name is optional if only one option
		else if(chain->m_compositors[link.a]->m_outputs.size()==1 && compositors[link.b].inputs.size()==1) {
			compositors[link.a].outputs.push_back(links.size());
			compositors[link.b].inputs[0] = links.size();
			links.push_back( CLink{link.a, 0} );
		}
		// Link all with matching names
		else for(size_t out=0; out<chain->m_compositors[link.a]->m_outputs.size(); ++out) {
			const char* name = chain->m_compositors[link.a]->m_outputs[out].name;
			int in = chain->m_compositors[link.b]->getInput(name);
			if(in>=0) {
				compositors[link.a].outputs.push_back(links.size());
				compositors[link.b].inputs[in] = links.size();
				links.push_back( CLink{link.a, (int)out} );
			}
		}
	}
	// We now have a convenient structured graph we can traverse. Yay
	
	// Buffer splitting
	for(CInfo& c: compositors) {
		for(size_t i=0; i<c.inputs.size(); ++i) {
			if(c.inputs[i] >= 0) {
				CLink& link = links[c.inputs[i]];
				int b = c.compositor->m_inputs[i].part;
				int a = compositors[link.from].compositor->m_outputs[link.out].part;
				if(a&&b) printf("Error: Invalid compositor input %s:%s\n", c.compositor->m_name, c.compositor->getInputName(i));
				else link.part = a? a: b;
			}
		}
	}
	

	std::vector<LinkBuffer> linkBuffers;
	linkBuffers.resize(links.size());
	int nextBuffer = 0;
	auto allocateLinkBuffer = [&](Compositor::Buffer* buffer=0){ assert(nextBuffer<(int)linkBuffers.size()); linkBuffers[nextBuffer].buffer=buffer; return &linkBuffers[nextBuffer++]; };

	// Output buffer - has only one input
	size_t outputIndex = chain->find(Compositor::Output);
	CInfo& outputInfo = compositors[outputIndex];
	if(outputIndex>=compositors.size() || outputInfo.inputs.empty() || outputInfo.inputs[0]<0) {
		printf("Error: Compositor chain not linked to output\n");
		return false; // Error - nothing links to output
	}
	CLink& link = links[ outputInfo.inputs[0] ];
	Compositor::Buffer outputBuffer;
	memset(&outputBuffer, 0, sizeof(outputBuffer));
	strcpy(outputBuffer.name, "OUTPUT");
	link.processed = true;
	link.data = allocateLinkBuffer(&outputBuffer);

	std::vector<size_t> reverseList;
	std::set<size_t> openList;
	if(compositors[link.from].outputs.size() == 1)
		openList.insert( link.from );

	// Non-connected compositiors - these will be skipped
	for(size_t i=0; i<compositors.size(); ++i) {
		if(i!=outputIndex && compositors[i].outputs.empty()) {
			compositors[i].used = i!=outputIndex;
			openList.insert(i);
		}
	}

	auto compareBuffers = [](const Compositor::Buffer* a, const Compositor::Buffer* b) {
		return a->width==b->width && a->height==b->height && a->relativeWidth==b->relativeWidth
			&& a->relativeHeight==b->relativeHeight && a->depth.format==b->depth.format
			&& a->colour[0].format==b->colour[0].format && a->colour[0].format==b->colour[0].format
			&& a->colour[2].format==b->colour[2].format && a->colour[3].format==b->colour[3].format
			&& !a->colour[0].input && !a->colour[1].input && !a->colour[2].input && !a->colour[3].input
			&& !b->colour[0].input && !b->colour[1].input && !b->colour[2].input && !b->colour[3].input
			&& !a->depth.input && !b->depth.input
			&& a->texture == b->texture;
	};
	auto getConnectorByBufferName = [](std::vector<Compositor::Connector>& list, const char* buffer) {
		for(size_t i=0; i<list.size(); ++i) if(list[i].part==0 && strcmp(list[i].buffer, buffer)==0) return(int)i;
		return -1;
	};
	auto createFrameBuffer = [w, h, &links](const Compositor::Buffer* buffer, const CInfo& info) {
		int bw = buffer->relativeWidth>0? buffer->relativeWidth * w: buffer->width;
		int bh = buffer->relativeHeight>0? buffer->relativeHeight * h: buffer->height;
		FrameBuffer* buf = new FrameBuffer(bw, bh);
		buf->bind();
		auto getInputTexture = [&](const Compositor::BufferAttachment& data) -> const Texture* {
			if(!data.input) return nullptr;
			int inputIndex = info.compositor->getInput(data.input);
			if(inputIndex < 0) return nullptr; // Invalid input
			const LinkBuffer* linkData = links[info.inputs[inputIndex]].data;
			if(!linkData) return nullptr;
			if(linkData->buffer && linkData->buffer->texture) return linkData->buffer->texture;
			if(!linkData->frameBuffer) return nullptr; // No framebuffer
			const FrameBuffer* b = linkData->frameBuffer;
			const Texture& result = data.part==Compositor::DepthIndex? b->depthTexture(): b->texture(data.part);
			if(result.getFormat() == Texture::NONE) return nullptr; // Slot not set
			else return &result;
		};
		if(buffer->depth.format) {
			buf->attachDepth((Texture::Format) buffer->depth.format);
			buf->depthTexture().setFilter(GL_NEAREST, GL_NEAREST);
		}
		else if(const Texture* tex = getInputTexture(buffer->depth)) {
			buf->attachDepth(*tex);
		}
		int attachment = 0;
		for(const auto& colour: buffer->colour) {
			if(const Texture* tex = getInputTexture(colour)) buf->attachColour(attachment, *tex);
			else if(colour.input) buf->attachColour(Texture::RGBA8); // Fallback if input not found
			else if(colour.format) buf->attachColour((Texture::Format) colour.format);
			else break;
			++attachment;
		}
		return buf;
	};


	const size_t nope = -1;
	while(!openList.empty()) {
		// Select a compositor with all outputs linked
		size_t c = nope;
		for(size_t o : openList) {
			int used = false;
			bool complete = true;
			CInfo& info = compositors[o];
			for(int l : info.outputs) {
				if(l>=0) {
					if(!links[l].processed) complete = false;
					if(links[l].used) used = true;
				}
			}
			if(complete && !used) compositors[o].used = false;
			if(complete && (c==nope || info.order > compositors[c].order)) c = o;
		}
		if(c==nope) {
			printf("Error: Compositor link failed\n");
			return false; // Invalid links. Possibly cyclic.
		}

		// Debug: Keep unused
		compositors[c].used = true;

		// Update open list
		openList.erase(c);
		CInfo& info = compositors[c];
		for(int i: info.inputs) {
			if(i>=0) {
				links[i].processed = true;
				openList.insert(links[i].from);
				links[i].used = info.used;
			}
		}

		if(!info.used) continue;
		reverseList.push_back(c);

		// Get all connected links by buffer name:w
		struct LinkMap { const char* name; std::vector<int> links; };
		std::vector<LinkMap> buffers;
		auto addLinkToMap = [&](const char* buffer, int link) {
			for(LinkMap& m: buffers) if(strcmp(m.name, buffer)==0) { m.links.push_back(link); return; }
			buffers.push_back(LinkMap{buffer});
			buffers.back().links.push_back(link);
		};
		for(int i: info.outputs) addLinkToMap(info.compositor->m_outputs[links[i].out].buffer, i);
		for(size_t i=0; i<info.inputs.size(); ++i) {
			if(info.inputs[i]>=0) addLinkToMap(info.compositor->m_inputs[i].buffer, info.inputs[i]);
		}

		// Then connect them
		for(const LinkMap& buffer: buffers) {
			LinkBuffer* data = 0;
			// Get from other links
			for(int i: buffer.links) {
				if(!data || !data->buffer) data = links[i].data;
				else if(links[i].data && links[i].data->buffer && !compareBuffers(data->buffer, links[i].data->buffer)) {
					printf("Error: Buffer mismatch within links from %s : %s\n", info.compositor->m_name, info.compositor->getOutputName(links[i].out));
					return false;
				}
			}
			// Get from compositior buffers
			if(Compositor::Buffer* buf = info.compositor->getBuffer(buffer.name)) {
				if(!data) data = allocateLinkBuffer(buf);
				else if(!data->buffer) data->buffer = buf;
				else if(!compareBuffers(buf, data->buffer)) {
					printf("Error: Buffer mismatch for %s:%s\n", info.compositor->m_name, buf->name);
					return false;
				}
			}
			// Set all to this
			if(!data) data = allocateLinkBuffer();
			for(int i: buffer.links) links[i].data = data;
		}
	}

	// Valiate buffers
	for(CLink& l: links) if(l.used && !(l.data && l.data->buffer)) {
		Compositor* c = compositors[l.from].compositor;
		printf("Error: Compositor output %s:%s has no data\n", c->m_name, c->getOutputName(l.out));
		return false;
	}
	for(uint ci=0; ci<compositors.size(); ++ci) for(size_t i=0; i<compositors[ci].inputs.size(); ++i) {
		if(compositors[ci].inputs[i]<0) printf("Warning: Compositior %s input '%s' not connected\n", chain->m_compositors[ci]->m_name, chain->m_compositors[ci]->m_inputs[i].name);
	}

	if(reverseList.size() < compositors.size() - 1) {
		printf("Warning: Skipped %d compositors\n", (int)compositors.size() - (int)reverseList.size() - 1);
	}

	// Setup passes
	std::vector<Expose> exposed;
	for(size_t i=reverseList.size()-1; i<reverseList.size(); --i) {
		size_t ci = reverseList[i];
		CInfo& info = compositors[ci];
		std::vector<FrameBuffer*> internal;
		internal.resize(info.compositor->m_buffers.size(), 0);
		size_t from = m_passes.size();
		if(info.compositor->m_passes.empty()) continue;
		for(Compositor::Pass& src : info.compositor->m_passes) {
			// Get target buffer - all links should have them at this point
			int in = getConnectorByBufferName(info.compositor->m_inputs, src.target);
			int out = getConnectorByBufferName(info.compositor->m_outputs, src.target);
			size_t linkIndex = nope;
			if(in>=0 && info.inputs[in]>=0) linkIndex = info.inputs[in];
			if(linkIndex==nope && out>=0) {
				for(int l:info.outputs) if(links[l].out==out) linkIndex = l;
			}

			FrameBuffer* target = 0;
			if(linkIndex != nope) {
				CLink& link = links[linkIndex];
				if(link.data->buffer->texture || link.part) {
					printf("Error: Compositor target %s is read only in %s\n", link.data->buffer->name, info.compositor->m_name);
					continue;
				}
				target = link.data->frameBuffer;
				if(!target && link.data->buffer!=&outputBuffer) {
					target = link.data->frameBuffer = createFrameBuffer(link.data->buffer, info);
					m_buffers.push_back(target);
				}

				for(size_t i=0; i<info.compositor->m_buffers.size(); ++i) {
					if(info.compositor->m_buffers[i] == link.data->buffer) internal[i] = target;
				}
			}
			else {
				size_t index = 0;
				for(auto b: info.compositor->m_buffers) { if(strcmp(b->name, src.target)==0) break; else ++index; }
				Compositor::Buffer* bufferData = info.compositor->getBuffer(index);
				if(!bufferData) {
					printf("Error: Compositor has no target buffer %s\n", src.target);
					continue;
				}
				if(bufferData->texture) {
					printf("Error: Compositor target %s is read only in %s\n", bufferData->name, info.compositor->m_name);
					continue;
				}
				if(!internal[index]) {
					//internal[index] = createBuffer(bufferData, w, h);
					internal[index] = createFrameBuffer(bufferData, info);
					m_buffers.push_back(internal[index]);
				}
				target = internal[index];
			}
			size_t camera = src.pass->getCamera()? getCameraIndex(src.pass->getCamera()): 0;
			m_passes.push_back( Pass{src.pass,target,camera} );
		}

		// No passes used. Probably missing connections or invalid outputs
		if(from==m_passes.size()) {
			printf("Compositor %s has no valid passes\n", info.compositor->m_name);
			continue;
		}

		// Create list of all buffers accessible in this compositor
		std::vector<Expose> expose;
		auto setExpose = [&expose](size_t id, FrameBuffer* fb, const Texture* tex) {
			if(expose.size() <= id) expose.resize(id+1, Expose());
			expose[id] = Expose{id, fb, tex};
		};

		for(size_t i=0; i<internal.size(); ++i) {
			Compositor::Buffer* buffer = info.compositor->m_buffers[i];
			if(internal[i] || buffer->texture) {
				size_t id = CompositorTextures::getInstance()->getId( buffer->name );
				setExpose(id, internal[i], buffer->texture);
			}
		}
		for(size_t i=0; i<info.compositor->m_inputs.size(); ++i) {
			if(info.inputs[i]>=0) {
				CLink& link = links[info.inputs[i]];
				size_t id = CompositorTextures::getInstance()->getId( info.compositor->m_inputs[i].buffer );
				if(link.data->buffer->texture) setExpose(id, 0, link.data->buffer->texture);
				else if(link.part==1) setExpose(id, 0, &link.data->frameBuffer->depthTexture());
				else if(link.part>1) setExpose(id, 0, &link.data->frameBuffer->texture(link.part-2));
				else setExpose(id, link.data->frameBuffer, 0);
			}
		}
		// Why were we exposing outputs?
		//for(int l: info.outputs) {
		//	size_t id = CompositorTextures::getInstance()->getId( info.compositor->m_outputs[ links[l].out].buffer );
		//	setExpose(id, links[l].fb, links[l].buffer->texture);
		//}

		// Add to pass expose/conceal lists
		size_t active = nope;
		auto isExposed = [&exposed](size_t i) { return i<exposed.size() && (exposed[i].buffer || exposed[i].texture); };
		auto toExpose  = [&expose](size_t i) { return i<expose.size() && (expose[i].buffer || expose[i].texture); };
		for(size_t i=0; i<exposed.size(); ++i) {
			if(isExposed(i) && !toExpose(i)) m_passes[from].conceal.push_back(i);
		}
		for(size_t i=0; i<expose.size(); ++i) {
			if(expose[i].buffer && expose[i].buffer == m_passes[from].target) active = i;
			else if(toExpose(i)) m_passes[from].expose.push_back(expose[i]);
		}
		// expose/conceal target buffers of active passes
		for(size_t p=from+1; p<m_passes.size(); ++p) {
			Pass& pass = m_passes[p];
			if(active==nope || pass.target != expose[active].buffer) {
				if(active!=nope && toExpose(active)) pass.expose.push_back(expose[active]); 
				for(size_t i=0;i<expose.size(); ++i) {
					if(pass.target && expose[i].buffer==pass.target) {
						pass.conceal.push_back(i);
						active = i;
						break;
					}
				}
			}
		}
		if(active!=nope) expose[active].buffer = 0;
		exposed.swap(expose);
	}

	m_compiled = true;
	return true;
}

/*
FrameBuffer* Workspace::createBuffer(Compositor::Buffer* b, int sw, int sh) {
	int w = b->relativeWidth>0? b->relativeWidth * sw: b->width;
	int h = b->relativeHeight>0? b->relativeHeight * sh: b->height;
	FrameBuffer* buf = new FrameBuffer(w, h);
	buf->bind();
	if(b->depth.format) {
		buf->attachDepth((Texture::Format) b->depth.format);
		buf->depthTexture().setFilter(GL_NEAREST, GL_NEAREST);
	}
	for(const auto& colour: b->colour) {
		if(colour.format == Texture::NONE) break;
		buf->attachColour((Texture::Format) colour.format);
	}
	if(!buf->isValid()) printf("Error: Invalid framebuffer %s, [%d]\n", b->name, (int)b->colour[0].format);
	FrameBuffer::Screen.bind();
	return buf;
}
*/

// ============================================================ //

CompositorGraph* base::getDefaultCompositor() {
	static CompositorGraph* graph = 0;
	if(graph) return graph;
	Compositor* c = new Compositor();
	c->addPass("0", new CompositorPassClear(3, 0x000020, 1) );
	c->addPass("0", new CompositorPassScene(0,255) );
	c->addOutput("0");
	graph = new CompositorGraph;
	graph->link(c, Compositor::Output);
	return graph;
}

Workspace* base::createDefaultWorkspace() {
	Workspace* workspace = new Workspace(getDefaultCompositor());
	workspace->compile(0,0); // params irrelevant if no relative buffers created
	return workspace;
}


