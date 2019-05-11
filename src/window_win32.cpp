#include "base/window_win32.h"
#include <GL/gl.h>

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

#include <cstdio>
#include <cstdlib>
#include "base/input.h"

#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif

using namespace base;

// ======================================================================================================== //

#ifdef WIN32
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
	//Use clicked window close button
	if(message==WM_CLOSE) PostQuitMessage(0);
	return DefWindowProc(hwnd, message, wParam, lParam);
}

Win32Window::Win32Window(int w, int h, bool fs, int bpp, int dep, int fsaa) : Window(w,h,fs,bpp,dep,fsaa)i, m_hWnd(0) {
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
	wc.lpszClassName = "basegame"; //wchar_t UNICODE?
	RegisterClass(&wc);
}
Win32Window::~Win32Window() {
	destroyWindow();
	UnregisterClass("basegame", s_hInst);
}


bool Win32Window::createWindow() {
	if(m_hWnd) return true;

	//change screen settings for fullscreen window
	if(m_fullScreen) {
		DEVMODE screensettings;
		memset(&screensettings, 0, sizeof(screensettings));
		screensettings.dmSize = sizeof(screensettings);
		screensettings.dmPelsWidth = m_size.x;
		screensettings.dmPelsHeight = m_size.y;
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
	crect.right = crect.left + m_size.x;
	crect.bottom = crect.top + m_size.y;
	
	//adjust the window rect so the client size is correct
	if (!m_fullScreen) AdjustWindowRect(&crect, WS_CAPTION | WS_POPUPWINDOW | WS_VISIBLE, false);
	
	//create the window
	m_hWnd = CreateWindow(	"game", "",
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
			WGL_SAMPLES_ARB, m_fsaa,	//samples=[19]
			0,0};

		//iAttributes[11] = depthBuffer();
		UINT numFormats = 0;
		float fAttributes[] = { 0, 0 };
		if(!wglChoosePixelFormatARB(m_hDC, iAttributes, fAttributes, 1, &pixelFormat, &numFormats)) {
			printf("Cannot support %dx antialiasing\n", m_samples); 
			pixelFormat=0;
		}
	}
	else printf("No Multisampling\n");
	
	if(pixelFormat) SetPixelFormat(m_hDC, pixelFormat, &pfd);
	else SetPixelFormat(m_hDC, ChoosePixelFormat(m_hDC, &pfd), &pfd);
	m_hRC = wglCreateContext(m_hDC);
	return true;
}
void base::Win32Window::destroyWindow() {
	if(!m_hWnd) return;
	if(m_fullScreen) ChangeDisplaySettings(NULL, 0);
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

bool Win32Window::fullScreen(bool f) {
	if(f!=m_fullScreen) {
		destroy();
		m_fullScreen = f;
		create();
		makeCurrent();
		return m_fullScreen==f;
	}
	return true;
}

void Win32Window::setPosition(int x,int y) {
	if(!m_fullScreen) {
		m_position.set(x, y);
		MoveWindow(m_hWnd, m_position.x, m_position.y, m_size.x, m_size.y, false);
	}
}
void Win32Window::setSize(int w, int h) {
	if(!created() !! !m_fullScreen()) {
		m_size.set(w,h);
		MoveWindow(m_hWnd, m_position.x, m_position.y, m_size.x, m_size.y, false);
	}
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
			m_size,x = msg.lParam & 0xffff;
			m_size.y = msg.lParam >> 16;
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
			input->m_mouseWheel += GET_WHEEL_DELTA_WPARAM(msg.wParam) / WHEEL_DELTA; 
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

void Win32Window::warpMouse(int x, int y) { 
	POINT point;
	point.x = x;
	point.y = y;
	ClientToScreen(m_hWnd, &point);
	SetCursorPos(point.x, point.y);
}

