#include <base/game.h>
#include <base/gamestate.h>
#include <base/input.h>
#include <base/scene.h>
#include <base/orbitcamera.h>
#include <base/compositor.h>
#include <base/resources.h>
#include <base/gui/gui.h>
#include <base/gui/font.h>
#include <base/debuggeometry.h>
#include <base/consolecomponent.h>
#include <base/scenecomponent.h>
#include <base/guicomponent.h>

using namespace base;
using script::Variable;

class Demo : public GameState {
	public:
	Demo();
	~Demo();
	void update() override;
	protected:
	OrbitCamera* m_camera;
	Scene* m_scene;
};


int main(int argc, char* argv[]) {
	Game* game = Game::create(1280, 1024);
	
	Input& in = *Game::input();
	in.bind(0, KEY_W, KEY_UP);
	in.bind(1, KEY_S, KEY_DOWN);
	in.bind(2, KEY_A, KEY_LEFT);
	in.bind(3, KEY_D, KEY_RIGHT);
	in.bind(4, MOUSE_RIGHT);
	in.bind(5, MOUSE_MIDDLE);
	Resources* res = new Resources();
	res->addFolder("data");

	gui::Font* consoleFont = new gui::Font("helvetica", 24);
	gui::Root* gui = new gui::Root(Game::getSize());

	Demo* state = new Demo();
	state->addComponent(new GUIComponent(gui));
	state->addComponent(new ConsoleComponent(Variable(), consoleFont));

	game->setInitialState(state);
	game->run();
	delete game;
	delete res;
	return 0;
}

Demo::Demo() {
	m_scene = new Scene();
	m_camera = new OrbitCamera(90, 0, 0.1, 200);
	m_camera->setPosition(1,1,5);
	m_camera->setModeBinding(4, 5);
	addComponent(new SceneComponent(m_scene, m_camera));

	static DebugGeometry dbg(SDG_FRAME_IF_DATA);
	dbg.line(vec3(), vec3(1,0,0), 0xff0000);
	dbg.line(vec3(), vec3(0,1,0), 0x00ff00);
	dbg.line(vec3(), vec3(0,0,1), 0x0000ff);
}

Demo::~Demo() {
	delete m_camera;
	delete m_scene;
}

void Demo::update() {
	if(Game::Pressed(KEY_ESC)) changeState(0);
}

