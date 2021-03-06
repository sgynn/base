#ifndef _BASE_SCENESTATE_
#define _BASE_SCENESTATE_

#include "math.h"
#include "state.h"

namespace base {

class CameraBase;

class SceneState : public GameState {
	public:
	SceneState();
	SceneState(float in, float out, StateFlags flags=NONE);
	virtual ~SceneState();

	virtual void update();
	virtual void draw();

	/** Skybox transformation */
	virtual void drawSky() {}
	/** Scene transformation */
	virtual void drawScene() {}
	/** Hud transformation */
	virtual void drawHUD() {}

	/** set camera position */
	void setCamera(const vec3& position, const vec3& target, const vec3& up = vec3(0,1,0));

	protected:
	float m_fps;
	CameraBase* m_camera;

	bool m_drawAxis;
	
};
};


#endif

