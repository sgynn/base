#include <base/gui.h>

#include <base/opengl.h>
#include <base/input.h>
#include <base/game.h>
#include <base/font.h>
#include <base/material.h>

#include <cstring>
#include <cstdio>

#include <assert.h>

using namespace base;
using namespace gui;

//// //// //// //// //// //// //// //// GUI STYLES //// //// //// //// //// //// //// ////

/** Default style if none set */
static Style gui_defaultStyle;

/**  Default style: Back:transparent, Front:white */
Style::Style() : m_ref(0), m_font(0), m_align(0), m_fadeTime(0) {
	setColour(BACK, transparent);
	memset(m_frames, 0, sizeof(m_frames)); // Set all frames to 0
}
/** Copy contsuctor from pointer */
Style::Style(const Style* s): m_ref(0) {
	if(!s) s = &gui_defaultStyle;
	memcpy(this, s, sizeof(Style));
}
/** Drop reference */
void Style::drop() {
	if(--m_ref<=0 && this!=&gui_defaultStyle) delete this;
}
/** Set colour with alpha */
void Style::setColour(int type, int state, const Colour& colour, float alpha) {
	Colour c(colour); c.a = alpha;
	setColour(type, state, c);
}
/** Set colour - can set multiple states at once */
void Style::setColour(int code, int state, const Colour& colour) {
	if(code>2) return; // check limits
	if(state==0) state=0xf;
	if(state & BASE)	 m_colours[ code ] = colour;
	if(state & OVER)	 m_colours[ code + 3 ] = colour;
	if(state & FOCUS)	 m_colours[ code + 6 ] = colour;
	if(state & DISABLED) m_colours[ code + 9 ] = colour;
}
/* Set frame for a state, or states */
void Style::setFrame(int state, int frame) {
	if(state==0) state=0xf;
	if(state & BASE)	 m_frames[ 0 ] = frame;
	if(state & OVER)	 m_frames[ 1 ] = frame;
	if(state & FOCUS)	 m_frames[ 2 ] = frame;
	if(state & DISABLED) m_frames[ 3 ] = frame;
}

//// //// //// //// //// //// //// ////  GUI Root Class  //// //// //// //// //// //// //// ////
Root::Root(Control* c) : m_ref(0), m_control(c), m_lastFocus(0), m_focus(0), m_over(0) {};
Root::~Root() { }
void Root::drop() {
	if(--m_ref<=0) delete this;
}
Control* Root::getControl(const char* name) const {
	if(m_names.empty() || !m_names.contains(name)) return 0;
	else return m_names[name];
}
void Root::merge(Root* r) {
	for(HashMap<Control*>::iterator it=r->m_names.begin(); it!=r->m_names.end(); ++it) m_names[it.key()] = *it;
}
int Root::update(Event& e) {
	if(!m_control) return 0;
	// Extract events
	Point mouse;
	int mb = Game::Mouse(mouse.x, mouse.y);
	int mc = Game::MouseClick();
	int mw = Game::MouseWheel();
	bool moved = mouse.x!=m_lastMouse.x || mouse.y!=m_lastMouse.y;
	// Focus changed
	if(m_focus != m_lastFocus) {
		if(m_lastFocus) m_lastFocus->loseFocus(e);
		if(m_focus) m_focus->gainFocus(e);
		m_lastFocus = m_focus;
	}
	// Mouse events can only affect the control under the mouse
	Control* c = 0;
	if(m_control->isContainer()) c = static_cast<Container*>(m_control)->getControl(mouse, true);
	else if(m_control->isOver(mouse.x, mouse.y)) c = m_control;
	// Fire events
	if(m_over && m_over!=c && m_over->enabled()) m_over->mouseExit(e);
	if(c && c->enabled()) {
		if(c!=m_over) c->mouseEnter(e);
		if(mc) c->mouseEvent(e, mouse, mc);
		if(mw) c->mouseWheel(e, mouse, mw);
		if(moved) c->mouseMove(e, mouse, m_lastMouse, mb);
		m_over = c;
	}
	// Mouse move event also to focused control for dragging stuff
	if(m_focus && moved && m_focus!=c && m_focus->enabled()) m_focus->mouseMove(e, mouse, m_lastMouse, mb);

	// Key event
	if(m_focus && m_focus->enabled() && Game::LastKey()) m_focus->keyEvent(e, Game::LastKey(), Game::LastChar());

	m_lastMouse = mouse;
	return e.id;
}
void Root::draw() { 
	if(m_control) m_control->draw();
}

Control* Control::getControl(const char* name) const {
	return m_root? m_root->getControl(name): 0;
}



