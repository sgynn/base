#ifndef _CAMERA_
#define _CAMERA_

#include "math.h"

//screw windows
#undef near
#undef far
namespace base {
class Camera {
	public:
	/** Default constructor - screen space orthographic camera */
	Camera();
	/** Perspective camera constructor */
	Camera(float fov, float aspect, float near, float far);
	/** Generic frustum */
	Camera(float left, float right, float bottom, float top, float near, float far);

	/** Just set camera position */
	void setPosition(const vec3& position) { m_position = position; }
	/** Set position and orientation from lookat vector */
	void lookat(const vec3& position, const vec3& target, const vec3& up=vec3(0,1,0));
	/** Set camera rotation based on standard coordinates */
	void setRotation(float pitch=0, float yaw=0, float roll=0);
	/** Rotate camera about local axis */
	void rotateLocal(M_AXIS axis, float radians);
	/** Update field of view - only works for perspective cameras */
	void setFov(float fov) { m_fov=fov; }
	/** Update the aspect ratio - only for perspective cameras */
	void setAspect(float aspect) { m_aspect = aspect; }
	/** Adjust near and far cliping planes */
	void adjustDepth(float near, float far) { m_near=near; m_far=far; }
	
	/** Get camera position */
	const vec3& getPosition() const { return m_position; }
	const vec3& getDirection() const { return *reinterpret_cast<const vec3*>(&m_rotation[8]); }
	const vec3& getLeft() const { return *reinterpret_cast<const vec3*>(&m_rotation[0]); }
	const vec3& getUp() const { return *reinterpret_cast<const vec3*>(&m_rotation[4]); }
	/** Get vertical field of view. Only works for perspective cameras */
	float getFov() const { return m_fov; }
	const Matrix& getRotation() const { return m_rotation; }
	const Matrix& getProjection() const { return m_projection; }
	Matrix getModelview() const;

	/** Update camera */
	virtual void update(float time) {}
	
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
	int onScreen(const aabb& box, int clipFlags=0x7e) const;
	/** Get all of the frustum planes */
	const float* getFrustumPlanes() const;
	/** Get a specific frustum plane */
	const float* getFrustumPlane(int id) const;
	


	/** Project a point in world coordinated to screen coordinates */
	vec3 project(const vec3& world) const;
	/** Unproject a point on the screen to world coordinates */
	vec3 unproject(const vec3& screen) const;

	
	/** FPS Camera All-In-One Update Function. Set turn=0 to unlock the mouse */
	void updateCameraFPS(float speed, float turn=0.004, const vec3* up=&defaultUp, float limit=1.5);
	void updateCameraOrbit(const vec3& target=vec3(), float turn=0.004, const vec3* up=&defaultUp, float limit=1.5);

	protected:
	int m_mode;	//0=screen, 1=perspective, 2=frustum
	float m_fov, m_aspect;
	float m_left, m_right;
	float m_bottom, m_top;
	float m_near, m_far;

	static const vec3 defaultUp; //default up vector for update functions
	vec3 m_position;	//camera position
	Matrix m_rotation;	//camera rotation

	Matrix m_projection;	//Projection matrix
	Matrix m_combined;	//Projection matrix * modelview matrix (cached)

	//Frustum planes
	struct Plane { vec3 n; float d; } m_frustum[6];

	public:
	//Some utility functions
	static Matrix perspective(float fov, float aspect, float near, float far);
	static Matrix orthographic(float left, float right, float bottom, float top, float near, float far);
	static Matrix frustum(float left, float right, float bottom, float top, float near, float far);

	int insidePlane(const vec3& n, float d, const vec3& point, float radius);
	int insidePlane(const vec3& n, float d, const aabb& box);

};
};

#endif

