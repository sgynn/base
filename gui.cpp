#include "gui.h"

#include "opengl.h"
#include "input.h"
#include "game.h"
#include "font.h"
#include "material.h"

#include <cstring>
#include <cstdio>

#include <assert.h>


// Use coordinates from top left rather than bottom left
#define FixY(py)	(Game::getSize().y-py-m_size.y)

using namespace base;
using namespace GUI;

Control* GUI::focus = 0;
Style GUI::defaultStyle;

/** default style */
Style::Style() : font(0), align(0), stime(0) {
	setColour(BACK, transparent); //set all back colours to transparent
	//everything else white
}
Style::Style(const Style* s) {
	memcpy(this, s, sizeof(Style));
}
void Style::setColour(int code, const Colour& colour) {
	if(code&12) {
		int ix = (code&3) + 3*(((code-4)>>2)&3);
		m_colours[ix] = colour;
	} else {
		setColour(code | BASE, colour);
		setColour(code | FOCUS, colour);
		setColour(code | OVER, colour);
	}
}

//// //// Constructors //// ////
Control::Control(Style* s, uint cmd) : m_style(s?s:&GUI::defaultStyle), m_command(cmd), m_visible(1), m_container(0) {
}
Control::~Control() {
	if(m_container) m_container->remove(this);
}
Container::Container(): Control(0, 0), m_clip(0) {
	m_size = Game::getSize();
}
Container::Container(int x, int y, int w, int h, bool clip, Style* s): Control(s, 0), m_clip(clip) {
	m_size = Point(w, h);
	m_position = Point(x, y);
}
Frame::Frame(int w, int h, const char* c, bool move, Style* s) : Container(0,0,w,h,true,s),  m_caption(c), m_moveable(move) {
}
Frame::Frame(int x, int y, int w, int h, const char* c, bool move, Style* s) : Container(x,y,w,h,true,s),  m_caption(c), m_moveable(move) {
}
Label::Label(const char* c, Style* s) : Control(s, 0), m_caption(c), m_offset(0) {
	if(c && m_style->font) {
		m_size.y = m_style->font->textHeight(c);
		m_size.x = m_style->font->textWidth(c);
	}
}
Button::Button(const char* c, Style* s, uint cmd) : Control(s, cmd), m_caption(c), m_state(0) {
	if(m_style->font && c) {
		m_size.x = m_style->font->textWidth(c);
		m_size.y = m_style->font->textHeight(c);
	} else m_size = Point(100, 20);
}
Button::Button(int w, int h, const char* c, Style* s, uint cmd) : Control(s, cmd), m_caption(c), m_state(0) {
	m_size = Point(w, h);
	m_textPos = textPosition(c); //Get text offset position
}
Checkbox::Checkbox(const char* c, bool v, Style* s, uint cmd) : Button(c,s,cmd), m_value(v) {
	m_size.x += m_size.y;
	if(!c) m_size.x = m_size.y;
}

Scrollbar::Scrollbar(int o, int w, int h, int min, int max, Style* s, uint cmd) : Control(s,cmd), m_min(min), m_max(max), m_value(min), m_orientation(o), m_held(-1), m_blockSize(16) {
	m_size = Point(w, h);
}

Listbox::Listbox(int w, int h, Style* s, uint cmd) : Control(s, cmd), m_item(0), m_hover(0), m_scroll(0,12,h,0,0,s,1) {
	m_size = Point(w,h);
	m_itemHeight = m_style->font? m_style->font->textHeight("X"): 20;
	m_scroll.hide();
}
DropList::DropList(int w, int h, int max, Style* s, uint cmd) : Listbox(w,h,s,cmd), m_max(max), m_open(0), m_state(0) {
	m_size.y = m_itemHeight;
}
GUI::Input::Input(int l, const char* t, Style* s, uint cmd) : Control(s,cmd), m_mask(0), m_state(0) {
	if(t) strcpy(m_text, t); else m_text[0] = 0;
	m_loc = m_len = strlen(m_text);
	m_size.x = l;
	m_size.y = m_style->font? m_style->font->textHeight("X"): 20;
}

