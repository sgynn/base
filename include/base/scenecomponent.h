#pragma once

#include <base/gamestate.h>
#include <base/scene.h>

namespace base {

class Camera;
class Renderer;
class Workspace;
class CompositorGraph;

class SceneComponent : public base::GameStateComponent {
	public:
	SceneComponent(Scene* scene, Camera* camera, CompositorGraph* graph=0);
	~SceneComponent();
	void update() override;
	void draw() override;
	void resized(const Point& s) override;
	bool setCompositor(base::CompositorGraph* graph);
	Workspace*& getWorkspace() { return m_workspace; }
	Renderer* getRenderer() { return m_renderer; }
	private:
	Scene*     m_scene;
	Workspace* m_workspace;
	Renderer*  m_renderer;
};

}

