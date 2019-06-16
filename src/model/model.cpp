#include "base/model.h"

using namespace base;
using namespace model;


Model::Model(): m_skeleton(0) {
	m_maps = new Maps;
	m_maps->ref = 1;
}

Model::Model(const Model& m) {
	// Copy skeleton
	if(m.m_skeleton) m_skeleton = new Skeleton(*m.m_skeleton);
	else m_skeleton = 0;

	// Reference index maps
	m_maps = m.m_maps;
	++m_maps->ref;

	// Copy meshes
	for(uint i=0; i<m.m_meshes.size(); ++i) {
		if(m.m_meshes[i].skinned) addSkin(m.m_meshes[i].source);
		else {
			Bone* b = 0;
			if(m_skeleton && m.m_meshes[i].bone) b = m_skeleton->getBone( m.m_meshes[i].bone->getIndex() );
			addMesh(m.m_meshes[i].source, 0, b);
		}
	}

	// Copy animation state
	m_animations = m.m_animations;
	for(uint i=0; i<m_animations.size(); ++i) {
		const Bone* b = m_animations[i].bone;
		if(b) m_meshes[i].bone = m_skeleton->getBone( b->getIndex() );
	}

	// Copy morph state
	m_morphs = m.m_morphs;
	for(uint i=0; i<m_morphs.size(); ++i) {
		m_morphs[i].morph->grab();
		if(m_morphs[i].value != 0) m_meshes[ m_morphs[i].mesh ].morphChanged = true;
	}

	// Build any meshes
	update(0);
}

Model::~Model() {
	if(m_skeleton) delete m_skeleton;
	// Delete meshes
	for(uint i=0; i<m_meshes.size(); ++i) {
		m_meshes[i].source->drop();
		if(m_meshes[i].morphed) m_meshes[i].morphed->drop();
		if(m_meshes[i].skinned) m_meshes[i].skinned->drop();
	}
	// Delete maps
	if(--m_maps->ref==0) {
		for(HashMap<Animation*>::iterator i=m_maps->animations.begin(); i!=m_maps->animations.end(); ++i) {
			i->value->drop();
		}
		delete m_maps;
	}
}


void Model::setSkeleton(Skeleton* s) {
	m_skeleton = s;
	// Cache and skin bone indices
	for(int i=0; i<getMeshCount(); ++i) {
		Mesh* mesh = getMesh(i);
		if(mesh->m_skins) {
			for(uint j=0; j<mesh->m_skins->size; ++j) {
				Skin& skin = mesh->m_skins->data[j];
				if(skin.name && skin.name[0]) {
					Bone* b = s->getBone(skin.name);
					if(b) skin.bone = b->getIndex();
				}
			}
		}
	}
}


int Model::addMesh(Mesh* mesh, const char* name, Bone* bone) {
	mesh->grab();
	MeshInfo info;
	info.source  = mesh;
	info.morphed = 0;
	info.skinned = 0;
	info.output  = mesh;
	info.bone    = bone;
	info.morphChanged = false;

	m_meshes.push_back(info);
	if(name) m_maps->meshes[name] = m_meshes.size()-1;
	return m_meshes.size()-1;
}

int Model::addSkin(Mesh* input, const char* name) {
	input->grab();
	MeshInfo info;
	info.source  = input;
	info.morphed = 0;
	info.skinned = new Mesh(*input);
	info.output  = info.skinned;
	info.bone    = 0;
	info.morphChanged = false;
	info.skinned->grab();

	m_meshes.push_back(info);
	if(name) m_maps->meshes[name] = m_meshes.size()-1;
	return m_meshes.size()-1;
}


void Model::setAnimation(const Animation* anim, const Bone* b) {
	AnimInfo* info = boneAnimation(b);
	if(info && info->animation!=anim) {
		info->animation = anim;
		info->frame = 0;

	} else if(!info) {
		AnimInfo ninfo;
		ninfo.animation = anim;
		ninfo.bone = b;
		ninfo.frame = 0;
		ninfo.speed = 1;
		m_animations.push_back(ninfo);
	}
}