//// //// Relative position //// ////
const Point Control::getPosition() const { 
	if(!m_container) return m_position;
	else {
		return Point( m_container->m_position.x-m_position.x,  m_container->m_position.y+m_container->m_size.y-m_position.y-m_size.y);
	}
}
void Control::setPosition(int x, int y) {
	if(!m_container) setAbsolutePosition(x, y);
	else {
		setAbsolutePosition(
			m_container->m_position.x + x,
			m_container->m_position.y+m_container->m_size.y - y - m_size.y
		);
	}
}
//// //// Container functions //// ////
Control* Container::add( Control* c, int x, int y ) {
	if(c->m_container) printf("Warning: Control already in a container\n");
	m_contents.push_back( c );
	c->m_container = this;
	c->setPosition(x, y);
	return c;
}
Control* Container::add( const char* caption, Control* c, int x, int y, int cw) {
	if(caption && caption[0]) {
		Label* label = new Label(caption, c->m_style);
		add(label, x, y);
	}
	return add(c, x+cw, y);
}
Control* Container::remove( Control* c ) {
	m_contents.remove(c);
	c->m_container = 0;
	return c;
}
void Container::clear() {
	std::list<Control*> tmp = m_contents; //clone list as control destructors edit original
	for(std::list<Control*>::iterator i=tmp.begin(); i!=tmp.end(); i++)  {
		delete *i;
	}
	m_contents.clear();
}
Control* Container::getControl(uint index) {
	for(std::list<Control*>::iterator i=m_contents.begin(); i!=m_contents.end(); i++) {
		if(index-- == 0) return *i;
	}
	return 0;
}
Control* Container::getControl(const Point& p) {
	Control* c = 0;
	for(std::list<Control*>::reverse_iterator i=m_contents.rbegin(); i!=m_contents.rend(); i++) {
		if((*i)->visible() && ((*i)->isOver(p.x, p.y) || ((*i)->isContainer() && !static_cast<Container*>(*i)->m_clip))) {
			if((*i)->isContainer()) c = static_cast<Container*>(*i)->getControl(p);
			if(c) return c;
			else return *i;
		}
	}
	return isOver(p.x, p.y)? this: 0;
}
void Container::setAbsolutePosition(int x, int y) {
	moveContents(x-m_position.x, y-m_position.y);
	m_position.x = x;
	m_position.y = y;
}
void Container::moveContents(int dx, int dy) {
	for(std::list<Control*>::iterator i=m_contents.begin(); i!=m_contents.end(); i++) {
		const Point& p = (*i)->m_position;
		(*i)->setAbsolutePosition( p.x+dx, p.y+dy );
	}
}
void Control::raise() {
	if(m_container) {
		m_container->m_contents.remove(this);
		m_container->m_contents.push_back(this);
	}
}

