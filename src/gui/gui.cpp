#include <base/gui/gui.h>
#include <base/gui/skin.h>
#include <base/gui/renderer.h>
#include <base/gui/widgets.h>
#include <base/gui/lists.h>
#include <base/gui/tree.h>
#include <base/gui/layouts.h>
#include <base/gui/font.h>
#include <cstdio>

using namespace gui;



/** Static definitions */
base::HashMap<Widget*(*)(const Rect&, Skin*)> Root::s_constuct;
base::HashMap<Layout*(*)(int,int)> Root::s_layouts;


Root::Root(int w, int h, Renderer* renderer) : m_focus(0), m_mouseFocus(0), m_keyMask(KeyMask::None), m_mouseState(0), m_wheelUsed(false), m_changed(false), m_renderer(renderer) {
	m_root = new Widget( Rect(0,0,w,h), 0 );
	m_root->setTangible(Tangible::CHILDREN);
	m_root->m_root = this;
	if(!m_renderer) setRenderer(new Renderer());

	// Register defaut widgets
	if(!s_constuct.contains("Widget")) {
		registerClass<Widget>();
		registerClass<Icon>();
		registerClass<Image>();
		registerClass<Label>();
		registerClass<Button>();
		registerClass<Checkbox>();
		registerClass<DragHandle>();
		registerClass<ProgressBar>();
		registerClass<Textbox>();
		registerClass<Spinbox>();
		registerClass<SpinboxFloat>();
		registerClass<Scrollbar>();
		registerClass<Scrollpane>();
		registerClass<TabbedPane>();
		registerClass<Window>();
		registerClass<Listbox>();
		registerClass<Combobox>();
		registerClass<TreeView>();
		registerClass<Table>();
		registerClass<SplitPane>();
		registerClass<CollapsePane>();
		registerClass<Popup>();
		registerClass<ScaleBox>();
		registerClass<Slider>();

		registerLayout<HorizontalLayout>();
		registerLayout<VerticalLayout>();
		registerLayout<FlowLayout>();
		registerLayout<FixedGridLayout>();
		registerLayout<DynamicGridLayout>();
	}
}

Root::~Root() {
	setRenderer(0);
	delete m_root;
	for(auto& i: m_fonts) delete i.value;
	for(auto& i: m_skins) delete i.value;
	for(auto& i: m_iconLists) delete i.value;
	for(auto& i: m_templates) delete i.value;
}

void Root::setRenderer(Renderer* r) {
	if(m_renderer && --m_renderer->m_references==0) delete m_renderer;
	if(r) ++r->m_references;
	m_renderer = r;
}

Widget* Root::createWidget(const char* skin, const char* type, const char* name, bool forceType) const {
	Widget* w;
	if(m_templates.contains(skin)) {
		Widget* src = m_templates[skin];
		w = src->clone(forceType? type: 0);
		
		if(forceType && !w->isType(src->getType())) {
			printf("Warning: Creating %s widget as %s\n", src->getTypeName(), type);
		}
	}
	else if(m_skins.contains(skin) && s_constuct.contains(type)) {
		w = s_constuct[ type ] (Rect(0,0,100,100), m_skins[skin]);
		w->initialise(this, PropertyMap());
	}
	else {
		printf("Error: widget template %s not found\n", skin);
		return 0;
	}
	w->m_name = name;
	w->m_states &= ~0x20;	// Not template
	return w;
}

Layout* Root::createLayout(const char* name, int m, int s) {
	auto it = s_layouts.find(name);
	if(it!=s_layouts.end()) return it->value(m, s);
	else return 0;
}

void setTemplateFlag(Widget* w) {
	w->setAsTemplate();
	for(int i=0; i<w->getWidgetCount(); ++i) {
		setTemplateFlag(w->getWidget(i));
	}
}
void Root::createTemplate(const char* name, Widget* w, const char* type) {
	addTemplate(name, w->clone(type));
}
void Root::addTemplate(const char* name, Widget* w) {
	// Recursively set template flag
	setTemplateFlag(w);
	w->m_skipTemplate = w->m_children.size();
	w->m_client->m_skipTemplate = w->m_client->m_children.size();
	Widget* old = m_templates.get(name, 0);
	m_templates[name] = w;
	delete old;
}
const Widget* Root::getTemplate(const char* name) const {
	return findInMap(m_templates, name);
}

void Root::resize(int w, int h) {
	m_root->setSize(w, h);
}