//// //// //// //// //// //// //// ////  Constructors  //// //// //// //// //// //// //// ////
Control::Control(Style* s, uint cmd) : m_style(s?s:&gui_defaultStyle), m_command(cmd), m_visible(1), m_enabled(1), m_parent(0), m_root(0) {
	++m_style->m_ref;
}
Control::~Control() {
	if(m_parent) m_parent->remove(this);
	m_style->drop();
}
// Container //
Container::Container(): Control(0, 0), m_clip(0) {
	m_size = Game::getSize();
}
Container::Container(int x, int y, int w, int h, bool clip, Style* s): Control(s, 0), m_clip(clip) {
	m_size = Point(w, h);
	m_position = Point(x, y);
}
// Frame //
Frame::Frame(int w, int h, const char* c, bool move, Style* s) : Container(0,0,w,h,true,s),  m_caption(0), m_moveable(move), m_held(0) { setCaption(c); }
Frame::Frame(int x, int y, int w, int h, const char* c, bool move, Style* s) : Container(x,y,w,h,true,s),  m_caption(0), m_moveable(move), m_held(0) { setCaption(c); }
// Label //
Label::Label(const char* c, Style* s) : Control(s, 0) {
	strcpy(m_caption, c);
	m_size = textSize(c);
}
// Button //
Button::Button(const char* c, Style* s, uint cmd) : Control(s, cmd) {
	strcpy(m_caption, c?c:"");
	m_size = textSize(c) + Point(6,0);
	if(m_size.x+m_size.y==0) m_size = Point(100, 20);
	m_textPos.x = 3;
}
Button::Button(int w, int h, const char* c, Style* s, uint cmd) : Control(s, cmd) {
	strcpy(m_caption, c?c:"");
	m_size = Point(w, h);
	m_textPos = textPosition(c); //Get text offset position
}
// Checkbox //
Checkbox::Checkbox(int size, const char* c, bool v, Style* s, uint cmd) : Button(c,s,cmd), m_boxSize(size), m_value(v) {
	m_size.x += size;
	if(m_size.y<size) m_size.y = size;
}
// Slider //
Slider::Slider(int o, int w, int h, Style* s, uint cmd) : Control(s,cmd), m_min(0), m_max(1), m_value(0), m_step(0), m_orientation(o), m_held(-1) {
	m_size = Point(w, h);
}
// Scrollbar //
Scrollbar::Scrollbar(int o, int w, int h, int min, int max, Style* s, uint cmd) : Control(s,cmd), m_min(min), m_max(max), m_value(min), m_orientation(o), m_held(-1), m_blockSize(16) {
	m_size = Point(w, h);
}
// ListBase //
ListBase::ListBase(int w, int h, Style* s, uint cmd) : Control(s, cmd), m_itemHeight(0), m_selected(0), m_hover(0), m_scroll(0,12,h,0,0,s,1) {
	m_size = Point(w,h);
	m_scroll.hide();
}
// Listbox //
Listbox::Listbox(int w, int h, Style* s, uint cmd): ListBase(w,h,s,cmd) {
	m_itemHeight = textSize("X").y;
	if(m_itemHeight==0) m_itemHeight = 20;
}
// Droplist //
DropList::DropList(int w, int h, int max, Style* s, uint cmd) : Listbox(w,h,s,cmd), m_max(max), m_open(0), m_state(0) {
	m_size.y = m_itemHeight;
}
// Input //
gui::Input::Input(int w, int h, const char* t, Style* s, uint cmd) : Control(s,cmd), m_mask(0), m_state(0) {
	strcpy(m_text, t?t:"");
	m_loc = m_len = strlen(m_text);
	m_size = Point(w, h);
}

//// //// Update Function //// ////
uint Control::update() { Event e; return update(e); }
uint Control::update(Event& e) {
	if(m_parent) printf("Warning: Calling update on non-root gui element\n");
	// Add root if it doesnt exist
	if(!m_root) {
		Control* c = this;
		while(c->m_parent) c=c->m_parent;
		c->setRoot( new Root(c) );
	}
	// Call Root::update();
	return m_root->update(e);
}

//// //// Relative position //// ////
const Point Control::getPosition() const { 
	if(!m_parent) return m_position;
	else {
		return Point(
			m_position.x - m_parent->m_position.x,
			m_parent->m_position.y + m_parent->m_size.y - m_position.y - m_size.y
		);
	}
}
void Control::setPosition(int x, int y) {
	if(!m_parent) setAbsolutePosition(x, y);
	else {
		setAbsolutePosition(
			m_parent->m_position.x + x,
			m_parent->m_position.y + m_parent->m_size.y - y - m_size.y
		);
	}
}

void Control::setRoot(Root* r) {
	if(m_root) dropRoot();
	if(r) ++r->m_ref; // Add reference
	if(isContainer()) {
		Container* c = static_cast<Container*>(this);
		for(std::list<Control*>::iterator i=c->m_contents.begin(); i!=c->m_contents.end(); ++i) (*i)->setRoot(r);
	} 
	m_root = r;
}
void Control::dropRoot() {
	if(m_root) { m_root->drop(); m_root=0; }
	if(isContainer()) {
		Container* c = static_cast<Container*>(this);
		for(std::list<Control*>::iterator i=c->m_contents.begin(); i!=c->m_contents.end(); ++i) (*i)->dropRoot();
	} 
}

