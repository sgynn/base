#include <base/mesh.h>
#include <base/hardwarebuffer.h>

using namespace base;

Mesh::Mesh() : m_ref(0), m_polygonMode(PolygonMode::TRIANGLES), m_vertexBuffer(0), m_skinBuffer(0), m_indexBuffer(0), m_skinCount(0), m_weightsPerVertex(0), m_skinNames(0), m_morphCount(0), m_morphs(0) {
}

Mesh::Mesh(const Mesh& m) : m_ref(0), m_polygonMode(m.m_polygonMode) {
	if(m.m_vertexBuffer) {
		m_vertexBuffer = m.m_vertexBuffer;
		m_vertexBuffer->addReference();
	}
	if(m.m_skinBuffer) {
		m_skinBuffer = m.m_skinBuffer;
		m_skinBuffer->addReference();
	}
	if(m.m_indexBuffer) {
		m_indexBuffer = m.m_indexBuffer;
		m_indexBuffer->addReference();
	}
}

Mesh::Mesh(Mesh&& m) : m_ref(0), m_polygonMode(m.m_polygonMode) {
	m_vertexBuffer = m.m_vertexBuffer;
	m_skinBuffer = m.m_skinBuffer;
	m_indexBuffer = m.m_indexBuffer;
	m.m_vertexBuffer = 0;
	m.m_skinBuffer = 0;
	m.m_indexBuffer = 0;
}

Mesh::~Mesh() {
	if(m_vertexBuffer) m_vertexBuffer->dropReference();
	if(m_skinBuffer)   m_skinBuffer->dropReference();
	if(m_indexBuffer)  m_indexBuffer->dropReference();
	for(int i=0; i<m_skinCount; ++i) if(m_skinNames[i]) free(m_skinNames[i]);
	if(m_skinNames) delete [] m_skinNames;
	delete [] m_morphs;
}

// ----------------------------------------------------------------------------------- //

Mesh* Mesh::addReference() {
	++m_ref;
	return this;
}

const Mesh* Mesh::addReference() const {
	++m_ref;
	return this;
}

int Mesh::dropReference() {
	if(--m_ref == 0) {
		delete this;
	}
	return m_ref;
}


// ----------------------------------------------------------------------------------- //

void Mesh::setPolygonMode(PolygonMode m) { m_polygonMode = m; }
PolygonMode Mesh::getPolygonMode() const { return m_polygonMode; }

// ----------------------------------------------------------------------------------- //

void Mesh::setVertexBuffer(HardwareVertexBuffer* b) {
	if(m_vertexBuffer) m_vertexBuffer->dropReference();
	b->addReference();
	m_vertexBuffer = b;
	// Cache offsets
	const Attribute& v = b->attributes.get(VA_VERTEX);
	const Attribute& n = b->attributes.get(VA_NORMAL);
	const Attribute& t = b->attributes.get(VA_TANGENT);
	m_vertexOffset = v? v.offset / sizeof(VertexType): ~0u;
	m_normalOffset = n? n.offset / sizeof(VertexType): ~0u;
	m_tangentOffset = t? t.offset / sizeof(VertexType): ~0u;

}
void Mesh::setSkinBuffer(HardwareVertexBuffer* b) {
	if(m_skinBuffer) m_skinBuffer->dropReference();
	if(b) b->addReference();
	m_skinBuffer = b;
}
void Mesh::setIndexBuffer(HardwareIndexBuffer* b) {
	if(m_indexBuffer) m_indexBuffer->dropReference();
	if(b) b->addReference();
	m_indexBuffer = b;
}

void Mesh::setMorphs(int count, Morph* data) {
	delete [] m_morphs;
	m_morphCount = count;
	m_morphs = new Morph[count];
	for(int i=0; i<count; ++i) m_morphs[i] = std::move(data[i]);
}

// ----------------------------------------------------------------------------------- //

void Mesh::initialiseSkinData(int count, int wpv) {
	m_skinNames = new char*[count];
	memset(m_skinNames, 0, count*sizeof(char*));
	m_skinCount = count;
	m_weightsPerVertex = wpv;
}

void Mesh::setSkinName(int index, const char* name) {
	if(m_skinNames[index]) free(m_skinNames[index]);
	if(name) m_skinNames[index] = strdup(name);
}

const char* Mesh::getSkinName(int i) const {
	if(i<0 || i>=m_skinCount) return 0;
	return m_skinNames[i];
}

size_t Mesh::getSkinCount() const {
	return m_skinCount;
}
size_t Mesh::getWeightsPerVertex() const {
	return m_weightsPerVertex;
}

// ----------------------------------------------------------------------------------- //


base::HardwareIndexBuffer* Mesh::getIndexBuffer()		{ return m_indexBuffer; }
base::HardwareVertexBuffer* Mesh::getVertexBuffer()	{ return m_vertexBuffer; }
base::HardwareVertexBuffer* Mesh::getSkinBuffer()		{ return m_skinBuffer; }

// ----------------------------------------------------------------------------------- //


size_t Mesh::getVertexCount() const {
	if(!m_vertexBuffer) return 0;
	return m_vertexBuffer->getVertexCount();
}

size_t Mesh::getIndexCount() const {
	if(!m_indexBuffer) return 0;
	return m_indexBuffer->getIndexCount();
}


// ----------------------------------------------------------------------------------- //


const vec3& Mesh::getVertex(uint index) const {
	const VertexType* p = m_vertexBuffer->getVertex(index);
	return *reinterpret_cast<const vec3*>(p + m_vertexOffset);
}
const vec3& Mesh::getNormal(uint index) const {
	const VertexType* p = m_vertexBuffer->getVertex(index);
	return *reinterpret_cast<const vec3*>(p + m_normalOffset);
}

const vec3& Mesh::getIndexedVertex(uint index) const {
	uint i = m_indexBuffer->getIndex(index);
	return getVertex(i);
}
const vec3& Mesh::getIndexedNormal(uint index) const {
	uint i = m_indexBuffer->getIndex(index);
	return getNormal(i);
}


// ----------------------------------------------------------------------------------- //

const BoundingBox& Mesh::calculateBounds() {
	m_bounds.setInvalid();
	int count = getVertexCount();
	for(int i=0; i<count; ++i) {
		m_bounds.include( getVertex(i) );
	}
	return m_bounds;
}
const BoundingBox& Mesh::getBounds() const {
	return m_bounds;
}


// ----------------------------------------------------------------------------------- //


int Mesh::calculateNormals() {
	// ToDo:
	//   if no normals, expand buffer
	//   calculate flat shaded normals from face data
	//   smooth normals?
	return 0;
}

int Mesh::calculateTangents() {
	// ToDo:
	//   if no tangents, expand buffer
	//   calculate tangents for each face
	return 0;
}

