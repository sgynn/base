#include "base/fpscamera.h"
#include "base/orbitcamera.h"
#include "base/input.h"
#include "base/game.h"
#include <cstdio>

using namespace base;

constexpr unsigned unbound = ~0u;

CameraBase::CameraBase(float fov, float aspect, float near, float far) 
	: Camera(fov,aspect?aspect:Game::aspect(),near,far), m_active(true), m_grabMouse(false),
	m_useUpVector(false), m_constraint(-8,8), m_binding{0,1,2,3,unbound,unbound,unbound,unbound,unbound,unbound,unbound} {}

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
	if(g == m_grabMouse) return;
	Input& in = *Game::input();
	if(!m_grabMouse) m_lastMouse = in.mouse;
	else in.warpMouse(m_lastMouse.x, m_lastMouse.y);
	m_grabMouse = g;
}

void CameraBase::setMoveBinding(unsigned forward, unsigned back, unsigned left, unsigned right, unsigned up, unsigned down) {
	m_binding.forward = forward;
	m_binding.back = back;
	m_binding.left = left;
	m_binding.right = right;
	m_binding.up = up;
	m_binding.down = down;
}

void CameraBase::setModeBinding(unsigned rotate, unsigned pan, unsigned zoom) {
	m_binding.rotate = rotate;
	m_binding.pan = pan;
	m_binding.zoom = zoom;
}

void CameraBase::setRotateBinding(unsigned yaw, unsigned pitch) {
	m_binding.yaw = yaw;
	m_binding.pitch = pitch;
}


//// //// //// //// //// //// //// //// //// //// //// //// //// //// //// ////

FPSCamera::FPSCamera(float f, float a, float near, float far) : CameraBase(f,a,near,far) {
	setUpVector( vec3(0,1,0) );
}
void FPSCamera::update(int mask) {
	if(!m_active) return;
	Input& in = *Game::input();

	// Key Movement
	if(mask & CU_KEYS) {
		vec3 move;
		if(in.check(m_binding.forward)) move.z = -1;
		if(in.check(m_binding.back))    move.z =  1;
		if(in.check(m_binding.left))    move.x = -1;
		if(in.check(m_binding.right))   move.x =  1;
		if(in.check(m_binding.up))      move.y =  1;
		if(in.check(m_binding.down))    move.y = -1;
		m_velocity += move * (m_moveSpeed * m_moveAcc);
		m_position += m_rotation * m_velocity;
		if(m_moveAcc<1) m_velocity -= m_velocity * m_moveAcc;
		else m_velocity = vec3();
	}
	
	//Rotation
	{
		vec2 delta;
		if(m_binding.yaw) delta.x = in.value(m_binding.yaw);
		if(m_binding.pitch) delta.y = in.value(m_binding.pitch);
		if((mask&CU_MOUSE) && (m_binding.rotate==unbound || in.check(m_binding.rotate))) delta -= vec2(in.mouse.delta.x, in.mouse.delta.y) * m_rotateSpeed;
		m_rVelocity += delta * m_rotateAcc;

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
			if(direction.dot(m_upVector) > 0) angle *= -1;
			if(!m_constraint.contains(angle)) {
				angle = m_constraint.clamp(angle);
				direction = face * cos(angle) - m_upVector * sin(angle);
			}
			float inv = face.dot(direction)>0? 1: -1;
			lookat(m_position, m_position-direction, m_upVector * inv);
		}

		// Warp mouse
		if(m_grabMouse && (in.mouse.delta.x || in.mouse.delta.y)) {
			in.warpMouse(Game::width()/2, Game::height()/2);
		}
	}
}

//// //// //// //// //// //// //// //// //// //// //// //// //// //// //// ////

