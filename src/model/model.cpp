#include <base/model.h>
#include <base/hardwarebuffer.h>
#include <cstring>
#include <cstdio>

using namespace base;

Model::Model() : m_skeleton(0), m_animationState(0) {
}

Model::~Model() {
	// memory leak
	// need to delete meshes, skeletons, animations, morphs.
	// everything should be reference counted
}

// --------------------------------------------------------------------------------------- //

int Model::getMeshCount() const {
	return m_meshes.size();
}

int Model::getMeshIndex(const char* name) const {
	if(name) for(size_t i=0; i<m_meshes.size(); ++i) {
		if(strcmp(name, m_meshes[i].name)==0) return i;
	}
	return -1;
}

Mesh* Model::getMesh(int index) const {
	return m_meshes[index].mesh;
}
Mesh* Model::getMesh(const char* name) const {
	int index = getMeshIndex(name);
	return index<0? 0: m_meshes[index].mesh;
}

int Model::addMesh(const char* name, Mesh* mesh, const char* materialName) {
	mesh->addReference();
	MeshInfo info;
	info.mesh = mesh;
	info.name = name && name[0]? strdup(name): 0;
	info.skinMap = createSkinMap(m_skeleton, mesh);
	info.materialName = materialName && materialName[0]? strdup(materialName): 0;
	m_meshes.push_back(info);
	return m_meshes.size() - 1;
}

void Model::removeMesh(int i) {
	MeshInfo& m = m_meshes[i];
	if(m.skinMap) delete [] m.skinMap;
	if(m.materialName) free(m.materialName);
	if(m.name) free(m.name);
	m.mesh->dropReference();
	m_meshes.erase( m_meshes.begin() + i);
}

void Model::removeMesh(const char* name) {
	for(uint i=0; i<m_meshes.size(); ++i) {
		if( strcmp(m_meshes[i].name, name) == 0 ) {
			removeMesh(i);
			return;
		}
	}
}
void Model::removeMesh(const Mesh* m) {
	for(uint i=0; i<m_meshes.size(); ++i) {
		if( m_meshes[i].mesh == m ) {
			removeMesh(i);
			return;
		}
	}
}

// --------------------------------------------------------------------------------------- //

const char* Model::getMeshName(int index) const {
	return m_meshes[index].name;
}
const char* Model::getMeshName(const Mesh* mesh) const {
	for(size_t i=0; i<m_meshes.size(); ++i) {
		if(m_meshes[i].mesh==mesh) return m_meshes[i].name;
	}
	return 0;
}

// --------------------------------------------------------------------------------------- //

void Model::setMaterialName(const char* material) {
	for(uint i=0; i<m_meshes.size(); ++i) setMaterialName(i, material);
}
void Model::setMaterialName(int index, const char* material) {
	if(m_meshes[index].materialName) free(m_meshes[index].materialName);
	if(material && material[0]) m_meshes[index].materialName = strdup(material);
	else m_meshes[index].materialName = 0;
}
const char* Model::getMaterialName(int index) const {
	return m_meshes[index].materialName? m_meshes[index].materialName: "";
}
const char* Model::getMaterialName(const char* mesh) const {
	int index = getMeshIndex(mesh);
	return index<0? "": getMaterialName(index);
}
const char* Model::getMaterialName(const Mesh* mesh) const {
	for(size_t i=0; i<m_meshes.size(); ++i) {
		if(mesh == m_meshes[i].mesh) return m_meshes[i].materialName? m_meshes[i].materialName: "";
	}
	return "";
}


// --------------------------------------------------------------------------------------- //


void Model::setSkeleton(Skeleton* s) {
	m_skeleton = s;
	// Create skin maps
	for(uint i=0; i<m_meshes.size(); ++i) {
		if(m_meshes[i].skinMap) delete [] m_meshes[i].skinMap;
		m_meshes[i].skinMap = createSkinMap(s, m_meshes[i].mesh);
	}
}
Skeleton* Model::getSkeleton() const {
	return m_skeleton;
}

int* Model::createSkinMap(Skeleton* s, Mesh* m) {
	int count = m->getSkinCount();
	if(count == 0 || !s) return 0;

	int* map = new int[count];
	for(int i=0; i<count; ++i) {
		int bone = s->getBoneIndex( m->getSkinName(i) );
		if(bone<0) printf("Error: No bone for skin %s\n", m->getSkinName(i));
		map[i] = bone<0? 0: bone;
	}
	return map;
}