//// //// List Functions //// ////
uint Listbox::addItem( const char* itemText, bool s) {
	ListItem item; item.name = itemText; item.state=0;
	m_items.push_back(item);
	if(s) m_item = m_items.size()-1;
	int scrollMax = m_items.size()*m_itemHeight - m_size.y;
	if(scrollMax>0) m_scroll.setRange(0, scrollMax);
	if(scrollMax>0) m_scroll.show();
	return m_items.size();
}
uint Listbox::removeItem(uint index) {
	for(uint i=index; i<m_items.size()-1; i++) m_items[i] = m_items[i+1];
	m_items.pop_back();
	int scrollMax = m_items.size()*m_itemHeight - m_size.y;
	if(scrollMax>0) m_scroll.setRange(0, scrollMax);
	if(!scrollMax>0) m_scroll.hide();
	return m_items.size();
}
void Listbox::clearItems() {
	m_items.clear();
	m_item = m_hover = 0;
	m_scroll.hide();
}
void Listbox::scrollTo(uint index) {
	//Scroll until index is on screen
	if(m_scroll.getValue()>(int)(index*m_itemHeight)) m_scroll.setValue(index*m_itemHeight);
	else if(m_scroll.getValue() < (int)((index+1)*m_itemHeight)-m_size.y) m_scroll.setValue((index+1)*m_itemHeight-m_size.y);
}
void Listbox::setAbsolutePosition(int x, int y) {
	Control::setAbsolutePosition(x, y);
	m_scroll.setAbsolutePosition(x + m_size.x - m_scroll.getSize().x, y);
}
void Listbox::setSize(int w, int h) {
	Control::setSize(w,h);
	//m_scroll.setPosition(m_position.x+m_size.x-m_scroll.getSize().x, m_position.y);
	m_scroll.setAbsolutePosition(m_position.x + m_size.x - m_scroll.getSize().x, m_position.y);
	m_scroll.setSize(m_scroll.getSize().x, h);
	int scrollMax = m_items.size()*m_itemHeight - m_size.y;
	if(scrollMax>0) m_scroll.setRange(0, scrollMax);
	if(scrollMax>0) m_scroll.show(); else m_scroll.hide();
}

//// //// Accessor Functions //// ////
void Label::setCaption(const char* caption) {
	m_caption = caption;
}
void Listbox::selectItem(uint index) {
	m_item = index;
}
void GUI::Input::setText(const char* c) {
	strcpy(m_text, c);
	m_len = m_loc = strlen(c);
}

//// //// Utility Functions //// ////
float Control::updateState(bool on, float state) const {
	if(m_style->stime>0) {
		float time = Game::frameTime();
		if(on) { if(state<1) state+=time/m_style->stime; else state=1; }
		else if(state>0) state-=time/m_style->stime; else state=0;
	} else state = on? 1:0;
	return state;
}
uint Control::setEvent(Event* e, uint value, const char* txt) {
	if(e) {
		e->command = m_command;
		e->value = value;
		e->text = txt;
		e->control = this;
	}
	return m_command;
}

Point Control::textPosition(const char* c, int oa, int ob) {
	Point p;
	if(!m_style->font || !c || !c[0]) return p; //No text
	switch(m_style->align) {
	case Style::LEFT:
		p.x = oa;
		break;
	case Style::CENTRE:
		p.x = (m_size.x-oa-ob)/2 - m_style->font->textWidth(c)/2 + oa;
		p.y = m_size.y/2 - m_style->font->textHeight(c)/2;
		break;
	case Style::RIGHT:
		p.x = m_size.x - m_style->font->textWidth(c) - ob;
		break;
	}
	return p;
}

