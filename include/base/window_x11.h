#ifndef _BASE_WINDOW_X11_

#include "window.h"

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

		bool created() const { return m_display; }
		static const Point& getScreenResolution();
		static const PointList& getValidResolutions();

		bool makeCurrent();
		void swapBuffers(); 
		uint pumpEvents(Input* input);
		void warpMouse(int x, int y);
		Point queryMouse();

		virtual void setCursor(unsigned c);
		virtual void createCursor(unsigned id, const char* image, int w, int h, int x=0, int y=0);
		
		bool createWindow();
		void destroyWindow();

		Display *getXDisplay() { return m_display; };
		::Window getXWindow()  { return m_window; };
		int getXScreen()       { return m_screen; };

		protected:
		Display*             m_display;
		XVisualInfo*         m_visual;
		Colormap             m_colormap;
		XSetWindowAttributes m_swa;
		GLXContext           m_context;
		::Window             m_window;
		XF86VidModeModeInfo  m_deskMode;
		int                  m_screen;
		size_t               m_cursors[32];
	};
}

#endif

