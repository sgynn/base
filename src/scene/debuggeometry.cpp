#include <base/debuggeometry.h>
#include <base/drawable.h>
#include <base/renderer.h>
#include <base/material.h>
#include <base/shader.h>
#include <base/scene.h>
#include <base/autovariables.h>
#include <base/hardwarebuffer.h>
#include <base/opengl.h>
#include <assert.h>
#include <cstring>
#include <cstdio>


using namespace base;

class DebugGeometrySceneNode : public SceneNode {
	public:
	DebugGeometrySceneNode() : SceneNode("Debug Geometry") {}
	~DebugGeometrySceneNode() { DebugGeometryManager::getInstance()->removeNode(this); }
};


DebugGeometryManager* DebugGeometryManager::s_instance = 0;
DebugGeometryManager::DebugGeometryManager() : m_renderQueue(10), m_material(0) {
	s_instance = this;
}
DebugGeometryManager::~DebugGeometryManager() {
	if(s_instance==this) s_instance=0;
	for(auto& i: m_drawables) delete i.second;
	while(!m_nodes.empty()) delete m_nodes[0];
}

void DebugGeometryManager::initialise(Scene* scene, int queue, Material* mat) {
	if(!s_instance) new DebugGeometryManager();
	if(!mat && !s_instance->m_material) {
		s_instance->setMaterial( getDefaultMaterial() );
	}
	s_instance->setRenderQueue(queue);
	if(scene) {
		SceneNode* node = new DebugGeometrySceneNode();
		for(auto& i: s_instance->m_drawables) node->attach(i.second);
		s_instance->m_nodes.push_back(node);
		scene->add(node);
	}
}

void DebugGeometryManager::initialise(Scene* scene, int queue, bool onTop) {
	initialise(scene, queue);
	if(onTop) getDefaultMaterial()->getPass(0)->state.depthTest = DEPTH_ALWAYS;
}

void DebugGeometryManager::resetAll() {
	if(s_instance) for(auto d: s_instance->m_drawables) d.first->reset();
}

void DebugGeometryManager::setVisible(bool vis) {
	for(SceneNode* n: m_nodes) n->setVisible(vis);
}

void DebugGeometryManager::removeNode(SceneNode* node) {
	for(size_t i=0; i<m_nodes.size(); ++i) {
		if(m_nodes[i] == node) {
			m_nodes[i] = m_nodes.back();
			m_nodes.pop_back();
		}
	}
}

Material* DebugGeometryManager::getDefaultMaterial() {
	static Material* defaultMaterial = 0;
	if(!defaultMaterial) {
		// Create new default material
		static const char* vs_src =
		"#version 130\n"
		"in vec4 vertex;\n"
		"in vec4 colour;\n"
		"out vec4 vColour;\n"
		"uniform mat4 transform;\n"
		"void main() { gl_Position=transform * vertex; vColour=colour; }\n";
		static const char* fs_src = 
		"#version 130\n"
		"in vec4 vColour;\n"
		"out vec4 fragment;\n"
		"void main() { fragment = vColour; }\n";
		ShaderPart* vs = new ShaderPart(VERTEX_SHADER, vs_src);
		ShaderPart* fs = new ShaderPart(FRAGMENT_SHADER, fs_src);
		Shader* shader = new Shader();
		shader->attach(vs);
		shader->attach(fs);
		shader->bindAttributeLocation("vertex", 0);
		shader->bindAttributeLocation("colour", 4);

		defaultMaterial = new Material();
		Pass* pass = defaultMaterial->addPass("default");
		pass->setShader(shader);
		pass->getParameters().setAuto("transform", AUTO_MODEL_VIEW_PROJECTION_MATRIX);
		pass->compile();
		if(!shader->isCompiled()) printf("Error: Default debug material not compiled\n");
	}
	return defaultMaterial;
}

void DebugGeometryManager::add(DebugGeometry* g) {
	MutexLock lock(m_mutex);
	m_addList.push_back(g);
}
void DebugGeometryManager::remove(DebugGeometry* g) {
	MutexLock lock(m_mutex);
	m_deleteList.push_back(g);
}
void DebugGeometryManager::update(bool paused) {
	std::map<DebugGeometry*, Drawable*>::iterator it;

	// Add items
	{
	MutexLock lock(m_mutex);
	for(DebugGeometry* item: m_addList) {
		bool valid = true;
		for(DebugGeometry* removed: m_deleteList) if(item == removed) { valid=false; break; }
		if(valid) {
			m_drawables[item] = item->m_drawable;
			for(SceneNode* node: m_nodes) node->attach(item->m_drawable);
			item->m_drawable->setMaterial(m_material);
			item->m_drawable->setRenderQueue(m_renderQueue);
		}
		else printf("Error: DebugLines probably not static.\n");
	}


	// Remove items
	for(DebugGeometry* item: m_deleteList) {
		it = m_drawables.find(item);
		if(it!=m_drawables.end()) {
			for(SceneNode* node: m_nodes) node->detach(it->second);
			delete it->second;
			m_drawables.erase(it);
		}
	}

	m_addList.clear();
	m_deleteList.clear();
	}

	// auto-flush
	for(it=m_drawables.begin(); it!=m_drawables.end(); ++it) {
		switch(it->first->m_mode) {
		case SDG_APPEND:
		case SDG_FRAME: it->first->flush();	break;
		case SDG_UNPAUSED: if(!paused) it->first->flush(); break;
		case SDG_FRAME_IF_DATA: if(!it->first->m_buffer->empty()) it->first->flush();	break;
		default: break;
		}
	}
}

