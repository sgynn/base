#pragma once

#include <base/gui/gui.h>

namespace editor {
class GradientBox : public gui::Widget {
	WIDGET_TYPE(GradientBox)
	GradientBox(const Rect& r, gui::Skin* s) : Widget(r,s) {}
	void draw() const override {
		gui::Renderer& renderer = *getRoot()->getRenderer();
		if(gradient.data.size() <= 1) renderer.drawRect(m_rect, gradient.getValue(0));
		else {
			renderer.push(m_rect);
			Rect r = m_rect;
			for(size_t i=1; i<gradient.data.size(); ++i) {
				r.x = m_rect.x + getKeyPos(gradient.data[i-1].key);
				r.width = m_rect.x + getKeyPos(gradient.data[i].key) - r.x;
				renderer.drawGradient(0, r, gradient.data[i-1].value, gradient.data[i].value, 0);
			}
			int a = getKeyPos(gradient.data[0].key);
			int b = getKeyPos(gradient.data.back().key);
			if(a>0) renderer.drawRect(Rect(m_rect.x, m_rect.y, a, m_rect.height), gradient.data[0].value);
			if(b<m_rect.width) renderer.drawRect(Rect(m_rect.x+b, m_rect.y, m_rect.width-b, m_rect.height), gradient.data.back().value);
			renderer.pop();
		}
		drawChildren();
	}
	// Get the key value of a pixel
	float getKey(int local) const {
		return min + (max-min) * ((float)local / m_rect.width);
	}
	// Get pixel position of a key
	int getKeyPos(float key) const {
		if(gradient.data.empty()) return 0;
		if(min == max) return m_rect.x;
		int pos = m_rect.width * ((key - min) / (max - min));
		if(pos == m_rect.width) --pos; // make sure the one on the end is visible
		return pos;
	}
	// Set the key of a node, returns new index if node index changes
	int setKey(int index, float key) {
		gradient.data[index].key = key;
		int end = gradient.data.size()-1;
		while(index>0 && key < gradient.data[index-1].key) { std::swap(gradient.data[index], gradient.data[index-1]); --index; }
		while(index<end && key > gradient.data[index+1].key) { std::swap(gradient.data[index], gradient.data[index+1]); ++index; }
		refreshLayout();
		return index;
	}
	void refreshLayout() override {
		if(!getWidgetCount()) return;
		Widget* sel = getWidget(0);
		if(selected>=0) {
			int s = m_rect.height * 0.3;
			sel->setRect(getKeyPos(gradient.data[selected].key)-s/2, m_rect.height-s/2, s, s);
			sel->setColour(getWidget(selected+1)->getColourARGB());
		}
		sel->setVisible(selected>=0);
		for(int i=1; i<getWidgetCount(); ++i) {
			getWidget(i)->setRect(getKeyPos(gradient.data[i-1].key), 0, 1, m_rect.height);
		}
	}
	void setColour(int index, uint colour) {
		gradient.data[index].value = colour;
		// Update marker colour
		int k = (colour&0xff) + ((colour>>8)&0xff)*2 + ((colour>>16)&0xff);
		uint c = k < 512? 0xffffffff: 0xff000000;
		getWidget(index+1)->setColour(c);
		if(selected == index) getWidget(0)->setColour(c);
	}
	// Reset range to gradient bounds
	void resetRange() {
		float oldMin=min, oldMax=max;
		if(gradient.data.size() > 1) {
			min = gradient.data[0].key;
			max = gradient.data.back().key;
		}
		else {
			float key = gradient.data.empty()? 0: gradient.data[0].key;
			min = fmin(0, key);
			max = fmax(1, key);
		}
		if(min!=oldMin || max!=oldMax) {
			refreshLayout();
			if(eventRangeChanged) eventRangeChanged(min, max);
		}
	}
	// Create edit mrkers
	void createMarkers() {
		if(getWidgetCount() != (int)gradient.data.size() + 1) {
			selected = 0;
			pauseLayout();
			deleteChildWidgets();
			gui::Image* sel = new gui::Image(Rect(0,0,10,10), 0);
			sel->setTangible(gui::Tangible::NONE);
			sel->setAngle(PI/4);
			sel->setImage(0);
			add(sel);

			for(auto& i: gradient.data) {
				gui::Image* m = new gui::Image(Rect(0,0,1,1),0);
				int k = (i.value&0xff) + ((i.value>>8)&0xff)*2 + ((i.value>>16)&0xff);
				m->setColour(k < 512? 0xffffffff: 0xff000000);
				m->setTangible(gui::Tangible::NONE);
				m->setImage(0);
				add(m);
			}
			resumeLayout();
		}
	}

	void onMouseButton(const Point& p, int d, int u) override {
		Super::onMouseButton(p, d, u);
		if(!editable) return;
		if(held>=0 && u==1) resetRange();

		held = -1;
		int index = -1;
		for(Widget* w: *this) if(abs(p.x - w->getPosition().x) < 6) index = w->getIndex() - 1;
		if(index<0 && d==1) {	// Add
			float key = getKey(p.x - m_rect.x);
			selected = held = gradient.add(key, gradient.getValue(key));
			createMarkers();
			if(eventSelected) eventSelected(this, selected);
			if(eventChanged) eventChanged(this, selected);
		}
		else if(index>=0 && d&4) { // Delete
			gradient.data.erase(gradient.data.begin() + index);
			if(selected>=index) --selected;
			createMarkers();
			if(selected==index-1 && eventSelected) eventSelected(this, selected);
			if(eventChanged) eventChanged(this, selected);
			held = -1;
		}
		else if(index>=0 && d==1) { // Select
			held = index;
			selected = index;
			refreshLayout();
			if(eventSelected) eventSelected(this, selected);
		}
		else held = -1;
	}
	void onMouseMove(const Point&, const Point& p, int b) override {
		if(editable && held>=0 && b==1) {
			float key = getKey(p.x);
			selected = held = setKey(held, key);
			if(eventChanged) eventChanged(this, held);
		}
	}

	particle::Gradient gradient;
	bool editable = false;
	int selected = 0;
	int held = 0;
	float min = 0;
	float max = 1;

	Delegate<void(float, float)> eventRangeChanged;
	Delegate<void(GradientBox*, int)> eventSelected;
	Delegate<void(GradientBox*, int)> eventChanged;
};
}

