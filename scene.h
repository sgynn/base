#ifndef _SCENESTATE_
#define _SCENESTATE_

#include "math.h"
#include "state.h"

namespace base {

class Camera;

class SceneState : public GameState {
	public:
	SceneState();
	SceneState(float in, float out, StateFlags flags=NONE);
	virtual ~SceneState();

	virtual void update(float time);
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
	Camera* m_camera;

	bool m_drawAxis;
	
};
};


#endif

