#include <base/hardwarebuffer.h>
#include <base/opengl.h>
#include <cstdio>

// Extensions
#ifdef WIN32
#define GL_ARRAY_BUFFER          0x8892
#define GL_ELEMENT_ARRAY_BUFFER  0x8893
#define GL_STATIC_DRAW           0x88E4

typedef ptrdiff_t GLsizeiptr;
typedef void (*PFNGLBINDBUFFERPROC) (GLenum target, GLuint buffer);
typedef void (*PFNGLDELETEBUFFERSPROC) (GLsizei n, const GLuint *buffers);
typedef void (*PFNGLGENBUFFERSPROC) (GLsizei n, GLuint *buffers);
typedef void (*PFNGLBUFFERDATAPROC) (GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage);

PFNGLBINDBUFFERPROC    glBindBuffer    = 0;
PFNGLDELETEBUFFERSPROC glDeleteBuffers = 0;
PFNGLGENBUFFERSPROC    glGenBuffers    = 0;
PFNGLBUFFERDATAPROC    glBufferData    = 0;
void setupExtensions() {
	if(glBindBuffer) return;
	glBindBuffer    = (PFNGLBINDBUFFERPROC)    wglGetProcAddress("glBindBuffer");
	glDeleteBuffers = (PFNGLDELETEBUFFERSPROC) wglGetProcAddress("glDeleteBuffers");
	glGenBuffers    = (PFNGLGENBUFFERSPROC)    wglGetProcAddress("glGenBuffers");
	glBufferData    = (PFNGLBUFFERDATAPROC)    wglGetProcAddress("glBufferData");
}
#else
void setupExtensions() {}
#endif

using namespace base;


// ------------------------------------------------------------------------------------- //




HardwareBuffer::HardwareBuffer(Usage u, bool keep) :
	m_ref(0), m_buffer(~0u), m_target(0), m_usage(u), m_size(0), m_data(0), m_keepCopy(keep), m_ownsData(false) {
		setupExtensions();
}
HardwareBuffer::~HardwareBuffer() {
}

HardwareBuffer* HardwareBuffer::addReference() {
	++m_ref;
	return this;
}
int HardwareBuffer::dropReference() {
	if(--m_ref == 0) {
		destroyBuffer();
		clearLocalData();
	}
	return m_ref;
}

// --------------------------------------------------------------- //

bool HardwareBuffer::isBuffer() const {
	return m_buffer != ~0u;
}

void HardwareBuffer::createBuffer() {
	if(!isBuffer()) {
		glGenBuffers(1, &m_buffer);
		if(m_data) writeBuffer();
	}
}
void HardwareBuffer::destroyBuffer() {
	if(isBuffer()) glDeleteBuffers(1, &m_buffer);
}

void HardwareBuffer::writeBuffer() {
	if(!isBuffer()) return;
	static const unsigned usage[] = {
		GL_STATIC_DRAW, GL_STATIC_READ, GL_STATIC_COPY, 
		GL_DYNAMIC_DRAW, GL_DYNAMIC_READ, GL_DYNAMIC_COPY,
		GL_STREAM_DRAW, GL_STREAM_READ, GL_STREAM_COPY
	};
	glBindBuffer(m_target, m_buffer);
	glBufferData(m_target, m_size, m_data, usage[m_usage]);
	glBindBuffer(m_target, 0);
	GL_CHECK_ERROR;
}

void HardwareBuffer::readBuffer() {
	// Not implemented
}


// ------------------------------------------------------------------------------------- //


const char* HardwareBuffer::bind() const {
	if(isBuffer()) {
		glBindBuffer(m_target, m_buffer);
		return 0;
	}
	else {
		glBindBuffer(m_target, 0);
		return (const char*)m_data;
	}
}


// -------------------------------------------------------------------------------------- //


HardwareVertexBuffer::HardwareVertexBuffer(Usage usage) : HardwareBuffer(usage, true) {
	m_target = GL_ARRAY_BUFFER;
}

unsigned HardwareVertexBuffer::getDataType(AttributeType t) {
	switch(t) {
	case VA_FLOAT1: case VA_FLOAT2: case VA_FLOAT3: case VA_FLOAT4: return GL_FLOAT;
	case VA_SHORT1: case VA_SHORT2: case VA_SHORT3: case VA_SHORT4: return GL_SHORT;
	case VA_INT1:   case VA_INT2:   case VA_INT3:   case VA_INT4:   return GL_INT;
	case VA_UBYTE1:   case VA_UBYTE2:   case VA_UBYTE3:   case VA_UBYTE4: return GL_UNSIGNED_BYTE;
	case VA_ARGB: case VA_RGB: return GL_UNSIGNED_BYTE;
	default: return 0;	// error
	}
}

