#include "base/mesh.h"
#include "base/skeleton.h"
#include <cstring>
#include <cstdlib>

#ifndef GL_TRIANGLES
#define GL_TRIANGLES 0x0004
#endif

#define ResizeArray(type, array, oldSize, newSize)	{ type* tmp=array; array=new type[newSize]; if(oldSize) { memcpy(array, tmp, oldSize*sizeof(type)); delete [] tmp; } }

using namespace base;
using namespace model;

Mesh::Mesh() :
	m_ref(0),
	m_mode(GL_TRIANGLES),
	m_format(VERTEX_DEFAULT),
	m_stride(32),
	m_material(0), 
	m_vertexBuffer(0),
	m_indexBuffer(0),
	m_skins(0) {
}

Mesh::Mesh(const Mesh& m): m_ref(0), m_mode(m.m_mode), m_format(m.m_format), m_stride(m.m_stride) {
	// Copy material (or reference?)
	m_material = m.m_material;

	// Reference index buffer
	m_indexBuffer = m.m_indexBuffer;
	if(m_indexBuffer) ++m_indexBuffer->ref;
	
	// Reference skins
	m_skins = m.m_skins;
	if(m_skins) ++m_skins->ref;

	// Copy vertex buffer
	if(m.m_vertexBuffer) {
		m_vertexBuffer = new Buffer<VertexType>();
		m_vertexBuffer->data = new VertexType[ m.m_vertexBuffer->size ];
		memcpy(m_vertexBuffer->data, m.m_vertexBuffer->data, m.m_vertexBuffer->size*sizeof(VertexType));
		m_vertexBuffer->size = m.m_vertexBuffer->size;
		m_vertexBuffer->bufferObject = 0;
		m_vertexBuffer->ref = 1;
		cachePointers();
	}
}

Mesh::~Mesh() {
	// Delete buffer objects
	useBuffer(false);

	// Delete vertices
	if(m_vertexBuffer && --m_vertexBuffer->ref==0) {
		delete [] m_vertexBuffer->data;
		delete m_vertexBuffer;
	}
	
	// Delete indices
	if(m_indexBuffer && --m_indexBuffer->ref==0) {
		delete [] m_indexBuffer->data;
		delete m_indexBuffer;
	}
	//delete skins
	if(m_skins && --m_skins->ref==0) {
		for(unsigned int i=0; i<m_skins->size; i++) {
			Skin& skin = m_skins->data[i];
			delete [] skin.indices;
			delete [] skin.weights;
			if(skin.name) free(skin.name);
		}
		delete [] m_skins->data;
		delete m_skins;
	}
}


/*
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
*/

int Mesh::formatSize(uint format) {
	int size = 0;
	for(int i=0; i<8; ++i) size += (format>>(i*4)) & 0xf;
	return size;
}

void Mesh::cachePointers() {
	memset(m_pointers, 0, 8*sizeof(void*));
	int size, offset=0;
	VertexType* data = m_vertexBuffer->bufferObject? 0: m_vertexBuffer->data;
	for(int i=0; i<8; ++i) {
		size = (m_format>>(i*4))&0xf;
		m_pointers[i] = size? data + offset: 0;
		offset += size;
	}
}

void Mesh::setFormat(uint format) {
	if(format == m_format) return;

	// Resize vertex array
	int count = getVertexCount();
	int size = formatSize(format);
	VertexType* src  = m_vertexBuffer->data;
	VertexType* data = new VertexType[ size * count ];
	memset(data, 0, size * count * sizeof(VertexFormat));

	// Calculate offsets
	int sOff[8];	// Source offsets
	int dSize[8];	// Destination data sizes
	int dOff[8];	// Destination offsets
	sOff[0]=0; dOff[0]=0; dSize[0]=format&0xf;
	for(int i=1; i<8; ++i) {
		sOff[i] = sOff[i-1] + ((m_format>>(i*4))&0xf);
		dSize[i] = (format>>(i*4))&0xf * sizeof(VertexType);
		dOff[i] = dOff[i-1] + dSize[i-1]/sizeof(VertexType);
	}

	// Copy data
	int stride = size * sizeof(VertexType);
	for(int i=0; i<count; ++i) {
		for(int j=0; j<8; ++j) {
			if(dSize[j]) {
				memcpy(data + i*stride + dOff[j], src + i*m_stride + sOff[j], dSize[j]*sizeof(VertexType));
			}
		}
	}

	// Finalise
	m_stride = stride;
	m_format = format;
	m_vertexBuffer->data = data;
	cachePointers();
	delete [] src;
	// TODO: Update buffer object if it exists
}


