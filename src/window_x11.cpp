#define GLX_GLXEXT_PROTOTYPES
#include "base/window_x11.h"
#include <cstring>
#include <GL/gl.h>
#include <GL/glxext.h>
int attriblistAA[] = { GLX_DEPTH_SIZE, 32, GLX_DOUBLEBUFFER, GLX_RGBA, GLX_SAMPLE_BUFFERS_ARB, GL_TRUE, GLX_SAMPLES_ARB, 2 , None }; 
int attriblist[] = { GLX_DEPTH_SIZE, 32, GLX_DOUBLEBUFFER, GLX_RGBA, None };

#include <cstdio>
#include <cstdlib>
#include "base/input.h"

using namespace base;

X11Window::X11Window(int w, int h, bool fs, int bpp, int dep, int fsaa) : base::Window(w,h,fs,bpp,dep,fsaa), m_display(0) {
	memset(m_cursors, 0, sizeof(m_cursors));
}

X11Window::~X11Window() {
	for(int c: m_cursors) if(c) XFreeCursor(m_display, c);
	destroyWindow();
}


bool X11Window::createWindow() {
	if(m_display) return true; // Already created

	m_display = XOpenDisplay(0);
	if (m_display == 0) {
		printf("Cannot open X11 display\n"); 
		return false;
	}
	
	m_screen = DefaultScreen(m_display);

	//Get video modes
	XF86VidModeModeInfo **modes;
	int modeNum;
	int bestMode=0;

	if (m_fullScreen) {
		XF86VidModeGetAllModeLines(m_display, m_screen, &modeNum, &modes);
		//Save desktop mode
		m_deskMode = *modes[0];
	
		//Find the mode we want.
		for(int i=0; i<modeNum; i++) {
			if ((modes[i]->hdisplay == m_size.x) && (modes[i]->vdisplay == m_size.y)) {
				bestMode = i;
			}
		}
	}

	// NEW VERSION //
	static int visualAttribs [] = {
		GLX_X_RENDERABLE,   True,
		GLX_DRAWABLE_TYPE,  GLX_WINDOW_BIT,
		GLX_RENDER_TYPE,    GLX_RGBA_BIT,
		GLX_X_VISUAL_TYPE,  GLX_TRUE_COLOR,
		GLX_RED_SIZE,       8,
		GLX_GREEN_SIZE,     8,
		GLX_BLUE_SIZE,      8,
		GLX_ALPHA_SIZE,     8,
		GLX_DEPTH_SIZE,     24,
		GLX_STENCIL_SIZE,   8,
		GLX_DOUBLEBUFFER,   True,
		//GLX_SAMPLE_BUFFERS, 1,
		//GLX_SAMPLES,        m_fsaa,
		None
	};
	//if(!m_samples) visualAttribs[22] = None;

	// Get matching framebuffer configurations
	int fbCount;
	GLXFBConfig* fbc = glXChooseFBConfig(m_display, m_screen, visualAttribs, &fbCount);
	if(!fbc) {
		printf("glXChooseFBConfig error\n");
		XCloseDisplay(m_display);
		m_display = 0;
		return false;
	}

	// Possibly select best one from list ?
	m_fbConfig = fbc[0];
	XFree(fbc);
	m_visual = glXGetVisualFromFBConfig(m_display, m_fbConfig);

	// Extension
	PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB = (PFNGLXCREATECONTEXTATTRIBSARBPROC) glXGetProcAddress((GLubyte*)"glXCreateContextAttribsARB");

	// Create context
	int attribs[] = {
		GLX_CONTEXT_MAJOR_VERSION_ARB, 4,
		GLX_CONTEXT_MINOR_VERSION_ARB, 0,
		GLX_CONTEXT_PROFILE_MASK_ARB,  GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
		None
	};
	m_context = glXCreateContextAttribsARB(m_display, m_fbConfig, 0, GL_TRUE, attribs);

	// Create window
	m_colormap = XCreateColormap(m_display, RootWindow(m_display, m_visual->screen), m_visual->visual, AllocNone);
	m_swa.colormap = m_colormap;
	m_swa.border_pixel = 0;
	m_swa.event_mask =	ExposureMask |
				StructureNotifyMask |
				FocusChangeMask |
				KeyReleaseMask |
				KeyPressMask |
				ButtonPressMask |
				ButtonReleaseMask;

	if(m_fullScreen) {
		XF86VidModeSwitchToMode(m_display, m_screen, modes[bestMode]);
        	XF86VidModeSetViewPort(m_display, m_screen, 0, 0);
		m_swa.override_redirect = true;
		XFree(modes);
	}

	m_window = XCreateWindow(	m_display,
					RootWindow(m_display,m_visual->screen),
					(m_fullScreen) ? 0 : m_position.x,
					(m_fullScreen) ? 0 : m_position.y,
					m_size.x,
					m_size.y,
					0,
					m_visual->depth,
					InputOutput,
					m_visual->visual,
					CWBorderPixel|CWColormap|CWEventMask | (m_fullScreen? CWOverrideRedirect : 0),
					&m_swa);

	//Put window on the display.
	XMapWindow(m_display, m_window);

	if(m_fullScreen) {
		XWarpPointer(m_display, None, m_window, 0, 0, 0, 0, 0, 0);
		XGrabKeyboard(m_display, m_window, true, GrabModeAsync, GrabModeAsync, CurrentTime);
		XGrabPointer(m_display, m_window, true, ButtonPressMask, GrabModeAsync, GrabModeAsync, m_window, None, CurrentTime);
	}

	//Map close button event
	Atom wmDelete = XInternAtom(m_display, "WM_DELETE_WINDOW", true);
	XSetWMProtocols(m_display, m_window, &wmDelete, 1);
	XFree(m_visual);
	return true;
}

