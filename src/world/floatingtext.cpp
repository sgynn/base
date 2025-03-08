#include <base/world/floatingtext.h>
#include <base/game.h>
#include <base/camera.h>
#include <base/guicomponent.h>
#include <base/scenecomponent.h>
#include <base/gui/widgets.h>
#include <base/compositor.h>
#include <base/thread.h>

using namespace base;

FloatingText* FloatingText::s_instance = nullptr;

FloatingText::FloatingText(float duration, float speed) : GameStateComponent(0,0,PERSISTENT), m_duration(duration), m_speed(speed) {
	s_instance = this;
	m_parent = new gui::Widget();
	m_parent->setAnchor("tlrb");
	m_parent->setTangible(gui::Tangible::NONE);
}
FloatingText::~FloatingText() {
	if(s_instance == this) s_instance = nullptr;
}

void FloatingText::begin() {
	m_clearFlag = true;
	m_camera = nullptr;
	m_gui = nullptr;
}

void FloatingText::update() {
	if(!m_camera&& !setup()) return;
	{
		MutexLock lock(m_mutex);
		for(Item& i: m_new) {
			m_parent->add(i.widget);
			m_list.push_back(i);
		}
	}
	for(Item& i : m_list) {
		vec3 pos = i.pos + i.track;
		i.time += Game::frameTime();
		i.widget->setVisible(m_camera->getDirection().dot(pos - m_camera->getPosition()) < 0);
		if(m_clearFlag) i.time = m_duration;
		if(i.time < m_duration) {
			const Point& view = m_gui->getRootWidget()->getSize();
			pos.y += i.time * m_speed;
			vec3 screen = m_camera->project(pos, view);
			i.widget->setPosition(screen.x, view.y-screen.y);
			i.widget->setAlpha(m_duration - i.time);
		}
		else {
			i.widget->removeFromParent();
			delete i.widget;
			i.widget = 0;
		}
	}
	while(!m_list.empty() && !m_list.front().widget) m_list.pop_front();
	m_new.clear();
	m_clearFlag = false;
}

void FloatingText::add(const char* text, uint colour, const vec3& pos) {
	static vec3 zero;
	add(text, colour, zero, pos);
}

void FloatingText::add(const char* text, uint colour, const vec3& track, const vec3& offset) {
	if(!s_instance || !s_instance->m_gui) return;
	gui::Label* label = s_instance->m_gui->createWidget<gui::Label>("label");
	if(!label) return; // Bad setup
	label->setCaption(text);
	label->setColour(colour);
	label->setFontAlign(15);
	label->setSize(0, 0); // Labels are not clipped to their bounds
	MutexLock lock(s_instance->m_mutex);
	s_instance->m_new.push_back({label, offset, track, 0});
}

void FloatingText::clear() {
	if(s_instance) s_instance->m_clearFlag = true;
}

bool FloatingText::setup() {
	if(GUIComponent* gui = getState()->getComponent<GUIComponent>()) m_gui = gui->getGUI();
	if(SceneComponent* scene = getState()->getComponent<SceneComponent>()) m_camera = scene->getWorkspace()->getCamera();
	if(m_gui && m_parent->getRoot() != m_gui) {
		m_parent->removeFromParent();
		m_gui->getRootWidget()->add(m_parent, 0);
		m_parent->setSize(m_gui->getSize());
	}
	return m_gui && m_camera;
}