void Root::mouseEvent(const Point& p, int b, int w) {
	int mdown = b&~m_mouseState;
	int mup   = m_mouseState&~b;
	Point last = m_mousePos;

	// Remember state
	m_mousePos = p;
	m_mouseState = b;

	// Mouse moved
	bool moved = p != last;
	if(moved || m_changed) {
		if(moved && m_mouseFocus && m_mouseFocus->isEnabled() && m_mouseFocus->isParentEnabled()) {
			Point localLast = m_mouseFocus->m_derivedTransform.untransform(last);
			Point localPos = m_mouseFocus->m_derivedTransform.untransform(p);
			m_mouseFocus->onMouseMove(localLast, localPos, b);
		}
		Widget* over = m_root->getWidget(p, false, true);
		// Has it changed?
		if(over != m_mouseFocus && !b) {
			// additional mouse event here as changing focus misses it later
			if(m_mouseFocus && mup && m_mouseFocus->isEnabled() && m_mouseFocus->isParentEnabled()) {
				m_mouseFocus->onMouseButton(m_mouseFocus->m_derivedTransform.untransform(p), 0, mup);
			}
			if(over) {
				const Transform& t = over->m_derivedTransform;
				over->setMouseFocus();
				over->onMouseMove(t.untransform(last), t.untransform(p), b);
			}
			else if(m_mouseFocus) {
				m_mouseFocus->onMouseExit();
				m_mouseFocus = 0;
			}
		}
	}

	
	// Change focus
	if(mdown && m_focus!=m_mouseFocus) {
		if(m_mouseFocus) {
			m_mouseFocus->setFocus();
		}
		else if(m_focus) {
			Widget* last = m_focus;
			m_focus = 0;
			last->onLoseFocus();
			if(last->eventLostFocus) last->eventLostFocus(last);
		}
	}

	// Mouse wheel
	if(w) {
		Widget* focus = m_mouseFocus;
		while(focus && focus->isEnabled() && focus->isParentEnabled() && !focus->onMouseWheel(w)) focus = focus->m_parent;
		m_wheelUsed = focus!=0;
	}

	// Mouse event
	if(m_mouseFocus && m_mouseFocus->isEnabled() && m_mouseFocus->isParentEnabled() && (mdown || mup))  {
		if(Widget* p =m_mouseFocus->getParent(true)) p->onChildMouseDown(m_mouseFocus, mdown);
		m_mouseFocus->onMouseButton(m_mouseFocus->m_derivedTransform.untransform(p), mdown, mup);
	}
	
	// Mouseup may change mouse focus if outside widget
	if(mup && !b && m_mouseFocus && !m_mouseFocus->m_rect.contains(p)) {
		Widget* over = m_root->getWidget(p, false, true);
		if(over) over->setMouseFocus();
		else m_mouseFocus->onMouseExit(), m_mouseFocus=0;
	}
}

void Root::keyEvent(int code, wchar_t chr) {
	if(m_focus && m_focus->isEnabled() && m_focus->isParentEnabled()) {
		Widget* focus = m_focus;
		focus->onKey(code, chr, m_keyMask);	// Note: Can potentialy delete this widget
		if(focus == m_focus && focus->eventKeyPress) focus->eventKeyPress(m_focus, code, chr, m_keyMask);
	}
}

void Root::setKeyMask(KeyMask mask) {
	m_keyMask = mask;
}

void Root::setKeyMask(bool ctrl, bool shift, bool alt, bool meta) {
	m_keyMask = (KeyMask)((ctrl?1:0) | (shift?2:0) | (alt?4:0) | (meta?8:0));
}

void Root::update() {
	// Anything actually need updating?
}

void Root::draw(const Point& viewport) const {
	const Point& view = viewport.x? viewport: m_root->m_rect.size();
	getRenderer()->begin(m_root->m_rect.size(), view);
	m_root->draw();
	getRenderer()->end();
}


// ------------- //
template<class T> Root::StringList getMapKeys( const base::HashMap<T>& map) {
	Root::StringList list;
	for(typename base::HashMap<T>::const_iterator i=map.begin(); i!=map.end(); ++i) {
		if(i->value) list.push_back(i->key);
	}
	return list;
}


Root::StringList Root::getFonts() const { return getMapKeys(m_fonts); }
Root::StringList Root::getSkins() const { return getMapKeys(m_skins); }
Root::StringList Root::getIconLists() const { return getMapKeys(m_iconLists); }
Root::StringList Root::getTemplates() const { return getMapKeys(m_templates); }
Root::StringList Root::getWidgetTypes() const { return getMapKeys(s_constuct); }


// ==================================================================================== //

