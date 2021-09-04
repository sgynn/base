#ifndef _BASE_WINDOW_EM_

#include "window.h"

#include <emscripten/emscripten.h>
#include <emscripten/html5.h>

namespace base {
	class EMWindow : public Window {
		public:
		EMWindow(int w, int h, const char* canvas="canvas");
		~EMWindow();

		void setTitle(const char* title) override {}
		void setIcon() override {}
		void setPosition(int x, int y) override {}
		void setSize(int w, int h) override;
		bool created() const override;

		bool makeCurrent() override;
		void swapBuffers() override {}
		uint pumpEvents(Input* input) override { return 0; }
		void warpMouse(int x, int y) override {}
		Point queryMouse() override { return s_mousePos; }

		virtual void setCursor(unsigned c) override {}
		virtual void createCursor(unsigned id, const char* image, int w, int h, int x=0, int y=0) override {}
		
		bool createWindow() override;
		void destroyWindow() override {}

		protected:
		static EM_BOOL key_callback(int type, const EmscriptenKeyboardEvent* e, void*);
		static EM_BOOL mouse_callback(int type, const EmscriptenMouseEvent* e, void*);
		static EM_BOOL wheel_callback(int type, const EmscriptenWheelEvent* e, void*);

		int m_context;
		static Point s_mousePos;
		const char* m_canvas;
	};
}

#endif

