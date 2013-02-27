#include "base/fpscamera.h"
#include "base/orbitcamera.h"
#include "base/input.h"
#include "base/game.h"
#include <cstdio>

using namespace base;

CameraBase::CameraBase(float fov, float aspect, float near, float far) 
	: Camera(fov,aspect,near,far), m_active(true), m_grabMouse(false),
	m_moveSpeed(10), m_rotateSpeed(0.004), m_moveAcc(1), m_rotateAcc(1) {}

void CameraBase::setSpeed(float m, float r) {
	m_moveSpeed = m;
	m_rotateSpeed = r;
}

void CameraBase::setSmooth(float m, float r) {
	m_moveAcc = m;
	m_rotateAcc = r;
}

void CameraBase::setUpVector(const vec3& up) {
	m_upVector = up;
	float l = up.length();
	m_useUpVector = l>0;
	if(m_useUpVector) m_upVector *= 1/l;
}

void CameraBase::setPitchLimits(float min, float max) {
	m_constraint = Range(min, max);
}


//// //// //// //// //// //// //// //// //// //// //// //// //// //// //// ////

FPSCamera::FPSCamera(float f, float a, float near, float far) : CameraBase(f,a,near,far) {
	if(a==0) m_aspect = Game::aspect();
	setUpVector( vec3(0,1,0) );
}
void FPSCamera::update() {
	Input* in = Game::input();

	// Movement
	vec3 move;
	if(in->key(KEY_UP)    || in->key(KEY_W)) move.z = -1;
	if(in->key(KEY_DOWN)  || in->key(KEY_S)) move.z =  1;
	if(in->key(KEY_LEFT)  || in->key(KEY_A)) move.x = -1;
	if(in->key(KEY_RIGHT) || in->key(KEY_D)) move.x =  1;
	m_velocity += move * (m_moveSpeed * m_moveAcc);
	m_position += m_rotation * m_velocity;
	if(m_moveAcc<1) m_velocity -= m_velocity * m_moveAcc;
	else m_velocity = vec3();
	
	//Rotation
	Point m = in->queryMouse();
	if(m_active) {
		float dx = m_last.x - m.x;
		float dy = m_last.y - m.y;
		m_rVelocity += vec2(dx,dy) * (m_rotateSpeed * m_rotateAcc);

		if(fabs(m_rVelocity.x)>0.00001) rotateLocal(AXIS_Y, m_rVelocity.x);
		if(fabs(m_rVelocity.y)>0.00001) rotateLocal(AXIS_X, m_rVelocity.y);

		if(m_rotateAcc<1) m_rVelocity -= m_rVelocity * m_rotateAcc;
		else m_rVelocity = vec2();

		// Limits
		if(m_useUpVector) {
			vec3 face = getLeft().cross(m_upVector);
			vec3 direction = getDirection();
			float dot = face.dot(direction);
			float angle = dot>1? 0: dot<-1? PI: acos(dot);
			if(!m_constraint.contains(angle)) {
				angle = m_constraint.clamp(angle);
				direction = m_rotation * vec3(0, sin(angle), cos(angle));
			}
			float inv = face.dot(direction)>0? 1: -1;
			lookat(m_position, m_position-direction, m_upVector * inv);
		}

		// Warp mouse
		if(m_grabMouse && (m_last.x!=m.x || m_last.y!=m.y)) {
			in->warpMouse(m_last.x, m_last.y);
			in->queryMouse();
			m = m_last;
		}

	}
	m_last = m;
}

//// //// //// //// //// //// //// //// //// //// //// //// //// //// //// ////

OrbitCamera::OrbitCamera(float f, float a, float near, float far) : CameraBase(f,a,near,far) {
	if(a==0) m_aspect = Game::aspect();
	setUpVector( vec3(0,1,0) );
}
void OrbitCamera::setTarget(const vec3& t) {
	m_position += t - m_target;
	m_target = t;
}
void OrbitCamera::setPosition(float yaw, float pitch, float distance) {
	vec3 fwd = vec3( sin(yaw), 0, cos(yaw) ) * cos(pitch);
	fwd.y = sin(pitch);
	lookat(m_target + fwd * distance, m_target, m_upVector);
}

void OrbitCamera::update() {
	Input* in = Game::input();

	// Zoom
	const float factor = 0.2f;
	int wheel = in->mouseWheel();
	if(wheel) {
		float distance = m_target.distance(m_position);
		while(wheel<0) { distance *= 1+factor; wheel++; }
		while(wheel>0) { distance *= 1-factor; wheel--; }
		m_position = m_target + getDirection() * distance;
	}

	//Rotation
	Point m = in->queryMouse();
	if(m_active) {
		float dx = m.x - m_last.x;
		float dy = m.y - m_last.y;
		m_rVelocity += vec2(dx,dy) * (m_rotateSpeed * m_rotateAcc);

		float distance = m_target.distance(m_position);
		if(fabs(m_rVelocity.x)>0.00001) rotateLocal(AXIS_Y, m_rVelocity.x);
		if(fabs(m_rVelocity.y)>0.00001) rotateLocal(AXIS_X, m_rVelocity.y);

		if(m_rotateAcc<1) m_rVelocity -= m_rVelocity * m_rotateAcc;
		else m_rVelocity = vec2();

		// Limits
		if(m_useUpVector) {
			vec3 face = getLeft().cross(m_upVector);
			vec3 direction = getDirection();
			float dot = face.dot(direction);
			float angle = dot>1? 0: dot<-1? PI: acos(dot);
			if(!m_constraint.contains(angle)) {
				angle = m_constraint.clamp(angle);
				direction = m_rotation * vec3(0, sin(angle), cos(angle));
			}
			float inv = face.dot(direction)>0? 1: -1;
			lookat(m_target + getDirection()*distance, m_target, m_upVector * inv);
		} else {
			// Fix distance
			m_position = m_target + getDirection() * distance;
		}

		// Warp mouse
		if(m_grabMouse && (m_last.x!=m.x || m_last.y!=m.y)) {
			in->warpMouse(m_last.x, m_last.y);
			in->queryMouse();
			m = m_last;
		}

	}
	m_last = m;
}


