#pragma once

#include <base/window.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace base {
	class Win32Window : public Window {
		public:
		Win32Window(int w, int h, bool fs=false, int bpp=32, int depth=24, int fsaa=0);
		~Win32Window();

		void setTitle(const char* title) override;
		void setIcon() override;
		void setPosition(int x, int y) override;
		void setSize(int w, int h) override;
		bool setVSync(bool) override;

		bool created() const { return m_hWnd!=0; }
		static const Point& getScreenResolution();
		static const PointList& getValidResolutions();

		bool makeCurrent() override;
		void swapBuffers() override;
		uint pumpEvents(Input* input) override;
		bool warpMouse(int x, int y) override;
		Point queryMouse() override;

		virtual void setCursor(unsigned c) override;
		virtual void createCursor(unsigned c, const char* image, int w, int h, int x=0, int y=0) override;

		bool createWindow() override;
		void destroyWindow() override;

		HDC getHDC() { return m_hDC; };
		HWND getHWND() { return m_hWnd; };
		HGLRC getHGLRC() { return m_hRC; }

		protected:
		HWND  m_hWnd;
		HDC   m_hDC;
		HGLRC m_hRC;
		HCURSOR m_cursors[32];
	};
}

