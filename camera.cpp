#include "opengl.h"

#include "camera.h"
#include "game.h"
#include "input.h"

#include <cstdio>

using namespace base;

Camera::Camera() : m_mode(0), m_near(-1), m_far(1) {
	Point screen = Game::getSize();
	m_left=m_bottom=0;
	m_right = screen.x;
	m_top = screen.y;
}

Camera::Camera(float fov, float aspect, float near, float far) : m_mode(1), m_fov(fov), m_aspect(aspect), m_near(near), m_far(far) {

}


Camera::Camera(float left, float right, float bottom, float top, float near, float far)
	: m_mode(2), m_left(left), m_right(right), m_bottom(bottom), m_top(top), m_near(near), m_far(far) {
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
	//roll
	if(roll!=0) rotateLocal(AXIS_Z, roll);
}

/** Rotate a matrix about a local axis. Must be a rotation matrix */
void Camera::rotateLocal(M_AXIS axis, float radians) {
	Matrix& mat = m_rotation;
	//New axis vector
	vec3 v;
	switch(axis) {
	case AXIS_X: v = vec3(0,cos(radians),-sin(radians)); break; //X axis
	case AXIS_Y: v = vec3(sin(radians),0,cos(radians)); break; //Y axis
	case AXIS_Z: v = vec3(cos(radians),sin(radians),0); break; //Z axis
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

/// Frustun Stuff ///
#define FLEFT	0
#define FRIGHT	1
#define FBOTTOM	2
#define FTOP	3
#define FNEAR	4
#define FFAR	5

#include <iostream>
void Camera::updateFrustum() {
	//calculate combined matrix
	Matrix modelview = m_rotation.transpose();
	//translation
	vec3 t = modelview * (m_position * -1);
	modelview[12] = t.x;
	modelview[13] = t.y;
	modelview[14] = t.z;

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


int Camera::onScreen(const aabb& box, int flags) const {
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




void Camera::applyProjection() {
	switch(m_mode) {
	case 0:	m_projection = orthographic(m_left, m_right, m_bottom, m_top, m_near, m_far); break; //glOrtho(m_left, m_right, m_bottom, m_top, m_near, m_far); break;
	case 1: m_projection = perspective(m_fov, m_aspect, m_near, m_far); break;
	case 2:	m_projection = frustum(m_left, m_right, m_bottom, m_top, m_near, m_far); break;  //glFrustum(m_left, m_right, m_bottom, m_top, m_near, m_far); break;
	}
	glLoadMatrixf(m_projection);
}
void Camera::applyRotation() {
	//glMultMatrixf(m_rotation);
	float mat[16] = { m_rotation[0], m_rotation[4], m_rotation[8], 0,
			  m_rotation[1], m_rotation[5], m_rotation[9], 0,
			  m_rotation[2], m_rotation[6], m_rotation[10], 0, 0, 0, 0, 1 };
	glMultMatrixf(mat);
}
void Camera::applyTranslation() {
	glTranslatef(-m_position.x, -m_position.y, -m_position.z);
}

Matrix Camera::getModelview() const {
	Matrix out, mat = m_rotation;
	mat[12]=m_position.x;
	mat[13]=m_position.y;
	mat[14]=m_position.z;
	Matrix::inverseAffine(out, mat);
	return out;
}


/** Load perspective matrix - taken from Lib4 */
Matrix Camera::perspective(float fov, float aspect, float near, float far) {
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
	m[0] = 2/dx;
	m[5] = 2/dy;
	m[10] = -2/dz;
	m[12] = (right+left)/dx;
	m[13] = (top+bottom)/dy;
	m[14] = (near+far)/dz;
	return m;
}

Matrix Camera::frustum(float left, float right, float bottom, float top, float near, float far) {
	float dx = right-left;
	float dy = top-bottom;
	float dz = far-near;
	Matrix m;
	m[0] = 2*near / dx;
	m[5] = 2*near / dy;
	m[8] = (right+left) / dx;
	m[9] = (top+bottom) / dy;
	m[10] = -(far+near) / dz;
	m[11] = -1;
	m[14] = -2 * far * near / dz;
	m[15] = 0;
	return m;
}

#include "game.h"
#include "glhprojection.c"
vec3 Camera::project(const vec3& world) const {
	vec3 out;
	int viewport[4] = { 0, 0, Game::getSize().x, Game::getSize().y };
	//calculate modelview matrix
	Matrix modelview = m_rotation.transpose();
	//translation
	vec3 t = modelview * (m_position * -1);
	modelview[12] = t.x;
	modelview[13] = t.y;
	modelview[14] = t.z;
	//Project!
	glhProjectf( world.x, world.y, world.z, modelview, m_projection, viewport, out);
	return out;
}

vec3 Camera::unproject(const vec3& screen) const {
	vec3 out;
	int viewport[4] = { 0, 0, Game::getSize().x, Game::getSize().y };
	//calculate modelview matrix
	Matrix modelview = m_rotation.transpose();
	//translation
	vec3 t = modelview * (m_position * -1);
	modelview[12] = t.x;
	modelview[13] = t.y;
	modelview[14] = t.z;
	//Project!
	glhUnProjectf( screen.x, screen.y, screen.z, modelview, m_projection, viewport, out);
	return out;
}

const vec3 Camera::defaultUp(0,1,0);
void Camera::updateCameraFPS(float speed, float turn, const vec3* up, float limit) {
	Input* in = Game::input();

	//get movement vector
	#define Key(k) in->key(k)
	vec3 move;
	if(Key(KEY_UP)    || Key(KEY_W)) move.z = -speed;
	if(Key(KEY_DOWN)  || Key(KEY_S)) move.z =  speed;
	if(Key(KEY_LEFT)  || Key(KEY_A)) move.x = -speed;
	if(Key(KEY_RIGHT) || Key(KEY_D)) move.x =  speed;

	//Move camera
	//Matrix mat = getRotation();
	vec3 position = getPosition();
	position += getDirection() * move.z;
	position += getLeft() * move.x;
	setPosition(position);
	
	//Rotate Camera
	int mx, my;
	in->mouse(mx, my);
	static int lx=0, ly=0;
	if(turn!=0) {
		//rotate rotation matrix.
		if(mx!=lx) rotateLocal(AXIS_Y, (lx-mx)*turn);
		if(my!=ly) rotateLocal(AXIS_X, (ly-my)*turn);
		//Reset up vector?
		if(up) lookat(position, position - getDirection(), *up);
		if(lx!=mx || ly!=my) in->warpMouse(lx, ly);
	} else { lx=mx; ly=my; }
}

void Camera::updateCameraOrbit(const vec3& target, float turn, const vec3* up, float limit) {
	Input* in = Game::input();

	//Rotate camera
	int mx, my;
	in->mouse(mx, my);
	int mw = in->mouseWheel();
	static int lx=0, ly=0;
	float dp=0, dy=0;
	if(turn!=0) {
		if(mx!=lx) dy = (mx-lx)*turn;
		if(my!=ly) dp = (ly-my)*turn;
		if(dp!=0 || dy!=0) in->warpMouse(lx, ly);
	} else { lx=mx; ly=my; }

	//Update camera position
	if(dp!=0 || dy!=0 || mw!=0) {
		//Update camera distance
		float distance = m_position.distance(target);
		while(mw<0) { distance *= 1.2f; mw++; }
		while(mw>0) { distance *= 0.8f; mw--; }
		//Adjust camera
		if(dy!=0) rotateLocal(AXIS_Y, dy);
		if(dp!=0) rotateLocal(AXIS_X, dp);
		setPosition(target + getDirection()*distance);
		//Up vectors dont work very well with rotateLocal. May need a seperate case.
		//if(up) lookat(getPosition(), target, getUp().y<0? *up*-1: *up);
	}
}