Skin::Skin(int s) : m_states(0), m_stateCount(s), m_definedStates(0), m_font(0), m_fontSize(0), m_fontAlign(ALIGN_TOP|ALIGN_LEFT), m_image(-1) {
	if(s) m_states = new State[s];
}
Skin::~Skin() {
	delete [] m_states;
}
Skin::Skin(const Skin& s) : m_states(0), m_stateCount(s.m_stateCount), m_definedStates(s.m_definedStates),
	m_font(s.m_font), m_fontSize(s.m_fontSize), m_fontAlign(s.m_fontAlign), m_image(s.m_image) {
	m_states = new State[m_stateCount];
	memcpy(m_states, s.m_states, m_stateCount * sizeof(State));
}
void Skin::setStateCount(int count) {
	if(count==0) { count=1; m_definedStates=0; }
	else if(count!=1 && count!=4 && count!=8) { printf("Error: count %d\n", count); throw 1; }
	if(count > m_stateCount) {
		State* old = m_states;
		m_states = new State[count];
		if(old) memcpy(m_states, old, m_stateCount*sizeof(State));
		if(old) delete [] old;
	}
	else {
		m_definedStates &= ~(0xff << count);
	}
	m_stateCount = count;
}
void Skin::setState(int state, const Rect& r, const Border& b, unsigned fc, unsigned bc, const Point& o) {
	// Resize state count
	int count = state>3? 8: state>0? 4: 1;
	if(count > m_stateCount) setStateCount(count);
	// Set state
	m_states[state].rect = r;
	m_states[state].border = b;
	m_states[state].foreColour = fc;
	m_states[state].backColour = bc;
	m_states[state].textPos = o;
	m_definedStates |= 1<<state;
	// Ensure all states are set
	updateUndefinedStates();
}
void Skin::updateUndefinedStates() {
	auto copyState = [this](int state, int from) { memcpy(m_states+state, m_states+from, sizeof(State)); };
	if(~m_definedStates&1) {
		for(int i=1; i<m_stateCount; ++i) if(m_definedStates&1<<i) {
			copyState(0, i);
			break;
		}
	}
	int from[] = { 0, 0, 1, 0, 0, 4, 5, 3 };
	for(int i=0; i<m_stateCount; ++i) {
		if(~m_definedStates & 1<<i) copyState(i, from[i]);
	}
}
void Skin::setState(int index, const State& s) {
	setState(index, s.rect, s.border, s.foreColour, s.backColour, s.textPos);
}


// ==================================================================================== //

Widget* Widget::clone(const char* newType) const {
	if(!newType) newType = getTypeName();
	else if(!Root::s_constuct.contains(newType)) {
		printf("Error: Widget type %s not registered\n", newType);
		newType = getTypeName();
	}
	Widget* w = Root::s_constuct[ newType ](m_rect, m_skin);
	w->m_anchor = m_anchor;
	w->m_layout = m_layout;
	if(m_layout) ++m_layout->ref;
	if(m_relative) {
		w->m_relative = new float[4];
		memcpy(w->m_relative, m_relative, 4*sizeof(float));
	}
	w->m_states = m_states;
	w->m_colour = m_colour;
	w->m_parent = 0;
	w->m_root = 0;
	w->m_skipTemplate = m_skipTemplate;
	w->m_name = m_name;
	w->m_tip = m_tip;
	// children
	w->pauseLayout();
	for(uint i=0; i<m_children.size(); ++i) {
		Widget* c = m_children[i]->clone();
		w->m_children.push_back(c);
		c->m_parent = w;
	}

	w->copyData(this);
	w->initialise(m_root, PropertyMap()); // Note: m_root will nearly always be null
	w->resumeLayout();
	return w;
}

// ==================================================================================== //

Widget::Widget(const Rect& r, Skin* s): m_rect(r), m_skin(s), m_colour(-1), m_anchor(0), m_layout(0), m_relative(0),
		m_states(0xf), m_skipTemplate(0), m_parent(0), m_root(0) {
	m_client = this;
}

Widget::~Widget() {
	pauseLayout();
	if(m_relative) delete [] m_relative;
	if(m_parent && m_parent->getRoot()) removeFromParent();
	else if(m_root) setRoot(nullptr);
	for(Widget* w: m_children) delete w;
	if(m_layout && --m_layout->ref<=0) delete m_layout;
}

void Widget::initialise(const Root*, const PropertyMap&) {
	m_client = getTemplateWidget("_client");
	if(!m_client) m_client = this;
}

Rect Widget::getAbsoluteRect() const {
	if(m_parent) return m_parent->m_derivedTransform.transform(m_rect);
	else return m_rect;
}

Rect Widget::getClientRect() const {
	Rect r = m_client->m_rect;
	if(m_client != this) {
		for(Widget* w = m_client->m_parent; w && w!=this; w=w->m_parent) r.position() += w->m_rect.position();
	}
	return r;
}

