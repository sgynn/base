#include "base/model.h"
#include "base/opengl.h"

// Macro for resizing a c array
#define ResizeArray(type, array, oldSize, newSize)	{ type* tmp=array; array=new type[newSize]; if(oldSize) { memcpy(array, tmp, oldSize*sizeof(type)); delete [] tmp; } }

using namespace base;
using namespace model;

//Mesh format offsets : { Normal, Texture, Tangent, Size } tupes
size_t Mesh::s_offset[4][4] = {
	{0,0,0, 3}, // VERTEX_SIMPLE
	{0,3,0, 5}, // VETREX_TEXTURE
	{3,6,0, 8}, // VERTEX_DEFAULT
	{3,6,8, 11},// VERTEX_TANGENT
};

Mesh::Mesh() : m_format(VERTEX_DEFAULT), m_formatSize(8), m_drawMode(GL_TRIANGLES), m_vertexBuffer(0), m_indexBuffer(0), m_skinBuffer(0), m_referenceCount(0) {}
Mesh::Mesh(const Mesh& m, bool copy, bool skin) : m_format(m.m_format), m_formatSize(m.m_formatSize), m_drawMode(GL_TRIANGLES), m_referenceCount(0) {
	//vertex format data
	m_format = m.m_format;
	m_formatSize = m.m_formatSize;
	//copy or reference vertex array
	if(m.m_vertexBuffer) {
		if(copy) {
			m_vertexBuffer = new Buffer<float>();
			m_vertexBuffer->data = new float[ m.m_vertexBuffer->size * m.m_formatSize ];
			memcpy(m_vertexBuffer->data, m.m_vertexBuffer->data, m.m_vertexBuffer->size * m.m_vertexBuffer->stride);
			m_vertexBuffer->size = m.m_vertexBuffer->size;
			m_vertexBuffer->stride = m.m_vertexBuffer->stride;
			m_vertexBuffer->referenceCount = 1;
		} else {
			m_vertexBuffer = m.m_vertexBuffer;
			m_vertexBuffer->referenceCount++;
		}
	} else m_vertexBuffer = 0;
	
	//Indices if they exist
	if(m.m_indexBuffer) {
		m_indexBuffer = m.m_indexBuffer;
		m_indexBuffer->referenceCount++;
	} else m_indexBuffer = 0;
	
	//and skins? If we move vertices, the skin becomes invalid.
	if(m.m_skinBuffer && skin) {
		m_skinBuffer = m.m_skinBuffer;
		m_skinBuffer->referenceCount++;
	} else  m_skinBuffer = 0;
	
	//copy material
	m_material = m.m_material;
	memcpy(m_box, m.m_box, 6*sizeof(float)); //Bounding box
}
Mesh::~Mesh() {
	//delete buffers of not referenced elsewhere
	//also need to clear any VBOs if used
	if(m_vertexBuffer) {
		if(--m_vertexBuffer->referenceCount<=0) {
			delete [] m_vertexBuffer->data;
			delete m_vertexBuffer;
			deleteBufferObject();
		}
	}
	
	if(m_indexBuffer) {
		if(--m_indexBuffer->referenceCount<=0) {
			delete [] m_indexBuffer->data;
			delete m_indexBuffer;
		}
	}
	//delete skins
	if(m_skinBuffer) {
		if(--m_skinBuffer->referenceCount<=0) {
			for(unsigned int i=0; i<m_skinBuffer->size; i++) {
				delete [] m_skinBuffer->data[i]->indices;
				delete [] m_skinBuffer->data[i]->weights;
				if(m_skinBuffer->data[i]->jointName) delete [] m_skinBuffer->data[i]->jointName;
				delete m_skinBuffer->data[i];
			}
			delete [] m_skinBuffer->data;
			delete m_skinBuffer;
		}
	}
}
int Mesh::grab() { return ++m_referenceCount; }
int Mesh::drop() {
	int r = --m_referenceCount;
	if(r<=0) delete this;
	return r;
}

