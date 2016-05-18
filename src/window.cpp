#include "base/window.h"
#include <GL/gl.h>


// Antialiasing support
#ifdef LINUX
int attriblistAA[] = { GLX_DEPTH_SIZE, 32, GLX_DOUBLEBUFFER, GLX_RGBA, GLX_SAMPLE_BUFFERS_ARB, GL_TRUE, GLX_SAMPLES_ARB, 2 , None }; 
int attriblist[] = { GLX_DEPTH_SIZE, 32, GLX_DOUBLEBUFFER, GLX_RGBA, None };
#endif 

#ifdef WIN32
typedef BOOL (WINAPI * PFNWGLCHOOSEPIXELFORMATARBPROC) (HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats); 

#define WGL_DRAW_TO_WINDOW_ARB         0x2001
#define WGL_ACCELERATION_ARB           0x2003
#define WGL_SUPPORT_OPENGL_ARB         0x2010
#define WGL_FULL_ACCELERATION_ARB      0x2027
#define WGL_COLOR_BITS_ARB             0x2014
#define WGL_ALPHA_BITS_ARB             0x201B
#define WGL_STENCIL_BITS_ARB           0x2023
#define WGL_DEPTH_BITS_ARB             0x2022
#define WGL_DOUBLE_BUFFER_ARB          0x2011
#define WGL_SAMPLE_BUFFERS_ARB         0x2041
#define WGL_SAMPLES_ARB                0x2042

//Application instance handle
HINSTANCE s_hInst;

#endif


#include <cstdio>
#include <cstdlib>
#include "base/input.h"

#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif

using namespace base;

#ifdef WIN32
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
	//Use clicked window close button
	if(message==WM_CLOSE) PostQuitMessage(0);
	return DefWindowProc(hwnd, message, wParam, lParam);
}
#endif

bool base::Window::s_initialised = false;
bool base::Window::initialise() { 
	if(s_initialised) return true;
	atexit(base::Window::finalise);
	#ifdef WIN32
	WNDCLASS wc;
	s_hInst = GetModuleHandle(NULL);
	//register window class
	wc.style = CS_OWNDC;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = s_hInst;
	wc.hIcon = LoadIcon(s_hInst, IDI_APPLICATION);
	if(!wc.hIcon) wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "game"; //wchar_t UNICODE?
	RegisterClass(&wc);
	#endif
	s_initialised = true;
	return true;
}

void base::Window::finalise() {
	if(!s_initialised) return;
	#ifdef WIN32
	UnregisterClass("game", s_hInst);
	#endif
	s_initialised = false;
}

base::Window::Window(int width, int height, int bpp, int depth, bool fullscreen, int fsaa) 
: m_samples(fsaa), m_width(width), m_height(height), m_colourDepth(bpp), m_depthBuffer(depth), m_fullScreen(fullscreen)
{
	initialise();

	//center window on screen
	Point p = screenResolution();
	m_position.x = p.x/2 - m_width/2;
	m_position.y = p.y/2 - m_height/2;
	
	//signal the windowhandler to create the window
	create();

	//set this window as the current context
	makeCurrent();
	
}

base::Window::~Window() {
	destroy();
}

Point base::Window::screenResolution() {
	Point p;
	#ifdef WIN32
	DEVMODE dmod;
	EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dmod);
	p.x = (int) dmod.dmPelsWidth;
	p.y = (int) dmod.dmPelsHeight;
	#endif
	
	#ifdef LINUX
	Display* display = XOpenDisplay(0);
	int screen = DefaultScreen(display);
	XF86VidModeModeInfo **modes;
	int modeNum;
	XF86VidModeGetAllModeLines(display, screen, &modeNum, &modes);
	p.x = modes[0]->hdisplay;
	p.y = modes[0]->vdisplay;
	XCloseDisplay(display);
	XFree(modes);
	#endif
	
	return p;
}