const Point& Widget::getPosition() const {
	return m_rect.position();	
}

const Point& Widget::getSize() const {
	return m_rect.size();
}

void Widget::updateChildTransforms() {
	for(Widget* w: m_children) w->updateTransforms();
}
void Widget::updateTransforms() {
	m_derivedTransform = m_parent->m_derivedTransform;
	m_derivedTransform.translate(m_rect.x, m_rect.y);
	updateChildTransforms();
}
void Widget::shiftTransforms(float x, float y) {
	m_derivedTransform[4] += x;
	m_derivedTransform[5] += y;
	for(Widget* w: m_children) w->shiftTransforms(x, y);
}

void Widget::setPosition(int x, int y) {
	if(x == m_rect.x && y == m_rect.y) return; // No change
	
	bool layoutPaused = isLayoutPaused();
	pauseLayout();

	float sx = m_derivedTransform[4], sy = m_derivedTransform[5];
	m_derivedTransform = m_parent? m_parent->m_derivedTransform: Transform();
	m_derivedTransform.translate(x, y);
	sx = m_derivedTransform[4] - sx;
	sy = m_derivedTransform[5] - sy;
	for(Widget* w: m_children) w->shiftTransforms(sx, sy);

	if(!layoutPaused) resumeLayout(false);

	m_rect.x = x;
	m_rect.y = y;
	if(Widget* parent = getParent()) parent->onChildChanged(this);
	notifyChange();
}
void Widget::setSize(int w, int h) {
	if(w==m_rect.width && h==m_rect.height) return;

	bool layoutPaused = isLayoutPaused();
	pauseLayout();

	// Update children positions and sizes
	if(!m_layout) for(Widget* c : m_children) {
		if(c->m_relative) {
			float* r = c->m_relative;
			c->setPosition(r[0] * w, r[1] * h);
			c->setSize(r[2] * w, r[3] * h);
		}
		else if(c->m_anchor) {
			char a = c->m_anchor;
			Rect r = c->m_rect;
			// x axis
			switch(a&0xf) {
			case 0: break; // Left
			case 1: r.x += w - m_rect.width; break; // Right
			case 2: r.x += w/2 - m_rect.width/2; break; // Centre
			case 3: r.width += w - m_rect.width; break; // Both
			}
			// y axis
			switch((a>>4)&0xf) {
			case 0: break; // Top
			case 1: r.y += h-m_rect.height; break; // Bottom
			case 2: r.y += h/2 - m_rect.height/2; break; // Centre
			case 3: r.height += h - m_rect.height; break; // Both
			}
			c->setPosition(r.x, r.y);
			c->setSize(r.width, r.height);
		}
	}
	m_rect.width = w;
	m_rect.height = h;
	if(!layoutPaused) resumeLayout(true);
	if(Widget* parent = getParent()) parent->onChildChanged(this);
	notifyChange();
	if(eventResized) eventResized(this);
}

void Widget::setSizeAnchored(const Point& s) {
	Point size = s;
	Point pos = getPosition();
	switch(m_anchor&0xf) {
	case 0: break; /// Left
	case 1: pos.x -= (s.x - m_rect.width); break;	  // Right
	case 2: pos.x -= (s.x - m_rect.width) / 2; break; // Centre
	case 3: size.x = m_rect.width; break; // span - no change
	}

	switch((m_anchor>>4) & 0xf) {
	case 0: break; // Top
	case 1: pos.y -= s.y - m_rect.height; break;	// Bottom
	case 2: pos.y -= (s.y - m_rect.height) / 2; break; // Centre
	case 3: size.y = m_rect.height; break;	// Span - no change
	}
	setPosition(pos);
	setSize(size);
	
	// Autosize fix for spanned items. Above setSize may not refresh parent in this case.
	if(m_parent && m_parent->isAutosize() && (size.x!=s.x || size.y!=s.y) && size==getSize()) {
		m_parent->refreshLayout();
	}
}