//// //// Container functions //// ////
Control* Container::add( Control* c, int x, int y ) {
	if(c->m_parent) {
		printf("Warning: Control already in a container\n"); 
		c->m_parent->remove(this);
	}
	m_contents.push_back( c );
	c->m_parent = this;
	c->setPosition(x, y);
	// Merge m_root
	if(m_root && c->m_root) m_root->merge(c->m_root);
	c->setRoot( m_root );
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
	c->m_parent = 0;
	c->setRoot(0);
	return c;
}
void Container::clear() {
	std::list<Control*> tmp = m_contents; //clone list as control destructors edit original
	for(std::list<Control*>::iterator i=tmp.begin(); i!=tmp.end(); i++) delete *i;
	m_contents.clear(); // Should not be nessesary
}
Control* Container::getControl(uint index) {
	for(std::list<Control*>::iterator i=m_contents.begin(); i!=m_contents.end(); i++) {
		if(index-- == 0) return *i;
	}
	return 0;
}
Control* Container::getControl(const Point& p, bool recursive) {
	for(std::list<Control*>::reverse_iterator i=m_contents.rbegin(); i!=m_contents.rend(); i++) {
		if((*i)->visible() && ((*i)->isOver(p.x, p.y) || ((*i)->isContainer() && !static_cast<Container*>(*i)->m_clip))) {
			if(recursive && (*i)->isContainer()) {
				Control* c = static_cast<Container*>(*i)->getControl(p);
				if(c) return c;
			} else return *i;
		}
	}
	return isOver(p.x, p.y)? this: 0;
}
void Container::setAbsolutePosition(int x, int y) {
	int dx = x-m_position.x;
	int dy = y-m_position.y;
	m_position.x = x;
	m_position.y = y;
	moveContents(dx, dy);
}
void Container::moveContents(int dx, int dy) {
	if(dx==0 && dy==0) return;
	for(std::list<Control*>::iterator i=m_contents.begin(); i!=m_contents.end(); i++) {
		const Point& p = (*i)->m_position;
		(*i)->setAbsolutePosition( p.x+dx, p.y+dy );
	}
}
void Control::raise() {
	if(m_parent) {
		m_parent->m_contents.remove(this);
		m_parent->m_contents.push_back(this);
	}
}

//// //// //// ////  Accessor Functions //// //// //// ////
void Frame::setCaption(const char* caption) {
	if(m_caption) free(m_caption);
	m_caption = caption? strdup(caption): 0;
}
void Label::setCaption(const char* caption) {
	strcpy(m_caption, caption);
	Point s = textSize(caption);
	setSize(s.x, s.y);
}
void Button::setCaption(const char* caption) {
	strcpy(m_caption, caption);
	m_textPos = textPosition(caption);
}
void gui::Slider::setValue(float v) {
	if(m_step>0) v = round((v-m_min)/m_step)*m_step + m_min;
	m_value = v<m_min? m_min: v>m_max? m_max: v;
}
void gui::Input::setText(const char* c) {
	strcpy(m_text, c);
	m_len = m_loc = strlen(c);
}

//// //// //// //// Utility Functions //// //// //// ////
uint Control::setEvent(Event& e, uint value, const char* txt) {
	e.id = m_command;
	e.value = value;
	e.text = txt;
	e.control = this;
	return m_command;
}
Point Control::textSize(const char* text) const {
	if(text && m_style && m_style->m_font) {
		return Point(m_style->m_font->textWidth(text), m_style->m_font->textHeight(text));
	} else return Point();
}

Point Control::textPosition(const char* c) const {
	Point ts = textSize(c);
	switch(m_style->m_align) {
	case Style::LEFT: 	return Point();
	case Style::CENTRE: return Point(m_size.x/2 - ts.x/2, m_size.y/2 - ts.y/2);
	case Style::RIGHT:	return Point(m_size.y - ts.x, 0);
	default: return Point();
	}
}

bool Control::hasFocus() const { return m_root && m_root->m_focus==this; }
void Control::setFocus() { if(m_root) m_root->m_focus = this; }
int Control::getState() const {
	if(!m_enabled) return Style::DISABLED;
	if(!m_root) return Style::BASE;
	if(m_root->m_focus==this) return Style::FOCUS;
	if(m_root->m_over==this) return Style::OVER;
	return Style::BASE;
}

//// //// //// //// List Functions //// //// //// ////

void ListBase::updateBounds() {
	int scrollMax = size() * m_itemHeight - m_size.y;
	if(scrollMax>0) m_scroll.setRange(0, scrollMax);
	m_scroll.setVisible(scrollMax>0);
}
void ListBase::setAbsolutePosition(int x, int y) {
	Control::setAbsolutePosition(x, y);
	m_scroll.setAbsolutePosition(x + m_size.x - m_scroll.getSize().x, y);
}
void ListBase::setSize(int w, int h) {
	Control::setSize(w,h);
	m_scroll.setAbsolutePosition(m_position.x + m_size.x - m_scroll.getSize().x, m_position.y);
	m_scroll.setSize(m_scroll.getSize().x, h);
	updateBounds();
}
void ListBase::scrollTo(uint index) {
	//Scroll until index is on screen
	if(m_scroll.getValue()>(int)(index*m_itemHeight)) m_scroll.setValue(index*m_itemHeight);
	else if(m_scroll.getValue() < (int)((index+1)*m_itemHeight)-m_size.y) m_scroll.setValue((index+1)*m_itemHeight-m_size.y);
}


