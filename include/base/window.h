#ifndef _BASE_WINDOW_
#define _BASE_WINDOW_

#include "math.h"
#include <vector>

namespace base { 
	class Input;

	enum CursorType { CURSOR_NONE, CURSOR_DEFAULT, CURSOR_BUSY, CURSOR_HAND, CURSOR_CROSSHAIR, CURSOR_I, CURSOR_NO,
						CURSOR_MOVE, CURSOR_NS, CURSOR_EW, CURSOR_NESW, CURSOR_NWSE, CURSOR_CUSTOM };

	/// Window interface. Implementations in X11Window or Win32Window
	class Window {
		public:
		static bool initialise();
		static void finalise();

		Window(int width, int height, bool fullscreen=false, int bpp=32, int depth=24, int fsaa=0);
		virtual ~Window();
		
		virtual void setTitle(const char* title) = 0;	/// set the window title
		virtual void setIcon() = 0;						/// Set window icon
		virtual void setPosition(int x, int y) = 0;  	/// move the window to a new position
		virtual void setSize(int width, int height) = 0;/// Resize window
		virtual bool setFullScreen(bool f);		 		/// switch between fullscreen and windowed modes
		virtual bool setFSAA(int fsaa);

		virtual bool created() const = 0;
		virtual bool createWindow() = 0;
		virtual void destroyWindow() = 0;

		void         clear();
		void         centreScreen(); // Center the window on screen

		/// Get window properties
		bool isFullscreen() const { return m_fullScreen; }
		int  getFSAA() const      { return m_fsaa; }
		const Point& getPosition() const { return m_position; }
		const Point& getSize() const { return m_size; }
		

		/// Get the native screen resolution
		typedef std::vector<Point> PointList;
		static const Point&     getScreenResolution();
		static const PointList& getValidResolutions();
		
		/// set the window as the curent context
		virtual bool makeCurrent() = 0;
		
		/// call this at the end of the drawing loop to swap buffers
		virtual void swapBuffers() = 0;

		/// Process window events
		virtual uint pumpEvents(Input* input) = 0;

		/// Mouse data comes from window. Used by Input
		virtual Point queryMouse() { return Point(); }
		virtual bool warpMouse(int x, int y) { return false; }

		virtual void setCursor(unsigned c) = 0;
		virtual void createCursor(unsigned c, const char* image, int w, int h, int x=0, int y=0) = 0; // ARGB image

		protected:
		//Data
		int m_fsaa;
		Point m_position;
		Point m_size;
		int m_colourDepth;
		int m_depthBuffer;
		bool m_fullScreen;
		bool m_created;
	};
};
	
#endif

