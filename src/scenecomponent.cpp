#include <base/scenecomponent.h>
#include <base/game.h>
#include <base/scene.h>
#include <base/camera.h>
#include <base/renderer.h>
#include <base/compositor.h>
#include <base/framebuffer.h>
#include <base/autovariables.h>
#include <base/debuggeometry.h>

using namespace base;

SceneComponent::SceneComponent(Scene* scene, Camera* camera, CompositorGraph* graph) : GameStateComponent(80), m_scene(scene), m_workspace(0) {
	m_renderer = new Renderer;
	DebugGeometryManager::getInstance()->initialise(scene, 10, false);
	setCompositor(graph);
	m_workspace->setCamera(camera);
	m_time = Game::gameTime();
}

SceneComponent::~SceneComponent() {
	delete m_renderer;
	delete m_workspace;
}

void SceneComponent::update() {
	if(!getState()->isPaused()) m_time += Game::frameTime();
	m_scene->updateSceneGraph();
	m_renderer->getState().getVariableSource()->setTime(m_time, Game::frameTime());
}

void SceneComponent::draw() {
	DebugGeometryManager::getInstance()->update();
	m_workspace->execute(m_scene, m_renderer);
	m_renderer->getState().reset();
}

void SceneComponent::resized(const Point& s) {
	m_workspace->getCamera()->setAspect((float)s.x / s.y);
	if(m_workspace->getGraph()->requiresTargetSize()) {
		printf("Window resized to %dx%d - rebuilding compositor targets\n", s.x, s.y);
		m_workspace->compile(s.x, s.y);
	}
}

bool SceneComponent::setCompositor(CompositorGraph* graph) {
	Camera* cam = m_workspace? m_workspace->getCamera(): nullptr;
	if(graph) {
		Workspace* workspace = new Workspace(graph);
		if(workspace->compile(FrameBuffer::Screen.width(), FrameBuffer::Screen.height())) {
			delete m_workspace;
			m_workspace = workspace;
			return true;
		}
		else printf("Error: Failed to compile compositor graph");
	}
	else {
		delete m_workspace;
		m_workspace = nullptr;
	}
	if(!m_workspace) {
		m_workspace = createDefaultWorkspace();
		if(cam) m_workspace->setCamera(cam);
	}
	return !graph;
}