//// Listbox function ////
void Listbox::addItem(const char* item, bool s) {
	addItem(item, size(), s);
}
void Listbox::addItem(const char* item, uint index, bool s) {
	Item i; i.text = strdup(item);
	if(index<size()) m_items.insert( m_items.begin()+index, i);
	else m_items.push_back(i);
	updateBounds();
	if(s) m_selected = size()-1;
}
void Listbox::removeItem(uint index) {
	if(index<size()) {
		free(m_items[index].text);
		m_items.erase(m_items.begin()+index);
	}
	updateBounds();
}
void Listbox::clearItems() {
	for(uint i=0; i<size(); ++i) free(m_items[i].text);
	m_items.clear();
	updateBounds();
}
const char* Listbox::getItem(uint index) const {
	return index<size()? m_items[index].text: 0;
}

//// DropList ////
void DropList::openList() {
	m_open = true;
	int sy = m_itemHeight * m_items.size();
	if(sy > m_max && m_max>0) sy = m_max;
	setSize(m_size.x, sy);
}
void DropList::closeList() {
	m_open = false;
	m_position.y += m_size.y - m_itemHeight;
	m_size.y = m_itemHeight;
}

//// //// //// //// //// //// //// ////  EVENTS  //// //// //// //// //// //// //// ////

// Frame //
uint Frame::mouseEvent(Event& e, const Point& pos, int b) {
	if(m_moveable && b==1) {
		setFocus();
		m_held = true;
	} else m_held = false;
	return 0;
}
uint Frame::mouseMove(Event& e, const Point& pos, const Point& last, int b) {
	if(m_held) {
		Point delta = pos-last;
		moveContents(delta.x, delta.y);
		m_position = m_position + delta;
		if(b!=1) m_held = false;
	}
	return 0;
}

// Button //
uint Button::mouseEvent(Event& e, const Point& p, int b) {
	if(b==1) { setFocus(); return setEvent(e); }
	else return 0;
}

// Checkbox //
uint Checkbox::mouseEvent(Event& e, const Point& p, int b) {
	if(b==1) {
		setFocus();
		m_value=!m_value; 
		return setEvent(e, m_value);
	} else return 0;
}

// Slider //
uint Slider::mouseEvent(Event& e, const Point& p, int b) {
	if(b!=1) return 0;
	float vn = (float)(m_value-m_min) / (m_max-m_min); // Normalised value
	if(m_orientation==HORIZONTAL) {
		int bs = m_size.y;
		int block = m_position.x + vn * (m_size.x - bs);
		if(p.x>block && p.x<block+bs) m_held = p.x-block;
	} else {
		int bs = m_size.x;
		int block = m_position.y + (1-vn) * (m_size.y - bs);
		if(p.y>block && p.y<block+bs) m_held = p.y-block;
	}
	setFocus();
	return 0;
}
uint Slider::mouseMove(Event& e, const Point& pos, const Point& last, int b) {
	if(b!=1) m_held=0;
	if(m_held>0) {
		float s;
		if(m_orientation==HORIZONTAL) {
			int bs = m_size.y;
			s = (float)(pos.x - m_position.x - m_held) / (m_size.x - bs);
			printf("%f\n", s);
		} else {
			int bs = m_size.x;
			s = 1.0 - (float)(pos.y - m_position.y - m_held) / (m_size.y - bs);
		}
		int last = m_value;
		setValue(m_min + s*(m_max-m_min));
		if(m_value==last) return 0;
		else return setEvent(e, m_value);
	} else return 0;
}

// Scrollbar //
uint Scrollbar::mouseEvent(Event& e, const Point& p, int b) {
	if(b!=1) return 0;
	int last = m_value;
	float vn = (float)(m_value-m_min) / (m_max-m_min); // Normalised value
	if(m_orientation==HORIZONTAL) {
		int block = m_position.x + m_size.y + vn * (m_size.x-2*m_size.y-m_blockSize);
		int s = m_size.y;
		if(p.x<m_position.x+s)               { setValue(m_value-1); m_held=-3; } // Left
		else if(p.x>m_position.x+m_size.x-s) { setValue(m_value+1); m_held=-2; } // Right
		else if(p.x>block && p.x<block+m_blockSize) {  // Block
			m_held = p.x-block;
			setFocus();
		}
	} else { // Vertical
		int block = m_position.y + m_size.x + (1-vn) * (m_size.y-2*m_size.x-m_blockSize);
		int s = m_size.x;
		if(p.y<m_position.y+s)               { setValue(m_value+1); m_held=-3; } // Down
		else if(p.y>m_position.y+m_size.y-s) { setValue(m_value-1); m_held=-2; } // Up
		else if(p.y>block && p.y<block+m_blockSize) {  // Block
			m_held = p.y-block;
			setFocus();
		}
	}
	// Return event if value has changed
	if(last==m_value) return 0;
	else return setEvent(e, m_value);
}
uint Scrollbar::mouseMove(Event& e, const Point& pos, const Point& last, int b) {
	if(b!=1) m_held=0;
	if(m_held>0) {
		float s;
		if(m_orientation==HORIZONTAL) {
			s = (pos.x-m_held-m_position.x-m_size.y+m_blockSize) / (m_size.x-2.0*m_size.y);
		} else {
			s = 1.0 - (pos.y-m_held-m_position.y-m_size.x) / (m_size.y-2.0*m_size.x);
		}
		int last = m_value;
		setValue(m_min + s*(m_max-m_min));
		if(m_value==last) return 0;
		else return setEvent(e, m_value);
	} else return 0;
}

