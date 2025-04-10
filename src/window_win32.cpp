#include "base/window_win32.h"
#include "base/opengl.h"

typedef BOOL (WINAPI * PFNWGLCHOOSEPIXELFORMATARBPROC) (HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats); 
typedef HGLRC (WINAPI * PFNWGLCREATECONTEXTATTRIBSARBPROC) (HDC hdc, HGLRC hshareContext, const int *attribList);
typedef BOOL (WINAPI * PFNWGLSWAPINTERVALEXTPROC) (int);
static PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = nullptr;

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
#define WGL_CONTEXT_MAJOR_VERSION_ARB  0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB  0x2092
#define WGL_CONTEXT_FLAGS_ARB          0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB   0x9126

#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB          0x0001
#define WGL_CONTEXT_COMPATABILITY_PROFILE_BIT_ARB 0x0002

//Application instance handle
HINSTANCE s_hInst;

#include <cstdio>
#include <cstdlib>
#include "base/input.h"

#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif

#ifndef WIN32
#error "This file should not be compiled"
#endif

using namespace base;

// ======================================================================================================== //

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
	//Use clicked window close button
	if(message==WM_CLOSE) PostQuitMessage(0);
	else if(message==WM_SIZE) PostMessageW(hwnd, WM_SIZE, wParam, lParam); // Forward as message
	return DefWindowProc(hwnd, message, wParam, lParam);
}

Win32Window::Win32Window(int w, int h, WindowMode mode, int bpp, int dep, int fsaa) : Window(w,h,mode,bpp,dep,fsaa), m_hWnd(0) {
	WNDCLASS wc;
	s_hInst = GetModuleHandle(NULL);
	//register window class
	wc.style = CS_OWNDC;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = s_hInst;
	wc.hIcon = LoadIcon(s_hInst, "APPICON");
	if(!wc.hIcon) wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "basegame"; //wchar_t UNICODE?
	RegisterClass(&wc);

	memset(m_cursors, 0, sizeof(m_cursors));
}
Win32Window::~Win32Window() {
	destroyWindow();
	UnregisterClass("basegame", s_hInst);
}


bool Win32Window::createWindow() {
	if(m_hWnd) return true;

	//change screen settings for fullscreen window
	if(m_windowMode == WindowMode::Fullscreen) {
		DEVMODE screensettings;
		memset(&screensettings, 0, sizeof(screensettings));
		screensettings.dmSize = sizeof(screensettings);
		screensettings.dmPelsWidth = m_size.x;
		screensettings.dmPelsHeight = m_size.y;
		screensettings.dmBitsPerPel = m_colourDepth;
		screensettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
		if(ChangeDisplaySettings(&screensettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL) {
			m_windowMode = WindowMode::Window; // Perhaps use borderless instead ?
			printf("Unable to change to fullscreen.\n"); 
		}
	}
	
	RECT crect;
	crect.top = m_position.y;
	crect.left = m_position.x;
	crect.right = crect.left + m_size.x;
	crect.bottom = crect.top + m_size.y;
	
	bool fullscreen = m_windowMode != WindowMode::Window;
	unsigned windowStyle = WS_OVERLAPPEDWINDOW;
	//unsigned windowStyle = WS_CAPTION | WS_POPUPWINDOW;

	
	//adjust the window rect so the client size is correct
	if(!fullscreen) AdjustWindowRect(&crect, windowStyle | WS_VISIBLE, false);
	
	//create the window
	m_hWnd = CreateWindow(	"basegame", "",
				(fullscreen ? 0 : windowStyle) | WS_POPUPWINDOW | WS_VISIBLE,
				fullscreen ? 0 : m_position.x,
				fullscreen ? 0 : m_position.y,
				crect.right - crect.left,
				crect.bottom - crect.top,
				NULL, NULL, s_hInst, NULL);
	if(!m_hWnd) {
		printf("Failed to create window [%u]\n", (uint) GetLastError());
		return false;
	}

//	HICON hIcon = LoadIcon(s_hInst, "ICON");
//	if(hIcon) {
//		SendMessage(m_hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
//		SendMessage(m_hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
//	}


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
	
	// Need a ridiculous temporary context to get the function to create a context
	SetPixelFormat(m_hDC, ChoosePixelFormat(m_hDC, &pfd), &pfd);
	m_hRC = wglCreateContext(m_hDC);
	wglMakeCurrent(m_hDC, m_hRC);
	PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
	PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
	wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC) wglGetProcAddress("wglSwapIntervalEXT");
	wglDeleteContext(m_hRC);

	//set multisampling
	int pixelFormat = 0;
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
			WGL_SAMPLES_ARB, m_fsaa,	//samples=[19]
			0,0};

		//iAttributes[11] = depthBuffer();
		UINT numFormats = 0;
		float fAttributes[] = { 0, 0 };
		if(!wglChoosePixelFormatARB(m_hDC, iAttributes, fAttributes, 1, &pixelFormat, &numFormats)) {
			printf("Cannot support %dx antialiasing\n", m_fsaa); 
			pixelFormat=0;
		}
	}
	else printf("No Multisampling\n");

	int contextAttribs[] = {
		WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
		WGL_CONTEXT_MINOR_VERSION_ARB, 0,
		WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
		0
	};

	
	if(pixelFormat) SetPixelFormat(m_hDC, pixelFormat, &pfd);
	else SetPixelFormat(m_hDC, ChoosePixelFormat(m_hDC, &pfd), &pfd);

	if(wglCreateContextAttribsARB!=0) {
		m_hRC = wglCreateContextAttribsARB(m_hDC, 0, contextAttribs);
	}
	else {
		printf("Legacy context created\n");
		m_hRC = wglCreateContext(m_hDC);
	}


	// Need context to be current before initialising extensions
	makeCurrent();
	if(!initialiseOpenGLExtensions()) printf("Fatal: Failed to initialise extensions.\n");
	return true;
}
void base::Win32Window::destroyWindow() {
	if(!m_hWnd) return;
	if(m_windowMode == WindowMode::Fullscreen) ChangeDisplaySettings(NULL, 0);
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(m_hRC);
	ReleaseDC(m_hWnd, m_hDC);
	DestroyWindow(m_hWnd);
	m_hWnd = 0;
}

