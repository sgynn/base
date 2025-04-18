#include <base/gui/widgets.h>
#include <base/gui/renderer.h>
#include <base/gui/skin.h>
#include <base/gui/font.h>
#include <base/input.h>
#include <cstdio>
#include <ctime>

using namespace gui;

Textbox::Textbox()
	: m_buffer(0), m_length(0), m_cursor(0), m_selectLength(0)
	, m_multiline(false), m_readOnly(false), m_password(0)
	, m_held(0), m_offset(0), m_selection(0)
{
	m_selectRect.set(0,0,1,16);
	m_buffer = 16;
	m_text = new char[m_buffer];
	m_selectColour = 0x80ffff00;
	m_text[0] = 0;
}
Textbox::~Textbox() {
	if(m_text) delete [] m_text;
	if(m_selection) delete [] m_selection;
}
void Textbox::initialise(const Root*, const PropertyMap& p) {
	if(p.contains("text")) setText(p["text"]);
	if(p.contains("multiline")) m_multiline = atoi(p["multiline"]);
	if(p.contains("readonly")) m_readOnly = atoi(p["readonly"]);
	if(p.contains("password")) m_password = p["password"][0];
	if(p.contains("suffix")) setSuffix(p["suffix"]);
	if(p.contains("hint")) setHint(p["hint"]);
	if(m_skin) m_offset = m_skin->getState(0).textPos;
	m_selectRect.position() = getPosition() + m_offset;
}

void Textbox::copyData(const Widget* from) {
	if(const Textbox* t = cast<Textbox>(from)) {
		m_hint = t->m_hint;
		m_suffix = t->m_suffix;
		m_selectColour = t->m_selectColour;
		m_multiline = t->m_multiline;
		m_readOnly = t->m_readOnly;
		m_password = t->m_password;
		m_submitAction = t->m_submitAction;
	}
}

Point Textbox::getPreferredSize(const Point& hint) const {
	if(!isAutosize()) return getSize();
	Point s = m_skin->getFont()->getSize(m_text, m_skin->getFontSize());
	s += m_skin->getState(0).textPos;
	++s.x; // Space for cursor at the end of the longest line
	return s;
}

void Textbox::setPosition(int x, int y) {
	Point rel = m_selectRect.position() - m_rect.position();
	Widget::setPosition(x, y);
	m_selectRect.position() = m_rect.position() + rel;
}

void Textbox::setText(const char* t) {
	if(!t) t = "";
	m_length = strlen(t);
	if(m_length >= m_buffer) {
		delete [] m_text;
		m_buffer = m_length+16;
		m_text = new char[m_buffer];
		m_cursor = m_length;
	}
	memcpy(m_text, t, m_length+1);
	int tmp = m_cursor;
	m_cursor = -1;
	select(tmp);
	updateLineData();
	updateAutosize();
	m_offset = m_skin->getState(0).textPos;
}

void Textbox::setMultiLine(bool m) {
	m_multiline = m;
}

void Textbox::setReadOnly(bool r) {
	m_readOnly = r;
}

void Textbox::setPassword(char c) {
	m_password = c;
}

void Textbox::setSuffix(const char* s) {
	m_suffix = s;
}

void Textbox::setHint(const char* s) {
	m_hint = s;
}

void Textbox::setSubmitAction(SubmitOption s) {
	m_submitAction = s;
}

int Textbox::getLineCount() const {
	return m_lines.size() + 1;
}

const char* Textbox::getSelectedText() const {
	if(m_selection) return m_selection;
	if(m_selectLength==0) return "";
	int start = m_selectLength<0? m_cursor+m_selectLength: m_cursor;
	int length = abs(m_selectLength);
	m_selection = new char[length+1];
	memcpy(m_selection, m_text+start, length);
	m_selection[length] = 0;
	return m_selection;
}

