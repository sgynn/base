#include "base/model.h"
#include "modelmath.h"

using namespace base;
using namespace model;


Model::Model() : m_meshes(0), m_meshCount(0), m_skeleton(0),  m_aAnimation(0), m_aStart(0), m_aEnd(0), m_aFrame(0), m_aSpeed(30) {}
Model::Model(const Model& m) {
	//This needs to clone skeleton and any skined ot morphed meshes.
	//Also needs to increment reference counter on any mesh sources
	//And copy animation pointers. Maybe just reference the original animations map?
	
	if(m.m_skeleton) m_skeleton = new Skeleton(*m.m_skeleton);
	
	//Copy or reference meshes
	m_meshCount = m.m_meshCount;
	m_meshes = new MeshEntry[ m_meshCount ];
	memcpy(m_meshes, m.m_meshes, m_meshCount*sizeof(MeshEntry));
	//replace some meshes ...
	for(int i=0; i<m.m_meshCount; i++) {
		//duplicate mesh if derived.
		if(m_meshes[i].src) {
			m_meshes[i].mesh = new Mesh(*m.m_meshes[i].mesh, true, false);
		}
		m_meshes[i].mesh->grab();
		//also grab any sources
		if(m_meshes[i].src) m_meshes[i].src->grab();
	}
	
	//and copy all the automation properties too...
	m_aAnimation = m.m_aAnimation;
	m_aStart = m.m_aStart;
	m_aEnd   = m.m_aEnd;
	m_aFrame = m.m_aFrame;
	m_aSpeed = m.m_aSpeed;
	m_aLoop  = m.m_aLoop;
}
Model::~Model() {
	//delete member meshes if not referenced elsewhere
	if(m_skeleton) delete m_skeleton;
	//delete meshes
	for(int i=0; i<m_meshCount; i++) {
		m_meshes[i].mesh->drop();
		if(m_meshes[i].src) m_meshes[i].src->grab();
	}
}

Mesh* Model::getMesh(int index) { return index<m_meshCount? m_meshes[index].mesh: 0; }
Mesh* Model::getMesh(const char* name) {
	for(int i=0; i<m_meshCount; i++) if(strcmp(name, m_meshes[i].name)==0) return m_meshes[i].mesh;
	return 0;
}
Model::MeshFlag Model::getMeshFlag(const Mesh* mesh) const {
	for(int i=0; i<m_meshCount; i++) if(m_meshes[i].mesh==mesh) return m_meshes[i].flag;
	return MESH_OUTPUT;
}
void Model::setMeshFlag(const Mesh* mesh, MeshFlag flag) {
	for(int i=0; i<m_meshCount; i++) if(m_meshes[i].mesh==mesh) m_meshes[i].flag = flag;
}

Mesh* Model::addMesh(Mesh* mesh, const char* name, int joint) {
	ResizeArray(MeshEntry, m_meshes, m_meshCount, m_meshCount+1);
	m_meshes[ m_meshCount ].mesh = mesh;
	m_meshes[ m_meshCount ].flag = MESH_OUTPUT;
	m_meshes[ m_meshCount ].name = name;
	m_meshes[ m_meshCount ].joint = joint;
	m_meshes[ m_meshCount ].src = 0;
	m_meshCount++;
	mesh->grab(); //add reference
	return mesh;
}

void Model::addAnimation(const char* name, Animation* animation) {
	m_animations.insert(name, animation);
}

Animation* Model::getAnimation(const char* name) {
	if(name==0 && !m_animations.empty()) return *m_animations.begin();	//Return first animation
	else if(m_animations.contains(name)) return m_animations[name];		//Return named animation
	else return 0;								//Return no animation
}



//// //// //// //// //// //// //// //// Animation Stuff //// //// //// //// //// //// //// ////