Point Widget::getPreferredSize(const Point& hint) const { // Minimum size
	if(isAutosize()) {
		Point newSize(0, 0);
		const Point& clientSize = m_client->getSize();
		if(m_client->m_layout) {
			newSize = m_client->m_layout->getMinimumSize(m_client, hint);
		}
		else {
			auto setMax = [](int& value, int other) { if(other>value) value=other; };
			for(Widget* w: m_client->m_children) {
				if(w->isVisible()) {
					const Rect r(w->getPosition(), w->getPreferredSize());
					switch(w->m_anchor&0xf) {
					case 0: setMax(newSize.x, r.right()); break; // Left
					case 1: setMax(newSize.x, clientSize.x - r.x); break;	// Right
					case 2: setMax(newSize.x, (r.width/2 + abs(r.x+r.width/2 - clientSize.x/2)) * 2); break; // Centre
					case 3: setMax(newSize.x, r.x + clientSize.x - w->m_rect.right() + (w->isAutosize()? r.width:0)); break; // Span
					}
					switch(w->m_anchor>>4) {
					case 0: setMax(newSize.y, r.bottom()); break; // Top
					case 1: setMax(newSize.y, clientSize.y - r.y); break;	// Bottom
					case 2: setMax(newSize.y, (r.height/2 + abs(r.y+r.height/2 - clientSize.y/2)) * 2); break; // Centre
					case 3: setMax(newSize.y, r.y + clientSize.y - w->m_rect.bottom() + (w->isAutosize()? r.height: 0)); break; // Span
					}
				}
			}
		}
		return newSize + getSize() - clientSize;
	}
	return getSize();
}

void Widget::updateAutosize() {
	if(!isAutosize()) return;
	if(m_anchor == 0x33) return; // autosize messes up span anchors
	if(m_client != this && m_client->m_anchor != 0x33) return; // client must resize with widget for autosize
	Point newSize = getPreferredSize();
	if((m_anchor&0xf)==3) newSize.x = getSize().x; // Don't autosize spanned anchors
	if((m_anchor>>4)==3) newSize.y = getSize().y;
	assert(newSize.x<5000 && newSize.y<5000);
	setSizeAnchored(newSize);
}

void Widget::setPositionFloat(float x, float y, float w, float h) {
	if(!m_relative) m_relative = new float[4];
	m_relative[0] = x;
	m_relative[1] = y;
	m_relative[2] = w;
	m_relative[3] = h;
	if(m_parent) {
		float* r = m_relative;
		Point s = m_parent->getSize();
		setPosition(r[0] * s.x, r[1] * s.y);
		setSize(r[2] * s.x, r[3] * s.y);
	}
}

void Widget::useRelativePositioning(bool rel) {
	if(rel && !m_relative) {
		m_relative = new float[4];
		updateRelativeFromRect();
	}
	else if(!rel) {
		delete [] m_relative;
		m_relative = 0;
	}
}

void Widget::updateRelativeFromRect() {
	if(m_parent) {
		Point p = getPosition();
		Point s = m_parent->getSize();
		float w = s.x, h = s.y;
		m_relative[0] = p.x / w;
		m_relative[1] = p.y / h;
		m_relative[2] = m_rect.width / w;
		m_relative[3] = m_rect.height / h;
	}
	else m_relative[0] = m_relative[1] = m_relative[2] = m_relative[3] = 0;
}

bool Widget::isRelative() const {
	return m_relative;
}

void Widget::notifyChange() {
	if(m_root) m_root->m_changed = true;
}

void Widget::setVisible(bool v) { 
	if(v == isVisible()) return;
	m_states = v? m_states|1: m_states&~1;
	notifyChange();
	if(Widget* p = getParent()) p->onChildChanged(this);
	// Clear focus if hidden
	if(!v && m_root) {
		for(Widget* w=m_root->m_focus; w; w=w->m_parent) if(!w->isVisible()) { m_root->m_root->setFocus(); break; }
	}
}

void Widget::setEnabled(bool v) { m_states = v? m_states|2: m_states&~2; }
void Widget::setTangible(Tangible t) { m_states = (m_states&~0xc) | ((int)t << 2); }
void Widget::setSelected(bool v) { m_states = v? m_states|0x10: m_states&~0x10; }
void Widget::setInheritState(bool v) { m_states = v? m_states|0x40: m_states&~0x40; }
void Widget::setAsTemplate() { m_states |= 0x20; }
void Widget::setAutosize(bool v) {
	m_states = v? m_states|0x80: m_states&~0x80;
	if(v && !isLayoutPaused()) {
		updateAutosize();
		if(getAnchor()==0x33 && getParent()) getParent()->onChildChanged(this);
	}
}

Tangible Widget::getTangible() const { return (Tangible)(m_states>>2 & 3); }
bool Widget::isTangible() const {
	if(~m_states&4) return false; // Not SELF
	for(Widget* p=m_parent; p; p=p->m_parent) {
		if(~p->m_states&8) return false; // CHILDREN
	}
	return true;
}

bool Widget::isVisible() const { return m_states&1; }
bool Widget::isEnabled() const { return m_states&2; }
bool Widget::isSelected() const { return m_states&0x10; }
bool Widget::isTemplate() const { return m_states&0x20; }
bool Widget::isParentEnabled() const { return !m_parent || (m_parent->isEnabled() && m_parent->isParentEnabled()); }
bool Widget::isAutosize() const { return m_states&0x80; }

