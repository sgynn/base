#pragma once
#include <base/game.h>
#include <base/input.h>
#include <base/gamestate.h>
#include <base/gui/gui.h>
#include <base/gui/widgets.h>

namespace base {

class GUIComponent : public GameStateComponent {
	public:
	GUIComponent(gui::Root* gui, float scale=1) : GameStateComponent(-20, 60, PERSISTENT), m_gui(gui), m_mult(1) {
		setScale(scale);
	}
	explicit GUIComponent(const char* file, float scale=1) : GUIComponent(new gui::Root(Game::getSize()), scale) {
		m_gui->load(file);
	}
	~GUIComponent() {
		delete m_gui;
	}
	void update() override {
		const Mouse& mouse = Game::input()->mouse;
		m_gui->setKeyMask((gui::KeyMask)Game::input()->getKeyModifier());
		int wheel = hasComponentFlags(BLOCK_WHEEL)? 0: mouse.wheel;
		if(hasComponentFlags(BLOCK_MOUSE)) {
			m_gui->mouseEvent(Point(-1,-1), 0, wheel);
		}
		else {
			m_gui->mouseEvent(Point(mouse.x*m_mult, Game::height()-mouse.y*m_mult), mouse.button, wheel);
		}
		bool textboxWasFocused = m_gui->getFocusedWidget()->cast<gui::Textbox>();
		if(Game::LastKey() && !hasComponentFlags(BLOCK_KEYS)) {
			m_gui->keyEvent(Game::LastKey(), Game::LastChar());
		}
		m_gui->update();
		if(m_gui->getWidgetUnderMouse()) setComponentFlags(BLOCK_MOUSE);
		if(m_gui->getWheelEventConsumed()) setComponentFlags(BLOCK_WHEEL);
		if(textboxWasFocused) setComponentFlags(BLOCK_KEYS);
	}
	void draw() override {
		m_gui->draw(Game::getSize());
	}
	void resized(const Point& s) override {
		m_gui->resize(s.x*m_mult, s.y*m_mult);
	}
	void setScale(float scale) {
		m_mult = 1/scale;
		resized(Game::getSize());
	}
	gui::Root* getGUI() { return m_gui; }
	private:
	gui::Root* m_gui;
	float m_mult;
};

}