// =========================================================================================== //

const Point& Win32Window::getScreenResolution() {
	static Point p(0,0);
	if(p.x==0) {
		DEVMODE dmod;
		EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dmod);
		p.x = (int) dmod.dmPelsWidth;
		p.y = (int) dmod.dmPelsHeight;
	}
	return p;
}

const Window::PointList& Win32Window::getValidResolutions() {
	static PointList list;
	if(list.empty()) {
		DEVMODE dmod;
		int index = 0;
		while(EnumDisplaySettings(NULL, index++, &dmod)) {
			Point p((int)dmod.dmPelsWidth, (int)dmod.dmPelsHeight);
			for(size_t i=0; i<list.size(); ++i) if(list[i]==p) p.x=0;
			if(p.x>0) list.push_back(p);
		}

	}
	return list;
}

void Win32Window::setTitle(const char *title) {
	SetWindowTextA(m_hWnd, title);
}

void Win32Window::setIcon() {
}

void Win32Window::setPosition(int x,int y) {
	if(m_windowMode == WindowMode::Window) {
		m_position.set(x, y);
		MoveWindow(m_hWnd, m_position.x, m_position.y, m_size.x, m_size.y, false);
	}
}

void Win32Window::setSize(int w, int h) {
	if(!created() || m_windowMode == WindowMode::Window) {
		m_size.set(w,h);

		RECT rect;
		rect.top = m_position.y;
		rect.left = m_position.x;
		rect.right = rect.left + m_size.x;
		rect.bottom = rect.top + m_size.y;
		
		//adjust the window rect so the client size is correct
		if(m_windowMode == WindowMode::Window) AdjustWindowRect(&rect, WS_CAPTION | WS_POPUPWINDOW | WS_VISIBLE, false);
		MoveWindow(m_hWnd, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top, false);
	}
}

bool Win32Window::setVSync(bool on) {
	return wglSwapIntervalEXT(on? 1: 0);
}


bool Win32Window::makeCurrent() {
	if(!wglMakeCurrent(m_hDC, m_hRC)) {
		printf("wglMakeCurrent failed [%u]\n", (uint)GetLastError()); 
		return false;
	}
	return true;
}

void Win32Window::swapBuffers() {
	SwapBuffers(m_hDC);
}

// ==================================================================================================== //