void Model::setAnimationSpeed(float fps) {
	m_aSpeed = fps;
}
void Model::setAnimation(float start, float end, bool loop) {
	if(m_aStart!=start || m_aEnd!=end) setFrame(start);
	m_aStart = start;
	m_aEnd = end;
	m_aLoop = loop;
}
void Model::setAnimation(const char* name, float start, float end, bool loop) {
	setAnimation(start, end, loop);
	m_aAnimation = getAnimation(name);
}
void Model::setAnimation(const char* name, bool loop) {
	int start=0, end=0;
	m_aAnimation = getAnimation(name);
	if(m_aAnimation) m_aAnimation->getRange(start, end);
	setAnimation(start, end, loop);
}

void Model::setFrame(float frame) {
	if(frame != m_aFrame) {
		//Update skeleton
		if(m_skeleton) m_skeleton->animateJoints(m_aAnimation, frame);
		//update any skinned meshes - need to remember sources...
		for(int i=0; i<m_meshCount; i++) {
			if(m_meshes[i].src) skinMesh(m_meshes[i].mesh, m_meshes[i].src, m_skeleton);
		}
		m_aFrame = frame;
	}
}


//// //// //// //// //// //// //// //// Mesh Management //// //// //// //// //// //// //// ////


Mesh* Model::addMorphedMesh(Mesh* start, Mesh* end, float value, int joint) {
	Mesh* mesh = addMesh(morphMesh(0, start, end, value), joint);
	start->grab();
	end->grab();
	//flag sources as input meshes
	for(int i=0; i<m_meshCount; i++) if(m_meshes[i].mesh==start || m_meshes[i].mesh==end) m_meshes[i].flag=MESH_INPUT;
	return mesh;
}

Mesh* Model::addSkinnedMesh(Mesh* source) {
	Mesh* mesh = addMesh(skinMesh(0, source, 0));
	m_meshes[ m_meshCount-1 ].src = source;
	source->grab();
	//flag source as input mesh if referenced in this model
	for(int i=0; i<m_meshCount; i++) if(m_meshes[i].mesh==source) m_meshes[i].flag=MESH_INPUT;
	return mesh;
}



void Model::update(float time) {
	//advance frame
	float frame = m_aFrame;
	if(m_aStart==m_aEnd) {
		frame=m_aStart;
	} else {
		if(m_aEnd>m_aStart) {
			frame += m_aSpeed*time;
			if(frame>m_aEnd) frame = m_aLoop? frame-(m_aEnd-m_aStart): m_aEnd;
			else if(frame<m_aStart) frame += (m_aEnd-m_aStart);
		} else {
			frame -= m_aSpeed*time;
			if(frame<m_aEnd) frame = m_aLoop? frame+(m_aStart-m_aEnd): m_aEnd;
			else if(frame>m_aStart) frame -= (m_aStart-m_aEnd);
		}
	}
	setFrame(frame);
}



//// //// //// //// //// //// //// //// Static Mesh Manipulation Functions //// //// //// //// //// //// //// ////


