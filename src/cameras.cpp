#include "base/fpscamera.h"
#include "base/orbitcamera.h"
#include "base/input.h"
#include "base/game.h"
#include <cstdio>

using namespace base;

CameraBase::CameraBase(float fov, float aspect, float near, float far) 
	: Camera(fov,aspect?aspect:Game::aspect(),near,far), m_active(true), m_grabMouse(false),
	m_moveSpeed(10), m_rotateSpeed(0.004), m_moveAcc(1), m_rotateAcc(1),
	m_useUpVector(false), m_constraint(-8,8) {}

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

void CameraBase::grabMouse(bool g) {
	m_grabMouse = g;
	if(g && m_last.x==0 && m_last.y==0) {
		m_last.x = Game::width()/2;
		m_last.y = Game::height()/2;
		Game::input()->warpMouse(m_last.x, m_last.y);
	}
}


//// //// //// //// //// //// //// //// //// //// //// //// //// //// //// ////

FPSCamera::FPSCamera(float f, float a, float near, float far) : CameraBase(f,a,near,far) {
	setUpVector( vec3(0,1,0) );
}
void FPSCamera::update(int mask) {
	Input* in = Game::input();

	// Key Movement
	if(mask & CU_KEYS) {
		vec3 move;
		if(in->key(KEY_UP)    || in->key(KEY_W)) move.z = -1;
		if(in->key(KEY_DOWN)  || in->key(KEY_S)) move.z =  1;
		if(in->key(KEY_LEFT)  || in->key(KEY_A)) move.x = -1;
		if(in->key(KEY_RIGHT) || in->key(KEY_D)) move.x =  1;
		m_velocity += move * (m_moveSpeed * m_moveAcc);
		m_position += m_rotation * m_velocity;
		if(m_moveAcc<1) m_velocity -= m_velocity * m_moveAcc;
		else m_velocity = vec3();
	}
	
	//Rotation
	Point m = in->queryMouse();
	if(m_active && (mask&CU_MOUSE)) {
		float dx = m_last.x - m.x;
		float dy = m_last.y - m.y;
		m_rVelocity += vec2(dx,dy) * (m_rotateSpeed * m_rotateAcc);

		if(fabs(m_rVelocity.x)>0.00001) rotateLocal(1, m_rVelocity.x);
		if(fabs(m_rVelocity.y)>0.00001) rotateLocal(0, m_rVelocity.y);

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

OrbitCamera::OrbitCamera(float f, float a, float near, float far) : CameraBase(f,a,near,far), m_zoomFactor(0.2) {
	setUpVector( vec3(0,1,0) );
}
void OrbitCamera::setTarget(const vec3& t) {
	m_position += t - m_target;
	m_target = t;
}
void OrbitCamera::setPosition(float yaw, float pitch, float distance) {
	pitch = m_constraint.clamp(pitch);
	float d = (pitch + PI) / TWOPI;
	d -= floor(d);
	float inv = d<0.25 || d>0.75? -1: 1;
	vec3 dir( cos(pitch) * sin(yaw), sin(pitch), cos(pitch) * cos(yaw) );
	lookat( m_target + dir * distance, m_target, m_upVector * inv );
}

void OrbitCamera::update(int mask) {
	Input* in = Game::input();

	// Zoom
	int wheel = in->mouseWheel();
	if(wheel && (mask&CU_WHEEL)) {
		float distance = m_target.distance(m_position);
		while(wheel<0) { distance *= 1+m_zoomFactor; wheel++; }
		while(wheel>0) { distance *= 1-m_zoomFactor; wheel--; }
		m_position = m_target + getDirection() * distance;
	}

	//Rotation
	Point m = in->queryMouse();
	if(m_active && (mask&CU_MOUSE)) {
		float dx = m.x - m_last.x;
		float dy = m.y - m_last.y;
		m_rVelocity += vec2(dx,dy) * (m_rotateSpeed * m_rotateAcc);

		float distance = m_target.distance(m_position);
		if(m_useUpVector) {
			float yaw = atan2( getDirection().x, getDirection().z ) + m_rVelocity.x;
			float pitch = asin( getDirection().y );
			if(getUp().y < 0) pitch = PI - pitch, yaw += PI;
			pitch = m_constraint.clamp(pitch + m_rVelocity.y);
			if(pitch > PI) pitch -= TWOPI;
			float inv = pitch > -HALFPI && pitch < HALFPI? 1: -1;
			vec3 dir( cos(pitch) * sin(yaw), sin(pitch), cos(pitch) * cos(yaw) );
			lookat( m_target + dir * distance, m_target, m_upVector * inv );
		} else {
			// Rotate local axes and normalise
			if(fabs(m_rVelocity.x)>0.00001) rotateLocal(1, m_rVelocity.x);
			if(fabs(m_rVelocity.y)>0.00001) rotateLocal(0, m_rVelocity.y);
			m_position = m_target + getDirection() * distance;
		}

		if(m_rotateAcc<1) m_rVelocity -= m_rVelocity * m_rotateAcc;
		else m_rVelocity.x = m_rVelocity.y = 0.f;

		// Warp mouse
		if(m_grabMouse && (m_last.x!=m.x || m_last.y!=m.y)) {
			in->warpMouse(m_last.x, m_last.y);
			in->queryMouse();
			m = m_last;
		}

	}
	m_last = m;
}