void Widget::setAnchor(int a) {
	m_anchor = a;
	if(!isLayoutPaused()) updateAutosize();
	if(m_parent) m_parent->onChildChanged(this);
}
void Widget::setAnchor(const char* anchor) {
	m_anchor = 0;
	if(anchor) {
		int a[2] = {0, 0};
		for(const char* c = anchor; *c; ++c) {
			switch(*c) {
			case 'l': a[0] |= 0x1; break;
			case 'c': a[0] |= 0x2; break;
			case 'r': a[0] |= 0x4; break;
			case 't': a[1] |= 0x1; break;
			case 'm': a[1] |= 0x2; break;
			case 'b': a[1] |= 0x4; break;
			default: printf("Invalid anchor %s\n", anchor); return;
			}
		}
		for(int s: {0,1}) {
			int part = 0;
			switch(a[s]) {
			case 0: case 1: break;
			case 2: part = 2; break; // Centre
			case 4: part = 1; break; // Right
			case 5: part = 3; break; // span
			default: printf("Invalid anchor %s\n", anchor); return;
			}
			m_anchor |= part << (s*4);
		}
	}
	if(!isLayoutPaused()) updateAutosize();
	if(Widget* parent = getParent()) parent->onChildChanged(this);
}
int Widget::getAnchor(char* s) const {
	if(s) {
		const char* code = "lrcltbmt";
		*(s++) = code[m_anchor&0x0f];
		if((m_anchor&0x0f) == 3) *(s++) = code[1];
		*(s++) = code[(m_anchor>>4) + 4];
		if((m_anchor&0xf0) == 0x30) *(s++) = code[5];
		*s = 0;
	}
	return m_anchor;
}

void Widget::raise() {
	// Move to the end of parents widget list
	if(m_parent && !m_parent->m_children.empty()) {
		std::vector<Widget*>& list = m_parent->m_children;
		for(uint i=0; i<list.size(); ++i){
			if(list[i]==this) {
				list.erase(list.begin()+i);
				break;
			}
		}
		list.push_back(this);
		m_parent->refreshLayout();
	}
}
void Widget::setFocus() {
	if(m_root && m_root->m_focus != this) {
		Widget* last = m_root->m_focus;
		m_root->m_focus = this;
		if(last) {
			last->onLoseFocus();
			if(last->eventLostFocus) last->eventLostFocus(last);
		}
		onGainFocus();
		if(eventGainedFocus) eventGainedFocus(this);
	}
}
bool Widget::hasFocus() const {
	if(!m_root) return false;
	if(m_root->m_focus==this) return true;
	// Template children
	for(uint i=0; i<m_children.size(); ++i) {
		if((m_children[i]->isTemplate()) && m_children[i]->hasFocus()) return true;
	}
	return false;
}
void Widget::setMouseFocus() {
	if(m_root) {
		if(m_root->m_mouseFocus) m_root->m_mouseFocus->onMouseExit();
		m_root->m_mouseFocus = this;
		onMouseEnter();
	}
}
bool Widget::hasMouseFocus() const {
	if(!m_root) return false;
	if(m_root->m_mouseFocus==this) return true;
	// Template children
	for(uint i=0; i<m_children.size(); ++i) {
		if((m_children[i]->isTemplate()) && m_children[i]->hasMouseFocus()) return true;
	}
	return false;
}

Widget* Widget::getWidget(size_t index) const {
	index += m_client->m_skipTemplate;
	return index<m_client->m_children.size()? m_client->m_children[index]: 0;
}
Widget* Widget::getWidget(const Point& p, int typeMask, bool tg, bool tm, bool clip) {
	if(clip && !contains(p)) return nullptr;
	if(m_children.empty()) return nullptr;
	Widget* client = tm? this: m_client;
	Point clientOffset(0,0);
	for(Widget* w = client; w!=this; w=w->m_parent) clientOffset += w->m_rect.position();
	for(int i=client->m_children.size()-1; i>=0; --i) {
		Widget* child = client->m_children[i];
		if(child->isVisible() && (tg || child->getTangible()!=Tangible::NONE)) {
			Point local = p - clientOffset - child->m_rect.position();
			if(child->contains(local)) {
				if(tg || (child->m_states&8)) {
					Widget* w = child->getWidget(local, typeMask, tg, tm, false);
					if(w) return w;
				}

				// Filtering
				if(child->isTemplate() && !tm) continue;
				if(!(child->m_states&4) && !tg) continue;
				if(typeMask!=Widget::staticType() && !child->isType(typeMask)) continue;
				return child;
			}
		}
	}
	return nullptr;
}