uint Win32Window::pumpEvents(Input* input) {
	MSG msg;
	while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
		switch (msg.message) {
		case WM_QUIT:
			return 0x100; //Exit signal
		case WM_SIZE: //Window resized
		case WM_EXITSIZEMOVE:
			notifyResize(Point(msg.lParam & 0xffff, msg.lParam >> 16));
			break;
		case WM_SETFOCUS:
			notifyFocus(true);
			break;
		case WM_KILLFOCUS:
			notifyFocus(false);
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
		//#define MPOS(msg) Point( GET_X_LPARAM(msg.lParam), GET_Y_LPARAM(msg.lParam) )
		#define MPOS(msg) Point( LOWORD(msg.lParam), HIWORD(msg.lParam) )

		case WM_LBUTTONDOWN:	input->setButton(1, 1, MPOS(msg)); break;
		case WM_LBUTTONUP:		input->setButton(1, 0, MPOS(msg)); break;
		case WM_MBUTTONDOWN:	input->setButton(2, 1, MPOS(msg)); break;
		case WM_MBUTTONUP:		input->setButton(2, 0, MPOS(msg)); break;
		case WM_RBUTTONDOWN:	input->setButton(3, 1, MPOS(msg)); break;
		case WM_RBUTTONUP:		input->setButton(3, 0, MPOS(msg)); break;
		case WM_MOUSEWHEEL:
			input->m_mouseState.wheel += (float)GET_WHEEL_DELTA_WPARAM(msg.wParam) / WHEEL_DELTA; 
			break;
		
		case WM_MOUSEMOVE: //mouse moved, if not polling need some way of getting this? is it nessesary?
		default:
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return 0;
}

// ==================================================================================================== //

Point Win32Window::queryMouse() {
	POINT pnt;
	GetCursorPos(&pnt);
	ScreenToClient(m_hWnd, &pnt);
	return Point(pnt.x, pnt.y);
}

bool Win32Window::warpMouse(int x, int y) { 
	POINT point;
	point.x = x;
	point.y = y;
	ClientToScreen(m_hWnd, &point);
	SetCursorPos(point.x, point.y);
	return true;
}

// ==================================================================================================== //

void Win32Window::setCursor(unsigned c) {
	if(c==m_cursor) return;
	if((c==CURSOR_NONE) != (m_cursor==CURSOR_NONE)) ShowCursor(c!=CURSOR_NONE);
	if(c!=CURSOR_NONE && !m_cursors[c]) {
		switch(c) {
		default:
		case CURSOR_DEFAULT:   m_cursors[c] = LoadCursor(s_hInst, IDC_ARROW); break;
		case CURSOR_BUSY:      m_cursors[c] = LoadCursor(s_hInst, IDC_WAIT); break;
		case CURSOR_HAND:      m_cursors[c] = LoadCursor(s_hInst, IDC_HAND); break;
		case CURSOR_CROSSHAIR: m_cursors[c] = LoadCursor(s_hInst, IDC_CROSS); break;
		case CURSOR_I:         m_cursors[c] = LoadCursor(s_hInst, IDC_IBEAM); break;
		case CURSOR_NO:        m_cursors[c] = LoadCursor(s_hInst, IDC_NO); break;
		case CURSOR_MOVE:      m_cursors[c] = LoadCursor(s_hInst, IDC_SIZEALL); break;
		case CURSOR_SIZE_V:    m_cursors[c] = LoadCursor(s_hInst, IDC_SIZENS); break;
		case CURSOR_SIZE_H:    m_cursors[c] = LoadCursor(s_hInst, IDC_SIZEWE); break;
		case CURSOR_SIZE_BD:   m_cursors[c] = LoadCursor(s_hInst, IDC_SIZENESW); break;
		case CURSOR_SIZE_FD:   m_cursors[c] = LoadCursor(s_hInst, IDC_SIZENWSE); break;
		}
	}
	SetCursor(m_cursors[c]);
	m_cursor = c;
}

void Win32Window::createCursor(unsigned id, const char* image, int w, int h, int hotx, int hoty) {
	// Create bitmap objects
	HDC andDC = CreateCompatibleDC(m_hDC);
	HDC xorDC = CreateCompatibleDC(m_hDC);
	HBITMAP andMap = CreateCompatibleBitmap(m_hDC, w, h);
	HBITMAP xorMap = CreateCompatibleBitmap(m_hDC, w, h);
	HGDIOBJ oldAnd = SelectObject(andDC, andMap);
	HGDIOBJ oldXor = SelectObject(xorDC, xorMap);
	for(int x=0; x<w; ++x) {
		for(int y=0; y<w; ++y) {
			const char* pixel = image + (x + y*w) * 4;
			if(pixel[0]>=0) {
				SetPixel(andDC, x, y, RGB(255,255,255));
				SetPixel(xorDC, x, y, RGB(0,0,0));
			}
			else {
				SetPixel(andDC, x, y, RGB(0,0,0));
				SetPixel(xorDC, x, y, RGB(pixel[1], pixel[2], pixel[3]));
			}
		}
	}
	SelectObject(andDC, oldAnd);
	SelectObject(xorDC, oldXor);
	DeleteDC(andDC);
	DeleteDC(xorDC);

	ICONINFO iconInfo { FALSE, (uint)hotx, (uint)hoty, andMap, xorMap };
	HCURSOR cur = CreateIconIndirect(&iconInfo);
	m_cursors[id] = cur;
	if(m_cursor == id) SetCursor(cur);
}
