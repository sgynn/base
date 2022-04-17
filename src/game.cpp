#include "base/opengl.h"
#include "base/input.h"

#ifdef WIN32
#include "base/window_win32.h"
#endif
#ifdef LINUX
#include "base/window_x11.h"
#endif
#ifdef EMSCRIPTEN
#include "base/window_em.h"
#endif

#include "base/game.h"
#include "base/gamestate.h"
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

Game* Game::create(int width, int height, int bpp, bool fullscreen, int fps, uint antialiasing) {
	if(s_inst!=0) printf("Warning: A game instance already exists.\n");
	else s_inst = new Game(width, height, bpp, fullscreen, antialiasing);
	s_inst->m_targetFPS = fps;
	return s_inst;
}

Game::Game( int width, int height, int bpp, bool fullscreen, uint fsaa) : m_state(0) {
	if(s_inst!=0) {
	       printf("Warning: Cant have more than one Game instance\n"); 
	       return;
	}
	s_inst = this;

	//Create window
	#ifdef WIN32
	s_window = new Win32Window(width, height, fullscreen, bpp, 24, fsaa);
	#endif
	#ifdef LINUX
	s_window = new X11Window(width, height, fullscreen, bpp, 24, fsaa);
	#endif
	#ifdef EMSCRIPTEN
	s_window = new EMWindow(width, height);
	#endif

	s_window->centreScreen();
	s_window->createWindow();
	s_window->makeCurrent();
	s_window->clear();


	m_state = new GameStateManager();

	s_input = new Input();

	//Target fps
	m_targetFPS = 0;

	atexit(cleanup);
}

Game::~Game() {
	delete m_state;
}
void Game::cleanup() { 
	delete s_window;
	delete s_input;
}

Point Game::getSize() { return s_window->getSize(); } 
int Game::width() { return s_window->getSize().x; }
int Game::height() { return s_window->getSize().y; }

void Game::setInitialState(GameState* state) {
	m_state->changeState(state);
}

void Game::setTargetFPS(uint fps) {
	m_targetFPS = fps;
}

void Game::exit() {
	s_inst->m_state->quit();
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
	return 1;
}

#ifdef EMSCRIPTEN
EM_BOOL emscriptenUpdate(double totalTime, void* data) {
	GameStateManager* state = (GameStateManager*)data;
	state->update();
	state->draw();
	Game::input()->update();
	return state->running();
}
#endif


void Game::run() {
	#ifdef EMSCRIPTEN
	m_state->update();
	emscripten_request_animation_frame_loop(&emscriptenUpdate, m_state);
	
	#else
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	if(m_targetFPS==0) {
	
		uint64 frequency = getTickFrequency();
		uint64 time, delta, last;
		uint64 systemTime, updateTime, renderTime;
		last = systemTime = getTicks();
		m_totalTime = 0;
		while(m_state->running()) {
			time = getTicks();
			delta = m_elapsedTime = time - last;
			last = time;

			//Pump Events
			s_input->update();
			uint r = s_window->pumpEvents(s_input);
			if(r==0x100) m_state->quit();
			time = getTicks();
			m_systemTime = time - systemTime;
			
			//calculate frame time
			m_frameTime = (float)delta / frequency;
			m_totalTime += m_frameTime;
			m_gameTime = delta;
			
			//Update
			updateTime = time;
			m_state->update();
			time = getTicks();
			m_updateTime = time - updateTime;

			//Render
			renderTime = time;
			m_state->draw();
			s_window->swapBuffers();		
			time = getTicks();
			m_renderTime = time - renderTime;
			systemTime = time;
		}
	} else {

		// Fixed framerate version (hardcode 60fps)
		uint maxFrameSkip = 5;
		uint64 frequency = getTickFrequency();
		uint64 rate = frequency / m_targetFPS;
		uint64 skip = rate * (maxFrameSkip+1);
		uint64 time, last, delta=0;
		uint64 gameTime, systemTime, updateTime, renderTime;
		float frameTime = m_frameTime = 1.f / m_targetFPS;
		last = systemTime = getTicks();
		m_totalTime = 0;
		while(m_state->running()) {
			time = getTicks();
			delta += time - last;
			last = time;

			// If a frame has passed
			if(delta >= rate) {
				//Update timer
				gameTime = time;
				m_systemTime = time - systemTime;

				//update loop
				if(delta > skip) delta = skip;
				while(delta >= rate) {
					//update timer
					m_elapsedTime = rate; //This is the update time value
					m_totalTime += frameTime;
					updateTime = time;
					delta -= rate;

					//update
					s_input->update();
					uint r = s_window->pumpEvents(s_input);
					if(r==0x100) m_state->quit(); //Close button, catch SIGTERM?
					m_state->update();

					time = getTicks();
					m_updateTime = time - updateTime;
				}
				//Render
				renderTime = time;
				m_state->draw();
				s_window->swapBuffers();		

				time = systemTime = getTicks();
				m_renderTime = time - renderTime;
				m_gameTime = time - gameTime;
				m_frameTime = fmax(frameTime, (float)m_gameTime/frequency);
			} else {
				Sleep(1);
			}
		}
	}
	#endif
}

void Game::debugTime(uint& game, uint& system, uint& update, uint& render) {
	game 	= s_inst->m_gameTime;
	system	= s_inst->m_systemTime;
	update	= s_inst->m_updateTime;
	render	= s_inst->m_renderTime;
}

// Deprecated input functions Input functions
bool Game::Key(int k)              { return s_input->key(k); }
bool Game::Pressed(int k)          { return s_input->keyPressed(k); }
int  Game::Mouse()                 { return s_input->mouse.button; } 
int  Game::Mouse(int& x, int&y)    { x = s_input->mouse.x; y = s_input->mouse.y; return Mouse(); } 
int  Game::MouseClick()            { return s_input->mouse.pressed; }
void Game::WarpMouse(int x, int y) { s_input->warpMouse(x, y); }
float Game::MouseWheel()           { return s_input->mouse.wheel; }

uint Game::LastKey()	{ return s_input->lastKey();  }
uint16 Game::LastChar()	{ return s_input->lastChar(); }