Widget* Widget::findChildWidget(const char* name) const {
	// Breadth first search
	std::vector<const Widget*> queue;
	queue.push_back(this);
	for(size_t i=0; i<queue.size(); ++i) {
		for(Widget* c : *queue[i]) {
			if(name == c->m_name) return c;
			queue.push_back(c);
		}
	}
	return 0;
}

Widget* Widget::findTemplateWidget(const char* name) const {
	// Breadth first search
	std::vector<const Widget*> queue;
	queue.push_back(this);
	for(size_t i=0; i<queue.size(); ++i) {
		for(Widget* w: queue[i]->m_children) {
			if(w->isTemplate()) {
				if(name == w->m_name) return w;
				queue.push_back(w->m_client);
			}
			else break;
		}
	}
	return 0;
}

void Widget::setName(const char* name) {
	if(!name) name = "";
	if(name == m_name) return;
	if(m_root && !isTemplate()) {
		if(m_name && m_root->m_widgets.get(m_name, 0)==this) m_root->m_widgets.erase(m_name);
		if(name) m_root->m_widgets[name] = this;
	}
	m_name = name;
}

// =================================================== //

void Widget::add(Widget* w) {
	add(w, m_client->m_children.size());
}

void Widget::add(Widget* w, unsigned index) {
	if(!w->isTemplate()) index += m_client->m_skipTemplate;
	else if((int)index > m_client->m_skipTemplate && (!m_root || this!=m_root->getRootWidget())) index = m_client->m_skipTemplate;
	Point pos = w->getPosition();
	if(w->m_parent) w->m_parent->remove(w);
	// Add to children
	if(index<m_client->m_children.size())
		m_client->m_children.insert( m_client->m_children.begin()+index, w );
	else m_client->m_children.push_back( w );
	if(w->isTemplate()) ++m_client->m_skipTemplate;
	// Setup
	w->setRoot(m_root);
	w->m_parent = m_client;
	w->setPosition(pos.x, pos.y);
	if(w->m_relative) {
		if(w->m_relative[2] == 0 && w->m_relative[3]==0) w->updateRelativeFromRect();
		else w->setPositionFloat(w->m_relative[0], w->m_relative[1], w->m_relative[2], w->m_relative[3]);
	}
	w->updateTransforms();
	w->onAdded();
	onChildChanged(w);
	notifyChange();
}

void Widget::add(Widget* w, int x, int y) {
	add(w);
	w->setPosition(x, y);
}

bool Widget::remove(Widget* w) {
	for(unsigned i=0; i<m_client->m_children.size(); ++i) {
		if(m_client->m_children[i] == w) {
			m_client->m_children.erase( m_client->m_children.begin() + i );
			w->setRoot(0);
			w->m_parent = 0;
			if(w->isTemplate()) --m_skipTemplate;
			onChildChanged(w);
			notifyChange();
			return true;
		}
	}
	return false;
}

void Widget::removeFromParent() {
	Widget* parent = getParent();
	if(parent && parent->m_client != m_parent) parent = m_parent;
	if(parent) parent->remove(this);
}

int Widget::deleteChildWidgets() {
	int deleted = m_client->m_children.size() - m_client->m_skipTemplate;
	while((int)m_client->m_children.size() > m_client->m_skipTemplate) {
		Widget* w = m_client->m_children.back();
		m_client->m_children.pop_back();
		w->setRoot(0);
		delete w;
	}
	if(deleted) {
		refreshLayout();
		notifyChange();
	}
	return deleted;
}

void Widget::setRoot(Root* r) {
	if(m_root && m_root->m_focus==this) m_root->m_focus = 0;
	if(m_root && m_root->m_mouseFocus==this) m_root->m_mouseFocus = m_root->getRootWidget();
	bool added = r && r != m_root;

	// Register name
	if(m_name && !isTemplate()) {
		if(m_root && m_root!=r && m_root->m_widgets.get(m_name, 0) == this) m_root->m_widgets.erase(m_name);
		if(added) r->m_widgets[m_name] = this;
	}

	m_root = r;
	if(added) onAdded();
	for(uint i=0; i<m_children.size(); ++i) m_children[i]->setRoot(r);
}

int Widget::getIndex() const {
	if(!m_parent) return 0;
	// If template
	if(isTemplate()) {
		for(int i=0; i<m_parent->m_skipTemplate; ++i) {
			if(m_parent->m_children[i] == this) return i;
		}
	}
	else {
		for(int i=0; i<m_parent->getWidgetCount(); ++i) {
			if(m_parent->getWidget(i) == this) return i;
		}
	}
	return -1;
}