//// //// Update Functions //// ////
uint Control::update(Event* e) {
	if(!m_container && (Game::MouseClick()&1)) {
		Point p; Game::Mouse(p.x, p.y);
		if(isOver(p.x, p.y)) return click(e, p);
		else if(focus==this) focus = 0;
	}
	return 0;
}
uint Container::update(Event* e) {
	int r = Control::update(e);
	for(std::list<Control*>::reverse_iterator i=m_contents.rbegin(); i!=m_contents.rend(); i++) {
		if((*i)->m_visible) {
			int c = (*i)->update(r? 0: e);
			if(c) r = c;
		}
	}
	return e&&r?e->command:r;
}
uint Frame::update(Event* e) {
	int r = Container::update(e);
	//Drag frame
	if(m_moveable && r==0) {
		int mx, my, mb;
		mb = Game::Mouse(mx, my);
		if(Game::MouseClick() & 1) {
			if(isOver(mx, my) && focus==this) {
				m_held.x = mx - m_position.x;
				m_held.y = my - m_position.y;
			 } else if(focus == this) focus = 0;
		} else if((mb & 1) && focus==this) {
			int nx = mx - m_held.x;
			int ny = my - m_held.y;
			moveContents(nx-m_position.x, ny-m_position.y);
			m_position.x = nx;
			m_position.y = ny;
		}
	}
	return r;
}
uint Button::update(Event* e) {
	int mx, my;
	Game::Mouse(mx, my);
	bool over = isOver(mx, my);
	m_state = updateState(over||focus==this, m_state);
	if(focus==this && Game::Pressed(KEY_ENTER)) return setEvent(e);
	return Control::update(e);
}
uint Checkbox::update(Event* e) {
	uint r = Button::update(e);
	if(e && e->control == this) e->value = m_value = !m_value;
	return r;
}
uint Scrollbar::update(Event* event) {
	int mx, my;
	int lv = m_value;
	if(!Game::Mouse(mx,my)) m_held = -1;
	if(m_held>=0) {
		if(m_orientation) { //Horizontal
			int sx = m_position.x + m_size.y;
			int sh = m_size.x - 2 * m_size.y - m_blockSize; 
			int sp = mx - m_held - sx;
			setValue(m_min + sp * (m_max-m_min) / sh);
		} else { //Vertical
			int sy = m_position.y + m_size.x;
			int sh = m_size.y - 2 * m_size.x - m_blockSize; 
			int sp = my - m_held - sy;
			setValue(m_max - sp * (m_max-m_min) / sh);
		}
	} else if(Game::MouseWheel() && isOver(mx, my)) {
		setValue( m_value - Game::MouseWheel() );
	} else if(m_held==-2) {
		m_repeat -= Game::frameTime();
		if(m_repeat<0) { setValue(m_value+1); m_repeat = 0.05; }
	} else if(m_held==-3) {
		m_repeat-=Game::frameTime();
		if(m_repeat<0) { setValue(m_value-1); m_repeat = 0.05; }
	}
	if(m_value != lv) return setEvent(event, m_value);
	return Control::update(event);
}
uint Listbox::update(Event* e) {
	int mx, my;
	Game::Mouse(mx, my);
	bool over = isOver(mx, my);
	if(over && m_scroll.visible() && mx > m_scroll.getPosition().x) over = false;
	m_hover = over? (m_position.y + m_size.y - my + m_scroll.getValue()) / m_itemHeight: m_items.size();
	uint lItem = m_item;
	// Arrows
	if(focus==this && Game::Pressed(KEY_UP) && m_item>0) m_item--;
	if(focus==this && Game::Pressed(KEY_DOWN) && m_item<m_items.size()-1) m_item++;
	// Mouse wheel
	if(over && m_scroll.visible() && Game::MouseWheel()) {
		m_scroll.setValue( m_scroll.getValue() - Game::MouseWheel() * (int)m_itemHeight*0.5 );
	}
	// Scrollbar
	if(m_scroll.visible()) m_scroll.update(0);
	// Update mouseover stuff
	for(uint i=0; i<m_items.size(); i++) {
		m_items[i].state = updateState(i==m_hover||i==m_item, m_items[i].state);
	}
	//Return event
	if(lItem!=m_item) {
		if(m_item<m_items.size()) return setEvent(e, m_item, m_items[m_item].name);
		//else return setEvent(e, 0xffffff); //Deselect event?
	}
	return Control::update(e);
}
uint DropList::update(Event* e) {
	int mx, my;
	Game::Mouse(mx, my);
	bool over = isOver(mx, my);
	uint lItem = m_item;
	if(m_open) {
		Listbox::update(e);
		if(Game::Pressed(KEY_ENTER) && !Game::Mouse()) closeList();
	} else {
		if(focus==this && (Game::Pressed(KEY_UP) || Game::Pressed(KEY_DOWN))) {
			openList();
			Listbox::update(e);
		}
	}
	//Over state
	m_state = updateState(over||focus==this, m_state);
	//if(focus!=this) closeList();
	return lItem==m_item? 0 :m_command;
}
void DropList::openList() {
	m_open = true;
	int sy = m_itemHeight * m_items.size();
	if(sy > m_max && m_max>0) sy = m_max;
	//m_position.y -= sy - m_itemHeight;
	setSize(m_size.x, sy);
}
void DropList::closeList() {
	m_open = false;
	m_position.y += m_size.y - m_itemHeight;
	m_size.y = m_itemHeight;
}