/** Data Set Functions */
void Mesh::setVertices(int count, VertexType* data, uint format) {
	int size = formatSize(format);
	m_format = format;
	m_stride = size * sizeof(VertexType);
	m_vertexBuffer = new Buffer<VertexType>();
	m_vertexBuffer->data = data;
	m_vertexBuffer->size = count * size;
	m_vertexBuffer->bufferObject = 0;
	m_vertexBuffer->ref = 1;
	cachePointers();
	calculateBounds();
}

void Mesh::setIndices(int count, IndexType* data) {
	m_indexBuffer = new Buffer<IndexType>();
	m_indexBuffer->data = data;
	m_indexBuffer->size = count;
	m_indexBuffer->ref = 1;
}

void Mesh::addSkin(const Skin& skin) {
	if(!m_skins) {
		m_skins = new Buffer<Skin>();
		m_skins->ref = 1; //grab pointer
		m_skins->size = 0;
	}
	// Add skin
	ResizeArray( Skin, m_skins->data, m_skins->size, m_skins->size+1 );
	memcpy( &m_skins->data[ m_skins->size ], &skin, sizeof(Skin));
	++m_skins->size;
}

/** Data Processing */
int Mesh::calculateTangents() {
	setFormat( m_format | TANGENT );
	int size = m_stride / sizeof(VertexType);
	int no = m_format&0xf;						// Normal offset
	int to = formatSize(m_format & 0xff );  	// TexCoord0 offset
	int tp = formatSize(m_format & 0x0fffff); 	// Get the size up to tangent slot
	if(no==0 || to==0) return 0; // Cant calculate tangents if no normals and texture coords

	VertexType *a[3], *b[3], *c[3]; // position, normal, texture tuples for three points
	if(m_indexBuffer) {
		for(uint i=0; i<m_indexBuffer->size; i+=3) {
			// Get polygon points
			a[0] = m_vertexBuffer->data + m_indexBuffer->data[i] * size;
			b[0] = m_vertexBuffer->data + m_indexBuffer->data[i+1] * size;
			c[0] = m_vertexBuffer->data + m_indexBuffer->data[i+2] * size;
			a[1] = a[0] + no; a[2] = a[0] + to;
			b[1] = b[0] + no; b[2] = b[0] + to;
			c[1] = c[0] + no; c[2] = c[0] + to;
			// Calculate and add tangent
			calculateTangent(a,b,c, a[0]+tp);
			memcpy(b[0]+tp, a[0]+tp, 3*sizeof(VertexType));
			memcpy(c[0]+tp, a[0]+tp, 3*sizeof(VertexType));
		}
	} else if(m_vertexBuffer) {
		for(uint i=0; i<m_vertexBuffer->size; i+=3) {
			a[0] = m_vertexBuffer->data + i * size;
			b[0] = m_vertexBuffer->data + (i+1) * size;
			c[0] = m_vertexBuffer->data + (i+2) * size;
			a[1] = a[0] + no; a[2] = a[0] + to;
			b[1] = b[0] + no; b[2] = b[0] + to;
			c[1] = c[0] + no; c[2] = c[0] + to;
			// Calculate and add tangent
			calculateTangent(a,b,c, a[0]+tp);
			memcpy(b[0]+tp, a[0]+tp, 3*sizeof(VertexType));
			memcpy(c[0]+tp, a[0]+tp, 3*sizeof(VertexType));
		}
	} else return 0;
	return 1;
}


