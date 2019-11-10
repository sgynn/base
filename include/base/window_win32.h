#ifndef _BASE_WINDOW_WIN32_

#include "window.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace base {
	class Win32Window : public Window {
		public:
		Win32Window(int w, int h, bool fs=false, int bpp=32, int depth=24, int fsaa=0);
		~Win32Window();

		void setTitle(const char* title);
		void setIcon();
		void setPosition(int x, int y);
		void setSize(int w, int h);

		bool created() const { return m_hWnd!=0; }
		static const Point& getScreenResolution();
		static const PointList& getValidResolutions();

		bool makeCurrent();
		void swapBuffers(); 
		uint pumpEvents(Input* input);
		void warpMouse(int x, int y);
		Point queryMouse();

		virtual void setCursor(unsigned c);
		virtual void setCursor(const char* image, int w, int h, int mask, int x=0, int y=0);

		bool createWindow();
		void destroyWindow();

		HDC getHDC() { return m_hDC; };
		HWND getHWND() { return m_hWnd; };

		protected:
		HWND  m_hWnd;
		HDC   m_hDC;
		HGLRC m_hRC;
	};
}

#endif