void X11Window::destroyWindow() {
	if(!m_display) return;
	XLockDisplay(m_display);
	if(m_fullScreen) {
		XF86VidModeSwitchToMode(m_display, m_screen, &m_deskMode);
		XF86VidModeSetViewPort(m_display, m_screen, 0, 0);
	}
	glXDestroyContext(m_display, m_context);
	XDestroyWindow(m_display,m_window);
	XCloseDisplay(m_display);
	m_context = 0;
	m_display = 0;
	m_window = 0;
}

// =================================================================================================== //

const Point& X11Window::getScreenResolution() {
	static Point p(0,0);
	if(p.x==0) {
		Display* display = XOpenDisplay(0);
		int screen = DefaultScreen(display);
		XF86VidModeModeInfo **modes;
		int modeNum;
		XF86VidModeGetAllModeLines(display, screen, &modeNum, &modes);
		p.x = modes[0]->hdisplay;
		p.y = modes[0]->vdisplay;
		XCloseDisplay(display);
		XFree(modes);
	}
	return p;
}

const base::Window::PointList& X11Window::getValidResolutions() {
	static PointList list;
	if(list.empty()) {
		Display* display = XOpenDisplay(0);
		int screen = DefaultScreen(display);
		XF86VidModeModeInfo **modes;
		int modeNum;
		XF86VidModeGetAllModeLines(display, screen, &modeNum, &modes);
		for(int i=0; i<modeNum; ++i) {
			Point p(modes[i]->hdisplay, modes[i]->vdisplay);
			for(size_t i=0; i<list.size(); ++i) if(list[i]==p) p.x=0;
			if(p.x>0) list.push_back(p);
		}
		XCloseDisplay(display);
		XFree(modes);
	}
	return list;
}

void X11Window::setTitle(const char *title) {
	XStoreName(m_display, m_window, title);
}
void X11Window::setIcon() {
}