OrbitCamera::OrbitCamera(float f, float a, float near, float far)
	: CameraBase(f,a,near,far)
	, m_zoomDelta(0)
{
	setUpVector( vec3(0,1,0) );
	setZoomLimits(near, far);
	setSpeed(0.04);
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

void OrbitCamera::setZoomLimits(float min, float max) {
	m_zoomMin = min;
	m_zoomMax = max;
	float d = m_position.distance2(m_target);
	if(d<min*min) m_position = m_target - getDirection() * min;
	else if(d>max*max) m_position = m_target - getDirection() * max;
}

void OrbitCamera::enableMousePan(float d, const vec3& n) {
	m_panNormal = n;
	m_panOffset = d;
}
void OrbitCamera::disableMousePan() {
	m_panNormal.set(0,0,0);
}

void OrbitCamera::update(int mask) {
	Input& in = *Game::input();
	float distance = m_position.distance(m_target);

	// Zoom input
	int wheel = (mask&CU_WHEEL)? in.mouse.wheel: 0;
	if((mask&CU_KEYS) && in.check(m_zoomIn)) wheel += 1;
	if((mask&CU_KEYS) && in.check(m_zoomOut)) wheel -= 1;
	if(wheel) {
		float d = distance + m_zoomDelta;
		while(wheel<0) { d *= 1+m_zoomFactor; wheel++; }
		while(wheel>0) { d *= 1-m_zoomFactor; wheel--; }
		d = fmax(fmin(d, m_zoomMax), m_zoomMin);
		m_zoomDelta = d - distance;
	}
	if(m_zoomDelta) {
		float targetDistance = distance + m_zoomDelta;
		distance = distance * (1-m_zoomAcc) + targetDistance * m_zoomAcc;
		m_position = m_target + getDirection() * distance;
		m_zoomDelta = targetDistance - distance;
		if(fabs(m_zoomDelta)<1e-3) m_zoomDelta = 0;
	}


	// Movement input
	if(mask & CU_KEYS) {
		vec3 move;
		if(in.check(m_binding.forward)) move.z = -1;
		if(in.check(m_binding.back))    move.z =  1;
		if(in.check(m_binding.left))    move.x = -1;
		if(in.check(m_binding.right))   move.x =  1;
		if(in.check(m_binding.up))      move.y =  1;
		if(in.check(m_binding.down))    move.y = -1;
		if(getUp().dot(m_upVector) < 0) move *= -1;
		vec3 z = getDirection();
		vec3 x = m_upVector.cross(z).normalise();
		z = x.cross(m_upVector);
		move = x*move.x + m_upVector*move.y + z*move.z;
		m_velocity += move * (m_moveSpeed * m_moveAcc) * distance;
	}
	// Movement update
	m_target += m_velocity;
	m_position += m_velocity;
	if(m_moveAcc<1) m_velocity -= m_velocity * m_moveAcc;
	else m_velocity.set(0,0,0);


	// Mouse grab panning
	static const vec3 zero(0,0,0);
	if(m_active && m_panNormal != zero) {
		if((mask&CU_MOUSE) && in.pressed(m_binding.pan)) {
			Ray ray = getMouseRay(in.mouse, Game::getSize());
			if(fabs(ray.direction.dot(m_panNormal)) > 0.1) {
				float t = (m_panOffset - m_position.dot(m_panNormal)) / ray.direction.dot(m_panNormal);
				if(t>0) {
					m_panHold = ray.point(t);
					mask &= ~CU_MOUSE;
				}
			}
		}
		else if(in.check(m_binding.pan) && m_panHold != zero) {
			Ray ray = getMouseRay(in.mouse, Game::getSize());
			float t = (m_panOffset - m_position.dot(m_panNormal)) / ray.direction.dot(m_panNormal);
			if(t > 0) {
				vec3 shift = m_panHold - ray.point(t);
				m_target += shift;
				m_position += shift;
				m_velocity = lerp(m_velocity, shift, m_moveAcc);
			}
			mask &= ~CU_MOUSE;
		}
		else m_panHold = zero;
	}
	
	// Mouse drag zoom
	if(m_active && (mask&CU_MOUSE) && in.check(m_binding.zoom) && in.mouse.delta.y) {
		vec3 dir = getPosition() - getTarget();
		float dist = dir.normaliseWithLength();
		dist *= 1 - in.mouse.delta.y * 0.004f;
		distance = fclamp(dist, m_zoomMin, m_zoomMax);
		m_position = m_target + dir * distance;
	}

	// Mouse Rotation input
	if(m_active && (mask&CU_MOUSE) && (m_binding.rotate==unbound || in.check(m_binding.rotate))) {
		float dx = in.mouse.delta.x;
		float dy = in.mouse.delta.y;
		m_rVelocity += vec2(dx,dy) * (m_rotateSpeed * m_rotateAcc);

		// Warp mouse
		if(m_grabMouse && (in.mouse.delta.x || in.mouse.delta.y)) {
			in.warpMouse(Game::width()/2, Game::height()/2);
		}
	}

	// Joystick rotation input
	if(m_binding.yaw!=unbound) m_rVelocity.x += in.value(m_binding.yaw);
	if(m_binding.pitch!=unbound) m_rVelocity.y += in.value(m_binding.pitch);

	// Rotation update
	if(m_rVelocity.x || m_rVelocity.y) {
		if(m_useUpVector) {
			float yaw = atan2( getDirection().x, getDirection().z ) + m_rVelocity.x;
			float pitch = asin( getDirection().y );
			if(getUp().y < 0) pitch = PI - pitch, yaw += PI;
			pitch = m_constraint.clamp(pitch + m_rVelocity.y);
			if(pitch > PI) pitch -= TWOPI;
			float inv = pitch > -HALFPI && pitch < HALFPI? 1: -1;
			vec3 dir( cos(pitch) * sin(yaw), sin(pitch), cos(pitch) * cos(yaw) );
			lookat( m_target + dir * distance, m_target, m_upVector * inv );
		}
		else {
			// Rotate local axes and normalise
			if(fabs(m_rVelocity.x)>0.00001) rotateLocal(1, m_rVelocity.x);
			if(fabs(m_rVelocity.y)>0.00001) rotateLocal(0, m_rVelocity.y);
			m_position = m_target + getDirection() * distance;
		}

		if(m_rotateAcc < 1) {
			m_rVelocity -= m_rVelocity * m_rotateAcc;
			if(fabs(m_rVelocity.x) < 1e-6) m_rVelocity.x = 0;
			if(fabs(m_rVelocity.y) < 1e-6) m_rVelocity.y = 0;
		}
		else m_rVelocity.x = m_rVelocity.y = 0.f;
	}
}