bool base::Window::create() {

	#ifdef LINUX
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
			if ((modes[i]->hdisplay == m_width) && (modes[i]->vdisplay == m_height)) {
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
		//GLX_SAMPLES,        m_samples,
		None
	};
	//if(!m_samples) visualAttribs[22] = None;

	// Get matching framebuffer configurations
	int fbCount;
	GLXFBConfig* fbc = glXChooseFBConfig(m_display, m_screen, visualAttribs, &fbCount);
	if(!fbc) return false;

	// Possibly select best one from list ?
	GLXFBConfig fbConfig = fbc[0];
	XFree(fbc);
	m_visual = glXGetVisualFromFBConfig(m_display, fbConfig);

	// Extension
	PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB = (PFNGLXCREATECONTEXTATTRIBSARBPROC) glXGetProcAddress((GLubyte*)"glXCreateContextAttribsARB");

	// Create context
	int attribs[] = { GLX_CONTEXT_MAJOR_VERSION_ARB, 3, GLX_CONTEXT_MINOR_VERSION_ARB, 0 };
	m_context = glXCreateContextAttribsARB(m_display, fbConfig, 0, GL_TRUE, attribs);


	/*
	// OLD VERSION //

	//Window Attribute list
	int attriblist[] = { GLX_DEPTH_SIZE, m_depthBuffer, GLX_DOUBLEBUFFER, GLX_RGBA, GLX_SAMPLE_BUFFERS_ARB, GL_TRUE, GLX_SAMPLES_ARB, m_samples , None };  
	attriblist[4] = m_samples? GLX_SAMPLE_BUFFERS_ARB: None;

	m_visual = glXChooseVisual(m_display, m_screen, attriblist);
	if(!m_visual && m_samples>0) {
		printf("Cannot support %dx antialiasing\n", m_samples);
		attriblist[4] = None;
		m_visual = glXChooseVisual(m_display, m_screen, attriblist);
	}
	if(!m_visual) {
		printf("Invalid window\n"); 
		return false;
	}
	m_context = glXCreateContext(m_display, m_visual, 0, GL_TRUE);
	*/


	// Create window
	m_colormap = XCreateColormap(m_display, RootWindow(m_display, m_visual->screen), m_visual->visual, AllocNone);
	m_swa.colormap = m_colormap;
	m_swa.border_pixel = 0;
	m_swa.event_mask =	ExposureMask |
				StructureNotifyMask |
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
					m_width,
					m_height,
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
	
	#endif

	#ifdef WIN32
	
	//change screen settings for fullscreen window
	if(m_fullScreen) {
		DEVMODE screensettings;
		memset(&screensettings, 0, sizeof(screensettings));
		screensettings.dmSize = sizeof(screensettings);
		screensettings.dmPelsWidth = m_width;
		screensettings.dmPelsHeight = m_height;
		screensettings.dmBitsPerPel = m_colourDepth;
		screensettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

		if (ChangeDisplaySettings(&screensettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL) {
			m_fullScreen = false;
			printf("Unable to change to fullscreen.\n"); 
		}
	}
	
	RECT crect;
	crect.top = m_position.y;
	crect.left = m_position.x;
	crect.right = crect.left + m_width;
	crect.bottom = crect.top + m_height;
	
	//adjust the window rect so the client size is correct
	if (!m_fullScreen) AdjustWindowRect(&crect, WS_CAPTION | WS_POPUPWINDOW | WS_VISIBLE, false);
	
	//create the window
	m_hWnd = CreateWindow(	"game",
				"",
				((m_fullScreen) ? 0 : WS_CAPTION) | WS_POPUPWINDOW | WS_VISIBLE,
				((m_fullScreen) ? 0 : m_position.x),
				((m_fullScreen) ? 0 : m_position.y),
				crect.right-crect.left,
				crect.bottom-crect.top,
				NULL, NULL, s_hInst, NULL);
	if(!m_hWnd) {
		printf("Failed to create window [%u]\n", (uint) GetLastError());
		return false;
	}
	m_hDC = GetDC(m_hWnd);
	
	//Set pixel format for the DC
	PIXELFORMATDESCRIPTOR pfd;
	ZeroMemory(&pfd, sizeof(pfd));
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = m_colourDepth;
	pfd.cDepthBits = m_depthBuffer;
	pfd.iLayerType = PFD_MAIN_PLANE;
	
	//set multisampling
	int pixelFormat = 0;
	PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
	if(wglChoosePixelFormatARB!=0) {
		// These Attributes Are The Bits We Want To Test For In Our Sample
		// Everything Is Pretty Standard, The Only One We Want To 
		// Really Focus On Is The SAMPLE BUFFERS ARB And WGL SAMPLES
		// These Two Are Going To Do The Main Testing For Whether Or Not
		// We Support Multisampling On This Hardware
		int iAttributes[] = { WGL_DRAW_TO_WINDOW_ARB,GL_TRUE,
			WGL_SUPPORT_OPENGL_ARB,GL_TRUE,
			WGL_ACCELERATION_ARB,WGL_FULL_ACCELERATION_ARB,
			WGL_COLOR_BITS_ARB,24,
			WGL_ALPHA_BITS_ARB,8,
			WGL_DEPTH_BITS_ARB,16,
			WGL_STENCIL_BITS_ARB,0,
			WGL_DOUBLE_BUFFER_ARB,GL_TRUE,
			WGL_SAMPLE_BUFFERS_ARB,GL_TRUE,
			WGL_SAMPLES_ARB, m_samples,	//samples=[19]
			0,0};

		//iAttributes[11] = depthBuffer();
		UINT numFormats = 0;
		float fAttributes[] = { 0, 0 };
		if(!wglChoosePixelFormatARB(m_hDC, iAttributes, fAttributes, 1, &pixelFormat, &numFormats)) {
			printf("Cannot support %dx antialiasing\n", m_samples); 
			pixelFormat=0;
		}
	} else printf("No Multisampling\n");
	
	if(pixelFormat) SetPixelFormat(m_hDC, pixelFormat, &pfd);
	else SetPixelFormat(m_hDC, ChoosePixelFormat(m_hDC, &pfd), &pfd);
	
	m_hRC = wglCreateContext(m_hDC);
	#endif
	
	// Set up opengl stuff
	makeCurrent();
	
	
	//set up some openGL stuff - seperate function!
	glClearColor( 0.0, 0.0, 0.1f, 1.0f );
	glClear(GL_COLOR_BUFFER_BIT);
	glClearDepth(1.0f);                     // Depth buffer setup
	glEnable(GL_DEPTH_TEST);                // Enables depth testing
	glDepthFunc(GL_LEQUAL);                 // The type of depth testing to do
	glCullFace(GL_BACK);					//back face culling
	glEnable(GL_CULL_FACE);					
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);      // Really nice perspective calculations
	glEnable(GL_BLEND);					// Enable alpha blending of textures
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	int glerr;
	if ((glerr = glGetError()) != 0) {
		printf("An OpenGL error occured: %d\n", glerr); 
	}
	swapBuffers();

	return true;
}

