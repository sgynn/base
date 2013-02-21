#include "base/model.h"

using namespace base;
using namespace model;


Bone::Bone() : 
	m_skeleton(0), m_index(0), m_parent(0),
	m_name(""), m_length(1), m_mode(DEFAULT), m_state(0) {
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
	m_angle.fromEuler(pyr);
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

const vec3 Bone::getEuler() const {
	return m_angle.getEuler();
}

void Bone::updateLocal() {
	// Note: when absoluteMatrix is being set, local values mean nothing.
	switch(m_state) {
	case 1: // parts to matrix
		m_angle.toMatrix(m_local);
		memcpy(&m_local[12], m_position, sizeof(vec3));
		m_local[0] *= m_scale.x;
		m_local[5] *= m_scale.y;
		m_local[10] *= m_scale.z;
		m_state=3;
		break;
	case 2: // Matrix to parts
		m_angle.fromMatrix(m_local);
		memcpy(m_position, &m_local[12], sizeof(vec3));
		m_scale.x = vec3(&m_local[0]).length();
		m_scale.y = vec3(&m_local[4]).length();
		m_scale.z = vec3(&m_local[8]).length();
		m_state=3;
		break;
	}
}







//// //// //// //// //// //// //// //// //// //// //// //// //// //// ////


Skeleton::Skeleton(): m_count(1), m_last(0) {
	m_bones = new Bone[1];
	m_hints = new uint16[1];
	m_flags = new ubyte[1];
	m_bones[0].m_skeleton = this;
}
Skeleton::Skeleton(const Skeleton& s) : m_count(s.m_count), m_last(0) {
	m_bones = new Bone[ m_count ];
	m_hints = new uint16[ 3 * m_count ];
	m_flags = new ubyte[ m_count ];
	memcpy(m_bones, s.m_bones, m_count*sizeof(Bone));
	memset(m_hints, 0, 3 * m_count * sizeof(uint16));
	for(int i=0; i<m_count; ++i) m_bones[i].m_skeleton = this;
}
Skeleton::~Skeleton() {
	delete [] m_bones;
	delete [] m_hints;
	delete [] m_flags;
}


Bone* Skeleton::addBone(const Bone* p, const char* name, const float* local) {
	// Check parent is valid
	if(!p || p->m_skeleton!=this) {
		p = &m_bones[0];
	}
	// Resize array
	Bone* tmp = m_bones;
	m_bones = new Bone[ m_count+1 ];
	memcpy(m_bones, tmp, m_count*sizeof(Bone));
	delete [] tmp;
	// Resize hint arrays
	delete [] m_hints;
	delete [] m_flags;
	m_flags = new ubyte[ m_count+1 ];
	m_hints = new uint16[ (m_count+1) * 3 ];
	memset(m_hints, 0, (m_count+1) * 3 * sizeof(uint16));
	// Initialise bone data
	Bone& bone = m_bones[m_count];
	bone.m_skeleton = this;
	bone.m_index = m_count;
	bone.m_parent = p->getIndex();
	if(name)  bone.m_name = name;
	// Initial Transforms
	if(local) memcpy(bone.m_local, local, sizeof(Matrix));
	bone.m_combined = p->m_combined * bone.m_local;
	++m_count;
	return &bone;
}

Bone* Skeleton::getBone(const char* name) {
	for(int i=0; i<m_count; ++i) {
		if(strcmp(name, m_bones[i].m_name)==0) return &m_bones[i];
	}
	return 0;
}

void Skeleton::setMode(Bone::Mode m) {
	for(int i=0; i<m_count; ++i) m_bones[i].setMode(m);
}


//// Animate functions ////

void Skeleton::animate(const Animation* anim, float frame, const Bone* root, float weight) {
	m_last = anim;
	int start = root? root->getIndex(): 0;
	memset(m_flags, 0, m_count);
	// Loop through remaining bones
	for(int i=start; i<m_count; ++i) {
		// Determine what to do based on parent flag and bone mode
		int pFlag = i==start? 1: m_flags[ m_bones[i].getParent()->getIndex() ];
		m_flags[i] = pFlag;
		bool set = false;
		switch( m_bones[i].getMode() ) {
		case Bone::DEFAULT:  set = pFlag==1; break;
		case Bone::ANIMATED: set = pFlag>0;  break;
		case Bone::TRUNCATE: m_flags[i]=2;   break;
		case Bone::USER:     break;
		case Bone::FIXED:    break;
		}
		// Set bone values from animation
		if(set) animateBone(&m_bones[i], anim, frame, weight);
	}
}
bool Skeleton::animateBone(Bone* b, const Animation* anim, float frame, float weight) {
	static SlerpFunc slerp;
	static LerpFunc lerp;
	if(b->m_mode==Bone::DEFAULT || b->m_mode==Bone::ANIMATED) {
		vec3 pos, scl;
		Quaternion rot;
		// Get values from animation
		int index = b->getIndex();
		anim->getRotation(index, frame, m_hints[index*3],   rot);
		anim->getPosition(index, frame, m_hints[index*3+1], pos);
		anim->getScale   (index, frame, m_hints[index*3+2], scl);
		// Blended?
		if(weight != 1) {
			if(b->m_state==2) b->updateLocal();
			slerp(rot, b->m_angle, rot, weight);
			lerp(pos, b->m_position, pos, weight);
			lerp(scl, b->m_scale, scl, weight);
		}
		memcpy(&b->m_angle,    &rot, sizeof(Quaternion));
		memcpy(&b->m_position, &pos, sizeof(vec3));
		memcpy(&b->m_scale,    &scl, sizeof(vec3));
		b->m_state = 1;
		return true;
	} else return false;
}


void Skeleton::update() {
	// Build combined matrices - may be able to skip some
	bool skip = true;
	for(int i=0; i<m_count; ++i) {
		Bone& bone = m_bones[i];
		if(bone.getMode() != Bone::FIXED || (skip && bone.m_state==0)) {
			Bone& parent = m_bones[ bone.m_parent ];
			if(bone.m_state==1) bone.updateLocal(); // Update local matrix
			// Combine ...
			bone.m_combined = parent.m_combined * bone.m_local;
			skip = false;
		}
	}
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
