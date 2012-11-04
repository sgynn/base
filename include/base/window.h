#ifndef _BASE_WINDOW_
#define _BASE_WINDOW_

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#ifdef LINUX
#include <X11/Xlib.h>
#include <GL/glx.h>
#include <X11/extensions/xf86vmode.h>
#endif

#include "math.h"

namespace base { 
	class Input;
	class Window {
		public:
		static bool initialise();
		static void finalise();

		Window(int width, int height, int bpp=32, int depth=16, bool fullscreen=false, int fsaa=0);
		~Window();
		
		/// set the window title
		void setTitle(const char* title);
		/// move the window to a new position
		void setPosition(int x, int y);
		/// switch between fullscreen and windowed modes
		bool fullScreen(bool f);
		/// get whether the window is fullscreen
		bool fullScreen() const { return m_fullScreen; }
		/// set the sampling for antialiasing
		bool antialiasing(int samples);
		
		/// get window width
		int width() const { return m_width; }
		/// get window height
		int height() const { return m_height; }
		/// Get the window size
		Point size() const { return Point( m_width, m_height ); }
		//get window position
		Point position() const { return m_position; }
		
		/// Get the native screen resolution
		static Point screenResolution();
		
		/// set the window as the curent context
		bool makeCurrent();
		
		/// call this at the end of the drawing loop to swap buffers
		void swapBuffers(); 

		#ifdef WIN32
		HDC getHDC() { return m_hDC; };
		HWND getHWND() { return m_hWnd; };
		#endif

		#ifdef LINUX
		Display *getXDisplay() { return m_display; };
		::Window getXWindow() { return m_window; };
		int getXScreen() { return m_screen; };
		#endif

		//Pump window events and collate any input signals
		uint pumpEvents(Input* input);

		Point queryMouse();
		void warpMouse(int x, int y);

		private:
		static bool s_initialised;
		
		#ifdef LINUX
		Display *m_display;
		XVisualInfo *m_visual;
		Colormap m_colormap;
		XSetWindowAttributes m_swa;
		GLXContext m_context;
		::Window m_window;
		XF86VidModeModeInfo m_deskMode;
		int m_screen;
		#endif
		
		#ifdef WIN32
		HWND m_hWnd;
		HDC m_hDC;
		HGLRC m_hRC;
		#endif
		
		bool create();
		void destroy();
		
		//Data
		int m_samples;
		Point m_position;
		int m_width;
		int m_height;
		int m_colourDepth;
		int m_depthBuffer;
		bool m_fullScreen;
	};
};
	
#endif