void DebugGeometryManager::setRenderQueue(int queue) {
	m_renderQueue = queue;
	for(auto& i: m_drawables) {
		i.second->setRenderQueue(queue);
	}
}

void DebugGeometryManager::setMaterial(Material* m) {
	m_material = m;
	for(auto& i: m_drawables) {
		i.second->setMaterial(m);
	}
}

// ---------------------------------------------- //


class DebugGeometryDrawable : public Drawable {
	public:
	DebugGeometryDrawable(float w=1, float xray=0.2) : m_size(0), m_lineWidth(w), m_xray(xray) {}
	virtual void draw( RenderState& i ) {
		if(m_size) {
			i.setMaterial(m_material);
			bind();
			if(m_lineWidth!=1) glLineWidth(m_lineWidth);
			glDrawArrays(GL_LINES, 0, m_size);
			
			if(m_xray > 0) {
				glEnable(GL_BLEND);
				glDepthFunc(GL_GREATER);
				glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);
				glBlendColor(0,0,0,m_xray);
				glDrawArrays(GL_LINES, 0, m_size);
				m_material->getPass(0)->blend.bind();
				glDepthFunc(GL_LEQUAL);
			}

			if(m_lineWidth!=1) glLineWidth(1);
			GL_CHECK_ERROR;
		}
	}
	void setData( DebugGeometryVertex* vx, size_t size, const BoundingBox& box) {
		m_bounds = box;
		if(size==0 && m_size==0) return;
		m_vbo.copyData(vx, size, sizeof(DebugGeometryVertex));
		if(!m_binding) {
			m_vbo.createBuffer();
			m_vbo.attributes.add(VA_VERTEX, VA_FLOAT3);
			m_vbo.attributes.add(VA_COLOUR, VA_ARGB);
			addBuffer(&m_vbo);
		}
		m_size = size;
	}
	protected:
	size_t m_size;
	float  m_lineWidth;
	float  m_xray;
	HardwareVertexBuffer m_vbo;
};


// ---------------------------------------------- //

DebugGeometry::DebugGeometry(DebugGeometryManager* mgr, DebugGeometryFlush f, bool xray, float width) : m_manager(mgr), m_mode(f) {
	m_drawable = new DebugGeometryDrawable(width, xray?0.2:0);
	m_buffer = new GeometryList;
	m_manager->add(this);
}

DebugGeometry::DebugGeometry(DebugGeometryFlush f, bool xray, float width): m_mode(f) {
	m_drawable = new DebugGeometryDrawable(width, xray?0.2:0);
	m_buffer = new GeometryList;
	// Manager
	m_manager = DebugGeometryManager::getInstance();
	if(m_manager) m_manager->add(this);
	else printf("Debug Error: DebugGeometryManager does not exist\n");
}
DebugGeometry::~DebugGeometry() {
	if(m_manager) DebugGeometryManager::getInstance()->remove(this);
}
void DebugGeometry::flush() {
	// ToDo: Make thread safe
	DebugGeometryDrawable* d = static_cast<DebugGeometryDrawable*>(m_drawable);
	if(m_buffer->empty()) d->setData(0, 0, m_bounds);
	else d->setData(&m_buffer->at(0), m_buffer->size(), m_bounds);

	if(m_mode != SDG_APPEND) {
		m_bounds.setInvalid();
		m_buffer->clear();
	}
}
void DebugGeometry::reset() {
	m_bounds.setInvalid();
	m_buffer->clear();
	flush();
}


// ---------------------------------------------- //

inline void setColour(DebugGeometryVertex& v, int c) {
	v.r = (c>>16) & 0xff;
	v.g = (c>>8) & 0xff;
	v.b = c & 0xff;
	v.a = 0xff;
}

void DebugGeometry::line(const vec3& a, const vec3& b, int c) {
	DebugGeometryVertex v;
	setColour(v, c);
	v.pos = a;
	m_buffer->push_back(v);
	v.pos = b;
	m_buffer->push_back(v);
}