// Listbox //
uint ListBase::mouseEvent(Event& e, const Point& p, int b) {
	if(b==1) setFocus();
	// Pass event to scrollbar
	if(m_scroll.visible() && p.x>=m_scroll.getAbsolutePosition().x) {
		m_scroll.mouseEvent(e,p,b);
		return 0;
	}
	// Item position
	Point itm = m_position;
	itm.y += m_size.y - m_hover*m_itemHeight + m_scroll.getValue();
	// Click
	if(b==1 && m_hover<size() && mouseEvent(e, m_hover, p-itm, b)) {
		m_selected = m_hover;
		return e.id;
	} else return 0;
}
bool ListBase::mouseEvent(Event& e, uint index, const Point&, int b) {
	if(index!=m_selected) return m_command? setEvent(e, index): true;
	else return false;
}
uint ListBase::mouseMove(Event& e, const Point& pos, const Point& last, int b) {
	if(m_scroll.visible() && m_scroll.mouseMove(e,pos,last,b)) return 0; // Pass event to scrollbar
	if(pos.x > m_scroll.getAbsolutePosition().x) return 0; // Exit if mouse over scrollbar
	if(m_itemHeight && isOver(pos.x, pos.y)) {
		m_hover = (m_position.y + m_size.y - pos.y + m_scroll.getValue()) / m_itemHeight;
	} else m_hover = size();
	return 0;
}
uint ListBase::mouseWheel(Event& e, const Point& p, int w) {
	if(m_scroll.visible()) m_scroll.setValue( m_scroll.getValue() - w * (m_itemHeight/3) );
	return 0;
}

// DropList //
uint DropList::mouseEvent(Event& e, const Point& p, int b) {
	if(b==1) {
		setFocus();
		if(m_open) {
			Listbox::mouseEvent(e,p,b);
			closeList();
			return setEvent(e, m_selected, selectedItem());
		} else if(!m_open) {
			raise();
			openList();
		}
	}
	return 0;
}

// Input //
uint gui::Input::mouseEvent(Event& e, const Point& p, int b) {
	if(b==1) {
		setFocus();
		int size = 0;
		char tc[2] = "X";
		for(m_loc=0; m_loc<m_len; ++m_loc) {
			tc[0] = m_text[m_loc];
			size += textSize(tc).x;
			if(m_position.x + size > p.x) break;
		}
	}
	return 0;
}
uint gui::Input::keyEvent(Event& e, uint key, uint16 chr) {
	// Enter -  send event
	if(key==KEY_ENTER) {
		m_loc = m_len;
		return setEvent(e, 0, m_text);
	}
	// Control keys
	if(key==KEY_LEFT && m_loc>0) --m_loc;
	else if(key==KEY_RIGHT && m_text[m_loc]) ++m_loc;
	// Delete keys
	else if(key==KEY_BACKSPACE && m_loc>0) {
		--m_loc; --m_len;
		for(int i=m_loc; m_text[i]; ++i) m_text[i] = m_text[i+1];
		return setEvent(e, 1, m_text);
	} else if(key==KEY_DEL && m_text[m_loc]) {
		--m_len;
		for(int i=m_loc; m_text[i]; ++i) m_text[i] = m_text[i+1];
		return setEvent(e, 1, m_text);
	// Character
	} else if(chr>=32) {
		for(int i=m_len; i>=m_loc; --i) m_text[i+1]=m_text[i];
		m_text[m_loc] = chr;
		++m_loc; ++m_len;
		return setEvent(e,1,m_text);
	}
	return 0;
}



//// //// //// //// //// //// //// ////   DRAWING   //// //// //// //// //// //// //// //// 

