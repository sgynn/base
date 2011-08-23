#ifndef NO_MODEL_DRAW
# ifdef WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
# endif
# include <GL/gl.h>
# include "shader.h"
#endif

#include "model.h"
#include <cstring>


using namespace base;
using namespace model;

#define ResizeArray(type, array, oldSize, newSize)	{ type* tmp=array; array=new type[newSize]; if(oldSize) { memcpy(array, tmp, oldSize*sizeof(type)); delete [] tmp; } }
//Can probably optimise this as the bottom row will always be {0,0,0,1}
#define MultiplyMatrix(out, a, b)	{ 	out[0]  = a[0]*b[0]  + a[4]*b[1]  + a[8]*b[2]   + a[12]*b[3];  \
						out[1]  = a[1]*b[0]  + a[5]*b[1]  + a[9]*b[2]   + a[13]*b[3];  \
						out[2]  = a[2]*b[0]  + a[6]*b[1]  + a[10]*b[2]  + a[14]*b[3];  \
						out[3]  = a[3]*b[0]  + a[7]*b[1]  + a[11]*b[2]  + a[15]*b[3];  \
						out[4]  = a[0]*b[4]  + a[4]*b[5]  + a[8]*b[6]   + a[12]*b[7];  \
						out[5]  = a[1]*b[4]  + a[5]*b[5]  + a[9]*b[6]   + a[13]*b[7];  \
						out[6]  = a[2]*b[4]  + a[6]*b[5]  + a[10]*b[6]  + a[14]*b[7];  \
						out[7]  = a[3]*b[4]  + a[7]*b[5]  + a[11]*b[6]  + a[15]*b[7];  \
						out[8]  = a[0]*b[8]  + a[4]*b[9]  + a[8]*b[10]  + a[12]*b[11]; \
						out[9]  = a[1]*b[8]  + a[5]*b[9]  + a[9]*b[10]  + a[13]*b[11]; \
						out[10] = a[2]*b[8]  + a[6]*b[9]  + a[10]*b[10] + a[14]*b[11]; \
						out[11] = a[3]*b[8]  + a[7]*b[9]  + a[11]*b[10] + a[15]*b[11]; \
						out[12] = a[0]*b[12] + a[4]*b[13] + a[8]*b[14]  + a[12]*b[15]; \
						out[13] = a[1]*b[12] + a[5]*b[13] + a[9]*b[14]  + a[13]*b[15]; \
						out[14] = a[2]*b[12] + a[6]*b[13] + a[10]*b[14] + a[14]*b[15]; \
						out[15] = a[3]*b[12] + a[7]*b[13] + a[11]*b[14] + a[15]*b[15]; }
						
#define Lerp(out, a, b, v)	{		out[0]=a[0]+(b[0]-a[0])*v;	out[1]=a[1]+(b[1]-a[1])*v;	out[2]=a[2]+(b[2]-a[2])*v; }

#define SLerp(out, a, b, v)	{		float w1, w2; \
						float dot = a[0]*b[0] + a[1]*b[1] + a[2]*b[2] + a[3]*b[3]; \
						float theta = acos(dot); \
						float sintheta = sin(theta); \
						if (sintheta > 0.001) {	w1=sin((1.0f-v)*theta) / sintheta; w2 = sin(v*theta) / sintheta; } \
						else { w1 = 1.0f - v; w2 = v; } \
						out[0]=a[0]*w1+b[0]*w2; \
						out[1]=a[1]*w1+b[1]*w2; \
						out[2]=a[2]*w1+b[2]*w2; \
						out[3]=a[3]*w1+b[3]*w2; }