Mesh* Model::skinMesh(Mesh* out, const Mesh* source, const Skeleton* skeleton) {
	Mesh* dest = out;
	if(!dest) dest = new Mesh(*source, true, false);
	
	//Validate destination mesh
	if(dest->getIndexPointer()!=source->getIndexPointer() || dest->getVertexCount() != source->getVertexCount()) return const_cast<Mesh*>(source);
	
	//skeleton exists?
	if(!skeleton) return dest;
	
	//Now the fun part.
	for(uint i=0; i<dest->getVertexCount(); i++) {
		if(dest->getVertexPointer()) memset((char*)dest->getVertexPointer() + i*dest->getStride(), 0, 3*sizeof(float)); //clear vertices
		if(dest->getNormalPointer()) memset((char*)dest->getNormalPointer() + i*dest->getStride(), 0, 3*sizeof(float)); //clear normals
	}
	
	//Loop through all skins
	const float* sVertex = source->getVertexPointer();
	float* dVertex = (float*)dest->getVertexPointer();
	const float* src;
	float temp[3];
	float mat[16];
	const float* combined;
	const Mesh::Skin* skin=0;
	for(int i=0; i<source->getSkinCount(); i++) {
		skin = source->getSkin(i);
		//get jointID from joint name
		if(skin->joint<0 && skin->jointName) const_cast<Mesh::Skin*>(skin)->joint = skeleton->getJoint(skin->jointName)->getID();
		//Cache joint combined matrix
		combined = skeleton->getJoint(skin->joint)->getTransformation();
		//get final matrix
		Math::multiplyAffineMatrix(mat, combined, skin->offset);
		
		//transform vertices
		for(int j=0; j<skin->size; j++) {
			//get offset, depends on VertexFormat
			size_t offset = source->getVertex( skin->indices[j] ) - source->getVertexPointer();
			
			src = (sVertex + offset);
			temp[0] = mat[0]*src[0] + mat[4]*src[1] + mat[ 8]*src[2] + mat[12];
			temp[1] = mat[1]*src[0] + mat[5]*src[1] + mat[ 9]*src[2] + mat[13];
			temp[2] = mat[2]*src[0] + mat[6]*src[1] + mat[10]*src[2] + mat[14];
			//Combine with weight
			(dVertex+offset)[0] += temp[0] * skin->weights[j];
			(dVertex+offset)[1] += temp[1] * skin->weights[j];
			(dVertex+offset)[2] += temp[2] * skin->weights[j];
			
			//transforrm normals too
			if(dest->getNormalPointer()) {
				src = (sVertex + offset + 3); //normal
				temp[0] = mat[0]*src[0] + mat[4]*src[1] + mat[ 8]*src[2];
				temp[1] = mat[1]*src[0] + mat[5]*src[1] + mat[ 9]*src[2];
				temp[2] = mat[2]*src[0] + mat[6]*src[1] + mat[10]*src[2];
				//Combine with weight
				(dVertex+offset+3)[0] += temp[0] * skin->weights[j];
				(dVertex+offset+3)[1] += temp[1] * skin->weights[j];
				(dVertex+offset+3)[2] += temp[2] * skin->weights[j];
			}
			//Transform tangents?
		}
	}
	return dest;
}

Mesh* Model::morphMesh(Mesh* out, const Mesh* start, const Mesh* end, float value, int morphFlags) {
	Mesh* dest = out;
	if(!dest) {
		dest = new Mesh(*start, true, false);
		if(value==0) return dest; //return this if nothing to do
	}
	if(morphFlags==0) return dest;
	
	//now, morph everything
	int count = start->getVertexCount()<end->getVertexCount()? start->getVertexCount(): end->getVertexCount();
	int dStride = dest->getStride() / sizeof(float);
	int sStride = start->getStride() / sizeof(float);
	int eStride = end->getStride() / sizeof(float);
	const float* vStart;
	const float* vEnd;
	float* vDest;
	for(int i=0; i<count; i++) {
		if(morphFlags&1) { //morph vertices and normals (and tangents)
			vStart = start->getVertexPointer()+i*sStride;
			vEnd = end->getVertexPointer()+i*eStride;
			vDest = (float*)dest->getVertexPointer()+i*dStride;
			
			vDest[0] = vStart[0] + (vEnd[0]-vStart[0]) * value;
			vDest[1] = vStart[1] + (vEnd[1]-vStart[1]) * value;
			vDest[2] = vStart[2] + (vEnd[2]-vStart[2]) * value;
			
			//normals
			if(start->getNormalPointer()) {
				vStart = start->getNormalPointer()+i*sStride;
				vEnd = end->getNormalPointer()+i*eStride;
				vDest = (float*)dest->getNormalPointer()+i*dStride;
				
				vDest[0] = vStart[0] + (vEnd[0]-vStart[0]) * value;
				vDest[1] = vStart[1] + (vEnd[1]-vStart[1]) * value;
				vDest[2] = vStart[2] + (vEnd[2]-vStart[2]) * value;
			}
		}
		if(morphFlags&2 && start->getTexCoordPointer()) { //Morph texture coordinates
			vStart = start->getTexCoordPointer()+i*sStride;
			vEnd = end->getTexCoordPointer()+i*eStride;
			vDest = (float*)dest->getTexCoordPointer()+i*dStride;
			
			vDest[0] = vStart[0] + (vEnd[0]-vStart[0]) * value;
			vDest[1] = vStart[1] + (vEnd[1]-vStart[1]) * value;
		}
	}
	return dest;
}


