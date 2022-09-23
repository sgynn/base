#include <base/gui/gui.h>
#include <base/gui/layouts.h>

using namespace gui;

#define max(x,y) (x>y? x: y)

void Layout::assignSlot(const Rect& slot, Widget* w) const {
	// ToDo: Use anchor to determine how the slot is used
	Point p = slot.position();
	Point s = slot.size();
	int a = w->getAnchor();

	// x axis
	switch(a&0xf) {
	case 1: s.x = w->getSize().x; break; // Align left;
	case 2: s.x = w->getSize().x; p.x = slot.width/2 - s.x/2; break; // align centre
	case 4: s.x = w->getSize().x; p.x = slot.right() - s.x; break; // Align right
	}
	// y axis
	switch((a>>4)&0xf) {
	case 1: s.y = w->getSize().y; break; // Align top;
	case 2: s.y = w->getSize().y; p.y = slot.height/2 - s.y/2; break; // align middle
	case 4: s.y = w->getSize().y; p.y = slot.bottom() - s.y; break; // Align bottom
	}

	w->setPosition(p);
	w->setSize(s);
	// Warning - dynamically resized slots may make widgets bigger, then cant get smaller again.
	// Also how to the case of horizontal/vertical layouts if we want widgets to resize with parent?
	// Essentially they would need a spring value like the spring widget above, but that is not part of base widget.
}


void HorizontalLayout::apply(Widget* p) const {
	Rect slot(m_margin,m_margin,0,p->getSize().y-2*m_margin);
	int total = m_space * (getWidgets(p).size() - 1) + 2 * m_margin;
	float totalSpring = 0;
	for(Widget* w: getWidgets(p)) {
		if(!w->isVisible()) continue;
		if((w->getAnchor()&0xf)==0x5) totalSpring += w->isRelative()? getRelativeValues(w)[2]: 1.f; // lr
		else total += w->getSize().x;
	}
	for(Widget* w: getWidgets(p)) {
		if(!w->isVisible()) continue;
		if((w->getAnchor()&0xf)==0x5) {
			if(totalSpring==0) totalSpring = 1;
			float amount = (w->isRelative()? getRelativeValues(w)[2]: 1.f) / totalSpring;
			float s = (p->getSize().x - total) * amount;
			w->setSize(s<0?0:s, w->getSize().y);
			// FixMe: rounding errors
		}
		slot.width = w->getSize().x;
		assignSlot(slot, w);
		slot.x += slot.width + m_space;
	}
}
Point HorizontalLayout::getMinimumSize(const Widget* p) const {
	Point size(0,0);
	for(Widget* w: getWidgets(p)) {
		size.x += m_space;
		if(w->getAnchor() == 0x55 || !w->isVisible()) continue;
		Point s = w->getPreferredSize();
		if((w->getAnchor()&0xf) != 5) size.x += s.x;
		if(w->getAnchor()>>4 != 5) size.y = max(size.y, s.y);
	}
	size.x -= m_space;
	size += m_margin;
	return size;
}

// ======================================================================= //

void VerticalLayout::apply(Widget* p) const {
	Rect slot(m_margin, m_margin, p->getSize().x - 2*m_margin, 0);
	int total = m_space * (getWidgets(p).size() - 1) + 2 * m_margin;
	float totalSpring = 0;
	for(Widget* w: getWidgets(p)) {
		if(!w->isVisible()) continue;
		if((w->getAnchor()&0xf0)==0x50) totalSpring += w->isRelative()? getRelativeValues(w)[3]: 1.f; // tb
		else total += w->getSize().y;
	}
	for(Widget* w: getWidgets(p)) {
		if(!w->isVisible()) continue;
		if((w->getAnchor()&0xf0)==0x50) {
			if(totalSpring==0) totalSpring = 1;
			float amount = (w->isRelative()? getRelativeValues(w)[3]: 1.f) / totalSpring;
			float s = (p->getSize().y - total) * amount;
			w->setSize(w->getSize().x, s<0?0:s);
		}
		slot.height = w->getSize().y;
		assignSlot(slot, w);
		slot.y += slot.height + m_space;
	}
}
Point VerticalLayout::getMinimumSize(const Widget* p) const {
	Point size(0,0);
	for(Widget* w: getWidgets(p)) {
		size.y += m_space;
		if(w->getAnchor() == 0x55 || !w->isVisible()) continue;
		Point s = w->getPreferredSize();
		if((w->getAnchor()&0xf) != 5) size.x = max(size.x, s.x);
		if(w->getAnchor()>>4 != 5) size.y += s.y;
	}
	size.y -= m_space;
	size += m_margin;
	return size;
}