// --------------------------------------------------------------------------------------- //

void Model::setAnimationState(AnimationState* state) {
	m_animationState = state;
}

AnimationState* Model::getAnimationState() const {
	return m_animationState;
}

void Model::addAnimation(Animation* a) {
	m_animations.push_back(a);
}
Animation* Model::getAnimation(const char* name) const {
	for(uint i=0; i<m_animations.size(); ++i) {
		if(strcmp(m_animations[i]->getName(), name)==0) return m_animations[i];
	}
	return 0;
}
Animation* Model::getAnimation(int i) const {
	if(i<0 || (size_t)i >= m_animations.size()) return 0;
	return m_animations[i];
}
size_t Model::getAnimationCount() const {
	return m_animations.size();
}

// --------------------------------------------------------------------------------------- //

void Model::addExtension(ModelExtension* ext) {
	m_extensions.push_back(ext);
}

// --------------------------------------------------------------------------------------- //


void Model::update(float time) {
	// Update animation frames
	if(m_animationState) {
//		m_animationState->update(time);
//		if(m_skeleton->update()) {
			// Software skinning
			if(getMeshCount() == 1) addMesh("skinned", createSkinTarget(m_meshes[0].mesh));
			skinMesh(m_meshes[0].mesh, m_skeleton, m_meshes[0].skinMap, m_meshes[1].mesh);
//		}
	}
}




// --------------------------------------------------------------------------------------- //

Mesh* Model::createSkinTarget(Mesh* src) {
	Mesh* m = new Mesh();
	// Reference index buffer
	m->setIndexBuffer(src->getIndexBuffer());
	// Duplicate vertex buffer
	HardwareVertexBuffer* sbuf = src->getVertexBuffer();
	HardwareVertexBuffer* buf = new HardwareVertexBuffer();
	buf->copyData(sbuf->getData<char>(), sbuf->getVertexCount(), sbuf->getStride());
	buf->attributes = sbuf->attributes;
	m->setVertexBuffer(buf);
	return m;
}

inline size_t getOffset(const base::HardwareVertexBuffer* buf, base::AttributeSemantic s) {
	const base::Attribute& a = buf->attributes.get(s);
	return a? a.offset: ~0u;
}

void Model::skinMesh(Mesh* in, const Skeleton* s, int* map, Mesh* out) {
	int wpv = in->getWeightsPerVertex();
	const char* skinData = in->getSkinBuffer()->getData<char>();
	size_t wOffset = getOffset(in->getSkinBuffer(), VA_SKINWEIGHT);
	size_t iOffset = getOffset(in->getSkinBuffer(), VA_SKININDEX);
	size_t stride = in->getSkinBuffer()->getStride();

	// Setup skin matrices
	Matrix* m = new Matrix[in->getSkinCount()];
	for(uint i=0; i<in->getSkinCount(); ++i) {
		m[i] =  s->getBone( map[i] )->getAbsoluteTransformation() * s->getSkinMatrix( map[i] );
	}

	// Get vertex offsets from semantics
	size_t vOffset = getOffset(in->getVertexBuffer(), VA_VERTEX);
	//size_t nOffset = getOffset(in->getVertexBuffer(), VA_NORMAL);
	//size_t tOffset = getOffset(in->getVertexBuffer(), VA_TANGENT);
	size_t vStride = in->getVertexBuffer()->getStride();
	char* source = in->getVertexBuffer()->getData<char>();
	char* dest  = out->getVertexBuffer()->getData<char>();

	// Positions
	for(size_t i=0; i<in->getVertexCount(); ++i) {
		vec3* vertexIn  = (vec3*) (source + vOffset + i*vStride);
		vec3* vertexOut = (vec3*) (dest + vOffset + i*vStride);

		float* weights = (float*) (skinData + i * stride + wOffset);
		IndexType* indices = (IndexType*) (skinData + i*stride + iOffset);
		
		vertexOut->set(0,0,0);
		for(int j=0; j<wpv && weights[j]>0; ++j) {
			*vertexOut += m[ indices[j] ] * *vertexIn * weights[j];
		}
	}

	delete [] m;
}