int Widget::getTemplateIndex() const {
	if(!m_parent) return 0;
	for(int i=0; i<m_parent->getTemplateCount(); ++i) {
		if(m_parent->getTemplateWidget(i) == this) return i;
	}
	return -1;
}

Widget* Widget::getParent(bool includeTemplates) const {
	if(!m_parent) return 0;
	if(isTemplate() || includeTemplates) return m_parent;
	// Get root widget of parent template
	Widget* p = m_parent;
	while(p && p->isTemplate()) p = p->m_parent;
	if(p && p->m_client != m_parent) return m_parent; // Non-template widget attached to a non-client
	return p;
}

// =================================================== //

void Widget::onChildChanged(Widget* w) {
	if(!isLayoutPaused()) {
		if(isAutosize() && ((m_anchor&0xf)==3 || m_anchor>>4==3) && getParent()) getParent()->onChildChanged(this);
		refreshLayout();
	}
}

void Widget::setLayout(Layout* layout) {
	if(m_client->m_layout && --m_client->m_layout->ref<=0) delete m_client->m_layout;
	if(layout) ++layout->ref;
	m_client->m_layout = layout;
	if(!isLayoutPaused()) refreshLayout();
}
Layout* Widget::getLayout() const {
	return m_client->m_layout;
}

void Widget::refreshLayout() {
	pauseLayout();
	Point lsize = getSize();
	if(m_client != this && !m_client->isLayoutPaused()) m_client->refreshLayout();
	else if(m_layout) m_layout->apply(this);
	updateAutosize();
	if(m_client->m_layout && lsize != getSize()) m_client->m_layout->apply(m_client);
	resumeLayout(false);
}

bool Widget::isLayoutPaused() const { return m_states & 0x400; }
void Widget::pauseLayout() { m_states |= 0x400; }
void Widget::resumeLayout(bool update) {
	m_states &= ~0x400;
	if(update) refreshLayout();
}

// =================================================== //


void Widget::onMouseButton(const Point& p, int d, int u) {
	if(hasFocus()) {
		if(d && eventMouseDown)  eventMouseDown(this, p, d);
		if(u && eventMouseUp)    eventMouseUp(this, p, u);
	}
}
void Widget::onMouseMove(const Point& lp, const Point& p, int b) {
	if(eventMouseMove) eventMouseMove(this, p, b);
}
bool Widget::onMouseWheel(int w) {
	if(eventMouseWheel) eventMouseWheel(this, w);
	return eventMouseWheel;
}
void Widget::onMouseEnter() {
	if(eventMouseEnter) eventMouseEnter(this);
}
void Widget::onMouseExit() {
	if(eventMouseExit) eventMouseExit(this);
}


// =================================================== //

Skin* Widget::getSkin() const {
	return m_skin;
}
void Widget::setSkin(Skin* s) {
	for(int i=0; i<getTemplateCount(); ++i) {
		Widget* w = getTemplateWidget(i);
		if(w->m_skin==m_skin) w->setSkin(s);
	}
	m_skin = s;
}

int Widget::getState() const {
	if(m_states&0x40 && m_parent) return m_parent->getState();
	int state = 0; // calculate state
	if(!isEnabled() || !isParentEnabled()) state = 3;
	else if(m_root && m_root->m_mouseFocus==this) {
		state = 1; // or 2 of mouse is pressed
		if(m_root->m_mouseState) {
			Point localMouse = m_root->m_mousePos;
			if(m_parent) localMouse = m_parent->m_derivedTransform.untransform(localMouse);
			if(m_rect.contains(localMouse)) state = 2;
		}
	}
	if(isSelected()) state |= 4;
	return state;
}

unsigned Widget::deriveColour(unsigned base, unsigned custom, short flags) {
	unsigned result = (flags&0x100? custom: base) & 0xffffff;
	result |= (flags&0x200? custom: base) & 0xff000000;
	return result;
}

void Widget::draw() const {
	if(m_rect.width<=0 || m_rect.height<=0 || !isVisible()) return;
	drawSkin();
	drawChildren();
}
void Widget::drawSkin() const {
	if(m_skin && m_skin->getImage()>=0) {
		unsigned colour = deriveColour(m_skin->getState(getState()).backColour, m_colour, m_states);
		m_root->getRenderer()->drawSkin(m_skin, m_rect, colour, getState());
	}
}
void Widget::drawChildren() const {
	if(!m_children.empty()) {
		m_root->getRenderer()->push(m_rect);
		m_root->getRenderer()->setTransform(m_derivedTransform);
		for(unsigned i=0; i<m_children.size(); ++i) {
			m_children[i]->draw();
		}
		if(m_parent) m_root->getRenderer()->setTransform(m_parent->m_derivedTransform);
		m_root->getRenderer()->pop();
	}
}

