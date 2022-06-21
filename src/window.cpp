#include "base/window.h"
#include "base/opengl.h"
#include "base/game.h"
#include "base/gamestate.h"
#include "base/framebuffer.h"
#include <cstdio>

base::Window::Window(int width, int height, bool fullscreen, int bpp, int depth, int fsaa) 
	: m_fsaa(fsaa), m_size(width, height), m_colourDepth(bpp), m_depthBuffer(depth), m_fullScreen(fullscreen)
{
	base::FrameBuffer::setScreenSize(width, height);
}

base::Window::~Window() {
}

bool base::Window::setFullScreen(bool fs) {
	bool r = true;
	if(!created()) m_fullScreen = fs;
	else if(fs!=m_fullScreen) {
		destroyWindow();
		m_fullScreen = fs;
		r = createWindow();
		if(r) makeCurrent();
	}
	return r;
}
bool base::Window::setFSAA(int samples) {
	bool r = true;
	if(samples != m_fsaa) {
		m_fsaa = samples;
		if(created()) {
			destroyWindow();
			r = createWindow();
			if(r) makeCurrent();
		}
	}
	return r;
}


void base::Window::centreScreen() {
	if(m_fullScreen) return;
	Point s = getScreenResolution();
	setPosition( s.x/2-m_size.x/2, s.y/2-m_size.y/2);
}

void base::Window::clear() {
	if(!created()) return;

	//set up some openGL stuff - seperate function!
	glClearColor( 0.0, 0.0, 0.1f, 1.0f );
	glClear(GL_COLOR_BUFFER_BIT);
	glClearDepth(1.0f);                     // Depth buffer setup
	glEnable(GL_DEPTH_TEST);                // Enables depth testing
	glDepthFunc(GL_LEQUAL);                 // The type of depth testing to do
	glCullFace(GL_BACK);					//back face culling
	glEnable(GL_CULL_FACE);					
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);      // Really nice perspective calculations
	glEnable(GL_BLEND);						// Enable alpha blending of textures
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	GL_CHECK_ERROR;
	swapBuffers();
}

void base::Window::notifyResize(Point&& s) {
	m_size = s;
	base::FrameBuffer::setScreenSize(s.x, s.y);
	base::Game::s_inst->m_state->onResized(s);
}

void base::Window::notifyFocus(bool focus) {
	m_focus = focus;
	base::Game::s_inst->m_state->onFocusChanged(focus);
}

// -------------------------------------------------------------------- //

#ifdef WIN32
#include "base/window_win32.h"
#endif
#ifdef LINUX
#include "base/window_x11.h"
#endif

const Point& base::Window::getScreenResolution() {
	#ifdef WIN32
	return Win32Window::getScreenResolution();
	#endif
	#ifdef LINUX
	return X11Window::getScreenResolution();
	#endif

	static Point nope;
	return nope;
}
const base::Window::PointList& base::Window::getValidResolutions() {
	#ifdef WIN32
	return Win32Window::getValidResolutions();
	#endif
	#ifdef LINUX
	return X11Window::getValidResolutions();
	#endif

	static PointList nope;
	return nope;
}