int Mesh::calculateTangent(VertexType*const* a, VertexType*const* b, VertexType*const* c, VertexType* t) {
	// Position vectors
	VertexType ab[3] = { b[0][0]-a[0][0], b[0][1]-a[0][1], b[0][2]-a[0][2] };
	VertexType ac[3] = { c[0][0]-a[0][0], c[0][1]-a[0][1], c[0][2]-a[0][2] };

	//Project ab,ac onto normal
	const VertexType* n = a[1]; // Normal - just use the one
	VertexType abn = ab[0]*n[0] + ab[1]*n[1] + ab[2]*n[2];
	VertexType acn = ac[0]*n[0] + ac[1]*n[1] + ac[2]*n[2];
	ab[0]-=n[0]*abn; ab[1]-=n[1]*abn; ab[2]-=n[2]*abn;
	ac[0]-=n[0]*acn; ac[1]-=n[1]*acn; ac[2]-=n[2]*acn;

	//Texture coordinate deltas
	VertexType abu = b[2][0] - a[2][0];
	VertexType abv = b[2][1] - a[2][1];
	VertexType acu = c[2][0] - a[2][0];
	VertexType acv = c[2][1] - a[2][1];
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

int Mesh::normaliseWeights() {
	int scount = getSkinCount();
	int vcount = getVertexCount();
	if(!scount || !vcount) return 0; // Nothing to do
	int* ix = new int[ scount ];
	memset(ix, 0, scount * sizeof(int));
	int changed = 0;
	// Loop through vertices
	for(int i=0; i<vcount; ++i) {
		// Get total weights on this vertex
		float total = 0;
		for(int j=0; j<scount; ++j) {
			if(m_skins->data[j].indices[ ix[j] ] == i) {
				total += m_skins->data[j].weights[ ix[j] ];
			}
		}
		if(total!=0 && total!=1) ++changed;
		float mult = total==0? 1.0: 1.0 / total;
		for(int j=0; j<scount; ++j) {
			if(m_skins->data[j].indices[ ix[j] ] == i) {
				m_skins->data[j].weights[ ix[j] ] *= mult;
				++ix[j];
			}
		}
	}
	return changed;
}

int Mesh::calculateNormals(float smooth) {
	setFormat(m_format | NORMAL);
	// Calculate all flat normals
	// Actually, if indexed, vertices are shared so we have to smooth that anyway
	int no = m_format & 0xf;
	int size = formatSize(m_format);
	VertexType *a, *b, *c, n[3];
	if(hasIndices()) {
		// Need to check if we have already calculated a point
		char* check = new char[ getVertexCount() ];
		memset(check, 0, getVertexCount());
		// Loop
		int count = getSize();
		int ia, ib, ic;
		for(int i=0; i<count; i+=3) {
			ia = m_indexBuffer->data[i];
			ib = m_indexBuffer->data[i+1];
			ic = m_indexBuffer->data[i+2];
			a = m_vertexBuffer->data + ia*size;
			b = m_vertexBuffer->data + ib*size;
			c = m_vertexBuffer->data + ic*size;
			// Normal
			n[0] = (b[1]-a[1]) * (b[2]-a[2]) - (b[2]-a[2]) * (b[1]-a[1]);
			n[1] = (b[2]-a[2]) * (b[0]-a[0]) - (b[0]-a[0]) * (b[2]-a[2]);
			n[2] = (b[0]-a[0]) * (b[1]-a[1]) - (b[1]-a[1]) * (b[2]-a[2]);
			// Normalise
			float l = sqrt(n[0]*n[0] + n[1]*n[1] + n[2]*n[2]);
			n[0]/=l; n[1]/=l; n[2]/=l;
			// set data
			if(check[ia]) { a[no]+=n[0]; a[no+1]+=n[1]; a[no+2]=n[2]; }
			else memcpy(a+no, n, 3*sizeof(VertexType));
			if(check[ib]) { b[no]+=n[0]; b[no+1]+=n[1]; b[no+2]=n[2]; }
			else memcpy(b+no, n, 3*sizeof(VertexType));
			if(check[ic]) { c[no]+=n[0]; c[no+1]+=n[1]; c[no+2]=n[2]; }
			else memcpy(c+no, n, 3*sizeof(VertexType));
			check[ia]=check[ib]=check[ic]=true;
		}
		delete [] check;
		// Re-normalise all for any combined ones
		count = getVertexCount();
		for(int i=0; i<count; ++i) {
			a = m_vertexBuffer->data + i*size + no;
			float l = sqrt(a[0]*a[0] + a[1]*a[1] + a[2]*a[2]);
			a[0]/=l; a[1]/=l; a[2]/=l;
		}

	} else {
		int count = getVertexCount();
		for(int i=0; i<count; i+=3) {
			a = m_vertexBuffer->data + i*size;
			b = m_vertexBuffer->data + (i+1) * size;
			c = m_vertexBuffer->data + (i+2) * size;
			// Normal
			n[0] = (b[1]-a[1]) * (b[2]-a[2]) - (b[2]-a[2]) * (b[1]-a[1]);
			n[1] = (b[2]-a[2]) * (b[0]-a[0]) - (b[0]-a[0]) * (b[2]-a[2]);
			n[2] = (b[0]-a[0]) * (b[1]-a[1]) - (b[1]-a[1]) * (b[2]-a[2]);
			// Normalise
			float l = sqrt(n[0]*n[0] + n[1]*n[1] + n[2]*n[2]);
			n[0]/=l; n[1]/=l; n[2]/=l;
			// Set data
			memcpy(a+no, n, 3*sizeof(VertexType));
			memcpy(b+no, n, 3*sizeof(VertexType));
			memcpy(c+no, n, 3*sizeof(VertexType));
		}
	}
	if(smooth>0) smoothNormals(smooth);
	return 0;
}


int Mesh::smoothNormals(float angle) {
	if(~m_format & NORMAL) return 0; // Nothing to do

	int affected = 0;
	int vcount = getVertexCount();
	int size = formatSize(m_format);	// vertex float count
	int no = m_format & 0xf;			// Normal offset
	int count;
	char group[32];
	VertexType* point;
	VertexType* point2;
	VertexType* normal[32];
	float threshold = cos(angle*PI/180);
	char* done = new char[vcount];
	memset(done, 0, vcount);
	for(int i=0; i<vcount; ++i) {
		if(!done[i]) {
			// Collect points
			point = m_vertexBuffer->data + i*size;
			normal[0] = point + no;
			count=1;
			for(int j=i+1; j<vcount; ++j) {
				point2 = m_vertexBuffer->data + j*size;
				if(memcmp(point, point2, 3*sizeof(VertexType))==0) {
					normal[count] = point2 + no;
					++count;
				}
			}
			// Collect into groups
			if(count>1) {
				int groupCount=0;
				memset(group, 0, 32);
				for(int j=0; j<count; ++j) {
					if(group[j]==0) group[j] = ++groupCount;
					for(int k=j+1; k<count; ++k) {
						float dot = normal[j][0]*normal[k][0] + normal[j][1]*normal[k][1] + normal[j][2]*normal[k][2];
						if(dot<threshold) {
							// Rename group?
							if(group[k]>0) for(int l=0; l<count; ++l) if(group[l]==group[k]) group[l]=group[j];
							else group[k] = group[j];
						}
					}
				}
				// Smooth each group
				int groupSize = 0;
				VertexType n[3];
				for(int j=1; j<count; ++j) {
					groupSize=0;
					n[0]=n[1]=n[2]=0;
					for(int k=0; k<count; ++k) {
						if(group[k]==j) {
							++groupSize;
							n[0] += normal[k][0];
							n[1] += normal[k][1];
							n[2] += normal[k][2];
						}
					}
					// Normalise then apply
					if(groupSize>1) {
						n[0] /= groupSize;
						n[1] /= groupSize;
						n[2] /= groupSize;
						for(int k=0; k<count; ++k) {
							if(group[k]==j) memcpy(normal[k], n, 3*sizeof(VertexType));
						}
						affected += groupSize;
					}
				}
			}
		}
	}
	delete [] done;
	return affected;
}

const aabb& Mesh::calculateBounds() {
	m_bounds.min = m_bounds.max = getVertex(0);
	for(uint i=1; i<getVertexCount(); ++i) {
		const vec3& v = getVertex(i);
		m_bounds.addPoint(v);
	}
	return m_bounds;
}