void Textbox::insertText(const char* t) {
	if(m_readOnly) return;
	int s = strlen(t);
	int start, end;
	if(m_selectLength<0) start = m_cursor + m_selectLength, end = m_cursor;
	else start = m_cursor, end = m_cursor+m_selectLength;
	int lastBit = m_length - end;
	if(m_buffer < start + s + lastBit + 1) {
		m_buffer = start + s + lastBit + 16;
		printf("Resize buffer %d\n", m_buffer);
		char* n = new char[m_buffer];
		if(m_cursor>0) memcpy(n, m_text, start);
		if(s>0)        memcpy(n+m_cursor, t, s);
		if(lastBit>0)  memcpy(n+m_cursor+s, m_text+end, lastBit); 
		delete [] m_text;
		m_text = n;
		m_length = start + s + lastBit;
	}
	else {
		int d = s - abs(m_selectLength);
		if(d<0) for(int i=end; i<m_length; ++i) m_text[i+d] = m_text[i];
		if(d>0) for(int i=m_length-1; i>=end; --i) m_text[i+d] = m_text[i];
		if(s>0) memcpy(m_text+start, t, s);
		m_length += d;
	}
	m_text[m_length] = 0;
	updateLineData();
	updateAutosize();
	select(start+s);
}
void Textbox::select(int s, int len, bool shift) {
	if(s<0) s=0;
	else if(s>m_length) s = m_length;
	if(s+len>m_length) len = m_length-s;
	else if(s+len<0) len = -s;
	if(s==m_cursor && m_selectLength==len) return; // no change
	
	// Shift selects to lastcursor position
	if(len==0 && shift) {
		len = m_cursor + m_selectLength - s;
	}

	// Clear selection buffer
	if(m_selection) {
		delete [] m_selection;
		m_selection = 0;
	}

	auto getTextSize = [this](const char* s, int len) { return m_skin->getFont()->getSize(s, m_skin->getFontSize(), len).x; };
	int start = len<0? s+len: s;
	if(m_multiline) {
		int lineHeight = m_skin->getFont()->getLineHeight(m_skin->getFontSize());
		int startLine=0, endLine=0;
		int end = start + abs(len);
		for(size_t i=0; i<m_lines.size(); ++i) {
			if(start >= m_lines[i]) startLine = i + 1;
			if(end >= m_lines[i]) endLine = i + 1;
		}
		m_selectRect.y = m_rect.y + m_offset.y + startLine * lineHeight;
		m_selectRect.height = (endLine-startLine+1) * lineHeight;
		int startStart = startLine? m_lines[startLine-1]: 0;
		m_selectRect.x = m_rect.x + m_offset.x + getTextSize(m_text+startStart, start-startStart);
		if(len==0) 	m_selectRect.width = 1;
		else if(startLine == endLine) m_selectRect.width = getTextSize(m_text+start, abs(len));
		else {
			int endStart = m_lines[endLine-1];
			m_selectRect.width = m_rect.x + getTextSize(m_text+endStart, end - endStart) - m_selectRect.x;
		}
		m_cursor = s;
	}
	else {
		if(s != m_cursor) {
			Point textSize = m_skin->getFont()->getSize(m_text, m_skin->getFontSize(), start);
			m_selectRect.x = m_rect.x + textSize.x + m_offset.x;
			m_selectRect.y = m_rect.y + m_offset.y;
			m_cursor = s;
		}
		if(len != 0) m_selectRect.width = getTextSize(m_text+start, abs(len));
		else m_selectRect.width = 1;
	}
	m_selectLength = len;
	updateOffset(false);
}

void Textbox::updateOffset(bool end) {
	Point textOffset = m_skin->getState(0).textPos;
	if(Scrollpane* pane = cast<Scrollpane>(getParent())) {
		int lineHeight = m_skin->getFont()->getLineHeight(m_skin->getFontSize());
		Point pos = end? m_selectRect.bottomRight(): m_selectRect.position();
		pos -= m_rect.position();
		if(end) pos.y -= lineHeight;
		pane->ensureVisible(pos);
		pos.y += lineHeight;
		pane->ensureVisible(pos);
		m_offset = textOffset;
	}
	else if(isAutosize()) m_offset = textOffset;
	else {
		int min = m_rect.x + textOffset.x;
		int max = m_rect.right() - 1;
		int x = end? m_selectRect.right(): m_selectRect.x;
		int shift = 0;
		if(x < min) shift = min - x;
		else if(x > max) shift = max - x;
		m_selectRect.x += shift;
		m_offset.x += shift;
	}
}

