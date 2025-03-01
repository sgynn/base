#include "base/window_em.h"
#include "base/input.h"
#include "base/game.h"
#include "base/framebuffer.h"

#include <emscripten.h>
#include <emscripten/html5.h>

#include <GL/gl.h>

#include <cstdio>

using namespace base;

Point EMWindow::s_mousePos;

static const char *emscripten_result_to_string(EMSCRIPTEN_RESULT result) {
        if(result == EMSCRIPTEN_RESULT_SUCCESS) return "EMSCRIPTEN_RESULT_SUCCESS";
        if(result == EMSCRIPTEN_RESULT_DEFERRED) return "EMSCRIPTEN_RESULT_DEFERRED";
        if(result == EMSCRIPTEN_RESULT_NOT_SUPPORTED) return "EMSCRIPTEN_RESULT_NOT_SUPPORTED";
        if(result == EMSCRIPTEN_RESULT_FAILED_NOT_DEFERRED) return "EMSCRIPTEN_RESULT_FAILED_NOT_DEFERRED";
        if(result == EMSCRIPTEN_RESULT_INVALID_TARGET) return "EMSCRIPTEN_RESULT_INVALID_TARGET";
        if(result == EMSCRIPTEN_RESULT_UNKNOWN_TARGET) return "EMSCRIPTEN_RESULT_UNKNOWN_TARGET";
        if(result == EMSCRIPTEN_RESULT_INVALID_PARAM) return "EMSCRIPTEN_RESULT_INVALID_PARAM";
        if(result == EMSCRIPTEN_RESULT_FAILED) return "EMSCRIPTEN_RESULT_FAILED";
        if(result == EMSCRIPTEN_RESULT_NO_DATA) return "EMSCRIPTEN_RESULT_NO_DATA";
        return "Unknown EMSCRIPTEN_RESULT!";
}

EMWindow::EMWindow(int w, int h, const char* canvas) : Window(w, h), m_context(0), m_canvas(canvas) {
	// Use size from element if not set
	if(w<=0 || h<=0) {
		emscripten_get_canvas_element_size(m_canvas, &m_size.x, &m_size.y);
		base::FrameBuffer::setScreenSize(m_size.x, m_size.y);
	}
}

EMWindow::~EMWindow() {
}

void EMWindow::setSize(int w, int h) {
	m_size.set(w, h);
	float ratio = 1; //emscripten_get_device_pixel_ratio();
	emscripten_set_element_css_size(m_canvas, m_size.x/ratio, m_size.y/ratio);
	emscripten_set_canvas_element_size(m_canvas, m_size.x, m_size.y);
}

bool EMWindow::created() const {
	return m_context;
}

bool EMWindow::createWindow() {
	emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, 1, EMWindow::keyCallback);
	emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, 1, EMWindow::keyCallback);
	emscripten_set_mousemove_callback(m_canvas, 0, 1, EMWindow::mouseCallback);
	emscripten_set_mousedown_callback(m_canvas, 0, 1, EMWindow::mouseCallback);
	emscripten_set_mouseup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, 1, EMWindow::mouseCallback);
	emscripten_set_wheel_callback(m_canvas, 0, 1, EMWindow::wheelCallback);
	emscripten_set_focus_callback(m_canvas, this, 1, EMWindow::focusCallback);
	emscripten_set_focusout_callback(m_canvas, this, 1, EMWindow::focusCallback);

	emscripten_set_gamepadconnected_callback(this, true, EMWindow::gamepadCallback);
	emscripten_set_gamepaddisconnected_callback(this, true, EMWindow::gamepadCallback);

	// Fullscreen button - must have id="fullscreen"
	emscripten_set_click_callback("#fullscreen", this, 1, [](int type, const EmscriptenMouseEvent* e, void* data) {
		EMWindow* wnd = (EMWindow*)data;
		EmscriptenFullscreenStrategy s;
		memset(&s, 0, sizeof(s));
		s.scaleMode = EMSCRIPTEN_FULLSCREEN_SCALE_STRETCH;
		s.canvasResolutionScaleMode = EMSCRIPTEN_FULLSCREEN_CANVAS_SCALE_HIDEF;
		s.canvasResizedCallbackUserData = data;
		s.canvasResizedCallback = [](int t, const void* r, void* data) {
			Point size;
			EMWindow* wnd = (EMWindow*)data;
			float ratio = emscripten_get_device_pixel_ratio();
			emscripten_get_canvas_element_size(wnd->m_canvas, &size.x, &size.y);
			printf("Resize: %dx%d (%g)\n", size.x, size.y, ratio);
			wnd->notifyResize(std::forward<Point>(size));
			return 0;
		};
		emscripten_request_fullscreen_strategy(wnd->m_canvas, 1, &s);
		return 0;
	});


	// Init webgl
	setSize(m_size.x, m_size.y);
	EmscriptenWebGLContextAttributes attr;
	emscripten_webgl_init_context_attributes(&attr);
	attr.alpha = 0;
	attr.majorVersion = 2;

	m_context = emscripten_webgl_create_context(m_canvas, &attr);

	emscripten_webgl_enable_extension(m_context, "EXT_color_buffer_half_float");
	emscripten_webgl_enable_extension(m_context, "EXT_color_buffer_float");

	return m_context;
}