uint GUI::Input::update(Event* e) {
	int mx, my; Game::Mouse(mx, my);
	bool over = isOver(mx, my);
	m_state = updateState(over||focus==this, m_state);
	
	if(focus == this) {
		//Read character input
		if(Game::LastKey() == KEY_ENTER) {
		       	m_loc = m_len; focus = 0;
		       	return setEvent(e, 0, m_text);
		}
		if(Game::LastKey() == KEY_LEFT && m_loc>0) m_loc--;
		if(Game::LastKey() == KEY_RIGHT && m_text[m_loc]) m_loc++;
		if(Game::LastKey() == KEY_BACKSPACE && m_loc>0) {
			m_loc--;
			for(int i=m_loc; m_text[i]; i++) m_text[i]=m_text[i+1];
			m_len--;
		       	return setEvent(e, 0, m_text);
		}
		//if(Game::LastKey()>=32 && Game::LastKey()<127) {
		//	char c = Game::LastKey();
		//	if(Game::Key(KEY_LSHIFT) || Game::Key(KEY_RSHIFT)) {
		//		static const char* cnum = ")!\"?$%^&*(";
		//		if(c>='a' && c<=125) c -= 'a'-'A';
		//		else if(c>='0' && c<='9') c = cnum[c-'0'];
		//	}
		char c = Game::input()->lastChar();
		if(c>=32) {
			for(int i=m_len; i>=m_loc; i--) m_text[i+1] = m_text[i];
			m_text[m_loc++] = c;
			m_len++;
		       	return setEvent(e, 0, m_text);
		}
	}
	return Control::update(e);
}

//// //// Click Functions //// ////
uint Container::click(Event* e, const Point& p) {
	//call click for the control under the mouse
	Control* c = getControl(p);
	if(c) {
		focus = c;
		if(!c->isContainer()) return c->click(e, p);
	}
	return 0;
}
uint Frame::click(Event* e, const Point& p) {
	if(m_moveable) {
		m_held.x = p.x - m_position.x;
		m_held.y = p.y - m_position.y;
	}
	return Container::click(e, p);
}
uint Button::click(Event* e, const Point& p) {
	return setEvent(e);
}
uint Checkbox::click(Event* e, const Point& p) {
	m_value = !m_value;
	return setEvent(e, m_value);
}
uint Scrollbar::click(Event* e, const Point& p) {
	if(isOver(p.x, p.y)) {
		int lv = m_value;
		if(m_orientation) { //Horizontal bar
			if(p.x < m_position.x + m_size.y) {
				setValue(m_value - 1); //Up
				m_held = -3;
			} else if(p.x > m_position.x + m_size.x - m_size.y) {
				setValue(m_value + 1); //Down 
				m_held = -2;
			} else {
				int px = m_value * (m_size.x-2*m_size.y-m_blockSize) / (m_max-m_min);
				int sp = m_position.x + m_size.y + px;
				if(p.x>sp && p.x<sp+m_blockSize) m_held = p.x-sp;
			}
		} else { //Vertical bar
			if(p.y < m_position.y + m_size.x) {
				setValue(m_value + 1); //Down
				m_held = -2;
			} else if(p.y > m_position.y + m_size.y - m_size.x) {
				setValue(m_value - 1); //Up 
				m_held = -3;
			} else {
				int sl = m_size.y - 2*m_size.x - m_blockSize;
				int py = m_value * (m_size.y-2*m_size.x-m_blockSize) / (m_max-m_min);
				int sp = m_position.y + m_size.x +  sl - py;
				if(p.y>sp && p.y<sp+m_blockSize) m_held = p.y-sp;
			}
		}
		m_repeat = 0.4;
		if(m_value != lv) return setEvent(e, m_value);
	} else m_held = -1;
	return 0;
}
uint Listbox::click(Event* e, const Point& p) {
	//scrollbar
	if(m_scroll.visible() && m_scroll.click(0, p)) return 0;
	if(p.x > m_scroll.getAbsolutePosition().x) return 0;
	//Item position
	int itmX = m_position.x;
	int itmY = m_position.y + m_size.y - m_hover*m_itemHeight + m_scroll.getValue();
	//Handle click
	if(m_hover<count() && clickItem(m_hover, m_items[m_hover].name, p.x-itmX, itmY-p.y)) {
		m_item = m_hover;
		return setEvent(e, m_item, m_items[m_item].name);
	} else return 0;
}
uint DropList::click(Event* e, const Point& p) {
	if(!m_open) {
		raise();
		openList();
		return 0;
	} else {
		closeList();
		return Listbox::click(e, p);
	}
}
uint GUI::Input::click(Event* e, const Point& p) {
	//get character location
	for(m_loc=0; m_loc<m_len; m_loc++) {
		char c = m_text[m_loc+1]; m_text[m_loc+1]=0;
		int len = m_style->font->textWidth(m_text);
		m_text[m_loc+1] = c;
		if(m_position.x + len > p.x) break;
	}
	return 0;
}