Mesh::Mesh() : m_format(VERTEX_DEFAULT), m_formatSize(8), m_drawMode(GL_TRIANGLES), m_vertexBuffer(0), m_indexBuffer(0), m_skinBuffer(0), m_referenceCount(0) {}
Mesh::Mesh(const Mesh& m, bool copy, bool skin) : m_format(m.m_format), m_formatSize(m.m_formatSize), m_drawMode(GL_TRIANGLES), m_referenceCount(0) {
	//vertex format data
	m_format = m.m_format;
	m_formatSize = m.m_formatSize;
	//copy or reference vertex array
	if(m.m_vertexBuffer) {
		if(copy) {
			m_vertexBuffer = new Buffer<float>();
			m_vertexBuffer->data = new float[ m.m_vertexBuffer->size * m.m_formatSize ];
			memcpy(m_vertexBuffer->data, m.m_vertexBuffer->data, m.m_vertexBuffer->size * m.m_vertexBuffer->stride);
			m_vertexBuffer->size = m.m_vertexBuffer->size;
			m_vertexBuffer->stride = m.m_vertexBuffer->stride;
			m_vertexBuffer->referenceCount = 1;
		} else {
			m_vertexBuffer = m.m_vertexBuffer;
			m_vertexBuffer->referenceCount++;
		}
	} else m_vertexBuffer = 0;
	
	//Indices if they exist
	if(m.m_indexBuffer) {
		m_indexBuffer = m.m_indexBuffer;
		m_indexBuffer->referenceCount++;
	} else m_indexBuffer = 0;
	
	//and skins? If we move vertices, the skin becomes invalid.
	if(m.m_skinBuffer && skin) {
		m_skinBuffer = m.m_skinBuffer;
		m_skinBuffer->referenceCount++;
	} else  m_skinBuffer = 0;
	
	//copy material
	m_material = m.m_material;
	memcpy(m_box, m.m_box, 6*sizeof(float)); //Bounding box
}
Mesh::~Mesh() {
	//delete buffers of not referenced elsewhere
	//also need to clear any VBOs if used
	if(m_vertexBuffer) {
		if(--m_vertexBuffer->referenceCount<=0) {
			delete [] m_vertexBuffer->data;
			delete m_vertexBuffer;
		}
	}
	
	if(m_indexBuffer) {
		if(--m_indexBuffer->referenceCount<=0) {
			delete [] m_indexBuffer->data;
			delete m_indexBuffer;
		}
	}
	//delete skins
	if(m_skinBuffer) {
		if(--m_skinBuffer->referenceCount<=0) {
			for(unsigned int i=0; i<m_skinBuffer->size; i++) {
				delete [] m_skinBuffer->data[i]->indices;
				delete [] m_skinBuffer->data[i]->weights;
				if(m_skinBuffer->data[i]->jointName) delete [] m_skinBuffer->data[i]->jointName;
				delete m_skinBuffer->data[i];
			}
			delete [] m_skinBuffer->data;
			delete m_skinBuffer;
		}
	}
}
int Mesh::grab() { return ++m_referenceCount; }
int Mesh::drop() {
	int r = --m_referenceCount;
	if(r<=0) delete this;
	return r;
}

void Mesh::useBufferObject(bool use) {}
void Mesh::updateBufferObject() {}
int Mesh::bindBuffer() const { return 0; }

void Mesh::changeFormat(VertexFormat format) {
	if(format==m_format) return;
	//resize vertex array, copy data
	int newFormatSize = format==VERTEX_DEFAULT? 8: format==VERTEX_SIMPLE? 3: 11;
	float* tmp = m_vertexBuffer->data;
	
	m_vertexBuffer->data = new float[m_vertexBuffer->size * newFormatSize];
	if(newFormatSize>m_formatSize) memset(m_vertexBuffer->data, 0, m_vertexBuffer->size*newFormatSize*sizeof(float));
	int copySize = (newFormatSize<m_formatSize? newFormatSize: m_formatSize) * sizeof(float);
	for(unsigned int i=0; i<m_vertexBuffer->size; i++) {
		memcpy(&m_vertexBuffer->data[i*newFormatSize], &tmp[i*m_formatSize],  copySize);
	}
	
	m_formatSize = newFormatSize;
	m_vertexBuffer->stride = m_formatSize * sizeof(float);
	m_format = format;
	delete [] tmp;
	//TODO: Update VBO if it exists
}

/** Data Set Functions */
void Mesh::setVertices(int count, float* data, VertexFormat format) {
	m_format = format;
	m_formatSize = format==VERTEX_DEFAULT? 8: format==VERTEX_SIMPLE? 3: 11;
	m_vertexBuffer = new Buffer<float>();
	m_vertexBuffer->data = data;
	m_vertexBuffer->size = count;
	m_vertexBuffer->stride = m_formatSize*sizeof(float);
	m_vertexBuffer->referenceCount = 1;
	updateBox();
}
void Mesh::setVertices(Buffer<float>* buffer, VertexFormat format) {
	m_format = format;
	m_formatSize = format==VERTEX_DEFAULT? 8: format==VERTEX_SIMPLE? 3: 11;
	m_vertexBuffer = buffer;
	buffer->referenceCount++;
	updateBox();
}
void Mesh::setIndices(int count, const unsigned short* data) {
	m_indexBuffer = new Buffer<const unsigned short>();
	m_indexBuffer->data = data;
	m_indexBuffer->size = count;
	m_indexBuffer->referenceCount = 1;
}
void Mesh::setIndices(Buffer<const unsigned short>* buffer) {
	m_indexBuffer = buffer;
	buffer->referenceCount++;
}