void Textbox::updateLineData() {
	if(!m_multiline) return;
	m_lines.clear();
	char* e = strchr(m_text, '\n');
	while(e) {
		m_lines.push_back(e - m_text + 1);
		e = strchr(e+1, '\n');
	}
}

int Textbox::getLineNumber(uint characterIndex) const {
	if(!m_multiline || m_lines.empty()) return 0;
	// Binary search
	uint start=0, end=m_lines.size();;
	while(start<end) {
		uint c = (start+end)/2;
		if(characterIndex >= (uint)m_lines[c]) start = c+1; else end=c;
	}
	return start;
}

void Textbox::onLoseFocus() {
	if(m_submitAction == SubmitOnLoseFocus && eventSubmit) eventSubmit(this);
}

void Textbox::onKey(int code, wchar_t chr, KeyMask mask) {
	if(code == base::KEY_ENTER && !m_multiline) {
		if(m_submitAction == ClearFocus) getRoot()->getRootWidget()->setFocus(); 
		if(eventSubmit) eventSubmit(this);
	}
	else if(code == base::KEY_LEFT)  select(m_cursor-1, 0, mask==KeyMask::Shift);
	else if(code == base::KEY_RIGHT) select(m_cursor+1, 0, mask==KeyMask::Shift);
	else if(code == base::KEY_UP && m_multiline) select(indexAt(Point(m_selectRect.x-m_rect.x, m_selectRect.y-m_rect.y-2)), 0, mask==KeyMask::Shift);
	else if(code == base::KEY_DOWN && m_multiline) select(indexAt(Point(m_selectRect.x-m_rect.x, m_selectRect.bottom()-m_rect.y+2)), 0, mask==KeyMask::Shift);
	else if(code == base::KEY_HOME) {
		uint line = getLineNumber(m_cursor);
		int home = line > 0? m_lines[line-1]: 0;
		select(home, 0, mask==KeyMask::Shift);
	}
	else if(code == base::KEY_END) {
		uint line = getLineNumber(m_cursor);
		int end = line < m_lines.size()? m_lines[line] - 1: m_length;
		select(end,   0, mask==KeyMask::Shift);
	}
	else if(code == base::KEY_BACKSPACE) {
		if(m_selectLength!=0) insertText("");
		else if(m_cursor>0 && !m_readOnly) {
			select(m_cursor-1);
			for(int i=m_cursor; i<m_length; ++i) m_text[i] = m_text[i+1];
			--m_length;
			updateLineData();
		}
		if(eventChanged) eventChanged(this, m_text);
	}
	else if(code == base::KEY_DEL) {
		if(m_selectLength!=0) insertText("");
		else if(m_cursor<m_length && !m_readOnly) {
			for(int i=m_cursor; i<m_length; ++i) m_text[i] = m_text[i+1];
			--m_length;
			updateLineData();
		}
		if(eventChanged) eventChanged(this, m_text);
	}
	else if((chr>=32 || chr==10) && !m_readOnly && mask!=KeyMask::Ctrl) {
		char tmp[2] = { (char)chr, 0 };
		insertText(tmp);
		if(eventChanged) eventChanged(this, m_text);
	}
}
int Textbox::indexAt(const Point& pos) const {
	int px = pos.x - m_offset.x;
	int start = 0;
	if(m_multiline && !m_lines.empty()) {
		int py = pos.y - m_offset.y;
		int line = py / m_skin->getFont()->getLineHeight(m_skin->getFontSize());
		if(line > (int)m_lines.size()) start = m_lines.back();
		else if(line > 0) start = m_lines[line-1];
	}
	for(int i=start; i<m_length; ++i) {
		Point characterSize = m_skin->getFont()->getSize(m_text[i], m_skin->getFontSize());
		if(px < characterSize.x/2) return i;
		if(m_text[i] == '\n') return i;
		px -= characterSize.x;
	}
	return m_length;
}
void Textbox::onMouseButton(const Point& p, int b, int u) {
	if(b==1) select( indexAt(p) ), m_held=m_cursor;
	Widget::onMouseButton(p, b, u);
}
void Textbox::onMouseMove(const Point&, const Point& p, int b) {
	if(b==1) {
		int i = indexAt(p);
		select(i, m_held-i);
		updateOffset(i>m_held);
	}
}