int HardwareVertexBuffer::getDataCount(AttributeType t) {
	switch(t) {
	case VA_FLOAT1: case VA_SHORT1: case VA_INT1: case VA_UBYTE1: return 1;
	case VA_FLOAT2: case VA_SHORT2: case VA_INT2: case VA_UBYTE2: return 2;
	case VA_FLOAT3: case VA_SHORT3: case VA_INT3: case VA_UBYTE3: return 3;
	case VA_FLOAT4: case VA_SHORT4: case VA_INT4: case VA_UBYTE4: return 4;
	case VA_ARGB: return 4;
	case VA_RGB: return 3;
	default: return 0;
	}
}

int HardwareVertexBuffer::getDataSize(AttributeType t) {
	switch(t) {
	case VA_FLOAT1: case VA_INT1: return  4;
	case VA_FLOAT2: case VA_INT2: return  8;
	case VA_FLOAT3: case VA_INT3: return  12;
	case VA_FLOAT4: case VA_INT4: return  16;
	case VA_UBYTE4: case VA_ARGB: return 4;
	case VA_SHORT1: return 2;
	case VA_SHORT2: return 4;
	case VA_SHORT3: return 6;
	case VA_SHORT4: return 8;
	case VA_UBYTE1: return 1;
	case VA_UBYTE2: return 2;
	case VA_UBYTE3: case VA_RGB: return 3;
	default: return 0;
	}
}

// -------------------------------------------------------------------------------------- //

HardwareIndexBuffer::HardwareIndexBuffer(IndexSize t) : HardwareBuffer(STATIC_DRAW, true), m_type(t) {
	m_target = GL_ELEMENT_ARRAY_BUFFER;
}

unsigned HardwareIndexBuffer::getDataType() const {
	static const unsigned types[] = { GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT, GL_UNSIGNED_INT };
	return types[(int)m_type];
}



// --------------------------------------------------------------------------------------------------- //

unsigned VertexAttributes::size() const {
	return m_attributes.size();
}

unsigned VertexAttributes::getStride() const {
	return m_stride;
}

unsigned VertexAttributes::calculateStride() const {
	unsigned stride = 0;
	for(const Attribute& a: m_attributes) {
		unsigned end = a.offset + HardwareVertexBuffer::getDataSize(a.type);
		if(end > stride) stride = end;
	}
	return stride;
}

Attribute& VertexAttributes::add(AttributeSemantic sem, AttributeType type, const char* name, unsigned div) {
	return add(sem, type, calculateStride(), name, div);
}

Attribute& VertexAttributes::add(AttributeSemantic sem, AttributeType type, unsigned offset, const char* name, unsigned div) {
	Attribute a;
	a.semantic = sem;
	a.divisor = div;
	a.offset = offset;
	a.type = type;
	a.name = name;
	m_attributes.push_back(a);
	m_stride = calculateStride();
	return m_attributes.back();
}

int VertexAttributes::remove(AttributeSemantic s, bool compact) {
	int removed = 0;
	int shift = 0;
	for(size_t i=0; i<m_attributes.size();) {
		if(m_attributes[i].semantic == s) {
			if(compact) shift += HardwareVertexBuffer::getDataSize(m_attributes[i].type);
			m_attributes.erase(m_attributes.begin() + i);
			++removed;
		}
		else {
			m_attributes[i].offset -= shift;
			++i;
		}
	}
	m_stride = calculateStride();
	return removed;
}

unsigned VertexAttributes::compact() {
	m_stride = 0;
	for(Attribute& a: m_attributes) {
		a.offset = m_stride;
		m_stride += HardwareVertexBuffer::getDataSize(a.type);
	}
	return m_stride;
}

Attribute& VertexAttributes::get(unsigned index) {
	if(index < m_attributes.size()) return m_attributes[index];
	static Attribute invalid { 0, VA_VERTEX, VA_INVALID, 0, 0 };
	return invalid;
}

const Attribute& VertexAttributes::get(unsigned index) const {
	if(index < m_attributes.size()) return m_attributes[index];
	static Attribute invalid { 0, VA_VERTEX, VA_INVALID, 0, 0 };
	return invalid;
}

Attribute& VertexAttributes::get(AttributeSemantic semantic, int index) {
	for(Attribute& a: m_attributes) {
		if(a.semantic == semantic && --index<0) return a;
	}
	return get(~0u);
}
int VertexAttributes::hasAttrribute(AttributeSemantic s) const {
	int count = 0;
	for(const Attribute& a: m_attributes) if(a.semantic==s) ++count;
	return count;
}

VertexAttributes& VertexAttributes::operator=(const VertexAttributes& o) {
	m_attributes = o.m_attributes;
	m_stride = o.m_stride;
	return *this;
}

bool VertexAttributes::operator==(const VertexAttributes& o) const {
	if(size() != o.size()) return false;
	for(size_t i=0; i<m_attributes.size(); ++i) {
		if(m_attributes[i].offset   != o.m_attributes[i].offset) return false;
		if(m_attributes[i].semantic != o.m_attributes[i].semantic) return false;
		if(m_attributes[i].divisor  != o.m_attributes[i].divisor) return false;
		if(m_attributes[i].type     != o.m_attributes[i].type) return false;
		// Name ?
	}
	return true;
}
bool VertexAttributes::operator!=(const VertexAttributes& o) const { return !(*this==o); }


