#include <base/skeleton.h>
#include <base/animation.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>

using namespace base;

Bone::Bone() : 
	m_skeleton(0), m_index(0), m_parent(0),
	m_name(""), m_length(1), m_mode(DEFAULT), m_state(0), m_scale(1,1,1) {
}
Bone::~Bone() {
	if(m_name[0]) free((char*)m_name);
}

void Bone::setPosition(const vec3& p) {
	m_position = p;
	m_state = 1;
}
void Bone::setScale(float s) {
	m_scale.x = m_scale.y = m_scale.z = s;
	m_state = 1;
}
void Bone::setScale(const vec3& s) {
	m_scale = s;
	m_state = 1;
}
void Bone::setEuler(const vec3& pyr) {
	m_angle.fromEuler(pyr.y, pyr.x, pyr.z);
	m_state = 1;
}
void Bone::setAngle(const Quaternion& q) {
	m_angle = q;
	m_state = 1;
}

void Bone::setTransformation(const Matrix& m) {
	m_local = m;
	m_state = 2;
}
void Bone::setAbsoluteTransformation(const Matrix& m) {
	m_combined = m;
	m_state = 4;
}

void Bone::move(const vec3& m) {
	m_position += m;
	m_state = 1;
}
void Bone::rotate(const Quaternion& q) {
	m_angle *= q;
	m_state = 1;
}

const EulerAngles Bone::getEuler() const {
	return m_angle.getEuler();
}

void Bone::updateLocal() {
	// Note: when absoluteMatrix is being set, local values mean nothing.
	switch(m_state&7) {
	case 1: // parts to matrix
		{
		const Matrix& rest = m_skeleton->getRestPose(m_index);
		Quaternion rot( rest );
		rot *= m_angle;
		rot.toMatrix(m_local);
		// Translation relative to rest pose of this bone
		m_local.setTranslation(rest * m_position);
		}
		m_state=3;
		break;
	case 2: // Matrix to parts
		m_angle.fromMatrix(m_local);
		memcpy(m_position, &m_local[12], sizeof(vec3));
		m_scale.x = vec3(&m_local[0]).length();
		m_scale.y = vec3(&m_local[4]).length();
		m_scale.z = vec3(&m_local[8]).length();
		{
		const Matrix& rest = m_skeleton->getRestPose(m_index);
		m_position.x -= rest[12];
		m_position.y -= rest[13];
		m_position.z -= rest[14];
		}
		m_state=3;
		break;
	}
}




// ================================================================================================ //





Skeleton::Skeleton(): m_bones(0), m_count(0), m_hints(0), m_flags(0), m_matrices(0), m_rest(0) {
}
Skeleton::Skeleton(const Skeleton& s) : m_count(s.m_count), m_matrices(0) {
	m_bones = new Bone*[ m_count ];
	m_hints = new uint16[ 3 * m_count ];
	m_flags = new ubyte[ m_count ];
	m_matrices = new Matrix[ m_count ];
	for(int i=0; i<m_count; ++i) {
		m_bones[i] = new Bone(*s.m_bones[i]);
		m_bones[i]->m_skeleton = this;
		if(m_bones[i]->m_name[0]) m_bones[i]->m_name = strdup( m_bones[i]->m_name );
	}
	memset(m_hints, 0, 3 * m_count * sizeof(uint16));
	if(s.m_matrices) memcpy(m_matrices, s.m_matrices, m_count*sizeof(Matrix));
	m_rest = s.m_rest;
	++m_rest->ref;
}
Skeleton::~Skeleton() {
	// Destroy bones
	if(m_count) {
		for(int i=0; i<m_count; ++i) delete m_bones[i];
		delete [] m_bones;
		delete [] m_hints;
		delete [] m_flags;
		delete [] m_matrices;
	}
	// Destroy rest pose data
	dropRestPose();
}

void Skeleton::setRestPose() {
	dropRestPose();
	m_rest = new RestPose();
	m_rest->ref = 1;
	m_rest->size = m_count;
	m_rest->local = new Matrix[m_count];
	m_rest->skin = new Matrix[m_count];
	m_rest->rot = new Quaternion[m_count];
	m_rest->scale = new vec3[m_count];
	// Copy pose matrices
	for(int i=0; i<m_count; ++i) {
		m_rest->scale[i] = m_bones[i]->getScale();
		m_rest->rot[i] = m_bones[i]->getAngle();
		memcpy(m_rest->local[i], m_bones[i]->getTransformation(), sizeof(Matrix));
		Matrix::inverseAffine(m_rest->skin[i], m_bones[i]->getAbsoluteTransformation());
	}
}