void Mesh::addSkin(Skin* skin) {
	if(!m_skinBuffer) {
		m_skinBuffer = new Buffer<Skin*>();
		m_skinBuffer->referenceCount = 1; //grab pointer
	}
	//resize skin array
	Skin** tmp = m_skinBuffer->data;
	m_skinBuffer->data = new Skin*[m_skinBuffer->size+1];
	if(m_skinBuffer->size) { memcpy(m_skinBuffer->data, tmp, m_skinBuffer->size*sizeof(Skin*) ); delete [] tmp; }
	//add new skin
	m_skinBuffer->data[ m_skinBuffer->size ] = skin;
	m_skinBuffer->size++;
}

/** Data Processing */
int Mesh::calculateTangents() {
	changeFormat( VERTEX_TANGENT );
	size_t s = m_formatSize;
	if(m_indexBuffer) {
		for(uint i=0; i<m_indexBuffer->size; i+=3) {
			float* a = m_vertexBuffer->data + m_indexBuffer->data[i]*s;
			float* b = m_vertexBuffer->data + m_indexBuffer->data[i+1]*s;
			float* c = m_vertexBuffer->data + m_indexBuffer->data[i+2]*s;
			
			tangent(a,b,c, a+8);
			memcpy(b+8, a+8, 3*sizeof(float));
			memcpy(c+8, a+8, 3*sizeof(float));
		}
	} else if(m_vertexBuffer) {
		for(uint i=0; i<m_vertexBuffer->size; i+=3) {
			float* a = m_vertexBuffer->data + i*s;
			float* b = m_vertexBuffer->data + (i+1)*s;
			float* c = m_vertexBuffer->data + (i+2)*s;

			tangent(a,b,c, a+8);
			memcpy(b+8, a+8, 3*sizeof(float));
			memcpy(c+8, a+8, 3*sizeof(float));
		}
	} else return 0;
	return 1;
}
int Mesh::normaliseWeights()  {  return 0; }