Model::AnimInfo* Model::boneAnimation(const Bone* b) {
	for(uint i=0; i<m_animations.size(); ++i) {
		if(m_animations[i].bone == b) return &m_animations[i];
	}
	return 0;
}
const Model::AnimInfo* Model::boneAnimation(const Bone* b) const {
	for(uint i=0; i<m_animations.size(); ++i) {
		if(m_animations[i].bone == b) return &m_animations[i];
	}
	return 0;
}


int Model::setMorph(int mesh, Morph* morph, float value) {
	// ToDo: Validate morph against mesh
	morph->grab();
	MorphInfo info;
	info.morph  = morph;
	info.mesh   = mesh;
	info.value  = value;
	info.target = value;
	info.speed  = 0;
	// Add to lists
	m_morphs.push_back(info);
	m_maps->morphs[ morph->getName() ] = m_morphs.size()-1;
	// Initial morph
	if(value!=0) {
		// Morph mesh here or flag update
		m_meshes[ mesh ].morphChanged = true;
	}
	return m_morphs.size()-1;
}

void Model::setMorphValue(const char* morph, float value) {
	int i = mapValue(m_maps->morphs, morph, -1);
	if(i<0) return;
	float oldValue = m_morphs[i].value;
	m_morphs[i].value = m_morphs[i].target = value;
	m_morphs[i].speed = 0;
	if(value != oldValue) {
		// Either morph mesh here, or flag a full update
		m_meshes[ m_morphs[i].mesh ].morphChanged = true;
	}
}

void Model::startMorph(const char* name, float target, float time) {
	int i = mapValue(m_maps->morphs, name, -1);
	if(i<0) return;
	m_morphs[i].target = target;
	m_morphs[i].speed  = (target - m_morphs[i].value) / time;
}


void Model::morphMesh(MeshInfo& mesh, MorphInfo& morph, bool clean) {
	// Setup output mesh
	if(mesh.source==0) {
		mesh.source = mesh.output;
		mesh.morphed = new Mesh(*mesh.source);
		mesh.morphed->grab();
	}
	// Morph mesh
	const Mesh* src = clean? mesh.source: mesh.morphed;
	morphMesh(mesh.morphed, src, morph.morph, morph.value);
	mesh.output = mesh.morphed;
}

void Model::update(float time) {
	// Update any animated morphs
	for(uint i=0; i<m_morphs.size(); ++i) {
		if(m_morphs[i].speed!=0) {
			// Update value
			float r = fabs(m_morphs[i].value - m_morphs[i].target);
			if(r <= fabs(m_morphs[i].speed)*time) {
				m_morphs[i].value = m_morphs[i].target;
				m_morphs[i].speed = 0;
			} else m_morphs[i].value += m_morphs[i].speed * time;
			// Flag change
			m_meshes[ m_morphs[i].mesh ].morphChanged = true;
		}
	}

	// Update the skeleton by any animations
	for(uint i=0; i<m_animations.size(); ++i) {
		if(m_animations[i].speed!=0) {
			AnimInfo& info = m_animations[i];
			const Animation* anim = info.animation;
			int len = anim->getLength();
			info.frame += anim->getSpeed() * info.speed * time;
			// Loop, or terminate
			if(anim->isLoop()) {
				if(info.frame > len) info.frame -= len;
				else if(info.frame<0) info.frame += len;
			} else if(info.frame<=0 && info.speed<0) {
				info.frame = 0;
				info.speed = 0;
			} else if(info.frame>=len && info.speed>0) {
				info.frame = len;
				info.speed = 0;
			}
			// Update skeleton
			m_skeleton->animate(anim, info.frame, info.bone, 1);
		}
	}

	// Update skeleton matrices
	bool moved = m_skeleton && m_skeleton->update();

	// Update any meshes - remorphing or skinning them
	for(uint i=0; i<m_meshes.size(); ++i) {
		bool morphed = false;
		MeshInfo& mesh = m_meshes[i];
		// Update morphs
		if(mesh.morphChanged) {
			mesh.morphChanged = false;
			for(uint j=0; j<m_morphs.size(); ++j) {
				if(m_morphs[j].mesh==i) {
					morphMesh(mesh, m_morphs[j], morphed);
					morphed = true;
				}
			}
		}
		// Reskin mesh
		if(mesh.skinned && (moved || morphed)) {
			skinMesh(mesh.skinned, mesh.morphed? mesh.morphed: mesh.source, m_skeleton);
			mesh.output = mesh.skinned;;
		}
	}
}