inline void Textbox::drawText(Point& p, const char* t, uint len, uint col) const {
	if(len>0 && t[0]) {
		p = m_root->getRenderer()->drawText(p, t, len, m_skin->getFont(), m_skin->getFontSize(), col);
	}
}
void Textbox::draw() const {
	if(!isVisible()) return;
	m_root->getRenderer()->drawSkin(m_skin, m_rect, m_colour, getState());
	// Selection - do we change text colour too?
	m_root->getRenderer()->push(m_rect);
	if(m_selectLength!=0) {
		uint col = m_skin->getState( getState() ).foreColour;
		uint scol = m_skin->getState(4).foreColour;
		if(m_multiline && !m_lines.empty()) {
			int lineHeight = m_skin->getFont()->getLineHeight(m_skin->getFontSize());
			int selectedLines = m_selectRect.height / lineHeight;
			// Selection background: 2 or 3 boxes
			if(selectedLines == 1) {
				m_root->getRenderer()->drawRect(m_selectRect, m_selectColour);
			}
			else {
				Rect r = m_selectRect;
				r.height = lineHeight;
				r.width = m_rect.right() - r.x;
				m_root->getRenderer()->drawRect(r, m_selectColour);
				r.x = m_rect.x;
				r.y = m_selectRect.bottom()-lineHeight;
				r.width = m_selectRect.right() - r.x;
				m_root->getRenderer()->drawRect(r, m_selectColour);
				if(selectedLines > 2) {
					r.width = m_rect.width;
					r.height = m_selectRect.height - 2*lineHeight;
					r.y = m_selectRect.y + lineHeight;
					m_root->getRenderer()->drawRect(r, m_selectColour);
				}
			}
			auto findEol = [](char* s) { char* r=strchr(s, '\n'); return r? r-s: 0; };
			int start = m_selectLength<0? m_cursor + m_selectLength: m_cursor;
			int end = m_selectLength<0? m_cursor: m_cursor + m_selectLength;
			int selectedEol = findEol(m_text + start);
			if(start + selectedEol > end) selectedEol = 0;
			int selectedBlock = abs(m_selectLength) - selectedEol;
			int postEol = findEol(m_text + end);

			Point tp = m_rect.position() + m_offset;
			drawText(tp, m_text, start, col);
			if(selectedEol>0 || m_text[start]=='\n') {
				drawText(tp, m_text+start, selectedEol, scol);
				tp.x = m_rect.x + m_offset.x;
			}
			if(selectedBlock>0) drawText(tp, m_text+start+selectedEol, selectedBlock, scol);
			if(postEol > 0 || m_text[end]=='\n') {
				drawText(tp, m_text+end, postEol, col);
				tp.x = m_rect.x + m_offset.x;
			}
			drawText(tp, m_text+end+postEol, m_length, col);
		}
		else {
			int start = m_selectLength<0? m_cursor+m_selectLength: m_cursor;
			m_root->getRenderer()->drawRect(m_selectRect, m_selectColour);
			Point tp = m_rect.position() + m_offset;
			drawText(tp, m_text, start, col);
			drawText(tp, m_text+start, abs(m_selectLength), scol);
			drawText(tp, m_text+start+abs(m_selectLength), ~0u, col);
			if(m_suffix) drawText(tp, m_suffix, ~0u, col);
		}
	}
	else {
		const Skin::State& state = m_skin->getState(getState());
		Point tp = m_rect.position() + m_offset;
		drawText(tp, m_text, -1u, state.foreColour);
		if(m_suffix) drawText(tp, m_suffix, -1u, state.foreColour);
		if(hasFocus() && (clock()&0x1000)) m_root->getRenderer()->drawRect(m_selectRect, m_selectColour); // I beam

		if(m_length==0 && m_hint && !hasFocus()) {
			unsigned hintColour = m_skin->getState(getState()).foreColour;
			hintColour = (hintColour & 0xffffff) | (hintColour >> 26) << 24;
			drawText(tp, m_hint, -1u, hintColour);
		}
	}
	m_root->getRenderer()->pop();
	drawChildren();
}




