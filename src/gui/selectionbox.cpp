#include <base/gui/selectionbox.h>
#include <base/gui/renderer.h>
#include <base/camera.h>

using namespace base;
using namespace gui;

SelectionBox::SelectionBox() : m_state(0) {
	setVisible(false);
}
void SelectionBox::update(int gameStateFlags) {
	if(!getRoot()) return;
	switch(m_state) {
	case 0:	// Cant start - over ui
		if(m_root->getMouseState()==0 && !m_root->getWidgetUnderMouse()) m_state = 1;
		break;
	case 1:	// Can start
		if(m_root->getWidgetUnderMouse()) m_state = 0;
		else if(m_root->getMouseState()==1) {
			m_start = m_end = m_root->getMousePos();
			m_state = 2;
		}
		break;
	case 2: // held
		if(m_root->getMouseState() != 1) m_state = 0;
		else if(abs(m_root->getMousePos().x-m_start.x) + abs(m_root->getMousePos().y-m_start.y) > 4) {
			m_rect.set(m_start,0,0);
			setVisible(true);
			setFocus();
			m_state = 3;
		}
		break;
	case 3: // Active
		if(m_end != m_root->getMousePos()) {
			m_end = m_root->getMousePos();
			m_rect.set(m_start, 0, 0);
			m_rect.include(m_end);
		}
		if(m_root->getMouseState() != 1) {
			if(eventApply) eventApply(this);
			cancel();
		}
		break;
	}
}
void SelectionBox::cancel() {
	setVisible(false);
	m_state = 0;
}
bool SelectionBox::isActive() const {
	return m_state == 3;
}

void SelectionBox::draw() const {
	if(isVisible()) {
		gui::Renderer& r = *getRoot()->getRenderer();
		r.drawRect(Rect(m_rect.x, m_rect.y, m_rect.width, 1), m_colour);
		r.drawRect(Rect(m_rect.x, m_rect.y, 1, m_rect.height), m_colour);
		r.drawRect(Rect(m_rect.x, m_rect.bottom()-1, m_rect.width, 1), m_colour);
		r.drawRect(Rect(m_rect.right()-1, m_rect.y, 1, m_rect.height), m_colour);
	}
}

base::PlaneVolume SelectionBox::getVolume(Camera* cam) const {
	base::PlaneVolume vol;
	const Point& view = m_root->getRootWidget()->getSize();
	const vec3& p0 = cam->getPosition();
	vec3 p1 = cam->unproject(vec3(m_rect.x, view.y-m_rect.y, 1), view);
	vec3 p2 = cam->unproject(vec3(m_rect.right(), view.y-m_rect.y, 1), view);
	vec3 p3 = cam->unproject(vec3(m_rect.right(), view.y-m_rect.bottom(), 1), view);
	vec3 p4 = cam->unproject(vec3(m_rect.x, view.y-m_rect.bottom(), 1), view);
	vol.addPlane(p0, p2, p1);
	vol.addPlane(p0, p3, p2);
	vol.addPlane(p0, p4, p3);
	vol.addPlane(p0, p1, p4);
	return vol;
}