void DebugGeometry::vector(const vec3& p, const vec3& dir, int c) {
	line(p, p+dir, c);
}

void DebugGeometry::arrow(const vec3& a, const vec3& b, int c) {
	DebugGeometryVertex v;
	setColour(v, c);
	v.pos = a;
	m_buffer->push_back(v);
	v.pos = b;
	m_buffer->push_back(v);
	// Arrowhead
	vec3 x = (b-a).cross(vec3(1,1,4));
	vec3 y = (b-a).cross(x);
	vec3 s = lerp(a, b, 0.8);
	static const float map[] = { -1,1,1 };
	for(int i=0; i<4; ++i) {
		v.pos = b;
		m_buffer->push_back(v);
		v.pos = s + x*map[i&1] + y*map[i&2];
		m_buffer->push_back(v);
	}
}

void DebugGeometry::marker(const vec3& p, float s, int c) {
	const vec3 x(s,0,0);
	const vec3 y(0,s,0);
	const vec3 z(0,0,s);
	line(p-x, p+x, c);
	line(p-y, p+y, c);
	line(p-z, p+z, c);
}

void DebugGeometry::marker(const vec3& p, const Quaternion& r, float s, int c) {
	const vec3 x = r.xAxis();
	const vec3 y = r.yAxis();
	const vec3 z = r.zAxis();
	line(p-x, p+x, c);
	line(p-y, p+y, c);
	line(p-z, p+z, c);
}

void DebugGeometry::axis(const Matrix& m, float s) {
	const vec3 c = &m[12];
	line(c, m*vec3(s,0,0), 0xff0000);
	line(c, m*vec3(0,s,0), 0x00ff00);
	line(c, m*vec3(0,0,s), 0x0000ff);
}


void DebugGeometry::circle(const vec3& p, const vec3& axis, float r, int seg, int c) {
	assert(seg<1024);
	if(seg >= 1024) seg = 32; // Probbaly mixed up parameters
	DebugGeometryVertex v;
	setColour(v, c);
	vec3 x = axis.cross( vec3(1,1,4) );
	vec3 y = axis.cross( x );
	x.normalise() *= r;
	y.normalise() *= r;
	float step = TWOPI / seg;
	v.pos = p + y;
	for(int i=1; i<=seg; ++i) {
		m_buffer->push_back(v);
		v.pos = p + x * sin(i*step)  + y * cos(i*step);
		m_buffer->push_back(v);
	}
}

void DebugGeometry::ellipse(const vec3& p, const Quaternion& orientation, const vec2& r, int seg, int c) {
	assert(seg<1024);
	if(seg >= 1024) seg = 32; // Probbaly mixed up parameters
	DebugGeometryVertex v;
	setColour(v, c);
	vec3 x = orientation.xAxis() * r.x;
	vec3 y = orientation.zAxis() * r.y;
	float step = TWOPI / seg;
	v.pos = p + y;
	for(int i=1; i<=seg; ++i) {
		m_buffer->push_back(v);
		v.pos = p + x * sin(i*step)  + y * cos(i*step);
		m_buffer->push_back(v);
	}
}

void DebugGeometry::arc(const vec3& centre, const vec3& dstart, const vec3& dend, int segments, int colour) {
	assert(segments<1024); // Probably wrong parameter
	DebugGeometryVertex v;
	setColour(v, colour);
	if(segments < 3) {
		v.pos = centre + dstart;
		m_buffer->push_back(v);
		v.pos = centre + dend;
		m_buffer->push_back(v);
	}
	else {
		float d0 = dstart.length();
		float d1 = dend.length();
		Quaternion arc = Quaternion::arc(dstart, dend);
		Quaternion step = slerp(Quaternion(), arc, 1.f/(segments-1));
		if(arc.w >= 1.0) return;
		v.pos = centre + dstart;
		vec3 dir = dstart / d0;
		for(int i=1; i<segments; ++i) {
			m_buffer->push_back(v);
			dir = step * dir;
			v.pos = centre + dir * flerp(d0, d1, (float)i / (float)segments);
			m_buffer->push_back(v);
		}
		m_buffer->push_back(v);
		v.pos = centre + dend;
		m_buffer->push_back(v);
	}
}

void DebugGeometry::capsule(const vec3& a, const vec3& b, float radius, int seg, int colour, float cap) {
	vec3 n = (b-a).normalise();
	circle(a, n, radius, seg, colour);
	circle(b, n, radius, seg, colour);
	vec3 basis(n.y, n.z, n.x);
	vec3 x = n.cross(basis).normalise();
	vec3 y = n.cross(x);
	x *= radius;
	y *= radius;
	line(a+x, b+x, colour);
	line(a-x, b-x, colour);
	line(a+y, b+y, colour);
	line(a-y, b-y, colour);
	// Caps
	if(cap > 0) {
		seg /= 4;
		n *= radius;
		arc(a,  x, n*-cap, seg, colour);
		arc(a, -x, n*-cap, seg, colour);
		arc(a,  y, n*-cap, seg, colour);
		arc(a, -y, n*-cap, seg, colour);
		arc(b,  x, n*cap, seg, colour);
		arc(b, -x, n*cap, seg, colour);
		arc(b,  y, n*cap, seg, colour);
		arc(b, -y, n*cap, seg, colour);
	}
}