bool Mesh::createBufferObject() {
	#ifndef NO_MODEL_DRAW
	if(m_vertexBuffer) glGenBuffers(1, &m_vertexBuffer->bufferObject);
	if(m_indexBuffer) glGenBuffers(1, &m_indexBuffer->bufferObject);
	updateBufferObject();
	return m_vertexBuffer->bufferObject;
	#endif
}
void Mesh::useBufferObject(bool use) {}
void Mesh::updateBufferObject() {
	#ifndef NO_MODEL_DRAW
	bindBuffer();
	if(m_vertexBuffer && m_vertexBuffer->bufferObject) glBufferData(GL_ARRAY_BUFFER, m_vertexBuffer->size * m_vertexBuffer->stride, m_vertexBuffer->data, GL_STATIC_DRAW);
	if(m_indexBuffer && m_indexBuffer->bufferObject)  glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer->size * m_indexBuffer->stride, m_indexBuffer->data, GL_STATIC_DRAW);
	#endif
}
int Mesh::bindBuffer() const { 
	#ifndef NO_MODEL_DRAW
	glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer? m_vertexBuffer->bufferObject: 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer? m_indexBuffer->bufferObject: 0);
	return 0;
	#endif
}
void Mesh::deleteBufferObject() {
	#ifndef NO_MODEL_DRAW
	if(m_vertexBuffer && m_vertexBuffer->bufferObject) glDeleteBuffers(1, &m_vertexBuffer->bufferObject);
	if(m_indexBuffer && m_indexBuffer->bufferObject)  glDeleteBuffers(1, &m_indexBuffer->bufferObject);
	#endif
}

void Mesh::changeFormat(VertexFormat format) {
	if(format==m_format) return;
	//resize vertex array, copy data
	int newFormatSize = s_offset[format][3];
	float* tmp = m_vertexBuffer->data;

	m_vertexBuffer->data = new float[m_vertexBuffer->size * newFormatSize];
	if(newFormatSize>m_formatSize) memset(m_vertexBuffer->data, 0, m_vertexBuffer->size*newFormatSize*sizeof(float));
	//int copySize = (newFormatSize<m_formatSize? newFormatSize: m_formatSize) * sizeof(float);
	size_t* sf = s_offset[m_format];
	size_t* df = s_offset[format];
	for(unsigned int i=0; i<m_vertexBuffer->size; i++) {
		float* src = tmp + i*m_formatSize;
		float* dst = m_vertexBuffer->data + i*newFormatSize;
		//Have to do this per section
		memcpy(dst, src, 3*sizeof(float)); //Position
		if(sf[0]&&df[0]) memcpy(dst+df[0], src+sf[0], 3*sizeof(float)); //Normal
		if(sf[1]&&df[1]) memcpy(dst+df[1], src+sf[1], 2*sizeof(float)); //Texture
		if(sf[2]&&df[2]) memcpy(dst+df[2], src+sf[2], 3*sizeof(float)); //Tangent
	}
	
	m_formatSize = newFormatSize;
	m_vertexBuffer->stride = m_formatSize * sizeof(float);
	m_format = format;
	delete [] tmp;
	//Update VBO if it exists
	if(m_vertexBuffer->bufferObject) updateBufferObject();
}

/** Data Set Functions */
void Mesh::setVertices(int count, float* data, VertexFormat format) {
	m_format = format;
	m_formatSize = s_offset[format][3];
	m_vertexBuffer = new Buffer<float>();
	m_vertexBuffer->data = data;
	m_vertexBuffer->size = count*m_formatSize;
	m_vertexBuffer->stride = m_formatSize*sizeof(float);
	m_vertexBuffer->referenceCount = 1;
	updateBox();
}
void Mesh::setVertices(Buffer<float>* buffer, VertexFormat format) {
	m_format = format;
	m_formatSize = s_offset[format][3];
	m_vertexBuffer = buffer;
	buffer->referenceCount++;
	updateBox();
}
void Mesh::setIndices(int count, const unsigned short* data) {
	m_indexBuffer = new Buffer<const unsigned short>();
	m_indexBuffer->data = data;
	m_indexBuffer->size = count;
	m_indexBuffer->referenceCount = 1;
}
void Mesh::setIndices(Buffer<const unsigned short>* buffer) {
	m_indexBuffer = buffer;
	buffer->referenceCount++;
}