void guiDrawArray(int mode, int size, const float* array, const ubyte* index=0) {
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, array);
	if(index) glDrawElements(mode, size, GL_UNSIGNED_BYTE, index);
	else glDrawArrays(mode, 0, size);
	glDisableClientState(GL_VERTEX_ARRAY);
}
void guiDrawArrayTex(int mode, int size, const float* array, const ubyte* index=0) {
	int stride = 4*sizeof(float);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glVertexPointer(2, GL_FLOAT, stride, array);
	glTexCoordPointer(2, GL_FLOAT, stride, array+2);
	if(index) glDrawElements(mode, size, GL_UNSIGNED_BYTE, index);
	else glDrawArrays(mode, 0, size);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

Sprite::Sprite(const Texture& t, int rows, int cols) : m_rows(rows), m_cols(cols) {
	m_texture = t.getGLTexture();
	m_width = t.width() / cols;
	m_height = t.height() / rows;
	m_one = vec2(1.0 / t.width(), 1.0/t.height());
	setBorder(0);
}
void Sprite::setBorder(int size) { setBorder(size, size, size, size); }
void Sprite::setBorder(int top, int left, int right, int bottom) {
	m_hasBorder = top || left || right || bottom;
	m_border.top = top;
	m_border.left = left;
	m_border.right = right;
	m_border.bottom = bottom;
}
void Sprite::drawGlyph(int frame, int cx, int cy) const {
	draw(frame, cx-m_width/2, cy-m_height/2, m_width, m_height, false);
}
void Sprite::draw(int frame, int x, int y, int w, int h, bool border) const {
	// Draw with no texture
	if(m_rows==0||m_cols==0) {
		float f[8] = { x,y, x,y+h, x+w,y+h, x+w,y };
		glDisable(GL_TEXTURE_2D);
		guiDrawArray(GL_QUADS, 4, f);
		glEnable(GL_TEXTURE_2D);
	} else { // Draw with texture
		float tx = (float)(frame%m_cols)/m_cols;
		float ty = floor(frame/m_cols)/m_rows;
		float tw = m_one.x * m_width;
		float th = m_one.y * m_height;
		if(border && m_hasBorder) { // Bordered version
			float fx[4] = {x, x+m_border.left, x+w-m_border.right,  x+w };
			float fy[4] = {y, y+m_border.top,  y+h-m_border.bottom, y+h };
			float ftx[4] = {tx, tx+m_border.left*m_one.x, tx+tw-m_border.right*m_one.x,   tx+tw };
			float fty[4] = {ty, ty+m_border.top*m_one.y,   ty+th-m_border.bottom*m_one.y, th+th };
			static float f[64];
			//static ubyte ix[32] = { 0,4, 1,5, 2,6, 3,7,   7,7,4,4,  4,8, 5,9, 6,10, 7,11,  11,11,8,8  8,12, 9,13, 10,14, 11,15 };
			static ubyte ix[36] = { 0,4,5,1, 1,5,6,2, 2,6,7,3,  4,8,9,5, 5,9,10,6, 6,10,11,7,  8,12,13,9, 9,13,14,10, 10,14,15,11 };
			for(int py=0; py<4; ++py) for(int px=0;px<4; ++px) {
				int k = px*4+py*16;
				f[k+0] = fx[px];
				f[k+1] = fy[py];
				f[k+2] = ftx[px];
				f[k+3] = fty[py];
			}
			glBindTexture(GL_TEXTURE_2D, m_texture);
			guiDrawArrayTex(GL_QUADS, 36, f, ix);
		} else {  // Simple version without border
			float f[16] = { x,y,tx,ty+th,  x+w,y,tx+tw,ty+th,  x+w,y+h,tx+tw,ty,  x,y+h,tx,ty };
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, m_texture);
			guiDrawArrayTex(GL_QUADS, 4, f);
		}
	}
}





