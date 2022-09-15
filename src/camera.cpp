#include "base/opengl.h"
#include "base/camera.h"

#include <cstdio>

using namespace base;

Camera::Camera() : m_mode(0), m_left(-1), m_right(1), m_bottom(-1), m_top(1), m_near(-1), m_far(1) {
}

Camera::Camera(int width, int height) : m_mode(0), m_near(-1), m_far(1) {
	m_left = m_bottom = 0;
	m_right = width;
	m_top = height;
	updateProjectionMatrix();
}

Camera::Camera(float fov, float aspect, float near, float far)
	: m_mode(1), m_fov(fov), m_aspect(aspect), m_near(near), m_far(far) {
	updateProjectionMatrix();
}


Camera::Camera(float left, float right, float bottom, float top, float near, float far)
	: m_mode(2), m_left(left), m_right(right), m_bottom(bottom), m_top(top), m_near(near), m_far(far) {
	updateProjectionMatrix();
}

void Camera::setProjection(int mode, float l, float r, float t, float b, float n, float f, float v, float a) {
	m_mode = mode;
	m_left = l;
	m_right = r;
	m_top = t;
	m_bottom = b;
	m_near = n;
	m_far = f;
	m_fov = v;
	m_aspect = a;
	updateProjectionMatrix();
}

void Camera::setOrthographic(int width, int height) {
	setProjection(0, 0,width,0,height,-1,1,0,0);
}
void Camera::setOrthographic(float width, float height, float near, float far) {
	setProjection(0, -width/2,width/2,-height/2,height/2,near,far,0,0);
}
void Camera::setPerspective(float fov, float aspect, float near, float far) {
	setProjection(1,0,0,0,0,near,far,fov,aspect);
}
void Camera::setPerspective(float left, float right, float bottom, float top, float near, float far) {
	setProjection(2,left,right,top,bottom,near,far,0,0);
}

void Camera::adjustDepth(float near, float far) {
	m_near=near;
	m_far=far;
	updateProjectionMatrix();
}

void Camera::setFov(float fov) {
	m_fov = fov;
	updateProjectionMatrix();
}

void Camera::setAspect(float aspect) {
	m_aspect = aspect;
	updateProjectionMatrix();
}

void Camera::lookat(const vec3& pos, const vec3& tgt, const vec3& up) {
	m_position = pos;
	
	//vectors overlapping matrix
	vec3& x = *reinterpret_cast<vec3*>(&m_rotation[0]);
	vec3& y = *reinterpret_cast<vec3*>(&m_rotation[4]);
	vec3& z = *reinterpret_cast<vec3*>(&m_rotation[8]);
	
	//calculate matrix
	z = (pos-tgt);
	y = up;
	z.normalise();
	y.normalise();
	x = y.cross(z).normalise();
	y = z.cross(x).normalise();
}

void Camera::setRotation(float pitch, float yaw, float roll) {
	vec3 fwd;
	fwd = vec3(sin(yaw), 0, cos(yaw)) * cos(pitch);
	fwd.y = sin(pitch);
	lookat(m_position, m_position+fwd, vec3(0,1,0));
	// roll - rotate in local Z axis
	if(roll!=0) rotateLocal(2, roll);
}

void Camera::setTransform(const Matrix& m) {
	m_position = vec3(&m[12]);
	m_rotation = m;
	m_rotation[12] = m_rotation[13] = m_rotation[14] = 0;
}

void Camera::setTransform(const vec3& pos, const Quaternion& rot) {
	rot.toMatrix(m_rotation);
	m_position = pos;
}

/** Rotate a matrix about a local axis. Must be a rotation matrix */
void Camera::rotateLocal(int axis, float radians) {
	Matrix& mat = m_rotation;
	//New axis vector
	vec3 v;
	switch(axis) {
	case 0: v = vec3(0,cos(radians),-sin(radians)); break; //X axis
	case 1: v = vec3(sin(radians),0,cos(radians)); break; //Y axis
	case 2: v = vec3(cos(radians),sin(radians),0); break; //Z axis
	default: return;
	}
	//Rotate axis vector
	vec3 a, b, c( &mat[axis*4] );
	a.x = mat[0]*v.x + mat[4]*v.y + mat[8]*v.z;
	a.y = mat[1]*v.x + mat[5]*v.y + mat[9]*v.z;
	a.z = mat[2]*v.x + mat[6]*v.y + mat[10]*v.z;
	//Rebuild matrix
	b = c.cross(a.normalise());
	int ia = ((axis+1) % 3) * 4;
	int ib = ((axis+2) % 3) * 4;
	//Copy back
	mat[ia]=a.x; mat[ia+1]=a.y; mat[ia+2]=a.z;
	mat[ib]=b.x; mat[ib+1]=b.y; mat[ib+2]=b.z;
}

