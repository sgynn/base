#pragma once

#include <base/window.h>

#include <X11/Xlib.h>
#include <GL/glx.h>
#include <X11/extensions/xf86vmode.h>

namespace base {
	class X11Window : public Window {
		public:
		X11Window(int w, int h, bool fs=false, int bpp=32, int depth=24, int fsaa=0);
		~X11Window();

		void setTitle(const char* title);
		void setIcon();
		void setPosition(int x, int y);
		void setSize(int w, int h);
		bool setFullScreen(bool f);

		bool created() const { return m_display; }
		static const Point& getScreenResolution();
		static const PointList& getValidResolutions();

		bool makeCurrent();
		void swapBuffers(); 
		uint pumpEvents(Input* input);
		bool warpMouse(int x, int y);
		Point queryMouse();

		virtual void setCursor(unsigned c);
		virtual void createCursor(unsigned id, const char* image, int w, int h, int x=0, int y=0);
		
		bool createWindow();
		void destroyWindow();

		Display *getXDisplay() const { return m_display; }
		::Window getXWindow() const  { return m_window; }
		int getXScreen() const       { return m_screen; }
		const GLXFBConfig& getFBConfig() const { return m_fbConfig; }

		protected:
		Display*             m_display;
		XVisualInfo*         m_visual;
		Colormap             m_colormap;
		XSetWindowAttributes m_swa;
		GLXContext           m_context;
		GLXFBConfig          m_fbConfig;
		::Window             m_window;
		XF86VidModeModeInfo  m_deskMode;
		int                  m_screen;
		size_t               m_cursors[32];
	};
}