void base::Window::destroy() {
	#ifdef LINUX
	XLockDisplay(m_display);
	if(m_fullScreen) {
		XF86VidModeSwitchToMode(m_display, m_screen, &m_deskMode);
        	XF86VidModeSetViewPort(m_display, m_screen, 0, 0);
	}
	glXDestroyContext(m_display, m_context);
	XDestroyWindow(m_display,m_window);
	XCloseDisplay(m_display);
	#endif
	
	#ifdef WIN32
	if(m_fullScreen) ChangeDisplaySettings(NULL, 0);
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(m_hRC);
	ReleaseDC(m_hWnd, m_hDC);
	DestroyWindow(m_hWnd);
	#endif
}

void base::Window::setTitle(const char *title) {
	#ifdef LINUX
	XStoreName(m_display, m_window, title);
	#endif
	
	#ifdef WIN32
	SetWindowTextA(m_hWnd, title);
	#endif
}

bool base::Window::fullScreen(bool f) {
	if(f!=m_fullScreen) {
		destroy();
		m_fullScreen = f;
		create();
		makeCurrent();
		return m_fullScreen==f;
	}
	return true;
}

bool base::Window::antialiasing(int samples) {
	if(m_samples != samples) {
		m_samples = samples;
		destroy();
		create();
		makeCurrent();
	}
	return true;
}

void base::Window::setPosition(int x,int y) {
	m_position.x = x;
	m_position.y = y;
	#ifdef WIN32
	MoveWindow(m_hWnd, x, y, m_width, m_height, false);
	#endif
	#ifdef LINUX
	XMoveWindow(m_display, m_window, x, y);
	#endif
}