void X11Window::setPosition(int x,int y) {
	m_position.set(x, y);
	if(created()) XMoveWindow(m_display, m_window, x, y);
}
void X11Window::setSize(int w,int h) {
	m_size.set(w, h);
	if(created()) {
		XResizeWindow(m_display, m_window, w, h);
		// Get actual buffer size
		unsigned int sw, sh;
		GLXDrawable drawable = glXGetCurrentDrawable();
		glXQueryDrawable(m_display, drawable, GLX_WIDTH, &sw);
		glXQueryDrawable(m_display, drawable, GLX_HEIGHT, &sh);
	}
}

bool X11Window::setFullScreen(bool full) {
	if(full==m_fullScreen) return true;
	m_fullScreen = full;
	if(!created()) return false;

	// Try sending window manager a message
	XEvent e;
	memset(&e, 0, sizeof(XEvent));
	e.xany.type = ClientMessage;
	e.xclient.message_type = XInternAtom(m_display, "_NET_WM_STATE", false);
	e.xclient.format = 32;
	e.xclient.window = m_window;
	e.xclient.data.l[0] = XInternAtom(m_display, full? "_NET_WM_STATE_ADD": "_NET_WM_STATE_REMOVE", false);
	e.xclient.data.l[1] = XInternAtom(m_display, "_NET_WM_STATE_FULLSCREEN", false);
	e.xclient.data.l[3] = 0l;
	XSendEvent(m_display, RootWindow(m_display, m_screen), 0, SubstructureNotifyMask | SubstructureRedirectMask, &e);
	XFlush(m_display);
	printf("Fullscreen message sent %d\n", full);


	return true;
}

bool X11Window::setVSync(bool on) {
	glXSwapIntervalEXT(m_display, glXGetCurrentDrawable(), on? 1: 0);
	return true;
}

bool X11Window::makeCurrent() {
	glXMakeCurrent(m_display, m_window, m_context);
	return true;
}
void X11Window::swapBuffers() {
	glXSwapBuffers(m_display, m_window);
}

// ================================================================================================= //

uint X11Window::pumpEvents(Input* input) {
	XEvent event;
	XKeyEvent *keyevent;
	XButtonEvent *buttonevent;
	bool down;
	while(XPending(getXDisplay())) {
		XNextEvent(getXDisplay(), &event);
		switch (event.type) {
			case ClientMessage:
				{
				char* atom = XGetAtomName(getXDisplay(), event.xclient.message_type);
				bool closeMessage = strcmp(atom, "WM_PROTOCOLS") == 0;
				XFree(atom);
				if(closeMessage) return 0x100;
				}
				break;
			case ConfigureNotify:
				if(event.xconfigure.width!=m_size.x || event.xconfigure.height!=m_size.y) {
					notifyResize(Point(event.xconfigure.width, event.xconfigure.height));
				}
				break;

			case FocusIn:
				notifyFocus(true);
				break;
			case FocusOut:
				notifyFocus(false);
				break;
				
			
			//Keyboard input
			case KeyPress:
			case KeyRelease:
				keyevent = (XKeyEvent*)&event;
				down = event.type == KeyPress;
				input->setKey(keyevent->keycode, down);
				break;
			
			//mouse input  -  butons 4 and 5 are the mouse wheel
			case ButtonPress:
				buttonevent = (XButtonEvent*)&event;
				if(buttonevent->button==4) input->m_mouseState.wheel ++;
				else if(buttonevent->button==5) input->m_mouseState.wheel --; 
				input->setButton( buttonevent->button, 1, Point(buttonevent->x, buttonevent->y));
				break;

			case ButtonRelease:
				buttonevent = (XButtonEvent*)&event;
				input->setButton( buttonevent->button, 0, Point(buttonevent->x, buttonevent->y));
				break;
				
			default:
				break;
		}
	}
	return 0;
}


// ================================================================================================= //

