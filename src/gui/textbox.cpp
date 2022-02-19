#include <base/gui/widgets.h>
#include <base/gui/renderer.h>
#include <base/gui/skin.h>
#include <base/gui/font.h>
#include <base/input.h>
#include <cstdio>
#include <ctime>

using namespace gui;

Textbox::Textbox(const Rect& r, Skin* s) : Widget(r,s), m_text(0), m_buffer(0), m_length(0), m_cursor(0), m_selectLength(0), m_readOnly(false), m_password(0), m_suffix(0), m_offset(0), m_selection(0) {
	int h = s&&s->getFont()? s->getFont()->getSize('x', s->getFontSize()).y: 16;
	m_selectRect.set(0,0,1,h);
	m_buffer = 16;
	m_text = new char[m_buffer];
	m_selectColour = 0x80ffff00;
	m_text[0] = 0;
	m_selectRect.position() = m_rect.position();
}
Textbox::~Textbox() {
	if(m_text) delete [] m_text;
	if(m_selection) delete [] m_selection;
	free(m_suffix);
}
void Textbox::initialise(const Root*, const PropertyMap& p) {
	if(p.contains("text")) setText(p["text"]);
	if(p.contains("readonly")) m_readOnly = atoi(p["readonly"]);
	if(p.contains("password")) m_password = p["password"][0];
	if(p.contains("suffix")) setSuffix(p["suffix"]);
}
void Textbox::setPosition(int x, int y) {
	int sx = m_selectRect.x - m_rect.x;
	Widget::setPosition(x, y);
	m_selectRect.x = m_rect.x + sx;
	m_selectRect.y = m_rect.y;
	
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
	select(m_cursor);
}

void Textbox::setReadOnly(bool r) {
	m_readOnly = r;
}

void Textbox::setPassword(char c) {
	m_password = c;
}

void Textbox::setSuffix(const char* s) {
	free(m_suffix);
	m_suffix = s && s[0]? strdup(s): 0;
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
	if(m_buffer < start + s + lastBit) {
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
	select(start+s);
	m_text[m_length] = 0;
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
	int start = len<0? s+len: s;
	if(s!=m_cursor) {
		Point textSize = m_skin->getFont()->getSize(m_text, m_skin->getFontSize(), start);
		m_selectRect.x = m_rect.x + textSize.x - m_offset;
		m_selectRect.y = m_rect.y;
		m_cursor = s;
	}
	if(len!=0) {
		Point textSize = m_skin->getFont()->getSize(m_text+start, m_skin->getFontSize(), abs(len));
		m_selectRect.width = textSize.x;
	}
	else m_selectRect.width = 1;
	m_selectLength = len;
	updateOffset(false);
}

void Textbox::updateOffset(bool end) {
	int min = m_rect.x + m_skin->getState( getState() ).textPos.x;
	int max = m_rect.right() - 1;
	int x = end? m_selectRect.right(): m_selectRect.x;
	int shift = 0;
	if(x < min) shift = min - x;
	else if(x > max) shift = max - x;
	m_selectRect.x += shift;
	m_offset -= shift;
}

void Textbox::onKey(int code, wchar_t chr, KeyMask mask) {
	//printf("Key %c mask %d\n", (char)chr, (int)mask);

	if(code == base::KEY_ENTER) {
		if(eventSubmit) eventSubmit(this);
	}
	else if(code == base::KEY_LEFT)  select(m_cursor-1, 0, mask==KeyMask::Shift);
	else if(code == base::KEY_RIGHT) select(m_cursor+1, 0, mask==KeyMask::Shift);
	else if(code == base::KEY_HOME)  select(0,          0, mask==KeyMask::Shift);
	else if(code == base::KEY_END)   select(m_length,   0, mask==KeyMask::Shift);
	else if(code == base::KEY_BACKSPACE) {
		if(m_selectLength!=0) insertText("");
		else if(m_cursor>0 && !m_readOnly) {
			select(m_cursor-1);
			for(int i=m_cursor; i<m_length; ++i) m_text[i] = m_text[i+1];
			--m_length;
		}
		if(eventChanged) eventChanged(this, m_text);
	}
	else if(code == base::KEY_DEL) {
		if(m_selectLength!=0) insertText("");
		else if(m_cursor<m_length && !m_readOnly) {
			for(int i=m_cursor; i<m_length; ++i) m_text[i] = m_text[i+1];
			--m_length;
		}
		if(eventChanged) eventChanged(this, m_text);
	}
	else if(chr>=32 && !m_readOnly && mask!=KeyMask::Ctrl) {
		char tmp[2] = { (char)chr, 0 };
		insertText(tmp);
		if(eventChanged) eventChanged(this, m_text);
	}
}
int Textbox::indexAt(int pos) const {
	pos -= m_skin->getState( getState() ).textPos.x - m_offset;
	for(int i=0; i<m_length; ++i) {
		Point characterSize = m_skin->getFont()->getSize(m_text[i], m_skin->getFontSize());
		if(pos < characterSize.x/2) return i;
		pos -= characterSize.x;
	}
	return m_length;
}
void Textbox::onMouseButton(const Point& p, int b, int u) {
	if(b==1) select( indexAt(p.x-m_rect.x) ), m_held=m_cursor;
	Widget::onMouseButton(p, b, u);
}
void Textbox::onMouseMove(const Point&, const Point& p, int b) {
	if(b==1) {
		int i = indexAt(p.x-m_rect.x);
		select(i, m_held-i);
		updateOffset(i>m_held);
	}
}



inline void Textbox::drawText(Point& p, char* t, uint len, uint col) const {
	if(len>0 && t[0]) {
		if(len==~0u) p = m_root->getRenderer()->drawText(p, t, m_skin, getState());
		else {
			char c = t[len]; t[len] = 0;
			p = m_root->getRenderer()->drawText(p, t, m_skin, getState());
			t[len] = c;
		}
	}
}
void Textbox::draw() const {
	if(!isVisible()) return;
	m_root->getRenderer()->drawSkin(m_skin, m_rect, m_colour, getState());
	// Selection - do we change text colour too?
	m_root->getRenderer()->push(m_rect);
	if(m_selectLength!=0) {
		int start = m_selectLength<0? m_cursor+m_selectLength: m_cursor;
		m_root->getRenderer()->drawRect(m_selectRect, m_selectColour);
		uint col = m_skin->getState( getState() ).foreColour;
		Point tp = m_rect.position();
		tp.x -= m_offset;
		drawText(tp, m_text, start, col);
		drawText(tp, m_text+start, abs(m_selectLength), col);
		drawText(tp, m_text+start+abs(m_selectLength), ~0u, col);
		if(m_suffix) drawText(tp, m_suffix, ~0u, col);
	}
	else {
		Point tp = m_rect.position();
		tp.x -= m_offset;
		tp = m_root->getRenderer()->drawText(tp, m_text, m_skin, getState());
		if(m_suffix) m_root->getRenderer()->drawText(tp, m_suffix, m_skin, getState());
		if(hasFocus() && (clock()&0x1000)) m_root->getRenderer()->drawRect(m_selectRect, m_selectColour); // I beam
	}
	m_root->getRenderer()->pop();
	drawChildren();
}