int Mesh::tangent(const float* a, const float* b, const float* c, float* t) {
	float ab[3] = { b[0]-a[0], b[1]-a[1], b[2]-a[2] };
	float ac[3] = { c[0]-a[0], c[1]-a[1], c[2]-a[2] };
	const float* n = a+3; //Normal

	//Project ab,ac onto normal
	float abn = ab[0]*n[0] + ab[1]*n[1] + ab[2]*n[2];
	float acn = ac[0]*n[0] + ac[1]*n[1] + ac[2]*n[2];
	ab[0]-=n[0]*abn; ab[1]-=n[1]*abn; ab[2]-=n[2]*abn;
	ac[0]-=n[0]*acn; ac[1]-=n[1]*acn; ac[2]-=n[2]*acn;

	//Texture coordinate deltas
	float abu = b[6] - a[6];
	float abv = b[7] - a[7];
	float acu = c[6] - a[6];
	float acv = c[7] - a[7];
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

void Mesh::updateBox() {
	const float* v = getVertex(0);
	m_box[0] = m_box[3] = v[0];
	m_box[1] = m_box[4] = v[1];
	m_box[2] = m_box[5] = v[2];
	for(int i=1; i<getVertexCount(); i++) {
		v = getVertex(i);
		if(v[0] < m_box[0])	 m_box[0]=v[0];
		else if(v[0] > m_box[3]) m_box[3]=v[0];
		if(v[1] < m_box[1])	 m_box[1]=v[1];
		else if(v[1] > m_box[4]) m_box[4]=v[1];
		if(v[2] < m_box[2])	 m_box[2]=v[2];
		else if(v[2] > m_box[5]) m_box[5]=v[2];
	}
}

/** *************************** *************************** ************************ ********************* ************************** ********* **/
static const float IdentityMatrix[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
Joint::Joint() : m_id(-1), m_parent(0), m_mode(JOINT_DEFAULT), m_length(1) {
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


/** *************************** *************************** ************************ ********************* ************************** ********* **/
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
	m_joints[m_jointCount-1].m_id = m_jointCount-1;
	m_joints[m_jointCount-1].m_parent = parent;
	if(defaultMatrix) setDefaultMatrix(m_jointCount-1, defaultMatrix);
	memset(&m_animationHints[(m_jointCount-1)*3], 0, 3*sizeof(int));
	m_names[name] = m_jointCount-1; //link jointID to Name
	//return id
	return m_jointCount-1;
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
				SLerp(joint.m_rotation, joint.m_rotation, rot, weight);
				Lerp(joint.m_position,  joint.m_position, pos, weight);
				Lerp(joint.m_scale,     joint.m_scale,    scl, weight);
			}
		}
		#define q joint.m_rotation //shorthand
		#define s joint.m_scale
		//create matrix
		float local[16];
		// First row
		local[ 0] = (1.0f - 2.0f * ( q[1] * q[1] + q[2] * q[2] )) * s[0]; 
		local[ 1] = 2.0f * (q[0] * q[1] + q[2] * q[3]);
		local[ 2] = 2.0f * (q[0] * q[2] - q[1] * q[3]);
		local[ 3] = 0.0f;
		// Second row
		local[ 4] = 2.0f * ( q[0] * q[1] - q[2] * q[3] );  
		local[ 5] = (1.0f - 2.0f * ( q[0] * q[0] + q[2] * q[2] )) * s[1]; 
		local[ 6] = 2.0f * (q[2] * q[1] + q[0] * q[3] );  
		local[ 7] = 0.0f;
		// Third row
		local[ 8] = 2.0f * ( q[0] * q[2] + q[1] * q[3] );
		local[ 9] = 2.0f * ( q[1] * q[2] - q[0] * q[3] );
		local[10] = (1.0f - 2.0f * ( q[0] * q[0] + q[1] * q[1] )) * s[2];  
		local[11] = 0.0f;  
		// Fourth row
		local[12] = joint.m_position[0];
		local[13] = joint.m_position[1];
		local[14] = joint.m_position[2];
		local[15] = 1.0f;
		//any change?
		//if(!(flags[joint.m_parent]&4) && memcmp(local, joint.m_local, 16*sizeof(float))==0) return 1;
		memcpy(joint.m_local, local, 16*sizeof(float));
		flags[jointID] |= 4;
		//combine with parent
		float* parent = m_joints[ joint.m_parent ].m_combined;
		if(joint.m_parent==jointID) memcpy(joint.m_combined, local, 16*sizeof(float));
		else MultiplyMatrix(joint.m_combined, parent, local);
	} else {
		//passthrough default matrix
		float* parent = m_joints[ joint.m_parent ].m_combined;
		float* defaultMatrix = joint.m_local;
		//reinterpret_cast<float*>(&m_matrices[jointID]);
		if(joint.m_parent==jointID) memcpy(joint.m_combined, defaultMatrix, 16*sizeof(float));
		else MultiplyMatrix(joint.m_combined, parent, defaultMatrix);
	}
	
	return 1;
}


/** *************************** *************************** ************************ ********************* ************************** ********* **/

Animation::Animation() : m_animations(0), m_count(0), m_start(0), m_end(0) {}
Animation::~Animation() {}
Animation::JointAnimation& Animation::getJointAnimation(int jointID) {
	if(jointID>=m_count) {
		//resize array
		ResizeArray(JointAnimation, m_animations, m_count, jointID+1)
		//initialise new animations
		for(int i=m_count; i<=jointID; i++) {
			m_animations[i].rotationCount=0;
			m_animations[i].positionCount=0;
			m_animations[i].scaleCount=0;
		}
		m_count = jointID+1;
	}
	return m_animations[jointID];
}

