#include <iostream>
#include "scene.h"
#include "game.h"
#include "camera.h"
#include "draw.h"
#include "material.h"
#include "input.h"

using namespace base;

Material nullMaterial;

SceneState::SceneState() : m_fps(0), m_drawAxis(true) {
	Point ws = Game::getSize();
	m_camera = new Camera(90, (float)ws.x/ws.y, 1, 10000);
	m_camera->lookat(vec3(0, 0, 10), vec3(0,0,0), vec3(0,1,0));
}
SceneState::SceneState(float in, float out, StateFlags f) : GameState(in,out,f), m_fps(0), m_drawAxis(true) {
	Point ws = Game::getSize();
	m_camera = new Camera(90, (float)ws.x/ws.y, 1, 10000);
	m_camera->lookat(vec3(0, 0, 10), vec3(0,0,0), vec3(0,1,0));
}
SceneState::~SceneState() {
	delete m_camera;
}

void SceneState::update() {
	if(Game::input()->pressed(KEY_ESC)) changeState(0);

	//calculate fps
	float time = Game::frameTime();
	if(time>0) m_fps = m_fps*(1-time) + (1/time)*time;
}

void SceneState::draw() {
	//projection
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	m_camera->applyProjection();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	m_camera->applyRotation();
	//Draw skybox here
	drawSky();
	m_camera->applyTranslation();
	
	//coloured axis
	if(m_drawAxis) {
		nullMaterial.bind();
		Draw::Line( vec3(), vec3(1, 0, 0), 0x0000ff, 2);
		Draw::Line( vec3(), vec3(0, 1, 0), 0x00ff00, 2);
		Draw::Line( vec3(), vec3(0, 0, 1), 0xff0000, 2);
	}

	//Draw scene
	drawScene();

	//Reset projection to default orthographic
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glClear(GL_DEPTH_BUFFER_BIT);

	//Any 2D HUD stuff here
	drawHUD();
}