/// Frustum Stuff ///
#define FLEFT	0
#define FRIGHT	1
#define FBOTTOM	2
#define FTOP	3
#define FNEAR	4
#define FFAR	5

#include <iostream>
void Camera::updateFrustum() {
	//calculate combined matrix
	Matrix modelview = m_rotation.transposed();
	//translation
	vec3 t = modelview * (m_position * -1);
	modelview.translate(t);

	//Multiply matrices
	m_combined = m_projection * modelview;


	//generate frustum planes.
	#define m m_combined
	Camera::Plane* p;
	p = &m_frustum[FLEFT];
	p->n.x = m[ 3]+m[ 0];
	p->n.y = m[ 7]+m[ 4];
	p->n.z = m[11]+m[ 8];
	p->d   = m[15]+m[12];

	p = &m_frustum[FRIGHT];
	p->n.x = m[ 3]-m[ 0];
	p->n.y = m[ 7]-m[ 4];
	p->n.z = m[11]-m[ 8];
	p->d   = m[15]-m[12];

	p = &m_frustum[FBOTTOM];
	p->n.x = m[ 3]+m[ 1];
	p->n.y = m[ 7]+m[ 5];
	p->n.z = m[11]+m[ 9];
	p->d   = m[15]+m[13];
	
	p = &m_frustum[FTOP];
	p->n.x = m[ 3]-m[ 1];
	p->n.y = m[ 7]-m[ 5];
	p->n.z = m[11]-m[ 9];
	p->d   = m[15]-m[13];

	p = &m_frustum[FNEAR];
	p->n.x = m[ 3]+m[ 2];
	p->n.y = m[ 7]+m[ 6];
	p->n.z = m[11]+m[10];
	p->d   = m[15]+m[14];

	p = &m_frustum[FFAR];
	p->n.x = m[ 3]-m[ 2];
	p->n.y = m[ 7]-m[ 6];
	p->n.z = m[11]-m[10];
	p->d   = m[15]-m[14];

	//Normalise planes
	for(int i=0; i<6; i++) {
		float l = 1.0 / m_frustum[i].n.length();
		m_frustum[i].n *= l;
		m_frustum[i].d *= l;
	}
}
const float* Camera::getFrustumPlane(int id) const { return &m_frustum[id].n.x; }
const float* Camera::getFrustumPlanes() const { return &m_frustum[0].n.x; }

int Camera::onScreen(const vec3& point, int flags) const {
	if(flags<=1) return flags; //No processing needed - flagged as inside or outside
	for(int i=0; i<6; i++) {
		if(flags & (2<<i)) {
			if(point.dot(m_frustum[i].n) + m_frustum[i].d<0) return 0;  //outside
		}
	}
	return 0x1; //inside (point cannot be intersecting)
}

int Camera::onScreen(const vec3& point, float radius, int flags) const {
	if(flags<=1) return flags; //No processing needed - flagged as inside or outside
	float v;
	for(int i=0; i<6; i++) {
		if(flags & (2<<i)) {
			v = point.dot(m_frustum[i].n) + m_frustum[i].d;
			if(v<=-radius) return 0;	//completely outside frustum
			if(v<radius) flags |= (2<<i);	//intersecting plane
			else flags &= ~(2<<i);		//inside plane
		}
	}
	if(flags==0) return 1; //completely inside
	return flags;
}


int Camera::onScreen(const BoundingBox& box, int flags) const {
	if(flags<=1) return flags; //No processing needed - flagged as inside or outside
	vec3 accept, reject;
	for(int i=0; i<6; i++) {
		if(flags & (2<<i)) {
			//select accept and reject points for each plane;
			accept.x = m_frustum[i].n.x>0? box.min.x: box.max.x;
			accept.y = m_frustum[i].n.y>0? box.min.y: box.max.y;
			accept.z = m_frustum[i].n.z>0? box.min.z: box.max.z;
			
			reject.x = m_frustum[i].n.x>0? box.max.x: box.min.x;
			reject.y = m_frustum[i].n.y>0? box.max.y: box.min.y;
			reject.z = m_frustum[i].n.z>0? box.max.z: box.min.z;

			//Which sides of the plane?
			if(reject.dot(m_frustum[i].n)+m_frustum[i].d<=0) return 0; //outside
			else if( accept.dot(m_frustum[i].n)+m_frustum[i].d>0) flags &= ~(2<<i) ; //inside
			else flags |= (2<<i); //intersect
		}
	}
	if(flags==0) return 1; //completely inside
	return flags;
}



