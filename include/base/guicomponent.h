#pragma once
#include <base/game.h>
#include <base/input.h>
#include <base/gamestate.h>
#include <base/gui/gui.h>
#include <base/gui/widgets.h>

namespace base {

class GUIComponent : public GameStateComponent {
	public:
	GUIComponent(gui::Root* gui, int height=0) : GameStateComponent(-20, 60, PERSISTENT), m_gui(gui), m_fixedHeight(height) {
		setHeight(height);
	}
	explicit GUIComponent(const char* file, int height=0) : GUIComponent(new gui::Root(Game::getSize()), height) {
		m_gui->load(file);
	}
	~GUIComponent() {
		delete m_gui;
	}
	void update() override {
		const Mouse& mouse = Game::input()->mouse;
		m_gui->setKeyMask((gui::KeyMask)(Game::input()->getKeyModifier()>>9));
		int wheel = hasComponentFlags(BLOCK_WHEEL)? 0: mouse.wheel;
		if(hasComponentFlags(BLOCK_MOUSE)) {
			m_gui->mouseEvent(Point(-1,-1), 0, wheel);
		}
		else {
			m_gui->mouseEvent(Point(mouse.x*m_mult, (Game::height()-mouse.y)*m_mult), mouse.button, wheel);
		}
		bool textboxWasFocused = cast<gui::Textbox>(m_gui->getFocusedWidget());
		if(Game::LastKey() && !hasComponentFlags(BLOCK_KEYS)) {
			m_gui->keyEvent(Game::LastKey(), Game::LastChar());
		}
		m_gui->updateAnimators(base::Game::frameTime());
		if(m_gui->getWidgetUnderMouse()) setComponentFlags(BLOCK_MOUSE);
		if(m_gui->getWheelEventConsumed()) setComponentFlags(BLOCK_WHEEL);
		if(textboxWasFocused) setComponentFlags(BLOCK_KEYS);
	}
	void draw() override {
		m_gui->draw(Game::getSize());
	}
	void resized(const Point& s) override {
		if(m_fixedHeight > 0) m_mult = m_fixedHeight / (float)s.y;
		m_gui->resize(s.x*m_mult, s.y*m_mult);
	}
	// Scale to a fixed height - width is calculated to maintain aspect ratio. Set to 0 to disable scaling
	void setHeight(int height) {
		m_fixedHeight = height;
		resized(Game::getSize());
	}
	// Set the gui scale factor
	void setScale(float scale) {
		m_fixedHeight = 0;
		m_mult = 1/scale;
		resized(Game::getSize());
	}
	float getScale() const { return 1.f / m_mult; }
	gui::Root* getGUI() { return m_gui; }
	private:
	gui::Root* m_gui;
	float m_mult = 1;
	float m_fixedHeight = 0;
};

}

