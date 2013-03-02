#include "base/morph.h"
#include <cstring>

using namespace base;
using namespace model;

Morph::Morph(): m_name(""), m_type(ABSOLUTE), m_size(0), m_max(0), m_format(0), m_ref(0) {}
Morph::Morph(const Morph& m) {
	memcpy(this, &m, sizeof(Morph));
	// Duplicate data
	if(m_size) {
		m_data = new VertexType[ m_size * m_formatSize ];
		m_indices = new IndexType[ m_size ];
		memcpy(m_data, m.m_data, m_size*m_formatSize*sizeof(VertexType));
		memcpy(m_indices, m.m_indices, m_size*sizeof(IndexType));
	}
	// Reset reference count
	m_ref = 0;
}
Morph::~Morph() {
	if(m_size) {
		delete [] m_data;
		delete [] m_indices;
	}
}


void Morph::setData(Type type, int size, IndexType* ix, VertexType* vx, uint format) {
	// Set data
	m_size = size;
	m_data = vx;
	m_indices = ix;
	m_format = format;
	m_type = type;

	// Get format size and pointer offsets
	m_offset[0] = 0;
	for(int i=0; i<8; ++i) m_parts[i] = format & (0xf<<(i*4));
	for(int i=0; i<7; ++i) m_offset[i+1] = m_offset[i] + m_parts[i];
	m_formatSize = m_parts[7] + m_offset[7];
}

void Morph::apply(const Mesh* input, Mesh* output, float value) const {
	// Validate operation
	if(input->getVertexCount()<m_max || output->getVertexCount()<m_max) return; // Morph indexes too high
	if(input->getFormat() != output->getFormat()) return;	// Input and output formats must match

	// Get some pointers
	const VertexType* in = input->getData();
	VertexType* out = output->getData();
	int stride = input->getStride() / sizeof(VertexType);

	// Calculate offsets for mesh format
	int off[8]; off[0]=0;
	for(int i=0; i<7; ++i) off[i+1] = off[i] + (input->getFormat() & (0xf<<(i*4)));

	if(m_type==ABSOLUTE) {
		// Loop through parts
		for(int i=0; i<8; ++i) {
			if(m_parts[i]) {
				// Loop through points in morph data
				for(int j=0; j<m_size; ++j) {
					int p = m_indices[j]*stride + off[i];
					float* t = m_data + j*m_formatSize + m_offset[i];
					// Interpolate values
					for(int k=0; k<m_parts[i]; ++k) {
						out[p+k] = in[p+k] + (t[k]-in[p+k]) * value;
					}
				}
			}
		}
	} else { // RELATIVE Morph
		// Loop through parts
		for(int i=0; i<8; ++i) {
			if(m_parts[i]) {
				// Loop through points in morph data
				for(int j=0; j<m_size; ++j) {
					int p = m_indices[j]*stride + off[i];
					float* t = m_data + j*m_formatSize + m_offset[i];
					// Interpolate values
					for(int k=0; k<m_parts[i]; ++k) {
						out[p+k] = in[p+k] + t[k] * value;
					}
				}
			}
		}

	}
}


Morph* Morph::create(Type t, Mesh* src, Mesh* tgt, uint format) {
	// List all vertices that are different
	// copy list into index array
	// copy tgt vertices into vertex array, probably changing the format (deltas if relative)
	return 0;
}



