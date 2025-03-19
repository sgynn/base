#include <base/navdrawable.h>
#include <base/navmesh.h>
#include <base/renderer.h>
#include <base/shader.h>
#include <base/hardwarebuffer.h>
#include <base/debuggeometry.h>
#include <base/opengl.h>
#include <base/assert.h>

using namespace base;


NavDrawable::SubDrawable::SubDrawable(int mode) : mode(mode) {
	buffer = new HardwareVertexBuffer();
	buffer->attributes.add(VA_VERTEX, VA_FLOAT3);
	buffer->attributes.add(VA_COLOUR, VA_ARGB);
	buffer->createBuffer();
	addBuffer(buffer);
}
NavDrawable::SubDrawable::~SubDrawable() {
	delete buffer;
}
void NavDrawable::SubDrawable::draw(RenderState& r) {
	if(buffer->getVertexCount()) {
		bind();
		glDrawArrays(mode, 0, buffer->getVertexCount());
	}
}


// ================================= //



NavDrawable::NavDrawable(const NavMesh* m, float offset, float alpha, Material* mat): m_nav(m), m_offset(offset), m_lc(0) {
	if(!mat) mat = DebugGeometryManager::getDefaultMaterial();
	setMaterial(mat);
	m_lines = new SubDrawable(GL_LINES);
	m_polys = new SubDrawable(GL_TRIANGLES);
	m_alpha = (int)(alpha*255.5) << 24;
}
NavDrawable::~NavDrawable() {
	delete m_lines;
	delete m_polys;
}

void NavDrawable::build() {

	size_t psize=0, lsize=0;
	for(const NavPoly* p: m_nav->getMeshData()) {
		if(p->size>=3) psize += (p->size - 2) * 3;
		lsize += p->size * 2;
	}

	struct Vertex { vec3 v; uint8 r,g,b,a; };
	auto setVertex = [this](Vertex& v, const vec3& pos, uint col) {
		v.v = pos;
		v.v.y += m_offset;
		v.r = col >> 16;
		v.g = col >> 8;
		v.b = col;
		v.a = col >> 24;
	};


	// Polygons
	size_t k = 0;
	if(m_alpha) {
		Vertex v;
		Vertex* polys = psize? new Vertex[psize]: 0;
		const uint col[] = { 0xff0000, 0x00ff00, 0x0000ff, 0xffff00, 0x00ffff, 0xff00ff, 0xff8000 };
		for(const NavPoly* p: m_nav->getMeshData()) {
			uint colour = col[p->typeIndex%7] | m_alpha;
			for(int i=2; i<p->size; ++i) {
				setVertex(polys[k], p->points[i-1], colour);
				setVertex(polys[k+1], p->points[0], colour);
				setVertex(polys[k+2], p->points[i], colour);
				k += 3;
			}
		}
		assert(k == psize);
		m_polys->buffer->setData(polys, k, sizeof(Vertex), true);
	}

	// Lines
	k = 0;
	Vertex* edges = lsize? new Vertex[lsize]: 0;
	for(const NavPoly* p: m_nav->getMeshData()) {
		for(const NavMesh::EdgeInfo& edge: p) {
			uint colour = edge.isConnected()? 0xff808080: 0xffff0000;
			setVertex(edges[k], p->points[edge.a], colour);
			setVertex(edges[k+1], p->points[edge.b], colour);
			k += 2;
		}
	}
	assert(k == lsize);
	m_lines->buffer->setData(edges, k, sizeof(Vertex), true);

	// Bounding box
	m_bounds.setInvalid();
	for(const NavPoly* p: m_nav->getMeshData()) {
		for(const vec3& v: p) m_bounds.include(v);
	}
}

void NavDrawable::draw(RenderState& info) {
	if(m_nav->getMeshData().empty()) return;

	if(m_nav->getMeshData().back()->id != m_lc) {
		build();
		m_lc = m_nav->getMeshData().back()->id;
	}

	info.setMaterial(m_material);
	m_polys->draw(info);
	m_lines->draw(info);
}