// ======================================================================= //

void FlowLayout::apply(Widget* p) const {
	Rect slot(m_margin, m_margin, 0, 0);
	int height = 0;
	for(Widget* w: getWidgets(p)) {
		if(!w->isVisible()) continue;
		slot.size() = w->getSize();
		if(slot.right() > p->getSize().x-m_margin && slot.x>m_margin) slot.x=m_margin, slot.y+=height+m_space, height=0;
		height = max(height, slot.height);
		assignSlot(slot, w); // Slot height should ideally be the row height
		slot.x += slot.width + m_space;
	}
}

Point FlowLayout::getMinimumSize(const Widget* p) const {
	Point size(0,0);
	if(getWidgets(p).size() > 0) {
		int baseWidth = p->getSize().x;
		int bottom=m_margin, right = m_margin - m_space;
		for(Widget* w: getWidgets(p)) {
			if(!w->isVisible()) continue;
			Point s = w->getPreferredSize();
			if(right + m_space + s.x < baseWidth) {
				right += s.x + m_space;
			}
			else {
				bottom = size.y;
				right = size.x + m_margin;
			}
			size.x = max(size.x, right);
			size.y = max(size.y, bottom + s.y);
		}
		size += m_margin;
	}
	else size += 2 * m_margin;
	return size;
}

// ======================================================================= //

void FixedGridLayout::apply(Widget* p) const {
	Rect slot(m_margin,m_margin,0,0);
	for(Widget* w: getWidgets(p)) {
		slot.width = max(slot.width, w->getSize().x);
		slot.height = max(slot.height, w->getSize().y);
	}
	for(Widget* w: getWidgets(p)) {
		assignSlot(slot, w);
		slot.x += slot.width + m_space;
		if(slot.right() > p->getSize().x-m_margin) slot.x=m_margin, slot.y += slot.height+m_space;
	}
}
Point FixedGridLayout::getMinimumSize(const Widget* p) const {
	if(getWidgets(p).size() == 0) return Point(m_margin*2, m_margin*2);
	Point slot;
	for(Widget* w: getWidgets(p)) {
		Point s = w->getPreferredSize();
		slot.x = max(slot.x, s.x);
		slot.y = max(slot.y, s.y);
	}
	int columns = max(1, p->getSize().x - 2*m_margin + m_space / (slot.x + m_space));
	int rows = getWidgets(p).size() / columns;
	return Point(columns * slot.x + (columns-1) * m_space + 2 * m_margin,
				 rows * slot.y + (rows-1) * m_space + 2 * m_margin);
}

// ======================================================================= //


// Dynamic grid - row and column sizes differ
void DynamicGridLayout::apply(Widget* p) const {
	// Calculate number of rows
	int count = p->getWidgetCount();
	uint columns = count;
	int total = 0;
	int width = p->getSize().x - 2*m_margin;
	for(uint i=0; i<columns; ++i) {
		int s = 0;
		for(int j=i; j<count; j+=columns) s = max(s, p->getWidget(j)->getSize().x);
		total += s;
		if(total + (int)(i-1)*m_space > width) columns = i, i=-1, total = 0; // restart loop
	}
	if(columns==0) columns = 1;
	uint rows = count / columns;

	// Calculate cell sizes
	struct Info { int offset, size; };
	std::vector<Info> colData, rowData;
	colData.resize(columns, Info{0,0});
	rowData.resize(rows, Info{0,0});
	for(int i=0; i<count; ++i) {
		int col = i%columns, row = i/columns;
		colData[col].size = max(colData[col].size, p->getWidget(i)->getSize().x);
		rowData[row].size = max(rowData[row].size, p->getWidget(i)->getSize().y);
	}
	// And cell offsets
	for(size_t i=0, v=m_margin; i<columns; ++i) colData[i].offset=v, v += colData[i].size + m_space;
	for(size_t i=0, v=m_margin; i<rows; ++i) rowData[i].offset=v, v += rowData[i].size + m_space;

	// Position widgets
	Rect slot;
	for(int i=0; i<count; ++i) {
		int col = i%columns, row = i/columns;
		slot.x = colData[col].offset;
		slot.y = rowData[row].offset;
		slot.width = colData[col].size;
		slot.height = rowData[row].size;
		assignSlot(slot, p->getWidget(i));
	}
}

Point DynamicGridLayout::getMinimumSize(const Widget* p) const {
	Point size;
	// complicated
	return size;
}