void Skeleton::dropRestPose() {
	if(m_rest && --m_rest->ref==0) {
		delete [] m_rest->local;
		delete [] m_rest->skin;
		delete [] m_rest->rot;
		delete [] m_rest->scale;
		delete m_rest;
		m_rest = 0;
	}
}

unsigned Skeleton::getMapID() const {
	size_t r = (size_t)m_rest;
	if(!r) return getBoneCount();
	return (unsigned) r;
}

Bone* Skeleton::addBone(const Bone* p, const char* name, const float* local, float length) {
	// Check parent is valid
	if(p && p->m_skeleton!=this) {
		printf("Warning: Parent '%s' for bone '%s' is invalid\n", p->getName(), name);
		p = 0;
	}
	// Resize array
	Bone** tmp = m_bones;
	m_bones = new Bone*[ m_count+1 ];
	// Delete old data
	if(m_count) { 
		memcpy(m_bones, tmp, m_count*sizeof(Bone*));
		delete [] tmp;
		delete [] m_hints;
		delete [] m_flags;
	}
	// Resize hint arrays
	m_flags = new ubyte[ m_count+1 ];
	m_hints = new uint16[ (m_count+1) * 3 ];
	memset(m_hints, 0, (m_count+1) * 3 * sizeof(uint16));
	// Initialise bone data
	Bone* bone = m_bones[m_count] = new Bone();
	bone->m_skeleton = this;
	bone->m_index = m_count;
	bone->m_parent = p? p->getIndex(): -1;
	bone->m_length = length;
	if(name && name[0]) bone->m_name = strdup(name);
	// Initial Transforms
	if(local) memcpy(bone->m_local, local, sizeof(Matrix));
	bone->m_combined = p? p->m_combined * bone->m_local: bone->m_local;
	// rest pose is probably invalid
	dropRestPose();
	++m_count;
	return bone;
}

int Skeleton::getBoneIndex(const char* name) const {
	if(!name || !name[0]) return  -1;
	for(int i=0; i<m_count; ++i) {
		if(strcmp(name, m_bones[i]->m_name)==0) return i;
	}
	return -1;
}

Bone* Skeleton::getBone(const char* name) const {
	int index = getBoneIndex(name);
	return index<0? 0: m_bones[index];
}

void Skeleton::setMode(Bone::Mode m) {
	for(int i=0; i<m_count; ++i) m_bones[i]->setMode(m);
}


// ============================ Pose functions ============================= //

void Skeleton::resetPose() {
	for(int i=0; i<m_count; ++i) resetPose(i);
}
void Skeleton::resetPose(int boneIndex) {
	Bone* bone = m_bones[boneIndex];
	if(bone->getMode() != Bone::FIXED && bone->getMode() != Bone::USER) {
		bone->m_local = m_rest->local[boneIndex];
		bone->m_scale = m_rest->scale[boneIndex];
		bone->m_angle = m_rest->rot[boneIndex];
		bone->m_position.set(0,0,0);
		bone->m_state = 3;
	}
}