Point X11Window::queryMouse() {
	Point result;
	::Window wroot, wchild;
	int rx,ry;
	unsigned int mask;
	XQueryPointer(	getXDisplay(),
			getXWindow(),
			&wroot,	//Root Window
			&wchild,//Child Window
			&rx,	//Root X
			&ry,	//Root Y
			&result.x,	//Window X -- newX set here
			&result.y,	//Window Y -- newY set here
			&mask);	//Mask
	return result;
}

bool X11Window::warpMouse(int x, int y) { 
	XWarpPointer(getXDisplay(), 0, getXWindow(), 0, 0, 0, 0, x, y);
	return true;
}


// ================================================================================================= //

#include <X11/cursorfont.h>
#include <X11/Xcursor/Xcursor.h>

size_t createBlankCursor(Display* display) {
	static char data[] = {0,0,0,0,0,0,0,0};
	XColor black { 0,0,0 };
	Pixmap empty = XCreateBitmapFromData(display, DefaultRootWindow(display), data, 8, 8);
	Cursor cur = XCreatePixmapCursor(display, empty, empty, &black, &black, 0, 0);
	XFreePixmap(display, empty);
	return cur;
}

void X11Window::setCursor(unsigned c) {
	if(c==m_cursor) return;
	m_cursor = c;
	if(!m_cursors[c]) {
		switch(c) {
		default:               c = CURSOR_DEFAULT; break;
		case CURSOR_NONE:      m_cursors[c] = createBlankCursor(m_display);  break;
		case CURSOR_DEFAULT:   m_cursors[c] = XCreateFontCursor(m_display, XC_left_ptr); break;
		case CURSOR_BUSY:      m_cursors[c] = XCreateFontCursor(m_display, XC_watch); break;
		case CURSOR_HAND:      m_cursors[c] = XCreateFontCursor(m_display, XC_hand1); break;
		case CURSOR_CROSSHAIR: m_cursors[c] = XCreateFontCursor(m_display, XC_crosshair); break;
		case CURSOR_I:         m_cursors[c] = XCreateFontCursor(m_display, XC_xterm); break;
		case CURSOR_NO:        m_cursors[c] = XCreateFontCursor(m_display, XC_X_cursor); break;
		case CURSOR_MOVE:      m_cursors[c] = XCreateFontCursor(m_display, XC_fleur); break;
		case CURSOR_SIZE_V:    m_cursors[c] = XCreateFontCursor(m_display, XC_sb_v_double_arrow); break;
		case CURSOR_SIZE_H:    m_cursors[c] = XCreateFontCursor(m_display, XC_sb_h_double_arrow); break;
		case CURSOR_SIZE_BD:
			m_cursors[c] = XcursorLibraryLoadCursor(m_display, "bd_double_arrow");
			if(!m_cursors[c]) m_cursors[c] = XcursorLibraryLoadCursor(m_display, "size_bdiag");
			break;
		case CURSOR_SIZE_FD:
			m_cursors[c] = XcursorLibraryLoadCursor(m_display, "fd_double_arrow");
			if(!m_cursors[c]) m_cursors[c] = XcursorLibraryLoadCursor(m_display, "size_fdiag");
			break;
		}
	}
	if(m_cursors[c]) XDefineCursor(m_display, m_window, m_cursors[c]);
	else XUndefineCursor(m_display, m_window);
}

void X11Window::createCursor(unsigned id, const char* image, int w, int h, int x, int y) {
	XcursorImage* cimg = XcursorImageCreate(w, h);
	cimg->xhot = x;
	cimg->yhot = y;
	memcpy(cimg->pixels, image, w*h); // ARGB data
	Cursor cur = XcursorImageLoadCursor(m_display, cimg);
	XcursorImageDestroy(cimg);
	if(m_cursors[id]) XFreeCursor(m_display, id);
	m_cursors[id] = cur;
	if(id == m_cursor) XDefineCursor(m_display, m_window, cur);
}