void Mesh::addSkin(Skin* skin) {
	if(!m_skinBuffer) {
		m_skinBuffer = new Buffer<Skin*>();
		m_skinBuffer->referenceCount = 1; //grab pointer
	}
	//resize skin array
	Skin** tmp = m_skinBuffer->data;
	m_skinBuffer->data = new Skin*[m_skinBuffer->size+1];
	if(m_skinBuffer->size) { memcpy(m_skinBuffer->data, tmp, m_skinBuffer->size*sizeof(Skin*) ); delete [] tmp; }
	//add new skin
	m_skinBuffer->data[ m_skinBuffer->size ] = skin;
	m_skinBuffer->size++;
}

/** Data Processing */
int Mesh::calculateTangents() {
	changeFormat( VERTEX_TANGENT );
	size_t s = m_formatSize;
	size_t offset = s_offset[m_format][2]; //should be 8
	if(m_indexBuffer) {
		for(uint i=0; i<m_indexBuffer->size; i+=3) {
			float* a = m_vertexBuffer->data + m_indexBuffer->data[i]*s;
			float* b = m_vertexBuffer->data + m_indexBuffer->data[i+1]*s;
			float* c = m_vertexBuffer->data + m_indexBuffer->data[i+2]*s;
			float* t = a + offset;

			tangent(a,b,c, t);
			memcpy(b+offset, t, 3*sizeof(float));
			memcpy(c+offset, t, 3*sizeof(float));
		}
	} else if(m_vertexBuffer) {
		for(uint i=0; i<m_vertexBuffer->size; i+=3) {
			float* a = m_vertexBuffer->data + i*s;
			float* b = m_vertexBuffer->data + (i+1)*s;
			float* c = m_vertexBuffer->data + (i+2)*s;
			float* t = a + offset;

			tangent(a,b,c, t);
			memcpy(b+offset, t, 3*sizeof(float));
			memcpy(c+offset, t, 3*sizeof(float));
		}
	} else return 0;
	return 1;
}
int Mesh::normaliseWeights()  {  return 0; }

int Mesh::tangent(const float* a, const float* b, const float* c, float* t) {
	float ab[3] = { b[0]-a[0], b[1]-a[1], b[2]-a[2] };
	float ac[3] = { c[0]-a[0], c[1]-a[1], c[2]-a[2] };
	const float* n = a+3; //Normal

	//Project ab,ac onto normal
	float abn = ab[0]*n[0] + ab[1]*n[1] + ab[2]*n[2];
	float acn = ac[0]*n[0] + ac[1]*n[1] + ac[2]*n[2];
	ab[0]-=n[0]*abn; ab[1]-=n[1]*abn; ab[2]-=n[2]*abn;
	ac[0]-=n[0]*acn; ac[1]-=n[1]*acn; ac[2]-=n[2]*acn;

	//Texture coordinate deltas
	float abu = b[6] - a[6];
	float abv = b[7] - a[7];
	float acu = c[6] - a[6];
	float acv = c[7] - a[7];
	if(acv*abu > abv*acu) {
		acv = -acv;
		abv = -abv;
	}

	//tangent
	t[0] = ac[0]*abv - ab[0]*acv;
	t[1] = ac[1]*abv - ab[1]*acv;
	t[2] = ac[2]*abv - ab[2]*acv;

	//Normalise tangent
	float l = sqrt(t[0]*t[0] + t[1]*t[1] + t[2]*t[2]);
	if(l!=0) {
		t[0]/=l; t[1]/=l; t[2]/=l;
		return 1;
	} else return 0;
}

void Mesh::updateBox() {
	const float* v = getVertex(0);
	m_box[0] = m_box[3] = v[0];
	m_box[1] = m_box[4] = v[1];
	m_box[2] = m_box[5] = v[2];
	for(uint i=1; i<getVertexCount(); i++) {
		v = getVertex(i);
		if(v[0] < m_box[0])	 m_box[0]=v[0];
		else if(v[0] > m_box[3]) m_box[3]=v[0];
		if(v[1] < m_box[1])	 m_box[1]=v[1];
		else if(v[1] > m_box[4]) m_box[4]=v[1];
		if(v[2] < m_box[2])	 m_box[2]=v[2];
		else if(v[2] > m_box[5]) m_box[5]=v[2];
	}
}

