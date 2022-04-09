#include "base/window_em.h"
#include "base/input.h"
#include "base/game.h"

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
}

EMWindow::~EMWindow() {
}

void EMWindow::setSize(int w, int h) {
	m_size.set(w, h);
	float ratio = emscripten_get_device_pixel_ratio();
	emscripten_set_element_css_size(m_canvas, m_size.x/ratio, m_size.y/ratio);
	emscripten_set_canvas_element_size(m_canvas, m_size.x, m_size.y);
}

bool EMWindow::created() const {
	return m_context;
}

bool EMWindow::createWindow() {
	emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, 1, EMWindow::key_callback);
	emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, 1, EMWindow::key_callback);
	emscripten_set_mousemove_callback(m_canvas, 0, 1, EMWindow::mouse_callback);
	emscripten_set_mousedown_callback(m_canvas, 0, 1, EMWindow::mouse_callback);
	emscripten_set_mouseup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, 1, EMWindow::mouse_callback);
	emscripten_set_wheel_callback(m_canvas, 0, 1, EMWindow::wheel_callback);


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


EM_BOOL EMWindow::key_callback(int type, const EmscriptenKeyboardEvent* e, void*) {
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

EM_BOOL EMWindow::mouse_callback(int type, const EmscriptenMouseEvent* e, void*) {
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
EM_BOOL EMWindow::wheel_callback(int type, const EmscriptenWheelEvent* e, void*) {
	Game::input()->m_mouseState.wheel -= e->deltaY / 3.0f;
	return 0;
}

bool EMWindow::setFullScreen(bool fullscreen) {
	EmscriptenFullscreenChangeEvent state;
	emscripten_get_fullscreen_status(&state);
	if(!state.isFullscreen && fullscreen) {
		int r = emscripten_request_fullscreen(m_canvas, 1);
		printf("Fullscreen mode = %s  (%s)\n", emscripten_result_to_string(r), m_canvas);
		m_fullScreen = true;
	}
	else if (state.isFullscreen && !fullscreen) {
		printf("Windowed mode\n");
		emscripten_exit_fullscreen();
		m_fullScreen = false;
	}
	return true;
}

void EMWindow::lockMouse(bool lock) {
	if(lock) emscripten_request_pointerlock(m_canvas, true);
	else emscripten_exit_pointerlock(); 
}