//// //// General Draw Functions //// ////
void Control::drawFrame(const Point& pt, const Point& sz, const char* title, Style* style) const {
	if(!style) style = &defaultStyle;
	if(!style->font) title = 0;
	float tw = title? style->font->textWidth(title) + 16: 0;
	float th = title? sz.y - style->font->textHeight(title): 0;
	float points[16] = { pt.x,pt.y,  pt.x+sz.x,pt.y,  pt.x+sz.x,pt.y+sz.y,  pt.x,pt.y+sz.y, /*title*/  pt.x,pt.y+th,  pt.x+tw,pt.y+th,  pt.x+tw+10,pt.y+th+10,  pt.x+tw+10,pt.y+sz.y };
	static const ubyte ix1[4] = { 0,1,2,3 };	//For background (GL_QUADS)
	//static const ubyte ix2[5] = { 0,4,5,6,7 };	//For title background (GL_TRIANGLE_FAN)
	static const ubyte ix3[9] = { 0,1,2,3,0,4,5,6,7 }; // For lines (GL_LINE_STRIP)

	glDisable(GL_TEXTURE_2D);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, points);
	glColor4fv(style->getColour(Style::BACK)); // Draw Background
	glDrawElements(GL_QUADS, 4, GL_UNSIGNED_BYTE, ix1);
	glColor4fv(style->getColour(Style::BORDER)); // Draw Border
	glDrawElements(GL_LINE_STRIP, title?9:5, GL_UNSIGNED_BYTE, ix3);
	glDisableClientState(GL_VERTEX_ARRAY);
	glEnable(GL_TEXTURE_2D);

	if(title) {
		glColor4fv(style->getColour(Style::TEXT));
		style->font->print(pt.x+8, pt.y+th, title);
	}
}
void Control::drawArrow(const Point& p, int direction, int size) const {
	//Non-texture arrow glyphs
	float v[8] = { 0,1,  -1,0,  0,-1,  1,0 };
	glDisable(GL_TEXTURE_2D);
	glBegin(GL_TRIANGLES);
	glColor4fv( m_style->getColour(Style::TEXT) );
	for(int i=0; i<3; i++) {
		int k = (i+direction+1)*2 % 8;
		glVertex2f( p.x+v[k]*size, p.y+v[k+1]*size);
	}
	glEnd();
	glEnable(GL_TEXTURE_2D);
}
Colour Control::blendColour(int type, int state, float value) const {
	const Colour& base = m_style->getColour(type);
	if((state>>2)<=1  || value == 0) {
		return base;
	} else {
		return Colour::lerp(base, m_style->getColour(type | state), value);
	}
}