void DebugGeometry::capsule2D(const vec3& a, const vec3& b, const vec3& basis, float radius, int seg, int colour) {
	vec3 n = (b-a).normalise();
	vec3 x = n.cross(basis).normalise() * radius;
	line(a+x, b+x, colour);
	line(a-x, b-x, colour);
	// Caps
	seg /= 4;
	n *= radius;
	constexpr float cap = 1;
	arc(a,  x, n*-cap, seg, colour);
	arc(a, -x, n*-cap, seg, colour);
	arc(b,  x, n*cap, seg, colour);
	arc(b, -x, n*cap, seg, colour);
}

// ---------------------------------------------- //

inline void setAdd(vec3& v, const vec3& a, float x, float y, float z) {
	v.x = a.x + x; v.y = a.y + y; v.z = a.z + z;
}
inline void getBoxPoints(const vec3& c, const vec3& e, vec3* out) {
	setAdd(out[0], c, -e.x, -e.y, -e.z);
	setAdd(out[1], c,  e.x, -e.y, -e.z);
	setAdd(out[2], c, -e.x,  e.y, -e.z);
	setAdd(out[3], c,  e.x,  e.y, -e.z);
	setAdd(out[4], c, -e.x, -e.y,  e.z);
	setAdd(out[5], c,  e.x, -e.y,  e.z);
	setAdd(out[6], c, -e.x,  e.y,  e.z);
	setAdd(out[7], c,  e.x,  e.y,  e.z);
}
inline void addBoxLines(const vec3* points, int colour, std::vector<DebugGeometryVertex>* buffer) {
	static const int map[24] = { 0,1, 1,3, 3,2, 2,0, 4,5, 5,7, 7,6, 6,4, 0,4, 1,5, 2,6, 3,7 };
	DebugGeometryVertex v;
	setColour(v, colour);
	for(int i=0; i<24; ++i) {
		v.pos = points[ map[i] ];
		buffer->push_back(v);
	}
}

void DebugGeometry::box(const BoundingBox& b, int c) {
	vec3 points[8];
	getBoxPoints( b.centre(), b.size()*0.5, points);
	addBoxLines(points, c, m_buffer);
}
void DebugGeometry::box(const BoundingBox& b, const Matrix& t, int c) {
	vec3 points[8];
	getBoxPoints( b.centre(), b.size()*0.5, points);
	for(int i=0; i<8; ++i) points[i] = t * points[i];
	addBoxLines(points, c, m_buffer);
}
void DebugGeometry::box(const BoundingBox& b, const vec3& o, const Quaternion& q, int c) {
	vec3 points[8];
	getBoxPoints( b.centre(), b.size()*0.5, points);
	for(int i=0; i<8; ++i) points[i] = q * points[i] + o;
	addBoxLines(points, c, m_buffer);
}
void DebugGeometry::box(const vec3& ext, const Matrix& t, int c) {
	vec3 points[8];
	getBoxPoints( points[7], ext, points);
	for(int i=0; i<8; ++i) points[i] = t * points[i];
	addBoxLines(points, c, m_buffer);
}
void DebugGeometry::box(const vec3& ext, const vec3& o, const Quaternion& q, int c) {
	vec3 points[8];
	getBoxPoints( points[7], ext, points);
	for(int i=0; i<8; ++i) points[i] = q * points[i] + o;
	addBoxLines(points, c, m_buffer);
}
void DebugGeometry::box(const vec3& ext, const vec3& o, int c) {
	vec3 points[8];
	getBoxPoints( points[7], ext, points);
	for(int i=0; i<8; ++i) points[i] += o;
	addBoxLines(points, c, m_buffer);
}


// ---------------------------------------------- //

void DebugGeometry::strip(int c, const vec3* p, int colour) {
	DebugGeometryVertex v;
	setColour(v, colour);
	for(int i=1; i<c; ++i) {
		v.pos = p[i-1];
		m_buffer->push_back(v);
		v.pos = p[i];
		m_buffer->push_back(v);
	}
}
void DebugGeometry::lines(int c, const vec3* p, int colour) {
	DebugGeometryVertex v;
	setColour(v, colour);
	for(int i=0; i<c; ++i) {
		v.pos = p[i];
		m_buffer->push_back(v);
	}
}


