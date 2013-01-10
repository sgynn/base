#include "base/model.h"
#include "modelmath.h"

using namespace base;
using namespace model;


Joint::Joint() : m_id(-1), m_parent(0), m_mode(JOINT_DEFAULT), m_length(1) {
	static const float IdentityMatrix[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
	memcpy(m_combined, IdentityMatrix, 16*sizeof(float));
	memset(m_rotation, 0, 4*sizeof(float));
	memset(m_position, 0, 3*sizeof(float));
	m_scale[0]=1; m_scale[1]=1; m_scale[2]=1;
}
void Joint::getTransformation(float* matrix) const {
	memcpy(matrix, m_combined, 16*sizeof(float));
}
void Joint::setPosition(const float* p) {
	memcpy(m_position, p, 3*sizeof(float));
}
void Joint::setRotation(const float* r) {
	memcpy(m_rotation, r, 4*sizeof(float));
}
void Joint::setScale(const float* s) {
	memcpy(m_scale, s, 3*sizeof(float));
}


//// //// //// //// //// //// //// //// //// //// //// //// //// //// ////


Skeleton::Skeleton() : m_jointCount(0), m_lastAnimation(0) {}
Skeleton::Skeleton(const Skeleton& s) : m_jointCount(s.m_jointCount), m_names(s.m_names), m_lastAnimation(s.m_lastAnimation) {
	//copy joints
	m_joints = new Joint[ m_jointCount ];
	memcpy(m_joints, s.m_joints, m_jointCount*sizeof(Joint));
	//copy matrices
	m_matrices = new Matrix[m_jointCount];
	memcpy(m_matrices, s.m_matrices, m_jointCount * sizeof(Matrix));
	//copy animationHints
	m_animationHints = new unsigned int[m_jointCount*3];
	memcpy(m_animationHints, s.m_animationHints, m_jointCount*3*sizeof(unsigned int));
}
Skeleton::~Skeleton() {
	if(m_jointCount) {
		delete [] m_joints;
		delete [] m_matrices;
		delete [] m_animationHints;
	}
}
int Skeleton::addJoint(int parent, const char* name, const float* defaultMatrix) {
	int newSize = m_jointCount+1;
	ResizeArray( Joint, m_joints, m_jointCount, newSize);
	ResizeArray( Matrix, m_matrices, m_jointCount, newSize);
	ResizeArray( unsigned int, m_animationHints, m_jointCount*3, newSize*3);
	m_jointCount = newSize;
	//initialise new joint
	int j = m_jointCount-1;
	m_joints[j].m_id = m_jointCount-1;
	m_joints[j].m_parent = parent;
	if(defaultMatrix) setDefaultMatrix(j, defaultMatrix);
	memset(&m_animationHints[(m_jointCount-1)*3], 0, 3*sizeof(int));
	m_names[name] = j; //link jointID to Name
	//return joint
	return j;
}
void Skeleton::setDefaultMatrix(int joint, const float* matrix) {
	memcpy(&m_matrices[joint], matrix, 16*sizeof(float) );
	reset(joint);
}
void Skeleton::reset() {
	for(int i=0; i<m_jointCount; i++) reset(i);
}
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
Joint* Skeleton::getJoint(int id) {
	if(id>=0 && id<m_jointCount) return &m_joints[id];
	else return 0;
}
const Joint* Skeleton::getJoint(int id) const {
	if(id>=0 && id<m_jointCount) return &m_joints[id];
	else return 0;
}
Joint* Skeleton::getJoint(const char* name) {
	return getJoint( m_names[name] );
}
const Joint* Skeleton::getJoint(const char* name) const {
	return getJoint( m_names[name] );
}
Joint* Skeleton::getParent(const Joint* j) {
	if(j->m_parent<0 || m_joints[j->m_parent].m_id >= j->m_id) return 0;
	else return &m_joints[j->m_parent];
}
int Skeleton::animateJoints(float frame, int joint, float weight) {
	return animateJoints(m_lastAnimation, frame, joint, weight);
}
int Skeleton::animateJoints(const Animation* animation, float frame, int joint, float weight) {
	m_lastAnimation = animation;
	unsigned char* flags = new unsigned char[m_jointCount];
	memset(flags, 0, m_jointCount);
	flags[joint] = 1; //animate from this joint
	for(int i=joint; i<m_jointCount; i++) animateJoint(i, frame, flags, weight);
	return 0;
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