int Animation::getRotation(float*rot, int joint, float frame, unsigned int& hint) const {
	unsigned int count = m_animations[joint].rotationCount;
	KeyFrame4* keys = m_animations[joint].rotationKeys;
	
	//trivial cases if there is only one keyframe, or none at all.
	if(count==0) {
		rot[0]=0; rot[1]=0; rot[2]=0; rot[3]=0;
	} else if(count==1) {
		memcpy(rot, keys[0].data, 4*sizeof(float));
	} else {	
		//get keyframe index
		if((hint<count && keys[hint].frame<frame) || (hint>0 && keys[hint-1].frame>frame)) {
			if(frame>keys[count-1].frame) hint=count;
			else for(unsigned int i=0; i<count; i++) if(keys[i].frame>frame) { hint = i; break; }
		}
		//interpolate
		if(hint==0) memcpy(rot, keys[0].data, 4*sizeof(float));
		else if(hint==count) memcpy(rot, keys[count-1].data, 4*sizeof(float));
		else {
			float v = (frame - keys[hint-1].frame) / (keys[hint].frame-keys[hint-1].frame);
			SLerp(rot, keys[hint-1].data, keys[hint].data, v);
		}
	}
	return 1;
}
int Animation::getPosition(float*pos, int joint, float frame, unsigned int& hint) const {
	unsigned int count = m_animations[joint].positionCount;
	KeyFrame3* keys = m_animations[joint].positionKeys;
	
	//trivial cases if there is only one keyframe, or none at all.
	if(count==0) {
		pos[0]=0; pos[1]=0; pos[2]=0;
	} else if(count==1) {
		memcpy(pos, keys[0].data, 3*sizeof(float));
	} else {	
		//get keyframe index
		if((hint<count && keys[hint].frame<frame) || (hint>0 && keys[hint-1].frame>frame)) {
			if(frame>keys[count-1].frame) hint=count;
			else for(unsigned int i=0; i<count; i++) if(keys[i].frame>frame) { hint = i; break; }
		}
		//interpolate
		if(hint==0) memcpy(pos, keys[0].data, 3*sizeof(float));
		else if(hint==count) memcpy(pos, keys[count-1].data, 3*sizeof(float));
		else {
			float v = (frame - keys[hint-1].frame) / (keys[hint].frame-keys[hint-1].frame);
			Lerp(pos, keys[hint-1].data, keys[hint].data, v);
		}
	}
	return 1;
}
int Animation::getScale(float*scl, int joint, float frame, unsigned int& hint) const {
	unsigned int count = m_animations[joint].scaleCount;
	KeyFrame3* keys = m_animations[joint].scaleKeys;
	
	//trivial cases if there is only one keyframe, or none at all.
	if(count==0) {
		scl[0]=1; scl[1]=1; scl[2]=1;
	} else if(count==1) {
		memcpy(scl, keys[0].data, 3*sizeof(float));
	} else {	
		//get keyframe index
		if((hint<count && keys[hint].frame<frame) || (hint>0 && keys[hint-1].frame>frame)) {
			if(frame>keys[count-1].frame) hint=count;
			else for(unsigned int i=0; i<count; i++) if(keys[i].frame>frame) { hint = i; break; }
		}
		//interpolate
		if(hint==0) memcpy(scl, keys[0].data, 3*sizeof(float));
		else if(hint==count) memcpy(scl, keys[count-1].data, 3*sizeof(float));
		else {
			float v = (frame - keys[hint-1].frame) / (keys[hint].frame-keys[hint-1].frame);
			Lerp(scl, keys[hint-1].data, keys[hint].data, v);
		}
	}
	return 1;
}

int Animation::getRange(int& start, int& end) {
	if(m_start==0 && m_end==0) {
		for(int i=0; i<m_count; i++) {
			int k0 = m_animations[i].positionCount? m_animations[i].positionKeys[ m_animations[i].positionCount-1 ].frame: 0;
			int k1 = m_animations[i].rotationCount? m_animations[i].rotationKeys[ m_animations[i].rotationCount-1 ].frame: 0;
			int k2 =m_animations[i].scaleCount?  m_animations[i].scaleKeys[ m_animations[i].scaleCount-1 ].frame: 0;
			if(k0>m_end) m_end=k0;
			if(k1>m_end) m_end=k1;
			if(k2>m_end) m_end=k2;
		}
	}
	start=m_start;
	end=m_end;
	return m_end-m_start;
}



/** *************** ******************** ************************ **************************** *********************** ************************ **/

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

