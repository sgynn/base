#ifndef _CAMERAS_
#define _CAMERAS_

#include "base/camera.h"

/** Additional cameras */

namespace base {

	/** Virtual base class for updatey camera */
	class CameraBase : public Camera {
		public:
		CameraBase(float fov, float aspect, float near, float far);
		/** camera state */
		void setEnabled(bool e) { m_active = e; }
		/** Update camera */
		virtual void update() = 0;
		/** Set camera speeds */
		void setSpeed(float movement, float rotation);
		/** Movement smoothing - parameters {0>x>=1} */
		void setSmooth(float movement, float rotation);
		/** Up vector - set to null vector if unconstrained */
		void setUpVector(const vec3& up);
		/** Set pitch constraint */
		void setPitchLimits(float min=-PI, float max=PI);

		protected:
		bool m_active;		// Does the camera update
		Point m_last;		// Last mouse coordinates
		float m_moveSpeed;	// Movement speed
		float m_rotateSpeed;// Rotation speed
		float m_moveAcc;	// Movement acceleration for smoothing
		float m_rotateAcc;	// Rotational acceleration for smoothing
		vec3 m_velocity;	// Camera velocity
		vec2 m_rVelocity;	// Rotational velocity
		bool m_useUpVector;	// Enforce up vector
		vec3 m_upVector;	// Up vector
		Range m_constraint;	// Pitch constraints
	};

	/** Camera that uses wasd/arrows to move, and mouse look */
	class FPSCamera : public CameraBase {
		public:
		FPSCamera(float fov, float aspect, float near, float far);
		/** Update the camera - uses Game::input() form mouse/keyboard */
		virtual void update();
	};

	/** Camera that orbits a point */
	class OrbitCamera : public CameraBase {
		public:
		OrbitCamera(float fov, float aspect, float near, float far);

		/** Update the camera - uses Game::input() form mouse/keyboard */
		virtual void update();

		/** set target point */
		void setTarget(const vec3& t);
		/** Set position */
		void setPosition(float yaw, float pitch, float distance);

		private:
		vec3 m_target;		// Target point
	};
};

#endif