bool EMWindow::makeCurrent() {
	if(created()) {
		emscripten_webgl_make_context_current(m_context);
	}
	return true;
}


EM_BOOL EMWindow::keyCallback(int type, const EmscriptenKeyboardEvent* e, void*) {
	switch(type) {
	case EMSCRIPTEN_EVENT_KEYDOWN:
		//printf("KeyDown %s (%s %lu : %lu %lu %lu)\n", e->key, e->code, e->location, e->charCode, e->keyCode, e->which);
		Game::input()->setKey(e->keyCode, true);
		break;
	case EMSCRIPTEN_EVENT_KEYUP:
		//printf("KeyUp %s (%s %lu : %lu %lu %lu)\n", e->key, e->code, e->location, e->charCode, e->keyCode, e->which);
		Game::input()->setKey(e->keyCode, false);
		break;
	}
	return 0;
}

EM_BOOL EMWindow::mouseCallback(int type, const EmscriptenMouseEvent* e, void*) {
	switch(type) {
	case EMSCRIPTEN_EVENT_MOUSEDOWN:
		Game::input()->setButton(e->button+1, 1, Point(e->targetX, e->targetY));
		break;
	case EMSCRIPTEN_EVENT_MOUSEUP:
		Game::input()->setButton(e->button+1, 0, Point(e->targetX, e->targetY));
		break;
	case EMSCRIPTEN_EVENT_MOUSEMOVE:
		s_mousePos.x = e->targetX;
		s_mousePos.y = e->targetY;
		Game::input()->m_mouseDelta.x += (int)e->movementX;
		Game::input()->m_mouseDelta.y -= (int)e->movementY;
		break;
	}

//	printf("Mouse %d %ld %ld %hu\n", type, e->canvasX, e->canvasY, e->buttons);
	return 0;
}
EM_BOOL EMWindow::wheelCallback(int type, const EmscriptenWheelEvent* e, void*) {
	Game::input()->m_mouseState.wheel -= fmin(1,fmax(-1,e->deltaY / 3.0f)); // clamp due to inconsistent browsers
	//printf("Mouse Wheel %g  %lu\n", e->deltaY, e->deltaMode);
	return 0;
}

EM_BOOL EMWindow::gamepadCallback(int type, const EmscriptenGamepadEvent *event, void*) {
	Game::input()->initialiseJoysticks(); // connected and disconected events
	return 0;
}

EM_BOOL EMWindow::focusCallback(int type, const EmscriptenFocusEvent* e, void* p) {
	switch(type) {
	case EMSCRIPTEN_EVENT_FOCUS:
		reinterpret_cast<EMWindow*>(p)->notifyFocus(true);
		break;
	case EMSCRIPTEN_EVENT_FOCUSOUT:
		reinterpret_cast<EMWindow*>(p)->notifyFocus(false);
		break;
	}
	return 0;
}

bool EMWindow::setMode(WindowMode mode) {
	bool fullscreen = mode != WindowMode::Window;
	EmscriptenFullscreenChangeEvent state;
	emscripten_get_fullscreen_status(&state);
	if(!state.isFullscreen && fullscreen) {
		int r = emscripten_request_fullscreen(m_canvas, 1);
		printf("Fullscreen mode = %s  (%s)\n", emscripten_result_to_string(r), m_canvas);
		m_windowMode = WindowMode::Fullscreen;
	}
	else if (state.isFullscreen && !fullscreen) {
		printf("Windowed mode\n");
		emscripten_exit_fullscreen();
		m_windowMode = WindowMode::Window;
	}
	return true;
}

void EMWindow::lockMouse(bool lock) {
	if(lock) emscripten_request_pointerlock(m_canvas, true);
	else emscripten_exit_pointerlock(); 
}





