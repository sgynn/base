#pragma once

#include <base/framebuffer.h>
#include <base/gamestate.h>
#include <base/console.h>

namespace base {

class ConsoleComponent : public GameStateComponent {
	public:
	ConsoleComponent(const script::Variable& root, gui::Font* font=nullptr) : GameStateComponent(-90, 90, PERSISTENT) {
		m_console = new Console(font, Point(FrameBuffer::Screen.width(), 400));
		if(root.isObject()) m_console->root() = root;
	}
	~ConsoleComponent() {
		delete m_console;
	}
	void setFont(gui::Font* font, int size) {
		m_console->setFont(font, size);
	}
	void update() override {
		m_console->update();
		if(m_console->isVisible()) setComponentFlags(BLOCK_KEYS | BLOCK_GRAB);
	}
	void draw() override {
		m_console->draw();
	}
	script::Variable& getData() {
		return m_console->root();
	}
	Console* getConsole() {
		return m_console;
	}
	private:
	Console* m_console;
};

}