void Camera::updateProjectionMatrix() {
	switch(m_mode) {
	case 0:	m_projection = orthographic(m_left, m_right, m_bottom, m_top, m_near, m_far); break; //glOrtho(m_left, m_right, m_bottom, m_top, m_near, m_far); break;
	case 1: m_projection = perspective(m_fov, m_aspect, m_near, m_far); break;
	case 2:	m_projection = frustum(m_left, m_right, m_bottom, m_top, m_near, m_far); break;  //glFrustum(m_left, m_right, m_bottom, m_top, m_near, m_far); break;
	}
}

Matrix Camera::getModelview() const {
	Matrix out, mat = m_rotation;
	mat.translate(m_position);
	Matrix::inverseAffine(out, mat);
	return out;
}


/** Load perspective matrix - taken from Lib4 */
Matrix Camera::perspective(float fov, float aspect, float near, float far) {
	if(aspect==0) printf("Error: Invalid camera aspect ratio\n");
	fov *= 3.14159265359f/180; //fov in radians
	float f = 1.0f / tan( fov / 2.0f ); //cotangent(fov/2);
	float m[16]; //the matrix;
	m[1]=m[2]=m[3]=m[4]=m[6]=m[7]=m[8]=m[9]=m[12]=m[13]=m[15]=0;
	m[0]  = f / aspect;
	m[5]  = f;
	m[10] = (far + near) / (near - far);
	m[11] = -1.0f;
	m[14] = (2 * far * near) / (near - far);
	return Matrix(m);
}


Matrix Camera::orthographic(float left, float right, float bottom, float top, float near, float far) {
	float dx = right-left;
	float dy = top-bottom;
	float dz = far-near;
	Matrix m;
	m[0] = 2 / dx;
	m[5] = 2 / dy;
	m[10] = -2 / dz;
	m[12] = -(right + left)/dx;
	m[13] = -(top + bottom)/dy;
	m[14] = (near + far)/dz;
	return m;
}

Matrix Camera::frustum(float left, float right, float bottom, float top, float near, float far) {
	top = tan(top);
	left = tan(left);
	right = tan(right);
	bottom = tan(bottom);

	float dx = right - left;
	float dy = top - bottom;
	float dz = far - near;

	Matrix m;
	/*
	m[0] = 2 * near / dx;
	m[5] = 2 * near / dy;
	m[8] = (right + left) / dx;
	m[9] = (top + bottom) / dy;
	m[10] = -(far + near) / dz;
	m[11] = -1;
	m[14] = -2 * far * near / dz;
	m[15] = 0;
	*/

	// From OpenXR SDK
	m[0] = 2.f / dx;
	m[5] = 2.f / dy;
	m[8] = (right + left) / dx;
	m[9] = (top + bottom) / dy;
	if(dz > 0) {
		m[10] = -(far + near) / dz;
		m[14] = -2 * far * near / dz;
	}
	else { // Infinite far plane
		m[10] = -1;
		m[14] = -(near + far);
	}
	m[11] = -1;
	m[15] = 0;
	return m;
}

extern int glhProjectf(float,float,float, const float*, const float*, const int*, float*);
extern int glhUnProjectf(float,float,float, const float*, const float*, const int*, float*);

vec3 Camera::project(const vec3& world, const Point& size) const {
	vec3 out;
	int viewport[4] = { 0, 0, size.x, size.y };
	//calculate modelview matrix
	Matrix modelview = m_rotation.transposed();
	//translation
	vec3 t = modelview * (m_position * -1);
	modelview.translate(t);
	//Project!
	glhProjectf( world.x, world.y, world.z, modelview, m_projection, viewport, out);
	return out;
}

vec3 Camera::unproject(const vec3& screen, const Point& size) const {
	vec3 out;
	int viewport[4] = { 0, 0, size.x, size.y };
	//calculate modelview matrix
	Matrix modelview = m_rotation.transposed();
	//translation
	vec3 t = modelview * (m_position * -1);
	modelview.translate(t);
	//Project!
	glhUnProjectf( screen.x, screen.y, screen.z, modelview, m_projection, viewport, out);
	return out;
}

Ray Camera::getMouseRay(const Point& mouse, const Point& size) const {
	vec3 end = unproject( vec3(mouse.x, mouse.y, 1), size );
	return Ray(m_position, end - m_position);
}