//// //// General Draw Functions //// ////
void Control::drawFrame(const Point& pt, const Point& sz, const char* title, int state) const {
	// Background
	const Colour& bg = m_style->getColour(Style::BACK, state);
	if(bg.a>0) {
		if(m_style->m_sprite.frames()) {
			glColor4fv(bg);
			int frame = m_style->getFrame(state);
			m_style->m_sprite.draw(frame, pt.x, pt.y, sz.x, sz.y);
		} else drawRect(pt.x, pt.y, sz.x, sz.y, bg, true);
	}
	// Border (with optional title)
	const Colour& fg = m_style->getColour(Style::BORDER, state);
	if(fg.a>0) {
		if(title && m_style->m_font) {
			// Text size
			float tw = m_style->m_font->textWidth(title) + 16;
			float th = sz.y - m_style->m_font->textHeight(title);
			// Border
			float points[16] = { pt.x+1,pt.y+1,  pt.x+sz.x,pt.y+1,  pt.x+sz.x,pt.y+sz.y,  pt.x+1,pt.y+sz.y, /*title*/  pt.x+1,pt.y+th,  pt.x+tw,pt.y+th,  pt.x+tw+10,pt.y+th+10,  pt.x+tw+10,pt.y+sz.y };
			static const ubyte ix[9] = { 0,1,2,3,0,4,5,6,7 }; // For lines (GL_LINE_STRIP)
			glColor4fv(fg);
			glDisable(GL_TEXTURE_2D);
			guiDrawArray(GL_LINE_STRIP, 9, points, ix);
			glEnable(GL_TEXTURE_2D);
			// Frame caption
			glColor4fv( m_style->getColour(Style::TEXT, state) );
			m_style->m_font->print(pt.x+8, pt.y+th, title);
		} else drawRect(pt.x, pt.y, sz.x, sz.y, fg, false);
	}
}
void Control::drawArrow(const Point& p, int direction, int size, int state) const {
	//Non-texture arrow glyphs
	static float v[8] = { 0,1,  -1,0,  0,-1,  1,0 };
	static float a[6];
	for(int i=0; i<3; i++) {
		int k = (i+direction+1)*2 % 8;
		a[i*2+0] = p.x + v[k]*size;
		a[i*2+1] = p.y + v[k+1]*size;
	}
	// Draw it
	if(state) glColor4fv( m_style->getColour(Style::TEXT, state) );
	else glColor4fv(black);
	glDisable(GL_TEXTURE_2D);
	guiDrawArray(GL_TRIANGLES, 3, a);
	glEnable(GL_TEXTURE_2D);
}
void Control::drawRect(int x, int y, int w, int h, const Colour& c, bool fill) {
	if(c.a>0) {
		int o = fill? 0: 1; //offset to fix lines
		float f[8] = { x+o,y+o, x+w,y+o, x+w,y+h, x+o,y+h };
		glDisable(GL_TEXTURE_2D);
		glColor4fv(c);
		guiDrawArray(fill?GL_QUADS:GL_LINE_LOOP, 4, f);
		glEnable(GL_TEXTURE_2D);
	}
}
void Control::drawCircle(int x, int y, float r, const Colour& col, bool fill, int s) {
	static float* f=0;
	static int ls=0;
	if(col.a>0) {
		// Cache vertices
		if(ls!=s) {
			ls=s;
			if(f) delete [] f;
			f = new float[s*2];
			float a = TWOPI / s;
			for(int i=0; i<s; ++i) { f[i*2]=sin(i*a); f[i*2+1]=-cos(i*a); }
		}
		// Draw circle
		glColor4fv(col);
		glDisable(GL_TEXTURE_2D);
		glPushMatrix();
		glTranslatef(x, y, 0);
		glScalef(r,r,1);
		guiDrawArray(fill?GL_POLYGON:GL_LINE_LOOP, s, f);
		glPopMatrix();
		glEnable(GL_TEXTURE_2D);
	}
}
void Control::drawText(int x, int y, const char* text, int state) const {
	if(state) glColor4fv( m_style->getColour(Style::TEXT, state) );
	m_style->m_font->print(x, y, text);
}
void Control::drawText(const Point& pos, const char* text, int state) const {
	if(state) glColor4fv( m_style->getColour(Style::TEXT, state) );
	m_style->m_font->print(pos.x, pos.y, text);
}