//// //// Control Draw Functions //// ////
struct Scissor { int x, y, w, h; };
std::vector<Scissor> scissorStack;
void scissorPushNew(int x, int y, int w, int h) {
	if(scissorStack.empty()) glEnable(GL_SCISSOR_TEST);
	Scissor s; s.x=x; s.y=y; s.w=w; s.h=h;
	glScissor(x, y, w, h);
	scissorStack.push_back(s);
}
void scissorPush(int x, int y, int w, int h) {
	if(scissorStack.empty()) scissorPushNew(x,y,w,h);
	else {
		Scissor& s = scissorStack.back();
		int x2 = s.x+s.w < x+w? s.x+s.w: x+w; 
		int y2 = s.y+s.h < y+h? s.y+s.h: y+h; 
		if(s.x>x) x=s.x;
		if(s.y>y) y=s.y;
		scissorPushNew(x, y, x2-x, y2-y);
	}
}
void scissorPop() {
	scissorStack.pop_back();
	if(scissorStack.empty()) glDisable(GL_SCISSOR_TEST);
	else glScissor(scissorStack.back().x, scissorStack.back().y, scissorStack.back().w, scissorStack.back().h);
}

void Container::draw() {
	if(m_contents.empty()) return;
	if(m_clip) scissorPush(m_position.x, m_position.y, m_size.x, m_size.y);
	for(std::list<Control*>::iterator i=m_contents.begin(); i!=m_contents.end(); i++) {
		if((*i)->m_visible) (*i)->draw();
	}
	if(m_clip) scissorPop();
}
void Frame::draw() {
	drawFrame(m_position, m_size, m_caption, m_style);
	Container::draw();	
}
void Label::draw() {
	glColor4fv(m_style->getColour(Style::TEXT));
	m_style->font->print(m_position.x, m_position.y, m_caption);
}
void Button::draw() {
	if(m_style->getColour(Style::BACK).a>0 || m_style->getColour(Style::BORDER).a>0) {
		drawFrame(m_position, m_size, 0, m_style);
	}
	if(m_caption) {
		glColor4fv( blendColour(Style::TEXT, focus==this? Style::FOCUS: Style::OVER, m_state) );
		m_style->font->print(m_position.x+m_textPos.x, m_position.y+m_textPos.y, m_caption);
	}
}
void Checkbox::draw() {
	drawFrame(m_position+Point(0, 3), Point(m_size.y-6, m_size.y-6), 0, m_style);
	glColor4fv( blendColour(Style::TEXT, focus==this? Style::FOCUS: Style::OVER, m_state) );
	m_style->font->print(m_position.x+m_size.y-3, m_position.y, m_caption);
	if(m_value) {
		glDisable(GL_TEXTURE_2D);
		Point c = m_position + Point(m_size.y/2-3, m_size.y/2);
		float s = (m_size.y-9) / 2;
		float v[8] = { c.x, c.y-s,  c.x+s,c.y,  c.x,c.y+s,  c.x-s,c.y };
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer( 2, GL_FLOAT, 0, v);
		glDrawArrays(GL_QUADS, 0, 4);
		glDisableClientState(GL_VERTEX_ARRAY);
		glEnable(GL_TEXTURE_2D);
	}
}
void Scrollbar::draw() { 
	//Draw track
	//glColor4fv(m_style->back);
	//drawImage(m_position, m_size);
	if(m_orientation) {
		Point btnSize(m_size.y, m_size.y);
		Point btnHalf(m_size.y/2+3, m_size.y/2);
		Point btnPos(m_position + m_size - btnSize);
		//Buttons
		drawFrame(m_position, btnSize, 0, m_style);
		drawArrow(m_position + btnHalf, 3, btnHalf.y*0.6);
		drawFrame(btnPos, btnSize, 0, m_style);
		drawArrow(m_position + m_size - btnHalf, 1, btnHalf.y*0.6);
		//Slider
		int px = m_value * (m_size.x-2*m_size.y-m_blockSize) / (m_max-m_min);
		int sp = m_position.x + m_size.y + px;
		drawFrame(Point(sp, m_position.y), Point(m_blockSize, m_size.y), 0, m_style); 
	} else {
		Point btnSize(m_size.x, m_size.x);
		Point btnHalf(m_size.x/2, m_size.x/2+3);
		Point btnPos(m_position + m_size - btnSize);
		//Buttons
		drawFrame(m_position, btnSize, 0, m_style);
		drawArrow(m_position + btnHalf, 0, btnHalf.x*0.6);
		drawFrame(btnPos, btnSize, 0, m_style);
		drawArrow(m_position + m_size - btnHalf, 2, btnHalf.x*0.6);
		//Slider
		int sl = m_size.y - 2*m_size.x - m_blockSize;
		int py = m_value * sl / (m_max-m_min);
		int sp = m_position.y + m_size.x + sl - py;
		drawFrame(Point(m_position.x, sp), Point(m_size.x, m_blockSize), 0, m_style);
	}
}
void Listbox::draw() {
	//Box
	drawFrame(m_position, m_size, 0, m_style);
	//Items (need to scissor this
	scissorPush(m_position.x, m_position.y, m_size.x, m_size.y);
	//Items
	uint ni = ceil(m_size.y / m_itemHeight) + 1;
	uint ix = floor(m_scroll.getValue() / m_itemHeight);
	int py = m_position.y + m_size.y - (ix+1) * m_itemHeight + m_scroll.getValue();
	int cellWidth = m_scroll.visible()? m_size.x-m_scroll.getSize().x: m_size.x;
	for(uint i=0; i<ni && ix+i<m_items.size(); i++) {
		drawItem(ix+i, m_items[ix+i].name, m_position.x, py-i*m_itemHeight, cellWidth, m_itemHeight, m_items[ix+i].state);
	}

	//Scrollbar
	if(m_scroll.visible()) m_scroll.draw();
	scissorPop();
}
void Listbox::drawItem(uint index, const char* item, int x, int y, int width, int height, float state) {
	int s = index==m_item? Style::FOCUS: index==m_hover? Style::OVER: 0;
	glColor4fv( blendColour(Style::TEXT, s, state));
	m_style->font->print(x, y, item);
}

