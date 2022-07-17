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
	uint8*  ix8  = (uint8*)iData;
	uint16* ix16 = (uint16*)iData;
	uint32* ix32 = (uint32*)iData;
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
		assert(item.mesh->getIndexBuffer()->getIndexSize() == IndexSize::I16);
		count = item.mesh->getIndexCount();
		const uint16* src = item.mesh->getIndexBuffer()->getData<uint16>(); // Fixme: Allow diffent types here
		switch(indexType) {
		case IndexSize::I8:
			for(int i=0; i<count; ++i) ix8[i] = src[i] + offset;
			ix8 += count;
			break;
		case IndexSize::I16:
			for(int i=0; i<count; ++i) ix16[i] = src[i] + offset;
			ix16 += count;
			break;
		case IndexSize::I32:
			for(int i=0; i<count; ++i) ix32[i] = src[i] + offset;
			ix32 += count;
			break;
		}
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