//// //// Control Draw Functions //// ////
struct Scissor { int x, y, w, h; };
std::vector<Scissor> scissorStack;
void Control::scissorPushNew(int x, int y, int w, int h) {
	if(scissorStack.empty()) glEnable(GL_SCISSOR_TEST);
	Scissor s; s.x=x; s.y=y; s.w=w; s.h=h;
	glScissor(x, y, w, h);
	scissorStack.push_back(s);
}
void Control::scissorPush(int x, int y, int w, int h) {
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
void Control::scissorPop() {
	scissorStack.pop_back();
	if(scissorStack.empty()) glDisable(GL_SCISSOR_TEST);
	else glScissor(scissorStack.back().x, scissorStack.back().y, scissorStack.back().w, scissorStack.back().h);
}

//// //// //// //// Drawing //// //// //// ////

void Container::draw() {
	if(m_contents.empty()) return;
	if(m_clip) scissorPush(m_position.x, m_position.y, m_size.x, m_size.y);
	for(std::list<Control*>::iterator i=m_contents.begin(); i!=m_contents.end(); i++) {
		if((*i)->m_visible) (*i)->draw();
	}
	if(m_clip) scissorPop();
}
void Frame::draw() {
	drawFrame(m_position, m_size, m_caption);
	Container::draw();	
}
void Label::draw() {
	drawText(m_position, m_caption, Style::BASE);
}
void Button::draw() {
	int state = getState();
	drawFrame(m_position, m_size, 0, state);
	if(m_caption) drawText(m_position+m_textPos, m_caption, state);
}
void Checkbox::draw() {
	Point bpos(0, (m_size.y - m_boxSize) / 2);
	drawFrame(m_position+bpos, Point(m_size.y-6, m_size.y-6), 0, getState());
	if(m_caption) drawText(m_position.x+m_boxSize, m_position.y, m_caption, getState());
	// Draw tick
	if(m_value) {
		glDisable(GL_TEXTURE_2D);
		Point c = m_position + bpos + Point(m_boxSize/2, m_boxSize/2); // Box centre
		float s = m_boxSize/2 - 3;
		float v[8] = { c.x, c.y-s,  c.x+s,c.y,  c.x,c.y+s,  c.x-s,c.y };
		guiDrawArray(GL_QUADS, 4, v);
		glEnable(GL_TEXTURE_2D);
	}
}
void Slider::draw() {
	float v = m_max==m_min? 0: (m_value-m_min) / (m_max-m_min); // Normalised value
	if(m_orientation==HORIZONTAL) {
		int h = m_size.y/2;
		drawRect(m_position.x, m_position.y + h-2, m_size.x, 4, m_style->getColour(Style::BORDER), true);
		int b = h + v * (m_size.x-m_size.y);
		drawCircle(m_position.x+b, m_position.y+h, h, m_style->getColour(Style::BACK, getState()), true);
		drawCircle(m_position.x+b, m_position.y+h, h, m_style->getColour(Style::BORDER, getState()), false);
	} else {
		int h = m_size.x/2;
		drawRect(m_position.x+h-2, m_position.y, 4, m_size.y, m_style->getColour(Style::BORDER), true);
		int b = h + (1-v) * (m_size.y-m_size.x);
		drawCircle(m_position.x+h, m_position.y+b, h, m_style->getColour(Style::BACK, getState()), true);
		drawCircle(m_position.x+h, m_position.y+b, h, m_style->getColour(Style::BORDER, getState()), false);
	}
}
void Scrollbar::draw() { 
	int state = getState();
	//Draw track
	drawRect(m_position.x, m_position.y, m_size.x, m_size.y, m_style->getColour(Style::BACK), true);
	if(m_orientation==HORIZONTAL) {
		Point btnSize(m_size.y, m_size.y);
		Point btnHalf(m_size.y/2+3, m_size.y/2);
		Point btnPos(m_position + m_size - btnSize);
		//Buttons
		drawFrame(m_position, btnSize, 0, state);
		drawArrow(m_position + btnHalf, 3, btnHalf.y*0.6, state);
		drawFrame(btnPos, btnSize, 0, state);
		drawArrow(m_position + m_size - btnHalf, 1, btnHalf.y*0.6, state);
		//Slider
		int px = m_value * (m_size.x-2*m_size.y-m_blockSize) / (m_max-m_min);
		int sp = m_position.x + m_size.y + px;
		drawFrame(Point(sp, m_position.y), Point(m_blockSize, m_size.y), 0, state); 
	} else {
		Point btnSize(m_size.x, m_size.x);
		Point btnHalf(m_size.x/2, m_size.x/2+3);
		Point btnPos(m_position + m_size - btnSize);
		//Buttons
		drawFrame(m_position, btnSize, 0, state);
		drawArrow(m_position + btnHalf, 0, btnHalf.x*0.6, state);
		drawFrame(btnPos, btnSize, 0, state);
		drawArrow(m_position + m_size - btnHalf, 2, btnHalf.x*0.6, state);
		//Slider
		int sl = m_size.y - 2*m_size.x - m_blockSize;
		int py = m_value * sl / (m_max-m_min);
		int sp = m_position.y + m_size.x + sl - py;
		drawFrame(Point(m_position.x, sp), Point(m_size.x, m_blockSize), 0, state);
	}
}
void ListBase::draw() {
	//Box
	drawFrame(m_position, m_size, 0, getState());
	//Items (need to scissor this
	scissorPush(m_position.x+1, m_position.y+1, m_size.x-2, m_size.y-2);
	//Items
	uint ni = ceil(m_size.y / m_itemHeight) + 1;
	uint ix = floor(m_scroll.getValue() / m_itemHeight);
	int py = m_position.y + m_size.y - (ix+1) * m_itemHeight + m_scroll.getValue();
	int cellWidth = m_scroll.visible()? m_size.x-m_scroll.getSize().x: m_size.x;
	for(uint i=0; i<ni && ix+i<size(); i++) {
		drawItem(ix+i, m_position.x+1, py-1-i*m_itemHeight, cellWidth-2, m_itemHeight);
	}

	//Scrollbar
	if(m_scroll.visible()) m_scroll.draw();
	scissorPop();
}
void Listbox::drawItem(uint index, int x, int y, int width, int height) {
	int s = index==m_selected? Style::FOCUS: index==m_hover? Style::OVER: Style::BASE;
	//Background
	if(index==m_selected) {
		glDisable(GL_TEXTURE_2D);
		drawRect(x,y,width,height, Colour(1,1,1,0.1), true);
		glEnable(GL_TEXTURE_2D);
	}
	//Text
	drawText(x+2, y, getItem(index), s);
}

void DropList::draw() {
	if(m_open) {
		scissorPushNew(m_position.x-1, m_position.y-1, m_size.x+1, m_size.y+1); 
		Listbox::draw();
		scissorPop();
	} else {
		int state = getState();
		drawFrame(m_position, m_size, 0, state);
		if(selectedItem()) drawText(m_position.x+3, m_position.y, selectedItem(), state);
		drawArrow(m_position + Point(m_size.x-8, m_size.y/2+2), 0, 4, state);
	}
}

void gui::Input::draw() {
	//Box
	int state = getState();
	drawFrame(m_position, m_size, 0, state);
	if(m_len>0) {
		//Masked
		static char masked[128];
		if(m_mask) { for(int i=0; i<m_len; i++) masked[i]=m_mask; masked[m_len]=0; }
		if(m_text[0]) drawText(m_position.x+2, m_position.y, m_mask?masked:m_text, state);
		//Caret
		if(hasFocus()) {
			char* tmp = m_mask?masked:m_text;
			char t = tmp[m_loc]; tmp[m_loc]=0;
			int cp = textSize(tmp).x;
			tmp[m_loc]=t;
			//draw it
			drawText(m_position.x+cp+1, m_position.y, "_", state);
		}
	}
}

