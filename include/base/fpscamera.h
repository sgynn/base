#ifndef _BASE_FPS_CAMERA_
#define _BASE_FPS_CAMERA_

#include "base/camera.h"
namespace base {
	/** Camera that uses wasd/arrows to move, and mouse look 
	 *  Uses Game::input to get input state
	 *	if enabled, the camera uses the mouse delta to rotate, and locks the mouse
	 * */
	class FPSCamera : public CameraBase {
		public:
		FPSCamera(float fov=90, float aspect=0, float near=1, float far=10000);
		virtual void update();
	};
};

#endif

