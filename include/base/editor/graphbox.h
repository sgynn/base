#pragma once

#include <base/gui/gui.h>
#include <base/bounds.h>

namespace editor {
class GraphBox : public gui::Widget {
	WIDGET_TYPE(GraphBox)
	GraphBox(const Rect& r, gui::Skin* s) : Widget(r,s) {}
	void draw() const override {
		// Axis
		gui::Renderer& renderer = *getRoot()->getRenderer();
		Point origin = getNodePos(vec2(0.f)) + m_rect.position();
		if(m_rect.contains(origin)) {
			renderer.drawImage(0, Rect(m_rect.x, origin.y, m_rect.width, 1), 0, 0xffffff, 0.4);
			renderer.drawImage(0, Rect(origin.x, m_rect.y, 1, m_rect.height), 0, 0xffffff, 0.4);
		}
		// Lines
		if(graph.data.size() == 1) {
			Point p = getNodePos(0, graph.data[0].value) + m_rect.position();
			renderer.drawImage(0, Rect(m_rect.x, p.y, m_rect.width, 1), 0, 0xffffff);
		}
		else for(size_t i=1; i<graph.data.size(); ++i) {
			Point line[2];
			line[0] = getNodePos(graph.data[i-1].key, graph.data[i-1].value);
			line[1] = getNodePos(graph.data[i].key, graph.data[i].value);
			renderer.drawLineStrip(2, line, 1, m_rect.position());
		}
		// Points
		for(size_t i=0; i<graph.data.size(); ++i) {
			Point p = getNodePos(graph.data[i].key, graph.data[i].value) + m_rect.position();
			renderer.drawImage(0, Rect(p.x-3, p.y-3,6,6), PI/4, (int)i==selected? 0xffff00: 0xffffff);
		}
	}
	// Get graph coordinates in 
	Point getNodePos(float key, float value) const { return getNodePos(vec2(key, value)); }
	Point getNodePos(const vec2& p) const {
		vec2 n = (p - bounds.min) / bounds.size();
		return Point(n.x*m_rect.width, n.y * m_rect.height);
	}
	// Get local point in graph coordinates
	vec2 getNode(const Point& local) const {
		vec2 r((float)local.x/m_rect.width, (float)local.y/m_rect.height);
		return bounds.min + r * bounds.size();
	}
	// Set the key of a node, returns new index if node index changes
	int setKey(int index, const vec2& p) {
		graph.data[index].key = p.x;
		graph.data[index].value = p.y;
		int end = graph.data.size() - 1;
		while(index>0 && p.x < graph.data[index-1].key) { std::swap(graph.data[index], graph.data[index-1]); --index; }
		while(index<end && p.x > graph.data[index+1].key) { std::swap(graph.data[index], graph.data[index+1]); ++index; }
		return index;
	}
	// Reset range to graph bounds
	void resetRange() {
		BoundingBox2D old = bounds;
		if(graph.data.empty()) {
			bounds.set(0,1,1,0);
		}
		else {
			bounds.set(0,0,1,1);
			for(auto& i: graph.data) bounds.include(vec2(i.key, i.value));
			std::swap(bounds.min.y, bounds.max.y);
		}
		vec2 e = getNode(Point(8,8)) - getNode(Point(0,0));
		bounds.min -= e;
		bounds.max += e;
		if(old != bounds && eventRangeChanged) eventRangeChanged(bounds);
	}
	// Get closest node in pixels
	int getClosestNode(const Point& pos, int limit) {
		int r = -1;
		for(size_t i=0; i<graph.data.size(); ++i) {
			Point p = getNodePos(graph.data[i].key, graph.data[i].value) - pos;
			int manhatten = abs(p.x) + abs(p.y);
			if(manhatten < limit) {
				r = (int)i;
				limit = manhatten;
			}
		}
		return r;
	}

	void onMouseButton(const Point& p, int d, int u) override {
		Super::onMouseButton(p, d, u);
		if(!editable) return;
		if(dragging && u==1) resetRange();
		dragging = false;
		int index = getClosestNode(p, 10);
		if(index<0 && d==1) { // Add node
			vec2 n = getNode(p);
			selected = graph.add(n.x, n.y);
			if(eventSelected) eventSelected(this, selected);
			if(eventChanged) eventChanged(this, selected);
			dragging = true;
		}
		else if(index>=0 && d==4) { // Delete node
			graph.data.erase(graph.data.begin()+index);
			if(selected>=index) --selected;
			if(selected==index-1 && eventSelected) eventSelected(this, selected);
			if(eventChanged) eventChanged(this, selected);
		}
		else if(index>=0 && d==1) { // Select
			selected = index;
			dragging = true;
			if(eventSelected) eventSelected(this, selected);
		}
	}

	void onMouseMove(const Point&, const Point& p, int b) override {
		if(dragging) {
			vec2 node = getNode(p);
			selected = setKey(selected, node);
			if(eventChanged) eventChanged(this, selected);
		}
	}

	particle::Graph graph;
	bool editable = false;
	BoundingBox2D bounds;
	int selected = 0;
	bool dragging = false;

	Delegate<void(const BoundingBox2D&)> eventRangeChanged;
	Delegate<void(GraphBox*, int)> eventSelected;
	Delegate<void(GraphBox*, int)> eventChanged;
};
}