int Skeleton::applyPose(const Animation* anim, float frame, int root, int blend, float weight, const unsigned char* map) {
	// Trivial null cases
	if(weight==0 && blend > 0) return 0;

	int modified = 0;
	int start = root<0? 0: root;
	if(!map) map = anim->getMap(this);
	// Loop through remaining bones
	for(int i=start; i<m_count; ++i) {
		// Determine what to do based on parent flag and bone mode
		Bone* bone = m_bones[i];
		m_flags[i] = i==root || root<0? 1: 0;
		if(bone->getParent()) m_flags[i] |= m_flags[ bone->getParent()->getIndex() ];

		bool set = false;
		switch( m_bones[i]->getMode() ) {
		case Bone::DEFAULT:  set = m_flags[i] == 1; break;	// stops at truncate flag
		case Bone::ANIMATED: set = m_flags[i] > 0;  break;	// overrides truncate flag
		case Bone::TRUNCATE: if(i==root) set=true; else m_flags[i]=2; break;	// truncate animation unless starting from here
		case Bone::USER:     break;	// user overridden
		case Bone::FIXED:    break;	// user overridden
		}
		// Set bone values from animation
		if(set && map[i]!=0xff && applyBonePose(m_bones[i], anim, map[i], frame, blend, weight)) ++modified;
	}
	return modified;
}
bool Skeleton::applyBonePose(Bone* b, const Animation* anim, int keyset, float frame, int blend, float weight) {
	static SlerpFunc slerp;
	static LerpFunc lerp;
	if(b->m_mode==Bone::DEFAULT || b->m_mode==Bone::ANIMATED) {
		vec3 pos, scl;
		Quaternion rot;
		// Get values from animation
		int index = b->getIndex();
		anim->getRotation(keyset, frame, m_hints[index*3],   rot);
		anim->getPosition(keyset, frame, m_hints[index*3+1], pos);
		anim->getScale   (keyset, frame, m_hints[index*3+2], scl);
		// Blending
		if(weight != 1) {
			if(blend==1) {	// MIX - interpolate between current and new values
				if(b->m_state==2) b->updateLocal();
				slerp(rot, b->m_angle, rot, weight);
				lerp(pos, b->m_position, pos, weight);
				lerp(scl, b->m_scale, scl, weight);
			} else {	// SET - scale new values
				slerp(rot, m_rest->rot[index], rot, weight);
				lerp(scl, m_rest->scale[index], scl, weight);
				pos *= weight;
			}
		}
		if(blend==2) {	// ADD - add new values
			if(b->m_state==2) b->updateLocal();
			rot = b->m_angle * rot;
			pos += b->m_position;
			scl *= b->m_scale;
		}
		// Set new values
		memcpy(&b->m_angle,    &rot, sizeof(Quaternion));
		memcpy(&b->m_position, &pos, sizeof(vec3));
		memcpy(&b->m_scale,    &scl, sizeof(vec3));
		b->m_state = 1;
		return true;
	} else return false;
}


bool Skeleton::update() {
	// Build combined matrices - may be able to skip some
	bool changed = false;
	memset(m_flags, 0, m_count);
	if(!m_matrices) m_matrices = new Matrix[ m_count ];
	for(int i=0; i<m_count; ++i) {
		Bone* bone = m_bones[i];
		if(bone->getMode() != Bone::FIXED) { // Use local transformation
			if(bone->m_state<8 || (bone->m_parent>=0 && m_flags[bone->m_parent])) {
				m_flags[i] = 1;
				if(bone->m_state==1) bone->updateLocal();
				Bone* parent = bone->getParent();
				vec3 parentScale = parent? parent->m_combinedScale: vec3(1,1,1);
				Matrix local = bone->m_local;
				local[12] *= parentScale.x;
				local[13] *= parentScale.y;
				local[14] *= parentScale.z;
				bone->m_combinedScale = parentScale * bone->m_scale;
				bone->m_combined = parent? parent->m_combined * local: local;

				m_matrices[i] = bone->m_combined;
				m_matrices[i].scale(bone->m_combinedScale);
				m_matrices[i] *= m_rest->skin[i];

				changed = true;
			}
		} else if(bone->m_state==4) {	  // FIXED uses absolute transformation
			changed = true;
			m_flags[i] = 1;
			m_matrices[i] = bone->m_local;
			m_matrices[i].scale(bone->m_scale);
			m_matrices[i] *= m_rest->skin[i];
			bone->m_combinedScale = bone->m_scale;
		}
		bone->m_state |= 8;
	}
	return changed;
}

