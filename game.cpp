#include "opengl.h"
#include "window.h"
#include "input.h"

#include "game.h"
#include "state.h"
#include <sys/time.h>
#include <cstdio>
#include <cstdlib>

#ifndef WIN32
#include <unistd.h>
#define Sleep(t) usleep(t*1000)
#endif

using namespace base;

base::Window* Game::s_window;
Input* Game::s_input;
Game* Game::s_inst=0;
int Game::s_clearBits=0;

Game::Game( int width, int height, int bpp, bool fullscreen, uint fsaa) : m_state(0) {
	if(s_inst!=0) {
	       printf("Warning: Cant have more than one Game instance\n"); 
	       return;
	}
	s_inst = this;

	//Create window
	s_window = new Window(width, height, bpp, 16, fullscreen, fsaa);

	m_state = new StateManager();

	s_input = new Input();

	//What to clear every frame
	s_clearBits = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT; 

	atexit(cleanup);
}

Game::~Game() {
	delete m_state;
}
void Game::cleanup() { 
	delete s_window;
	delete s_input;
}

void Game::setTitle(const char* title) {
	s_window->setTitle(title);
}

void Game::resize(int width, int height) {
}

void Game::resize(bool fullscreen) {
}

void Game::resize(int width, int height, bool fullscreen) {
}

Point Game::getSize() { return s_window->size(); } 

void Game::setInitialState(GameState* state) {
	m_state->change(state);
}

void Game::exit() {
	m_state->quit();
}

uint64 Game::getTicks() { 
	#ifdef LINUX
	unsigned long long ticks;
	struct timeval now;
	gettimeofday(&now, NULL);
	ticks = ((uint64)now.tv_sec) * 1000000ull + ((uint64)now.tv_usec);
	return ticks;
	#endif

	#ifdef WIN32
	LARGE_INTEGER tks;
	QueryPerformanceCounter(&tks);
	return (((uint64)tks.HighPart << 32) + (uint64)tks.LowPart);
	#endif
	return 0;
}

uint64 Game::getTickFrequency() {
	#ifdef LINUX
	return 1000000ull;
	#endif
	#ifdef WIN32
	LARGE_INTEGER tks;
	QueryPerformanceFrequency(&tks);
	return (((uint64)tks.HighPart << 32) + (uint64)tks.LowPart);	
	#endif
}

void Game::run() {
	//Set up dome OpenGL stuff
	glMatrixMode(GL_PROJECTION);
	glOrtho(0, s_window->width(), 0, s_window->height(), -1, 1);
	glMatrixMode(GL_MODELVIEW);
	
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	
	uint64 fix = 0; //getTickFrequency() / 60ull; //Framerate fix?
	uint64 freq = getTickFrequency();
	unsigned long now, then = getTicks();
	while(m_state->running()) {
		//Pump Events
		s_input->update();
		uint r = s_window->pumpEvents(s_input);
		if(r==0x100) m_state->quit();
		
		//Clear scene (make this optional)
		glClear( s_clearBits );
		
		//update frame time
		now = getTicks();
		if(fix) while((now=getTicks()) < then+fix) Sleep(1);
		float time = (float)(now - then) / freq;
		then = now;
		
		//do game stuff
		m_state->update(time);
		m_state->draw();
		
		s_window->swapBuffers();		
	}
}

//Input functions *depricated* (These were links to SDL)
bool Game::Key(int k)		{ return s_input->key(k); }
bool Game::Pressed(int k)	{ return s_input->pressed(k); }
int  Game::Mouse()			{ return s_input->mouse(); } 
int  Game::Mouse(int& x, int&y) { return s_input->mouse(x, y); } 
int  Game::MouseClick()		{ return s_input->mouseClick(); }
void Game::WarpMouse(int x, int y) { s_input->warpMouse(x, y); }
int  Game::MouseWheel() 	{ return s_input->mouseWheel(); }

uint Game::LastKey()	{ return s_input->lastKey();  }
uint16 Game::LastChar()	{ return s_input->lastChar(); }

