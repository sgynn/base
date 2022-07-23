#ifndef _BASE_ORBIT_CAMERA_
#define _BASE_ORBIT_CAMERA_

#include "base/camera.h"
namespace base {
	/** Camera that orbits a point
	 *	Uses Game::input to read state
	 *	if enabled, the camera uses the mouse delta to rotate, and locks the mouse
	 * */
	class OrbitCamera : public CameraBase {
		public:
		OrbitCamera(float fov=90, float aspect=0, float near=1, float far=10000);
		/** Move camera */
		virtual void update(int mask=CU_ALL);

		/** set target point */
		void setTarget(const vec3& t);
		/** get camera target */
		const vec3& getTarget() const { return m_target; }
		/** Set position as angles */
		void setPosition(float yaw, float pitch, float distance);
		/** Set zoom factor  */
		void setZoomFactor(float z, float smooth=1) { m_zoomFactor = z; m_zoomAcc = smooth; }
		/** Get zoom factor */
		float getZoomFactor() const { return m_zoomFactor; }
		void setZoomLimits(float min, float max);

		private:
		vec3 m_target;		// Target point
		float m_zoomFactor;
		float m_zoomMin, m_zoomMax;
		float m_zoomAcc;
		float m_zoomDelta;
	};
};

#endif

