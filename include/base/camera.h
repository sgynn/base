#ifndef _BASE_CAMERA_
#define _BASE_CAMERA_

#include "math.h"

//screw windows
#undef near
#undef far
namespace base {
	/** Camera class
	 * Contains camera ttansformations and matrices
	 * Also methods for view frustum culling
	 * */
	class Camera {
		public:
		Camera();
		/** Orthographic camera */
		Camera(int width, int height);
		/** Perspective camera constructor */
		Camera(float fov, float aspect, float near, float far);
		/** Generic frustum */
		Camera(float left, float right, float bottom, float top, float near, float far);
		/** Destructor */
		virtual ~Camera() {}

		void setOrthographic(int width, int height);
		void setOrthographic(float width, float height, float near, float far);
		void setPerspective(float fov, float aspect, float near, float far);
		void setPerspective(float left, float right, float bottom, float top, float near, float far);


		/** Just set camera position */
		void setPosition(const vec3& position) { m_position = position; }
		/** Set position and orientation from lookat vector */
		void lookat(const vec3& position, const vec3& target, const vec3& up=vec3(0,1,0));
		/** Set camera rotation based on standard coordinates */
		void setRotation(float pitch=0, float yaw=0, float roll=0);
		/** Rotate camera about local axis */
		void rotateLocal(int axis, float radians);
		/** Update field of view - only works for perspective cameras */
		void setFov(float fov);
		/** Update the aspect ratio - only for perspective cameras */
		void setAspect(float aspect);
		/** Adjust near and far cliping planes */
		void adjustDepth(float near, float far);
		
		/** Get camera position */
		const vec3& getPosition() const { return m_position; }
		const vec3& getDirection() const { return *reinterpret_cast<const vec3*>(&m_rotation[8]); }
		const vec3& getLeft() const { return *reinterpret_cast<const vec3*>(&m_rotation[0]); }
		const vec3& getUp() const { return *reinterpret_cast<const vec3*>(&m_rotation[4]); }
		/** Get vertical field of view. Only works for perspective cameras */
		float getFov()  const { return m_fov;  }
		float getNear() const { return m_near; }
		float getFar()  const { return m_far;  }
		float getAspect() const { return m_aspect; }
		const Matrix& getRotation() const { return m_rotation; }
		const Matrix& getProjection() const { return m_projection; }
		Matrix getModelview() const;

		/** Apply camera transformations */
		void applyProjection();
		void applyRotation();
		void applyTranslation();

		/** Recalculate the view frustum planes for culling
		 *	For testing: 0=outside, 1=fully inside, bits[2-7]=intersecting this plane
		 */
		void updateFrustum();
		/** Is a sphere on screen? based on frutsum */
		int onScreen(const vec3& point, int clipFlags=0x7e) const;
		/** Is a sphere on screen? based on frutsum */
		int onScreen(const vec3& point, float radius, int clipFlags=0x7e) const;
		/** Is an axis aligned bounding box on screen */
		int onScreen(const BoundingBox& box, int clipFlags=0x7e) const;
		/** Get all of the frustum planes */
		const float* getFrustumPlanes() const;
		/** Get a specific frustum plane */
		const float* getFrustumPlane(int id) const;
		

		/** Project a point in world coordinated to screen coordinates */
		vec3 project(const vec3& world, const Point& size) const;
		/** Unproject a point on the screen to world coordinates */
		vec3 unproject(const vec3& screen, const Point& size) const;
		/** Get mouse ray in 3d space */
		Ray getMouseRay(const Point& mouse, const Point& viewport) const;

		protected:
		int m_mode;	//0=screen, 1=perspective, 2=frustum
		float m_fov, m_aspect;
		float m_left, m_right;
		float m_bottom, m_top;
		float m_near, m_far;

		vec3 m_position;	//camera position
		Matrix m_rotation;	//camera rotation

		Matrix m_projection;	//Projection matrix
		Matrix m_combined;	//Projection matrix * modelview matrix (cached)

		//Frustum planes
		struct Plane { vec3 n; float d=0; } m_frustum[6];

		// calculate projection matrix
		void updateProjectionMatrix();
		void setProjection(int mode, float l, float r, float t, float b, float n, float f, float fv, float as);

		public:
		//Some utility functions
		static Matrix perspective(float fov, float aspect, float near, float far);
		static Matrix orthographic(float left, float right, float bottom, float top, float near, float far);
		static Matrix frustum(float left, float right, float bottom, float top, float near, float far);

		int insidePlane(const vec3& n, float d, const vec3& point, float radius);
		int insidePlane(const vec3& n, float d, const BoundingBox& box);

	};

	enum CameraUpdateMask { CU_KEYS=1, CU_MOUSE=2, CU_WHEEL=4, CU_ALL=7 };

	/** Virtual base class for automated camera classes */
	class CameraBase : public Camera {
		public:
		CameraBase(float fov, float aspect, float near, float far);
		/** camera state */
		virtual void setEnabled(bool e) { m_active = e; }
		/** Grab mouse */
		virtual void grabMouse(bool g);
		/** Update camera */
		virtual void update(int mask=CU_ALL) = 0;
		/** Set camera speeds */
		void setSpeed(float movement=1.0, float rotation=0.004);
		/** Movement smoothing - parameters {0>x>=1}. Lower values are smoother */
		void setSmooth(float movement, float rotation);
		/** Up vector - set to null vector if unconstrained */
		void setUpVector(const vec3& up);
		/** Set pitch constraint */
		void setPitchLimits(float min=-PI, float max=PI);
		/** Set key input binding */
		void setKeys(unsigned forward, unsigned back, unsigned left, unsigned right, unsigned up, unsigned down);

		protected:
		bool m_active;		// Does the camera update
		bool m_grabMouse;	// Does this camera lock the mouse position
		float m_moveSpeed;	// Movement speed
		float m_rotateSpeed;// Rotation speed
		float m_moveAcc;	// Movement acceleration for smoothing
		float m_rotateAcc;	// Rotational acceleration for smoothing
		vec3 m_velocity;	// Camera velocity
		vec2 m_rVelocity;	// Rotational velocity
		bool m_useUpVector;	// Enforce up vector
		vec3 m_upVector;	// Up vector
		Range m_constraint;	// Pitch constraints
		unsigned m_keyBinding[6]; // Keys
	};


};

#endif