bool base::Window::makeCurrent() {
	#ifdef WIN32
	if(!wglMakeCurrent(m_hDC, m_hRC)) {
		printf("wglMakeCurrent failed [%u]\n", (uint)GetLastError()); 
		return false;
	}
	#endif
	#ifdef LINUX
	glXMakeCurrent(m_display, m_window, m_context);
	#endif
	
	return true;
}

void base::Window::swapBuffers() {
	#ifdef LINUX
	glXSwapBuffers(m_display, m_window);
	#endif
	#ifdef WIN32
	SwapBuffers(m_hDC);
	#endif
}

uint base::Window::pumpEvents(Input* input) {
	#ifdef LINUX
	XEvent event;
	XKeyEvent *keyevent;
	XButtonEvent *buttonevent;
	bool down;
	char* atom;
	while(XPending(getXDisplay())) {
		XNextEvent(getXDisplay(), &event);
		switch (event.type) {
			case ClientMessage:
				atom = XGetAtomName(getXDisplay(), event.xclient.message_type);
				//printf("Close Window\n");
				if(*atom==*"WM_PROTOCOLS") return 0x100;
				break;
			case ConfigureNotify:
				if(event.xconfigure.width!=m_width || event.xconfigure.height!=m_height) {
					m_width = event.xconfigure.width;
					m_height = event.xconfigure.height;
					//printf("Resize Window (%d, %d)\n", m_width, m_height);
				}
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
				if(buttonevent->button==4) input->m_mouseWheel ++;
				else if(buttonevent->button==5) input->m_mouseWheel --; 
				input->setButton( buttonevent->button, 1 );
				break;

			case ButtonRelease:
				buttonevent = (XButtonEvent*)&event;
				input->setButton( buttonevent->button, 0 );
				break;
				
			default:
				break;
		}
	}
	#endif

	#ifdef WIN32
	MSG msg;
	while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
		switch (msg.message) {
		case WM_QUIT:
			return 0x100; //Exit signal
		case WM_SIZE: //Window resized
			m_width = msg.lParam & 0xffff;
			m_height = msg.lParam >> 16;
			break;
		
		//Keyboard
		case WM_KEYDOWN:
		case WM_KEYUP:
			{
			int word = (msg.lParam & 0xffff0000) >> 16;
			bool down = msg.message==WM_KEYDOWN;
			bool extended = (word & 0x100) > 0;
			// bool repeat = (word & 0xf000) == 0x4000; // UNUSED
			int chr = word & 0xff;
			
			//if(repeat) break; //stop repeating keystrokes - maybe handle them differently?
			
			if(extended) chr += 0x80;
			input->setKey(chr, down);
						
			break;
			}
		
		//mouse
		case WM_LBUTTONDOWN:	input->setButton(1, 1); break;
		case WM_LBUTTONUP:		input->setButton(1, 0); break;
		case WM_MBUTTONDOWN:	input->setButton(2, 1); break;
		case WM_MBUTTONUP:		input->setButton(2, 0); break;
		case WM_RBUTTONDOWN:	input->setButton(3, 1); break;
		case WM_RBUTTONUP:		input->setButton(3, 0); break;
		case WM_MOUSEWHEEL:
			input->m_mouseWheel += GET_WHEEL_DELTA_WPARAM(msg.wParam) / WHEEL_DELTA; 
			break;
		
		case WM_MOUSEMOVE: //mouse moved, if not polling need some way of getting this? is it nessesary?
		default:
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	#endif
	return 0;
}



Point base::Window::queryMouse() {
	Point result;
	#ifdef WIN32
	POINT pnt;
	GetCursorPos(&pnt);
	ScreenToClient(m_hWnd, &pnt);
	result.x = pnt.x;
	result.y = pnt.y;
	#endif
	
	#ifdef LINUX
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
	#endif
	return result;
}

void base::Window::warpMouse(int x, int y) { 
	#ifdef WIN32
	POINT point;
	point.x = x;
	point.y = y;
	ClientToScreen(m_hWnd, &point);
	SetCursorPos(point.x, point.y);
	#elif LINUX
	XWarpPointer(getXDisplay(), 0, getXWindow(), 0, 0, 0, 0, x, y);
	#endif
}

