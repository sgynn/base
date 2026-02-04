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

SceneComponent::SceneComponent(Scene* scene, Camera* camera, CompositorGraph* graph, bool updateCamera)
	: GameStateComponent(80)
	, m_scene(scene)
	, m_workspace(0)
	, m_camera(camera)
	, m_updateCamera(updateCamera && dynamic_cast<CameraBase*>(camera))
{
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

int SceneComponent::translateCameraUpdateFlags(int componentFlags) {
	int cameraUpdateFlags = 0;
	if(!(componentFlags & BLOCK_KEYS)) cameraUpdateFlags |= CU_KEYS;
	if(!(componentFlags & BLOCK_MOUSE)) cameraUpdateFlags |= CU_MOUSE;
	if(!(componentFlags & BLOCK_WHEEL)) cameraUpdateFlags |= CU_WHEEL;
	return cameraUpdateFlags;
}

void SceneComponent::update() {
	if(m_updateCamera) {
		static_cast<CameraBase*>(m_camera)->update(translateCameraUpdateFlags(getComponentFlags()));
	}

	if(!getState()->isPaused()) m_time += Game::frameTime();
	m_scene->updateSceneGraph();
	m_renderer->getState().getVariableSource()->setTime(m_time, Game::frameTime());
}

void SceneComponent::draw() {
	DebugGeometryManager::getInstance()->update(getState()->isPaused());
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

void SceneComponent::setCamera(Camera* cam) {
	m_camera = cam;
	m_workspace->setCamera(cam);
}

void SceneComponent::setCamera(Camera* cam, bool update) {
	m_camera = cam;
	m_updateCamera = update && dynamic_cast<CameraBase*>(cam);
	m_workspace->setCamera(cam);
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