void DropList::draw() {
	if(m_open) {
		scissorPushNew(m_position.x-1, m_position.y-1, m_size.x+1, m_size.y+1); 
		Listbox::draw();
		scissorPop();
	} else {
		drawFrame(m_position, m_size, 0, m_style);
		if(m_item<m_items.size()) {
			glColor4fv( blendColour(Style::TEXT, focus==this? Style::FOCUS: Style::OVER, m_state) );
			m_style->font->print(m_position.x, m_position.y, m_items[m_item].name);
		}
	}
}

void GUI::Input::draw() {
	//Box
	drawFrame(m_position, m_size, 0, m_style);
	if(m_len>0) {
		//Masked
		static char masked[128];
		if(m_mask) { for(int i=0; i<m_len; i++) masked[i]=m_mask; masked[m_len]=0; }
		glColor4fv( blendColour(Style::TEXT, focus==this? Style::FOCUS: Style::OVER, m_state) );
		if(m_text[0]) m_style->font->print(m_position.x, m_position.y, m_mask?masked:m_text);
		//Caret
		if(focus==this) {
			char* tmp = m_mask?masked:m_text;
			char t = tmp[m_loc]; tmp[m_loc]=0;
			int cp = m_style->font->textWidth(tmp);
			tmp[m_loc]=t;
			//draw it
			m_style->font->print(m_position.x + cp, m_position.y, "_");
		}
	}
}

