#include <base/mesh.h>
#include <base/batcher.h>
#include <base/drawablemesh.h>
#include <assert.h>

using namespace base;

void Batcher::clear() {
	m_items.clear();
	attributes = VertexAttributes();
}

void Batcher::addMesh(Mesh* mesh, const Matrix& transform, const vec4& custom) {
	if(!mesh || mesh->getVertexCount()==0) return;
	m_items.push_back(BatchItem{mesh, transform, custom});
	if(attributes.size()==0) attributes = mesh->getVertexBuffer()->attributes;
	// ToDo validate attributes
}

void Batcher::addMesh(Mesh* mesh, const vec3& pos, const Quaternion& rot, const vec4& custom) {
	Matrix m;
	rot.toMatrix(m);
	m.translate(pos);
	addMesh(mesh, m, custom);
}

template<typename S, typename D>
inline void addIndices(const char* src, char* dst, uint count, uint offset) {
	D* dest = (D*)dst;
	const S* source = (const S*)src;
	for(uint i=0; i<count; ++i) dest[i] = source[i] + offset;
}

Mesh* Batcher::build(Mesh* out) const {
	int vCount=0, iCount=0;
	for(const BatchItem& i: m_items) {
		vCount += i.mesh->getVertexCount();
		iCount += i.mesh->getIndexCount();
	}
	IndexSize indexType = vCount<=0xff? IndexSize::I8: vCount<=0xffff? IndexSize::I16: IndexSize::I32;
	const int indexStride[3] = { 1,2,4 };

	int stride = attributes.calculateStride();
	char* vData = new char[vCount * stride];
	char* iData = new char[iCount * indexStride[(int)indexType]];
	char* dst = iData;
	uint32 offset = 0;

	// Potential for multithreaded build with thread pool

	for(const BatchItem& item: m_items) {
		// vertices
		int count = item.mesh->getVertexCount();
		for(const Attribute& a: attributes) {
			int size = HardwareVertexBuffer::getDataSize(a.type);
			char* vx = vData + offset * stride;

			if(a.semantic==VA_CUSTOM) { // Should use the custom key
				for(int i=0; i<count; ++i) {
					memcpy(vx + i*stride + a.offset, item.custom, size);
				}
			}
			else {
				int srcStride = item.mesh->getVertexBuffer()->getStride();
				Attribute& srcAttrib = item.mesh->getVertexBuffer()->attributes.get(a.semantic);
				vec3 temp;
				for(int i=0; i<count; ++i) {
					void* data = item.mesh->getVertexBuffer()->getData<char>() + i*srcStride + srcAttrib.offset;
					switch(a.semantic) {
					case VA_VERTEX: temp = item.transform * *reinterpret_cast<vec3*>(data); data=temp; break;
					case VA_NORMAL:
					case VA_TANGENT:temp = item.transform.rotate(*reinterpret_cast<vec3*>(data)); data=temp; break;
					default: break;
					}
					memcpy(vx + i*stride + a.offset, data, size);
				}
			}
		}

		// Indices
		IndexSize srcIndexType = item.mesh->getIndexBuffer()->getIndexSize();
		const char* src = item.mesh->getIndexBuffer()->getData<char>();
		count = item.mesh->getIndexCount();
		switch(indexType) {
		case IndexSize::I8:
			switch(srcIndexType) {
			case IndexSize::I8:  addIndices<uint8,  uint8>(src, dst, count, offset); break;
			case IndexSize::I16: addIndices<uint16, uint8>(src, dst, count, offset); break;
			case IndexSize::I32: addIndices<uint32, uint8>(src, dst, count, offset); break;
			}
			break;
		case IndexSize::I16:
			switch(srcIndexType) {
			case IndexSize::I8:  addIndices<uint8,  uint16>(src, dst, count, offset); break;
			case IndexSize::I16: addIndices<uint16, uint16>(src, dst, count, offset); break;
			case IndexSize::I32: addIndices<uint32, uint16>(src, dst, count, offset); break;
			}
			break;
		case IndexSize::I32:
			switch(srcIndexType) {
			case IndexSize::I8:  addIndices<uint8,  uint32>(src, dst, count, offset); break;
			case IndexSize::I16: addIndices<uint16, uint32>(src, dst, count, offset); break;
			case IndexSize::I32: addIndices<uint32, uint32>(src, dst, count, offset); break;
			}
			break;
		}
		dst += count * indexStride[(int)indexType];
		offset += item.mesh->getVertexCount();
	}

	// Build output mesh
	if(!out) {
		out = new Mesh();
		out->setIndexBuffer(new HardwareIndexBuffer(indexType));
		out->setVertexBuffer(new HardwareVertexBuffer());
		out->getVertexBuffer()->attributes = attributes;
		out->getVertexBuffer()->createBuffer();
		out->getIndexBuffer()->createBuffer();
	}

	out->getVertexBuffer()->setData(vData, vCount, stride, true);
	out->getIndexBuffer()->setData(iData, iCount * indexStride[(int)indexType], true);
	out->getIndexBuffer()->setIndexSize(indexType);
	return out;
}