/*

void Skeleton::reset(int id) {
	Joint* j=getJoint(id);		if(!j) return;
	memcpy(j->m_local, &m_matrices[id], 16*sizeof(float));
	float* m = j->m_local;
	//position
	j->setPosition(&m[12]);
	//scale
	float s[3];
	s[0] = sqrt(m[0]*m[0] + m[1]*m[1] + m[2]*m[2]);
	s[1] = sqrt(m[4]*m[4] + m[5]*m[5] + m[6]*m[6]);
	s[2] = sqrt(m[8]*m[8] + m[9]*m[9] + m[10]*m[10]);
	j->setScale(s);
	//rotation - http://www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToQuaternion/index.htm
	float *q = j->m_rotation; // q[4];
	float trace = m[0] + m[5] + m[10]; // I removed + 1.0f; see discussion with Ethan
	if( trace > 0 ) {// I changed M_EPSILON to 0
		float s = 0.5f / sqrtf(trace+ 1.0f);
		q[3] = 0.25f / s;
		q[0] = ( m[9] - m[6] ) * s;
		q[1] = ( m[2] - m[8] ) * s;
		q[2] = ( m[4] - m[1] ) * s;
	} else {
		if ( m[0]>m[5] && m[0]>m[10] ) {
			float s = 2.0f * sqrtf( 1.0f + m[0] - m[5] - m[10]);
			q[3] = (m[9] - m[6] ) / s;
			q[0] = 0.25f * s;
			q[1] = (m[1] + m[4] ) / s;
			q[2] = (m[2] + m[8] ) / s;
		} else if (m[5] > m[10]) {
			float s = 2.0f * sqrtf( 1.0f + m[5] - m[0] - m[10]);
			q[3] = (m[2] - m[8] ) / s;
			q[0] = (m[1] + m[4] ) / s;
			q[1] = 0.25f * s;
			q[2] = (m[6] + m[9] ) / s;
		} else {
			float s = 2.0f * sqrtf( 1.0f + m[10] - m[0] - m[5] );
			q[3] = (m[4] - m[1] ) / s;
			q[0] = (m[2] + m[8] ) / s;
			q[1] = (m[6] + m[9] ) / s;
			q[2] = 0.25f * s;
		}
	}
}

int Skeleton::animateJoint(int jointID, float frame, unsigned char* flags, float weight) {
	Joint& joint = m_joints[jointID];
	
	//flags and joint mode determine what to do: flag1=active;
	if(!(flags[jointID]&1) && !(flags[joint.m_parent]&1)) return 0; //parent skeleton unchanged
	flags[jointID]|=1; //flag to calculate children;
	if(joint.m_mode==JOINT_TRUNCATE || (flags[joint.m_parent]&2)) flags[jointID] |= 2; //truncate joint
	
	//Is this joint animated? flag2=truncate;
	JointMode mode = joint.getMode();
	bool active = mode==JOINT_ANIMATED || (!(flags[jointID]&2) && (mode==JOINT_DEFAULT || mode==JOINT_OVERRIDE));
	
	//use another flag for inactive joints that need thier absolute transformation updating (if a child is different)
	//dont need to process if nothing has changed
	if(!active && flags[joint.m_parent]&4) flags[jointID] |=4;
	
	if(active && (m_lastAnimation || mode==JOINT_OVERRIDE)) {
		//get animated values
		if(mode!=JOINT_OVERRIDE) {
			float rot[4], scl[3], pos[3];
			m_lastAnimation->getRotation(rot, jointID, frame, m_animationHints[jointID*3  ]);
			m_lastAnimation->getPosition(pos, jointID, frame, m_animationHints[jointID*3+1]);
			m_lastAnimation->getScale   (scl, jointID, frame, m_animationHints[jointID*3+2]);
			//perhaps have animation weight? interpolate between this and its last or initial values.
			//to blend two animations, call animateJoints with the first one at 100%, then call animateJoints with the second animation with a loser weight.
			if(weight==1) {
				memcpy(joint.m_rotation, rot, 4*sizeof(float));
				memcpy(joint.m_position, pos, 3*sizeof(float));
				memcpy(joint.m_scale,    scl, 3*sizeof(float));
			} else {
				//blending
				Math::slerp(joint.m_rotation, joint.m_rotation, rot, weight);
				Math::lerp(joint.m_position,  joint.m_position, pos, weight);
				Math::lerp(joint.m_scale,     joint.m_scale,    scl, weight);
			}
		}
		//create matrix
		float local[16];
		Math::matrix(local, joint.m_rotation, joint.m_scale, joint.m_position);
		//any change?
		//if(!(flags[joint.m_parent]&4) && memcmp(local, joint.m_local, 16*sizeof(float))==0) return 1;
		memcpy(joint.m_local, local, 16*sizeof(float));
		flags[jointID] |= 4;
		//combine with parent
		float* parent = m_joints[ joint.m_parent ].m_combined;
		if(joint.m_parent==jointID) memcpy(joint.m_combined, local, 16*sizeof(float));
		else Math::multiplyAffineMatrix(joint.m_combined, parent, local);
	} else {
		//passthrough default matrix
		float* parent = m_joints[ joint.m_parent ].m_combined;
		float* defaultMatrix = joint.m_local;
		//reinterpret_cast<float*>(&m_matrices[jointID]);
		if(joint.m_parent==jointID) memcpy(joint.m_combined, defaultMatrix, 16*sizeof(float));
		else Math::multiplyAffineMatrix(joint.m_combined, parent, defaultMatrix);
	}
	
	return 1;
}
*/