Mesh* Model::skinMesh(Mesh* out, const Mesh* source, const Skeleton* skeleton) {
	Mesh* dest = out;
	if(!dest) dest = new Mesh(*source, true, false);
	
	//Validate destination mesh
	if(dest->getIndexPointer()!=source->getIndexPointer() || dest->getVertexCount() != source->getVertexCount()) return const_cast<Mesh*>(source);
	
	//skeleton exists?
	if(!skeleton) return dest;
	
	//Now the fun part.
	for(int i=0; i<dest->getVertexCount(); i++) {
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
		MultiplyMatrix(mat, combined, skin->offset);
		
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


/** ********* ***********	Automation stuff	********** ************* ************* *********** ********** ******* ******* ********* **** **/

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

void Model::draw() {
	#ifndef NO_MODEL_DRAW
	int flags=0;
	
	//Draw all meshes
	Joint* joint = 0;
	for(int i=0; i<m_meshCount; i++) {
		if(m_meshes[i].flag==MESH_OUTPUT) {
			Mesh* mesh = m_meshes[i].mesh;
			mesh->getMaterial().bind();
			
			//transform - Need to know the joint this mesh is attached to.
			if(m_skeleton && m_meshes[i].joint) joint = m_skeleton->getJoint( m_meshes[i].joint ); else joint=0;
			if(joint) {
				glPushMatrix();
				glMultMatrixf( joint->getTransformation() );
			}
			
			drawMesh(mesh, flags);
			
			if(joint) glPopMatrix();
		}
	}
	
	if(flags&1) glDisableClientState(GL_VERTEX_ARRAY);
	if(flags&2) glDisableClientState(GL_NORMAL_ARRAY);
	if(flags&4) glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	#endif
}

void Model::drawMesh(const Mesh* mesh) {
	#ifndef NO_MODEL_DRAW
	int flags=0;
	drawMesh(mesh, flags);
	//disable client states afterwards
	if(flags&1) glDisableClientState(GL_VERTEX_ARRAY);
	if(flags&2) glDisableClientState(GL_NORMAL_ARRAY);
	if(flags&4) glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	#endif
}

void Model::drawMesh(const Mesh* mesh, int& glFlags) {
	#ifndef NO_MODEL_DRAW
	//enable client states
	if((glFlags&1)==0) { glEnableClientState(GL_VERTEX_ARRAY); glFlags |= 1; }
	if((glFlags&2)==0 && mesh->getFormat()!=VERTEX_SIMPLE) { glEnableClientState(GL_NORMAL_ARRAY); glFlags |= 2; }
	if((glFlags&4)==0 && mesh->getFormat()!=VERTEX_SIMPLE) { glEnableClientState(GL_TEXTURE_COORD_ARRAY); glFlags |= 4; }
	//bind material
	const_cast<Mesh*>(mesh)->getMaterial().bind();
	//set pointers
	glVertexPointer(3, GL_FLOAT, mesh->getStride(), mesh->getVertexPointer());
	glNormalPointer(GL_FLOAT, mesh->getStride(), mesh->getNormalPointer());
	glTexCoordPointer(2, GL_FLOAT, mesh->getStride(), mesh->getTexCoordPointer());
	//Tangent pointer
	if(mesh->getTangentPointer()) {
		Shader::current().AttributePointer("tangent", 3, GL_FLOAT, 0, mesh->getStride(), mesh->getTangentPointer());
	}

	//draw it!
	glDrawElements(mesh->getMode(), mesh->getPolygonCount()*3, GL_UNSIGNED_SHORT, mesh->getIndexPointer() );
	#endif
}

void Model::drawSkeleton(const Skeleton* skeleton) {
	#ifndef NO_MODEL_DRAW
	if(!skeleton || !skeleton->getJointCount()) return;
	
	//Joint model
	static float vx[18] = { 0,0,0,  0.06f,0.06f,0.1f, 0.06f,-0.06f,0.1f, -0.06f,-0.06f,0.1f, -0.06f,0.06f,0.1f, 0,0,1 };
	static unsigned char ix[12] = { 1,5,2, 1,0,2,  3,5,4, 3,0,4 };
	
	//Set up openGL
	Material().bind();
	glDisable(GL_DEPTH_TEST);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, vx);
	
	for(int i=0; i<skeleton->getJointCount(); i++) {
		const model::Joint* joint = skeleton->getJoint(i);
		glPushMatrix();
		//get the matrix of this joint. matrix is in the model local coordinates. need to transform if we move the model.
		glMultMatrixf( joint->getTransformation() );
		glScalef(joint->getLength(), joint->getLength(), joint->getLength());
		glColor3f(0.4, 0, 1);
		glDrawElements(GL_LINE_LOOP, 12, GL_UNSIGNED_BYTE, ix);
		glPopMatrix();
	}
	
	glDisableClientState(GL_VERTEX_ARRAY);
	glEnable(GL_DEPTH_TEST);
	
	#endif
}