//// //// //// //// //// //// //// //// Static Mesh Manipulation Functions //// //// //// //// //// //// //// ////

void Model::morphMesh(Mesh* out, const Mesh* src, const Morph* morph, float value) {
	morph->apply(src, out, value);
}

void Model::skinMesh(Mesh* out, const Mesh* src, const Skeleton* skeleton) {
	// skin modifies vertex, normal, tangent
	// src and out cannot be the same
	
	int  count = src->getVertexCount();
	int  size = src->getStride() / sizeof(VertexType);
	uint f = src->getFormat();
	int  no = f&0xf;
	int  to = no + ((f>>4)&0xf) + ((f>>8)&0xf) + ((f>>12)&0xf) + ((f>>16)&0xf);

	// Clear affected vertex data
	VertexType* r = out->getData();
	for(int i=0; i<count; ++i)               memset(r+i*size,   0, no*sizeof(VertexType));
	if(f&NORMAL)  for(int i=0; i<count; ++i) memset(r+no+i*size, 0, 3*sizeof(VertexType));
	if(f&TANGENT) for(int i=0; i<count; ++i) memset(r+to+i*size, 0, 3*sizeof(VertexType));

	// Apply skins
	const VertexType* in;
	for(int i=0; i<src->getSkinCount(); ++i) {
		const Skin* skin = src->getSkin(i);
		const Bone* bone = skeleton->getBone( skin->bone );
		Matrix mat = bone->getAbsoluteTransformation() * skin->offset;

		// Transform vertex positions
		for(uint j=0; j<skin->size; ++j) {
			float weight = skin->weights[j];
			uint ix      = skin->indices[j] * size;
			in = src->getData() + ix;
			r  = out->getData() + ix;
			// Do multiply manually to save creating new objects
			r[0] += weight * (mat[0]*in[0] + mat[4]*in[1] + mat[ 8]*in[2] + mat[12]);
			r[1] += weight * (mat[1]*in[0] + mat[5]*in[1] + mat[ 9]*in[2] + mat[13]);
			r[2] += weight * (mat[2]*in[0] + mat[6]*in[1] + mat[10]*in[2] + mat[14]);
		}

		// Normals
		if(f&NORMAL) for(uint j=0; j<skin->size; ++j) {
			float weight = skin->weights[j];
			uint ix      = skin->indices[j] * size + no;
			in = src->getData() + ix;
			r  = out->getData() + ix;
			// Do multiply manually to save creating new objects
			r[0] += weight * (mat[0]*in[0] + mat[4]*in[1] + mat[ 8]*in[2]);
			r[1] += weight * (mat[1]*in[0] + mat[5]*in[1] + mat[ 9]*in[2]);
			r[2] += weight * (mat[2]*in[0] + mat[6]*in[1] + mat[10]*in[2]);
		}

		// Tangents
		if(f&TANGENT) for(uint j=0; j<skin->size; ++j) {
			float weight = skin->weights[j];
			uint ix      = skin->indices[j] * size + to;
			in = src->getData() + ix;
			r  = out->getData() + ix;
			// Do multiply manually to save creating new objects
			r[0] += weight * (mat[0]*in[0] + mat[4]*in[1] + mat[ 8]*in[2]);
			r[1] += weight * (mat[1]*in[0] + mat[5]*in[1] + mat[ 9]*in[2]);
			r[2] += weight * (mat[2]*in[0] + mat[6]*in[1] + mat[10]*in[2]);
		}
	}
}


