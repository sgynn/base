#include <base/gui/widgets.h>
#include <base/gui/renderer.h>
#include <base/gui/skin.h>
#include <base/gui/font.h>
#include <base/gui/layouts.h>
#include <cstdio>

#include <base/input.h>

using namespace gui;

inline unsigned parseColour(const PropertyMap& p, const char* key, unsigned d=0xffffff) {
	char* e;
	const char* c = p.get(key, 0);
	if(!c || c[0]!='#') return d;
	unsigned r = strtol(c+1, &e, 16);
	return e>c? r: d;
}

// ===================================================================================== //


Label::Label(const Rect& r, Skin* s, const char* c) : Widget(r,s), m_caption(0), m_fontSize(0), m_fontAlign(0), m_wordWrap(0) {
	m_states = 0x3; // intangible
	m_caption = strdup(c);
}
Label::~Label() {
	free(m_caption);
}
void Label::initialise(const Root*, const PropertyMap& map) {
	if(map.contains("align")) m_fontAlign = atoi(map["align"]);
	if(map.contains("wrap")) setWordWrap( atoi(map["wrap"] ) );
	if(map.contains("fontsize")) m_fontSize = atoi(map["fontsize"]);
	if(map.contains("caption")) setCaption( map["caption"] );
}
Widget* Label::clone(const char* nt) const {
	Label* w = Widget::clone(nt)->cast<Label>();
	w->m_wordWrap = m_wordWrap;
	w->m_fontSize = m_fontSize;
	w->m_fontAlign = m_fontAlign;
	w->setCaption( m_caption );
	return w;
}
void Label::setCaption( const char* c) {
	free(m_caption);
	m_caption = c? strdup(c): 0;
	if(m_wordWrap) updateWrap();
	updateAutosize();
}
void Label::setSize(int w, int h) {
	Widget::setSize(w, h);
	if(m_wordWrap) updateWrap();
}
void Label::updateAutosize() {
	if(isAutosize() && m_skin->getFont()) {
		const char* t = m_caption && m_caption[0]? m_caption: "XX";
		int fontSize = m_fontSize? m_fontSize: m_skin->getFontSize();
		Point size;
		if(m_wrapValues.empty()) size = m_skin->getFont()->getSize(t, fontSize);
		else {
			for(int w: m_wrapValues) m_caption[w] = '\n';
			size = m_skin->getFont()->getSize(t, fontSize);
			for(int w: m_wrapValues) m_caption[w] = ' ';
		}
		Point& offset = m_skin->getState(0).textPos;
		Point lastSize = getSize();
		setSize(size.x + 2*offset.x, size.y + 2*offset.y);
		// Align dictates direction
		int a = m_fontAlign? m_fontAlign: m_skin->getFontAlign();
		if(a&10) {
			Point p = getPosition();
			if((a&3)==ALIGN_RIGHT) p.x -= getSize().x - lastSize.x;
			else if((a&3)==ALIGN_CENTER) p.x -= (getSize().x - lastSize.x)/2;
			if((a&12)==ALIGN_BOTTOM) p.y -= getSize().y - lastSize.y;
			else if((a&12)==ALIGN_MIDDLE) p.y -= (getSize().y - lastSize.y)/2;
			setPosition(p);
		}
	}
}
void Label::setWordWrap(bool w) {
	m_wordWrap = w;
	updateWrap();
}
void Label::updateWrap() {
	m_wrapValues.clear();
	if(!m_caption) return;
	int size = m_fontSize? m_fontSize: m_skin->getFontSize();
	int length = strlen(m_caption);
	Point full = m_skin->getFont()->getSize(m_caption, size, length);
	if(full.x > m_rect.width) {
		int start=0, end=0;
		const char* e = m_caption;
		while(end<length) {
			while(*e && *e!=' ') ++e;
			Point s = m_skin->getFont()->getSize(m_caption+start, size, e-m_caption-start);
			if(s.x>m_rect.width && e>m_caption+start && (m_wrapValues.empty()||end>m_wrapValues.back())) {
				m_wrapValues.push_back(end);
				start = end + 1;
			}
			else end = e-m_caption, ++e;
		}
	}
}
void Label::setFontSize(int s) {
	m_fontSize = s;
	if(m_wordWrap) updateWrap();
	updateAutosize();
}
void Label::setFontAlign(int a) {
	m_fontAlign = a;
}
const char* Label::getCaption() const {
	return m_caption;
}
void Label::draw() const {
	if(!isVisible()) return;
	if(m_caption[0] && m_skin->getFont()) {
		Skin::State& s = m_skin->getState( getState() );
		static Skin tempSkin(1);
		int align = m_fontAlign? m_fontAlign: m_skin->getFontAlign();
		int size = m_fontSize? m_fontSize: m_skin->getFontSize();
		tempSkin.setFont(m_skin->getFont(), size, align);
		tempSkin.getState(0).foreColour = deriveColour(s.foreColour, m_colour, m_states);
		tempSkin.getState(0).textPos = s.textPos;
		if(m_wrapValues.empty()) {
			m_root->getRenderer()->drawText(m_rect, m_caption, &tempSkin, 0);
		}
		else {
			for(int w: m_wrapValues) m_caption[w] = '\n';
			m_root->getRenderer()->drawText(m_rect, m_caption, &tempSkin, 0);
			for(int w: m_wrapValues) m_caption[w] = ' ';
		}
	}
	drawChildren();
}


// ===================================================================================== //

Icon::Icon(const Rect& r, Skin* s) : Widget(r, s), m_iconList(0), m_iconIndex(0), m_iconIndexAlt(-1), m_angle(0) {
	m_states = 0x43; // intangible, inherit state
}
IconList* Icon::getIconList() const { return m_iconList; }
int Icon::getIcon() const { return m_iconIndex; }
int Icon::getAltIcon() const { return m_iconIndexAlt; }
void Icon::setIcon(IconList* list, int index, int alt) {
	m_iconList = list;
	m_iconIndex = index;
	m_iconIndexAlt = alt;
	updateAutosize();
}
void Icon::setIcon(int index) {
	m_iconIndex = index;
	updateAutosize();
}
void Icon::setAltIcon(int index) {
	m_iconIndexAlt = index;
}
void Icon::setIcon(const char* name) {
	if(m_iconList) {
		m_iconIndex = m_iconList->getIconIndex(name);
		if(m_iconIndex<0 && name[0]) printf("Error: Icon %s not found\n", name);
		updateAutosize();
	}
}
void Icon::setAltIcon(const char* name) {
	if(m_iconList) {
		m_iconIndexAlt = m_iconList->getIconIndex(name);
		if(m_iconIndexAlt<0 && name[0]) printf("Error: Icon %s not found\n", name);
	}
}
void Icon::initialise(const Root* root, const PropertyMap& p) {
	if(root && p.contains("iconlist")) m_iconList = root->getIconList( p["iconlist"]);
	char* e;
	if(const char* icon = p.get("icon", 0)) {
		m_iconIndex = strtol(icon, &e, 10);
		if(e==icon) setIcon(icon);
	}
	if(const char* icon = p.get("selicon", 0)) {
		m_iconIndexAlt = strtol(icon, &e, 10);
		if(e==icon) setAltIcon(icon);
	}
	if(p.contains("angle")) m_angle = atof(p["angle"]);
}
Widget* Icon::clone(const char* nt) const {
	Icon* w = Widget::clone(nt)->cast<Icon>();
	w->m_iconList = m_iconList;
	w->m_iconIndex = m_iconIndex;
	w->m_iconIndexAlt = m_iconIndexAlt;
	return w;
}
void Icon::updateAutosize() {
	if(isAutosize() && m_iconList && m_iconIndex>=0) {
		setSizeAnchored(m_iconList->getIconRect(m_iconIndex).size());
	}
}
void Icon::draw() const {
	if(!isVisible() || !m_iconList) return;
	Rect r = m_rect;
	unsigned colour = m_colour;
	int stateIndex = getState();
	if(m_skin) {
		const Skin::State& state = m_skin->getState(stateIndex);
		r.position() += state.textPos;
		colour = deriveColour(state.foreColour, m_colour, m_states);
	}
	int icon = m_iconIndex;
	if(m_iconIndexAlt>=0 && stateIndex>=4) icon = m_iconIndexAlt;
	m_root->getRenderer()->drawIcon(m_iconList, icon, r, m_angle, colour);
	drawChildren();
}

// ===================================================================================== //

Image::Image(const Rect& r, Skin* s) : Widget(r, s), m_image(-1), m_angle(0) {
	m_states |= 0x40; // add inherit state
}
void Image::initialise(const Root* root, const PropertyMap& p) {
	if(p.contains("angle")) m_angle = atof(p["angle"]);
	if(root && p.contains("image")) {
		const char* file = p["image"];
		int index = root->getRenderer()->getImage(file);
		if(index<0) index = root->getRenderer()->addImage(file);
		setImage(index);
		// This is here because m_root may be null in updateAutosize()
		if(isAutosize() && m_image>=0) setSizeAnchored(root->getRenderer()->getImageSize(index));
	}
}
Widget* Image::clone(const char* nt) const {
	Widget* w = Widget::clone(nt);
	w->cast<Image>()->m_image = m_image;
	return w;
}
void Image::setImage(const char* file) {
	int index = m_root->getRenderer()->getImage(file);
	if(index<0) index = m_root->getRenderer()->addImage(file);
	setImage(index);
}
void Image::setImage(int image) {
	m_image = image;
	updateAutosize();
}
int Image::getImage() const {
	return m_image;
}

void Image::updateAutosize() {
	if(isAutosize() && m_image>=0 && m_root) {
		Point s = m_root->getRenderer()->getImageSize(m_image);
		setSizeAnchored(s);
	}
}

void Image::draw() const {
	if(!isVisible() /*|| m_image<0*/) return;
	Rect r = m_rect;
	if(m_skin) r.position() += m_skin->getState( getState() ).textPos;
	m_root->getRenderer()->drawImage(m_image, r, m_angle, m_colour, (m_colour>>24)/255.0);
	drawChildren();
}

// ===================================================================================== //

void IconInterface::initialiseIcon(Widget* w, const Root* root, const PropertyMap& p) {
	m_icon = w->getTemplateWidget("_icon")->cast<Icon>();
	if(m_icon) m_icon->initialise(root, p);
}
void IconInterface::setIcon(IconList* list, int index, int alt) {
	if(m_icon) m_icon->setIcon(list, index, alt);
}
void IconInterface::setIcon(int index) {
	if(m_icon) m_icon->setIcon(index);
}
void IconInterface::setIcon(const char* name) {
	if(m_icon) m_icon->setIcon(name);
}
int IconInterface::getIcon() {
	return m_icon? m_icon->getIcon(): 0;
}
void IconInterface::setAltIcon(int index) {
	if(m_icon) m_icon->setAltIcon(index);
}
void IconInterface::setAltIcon(const char* name) {
	if(m_icon) m_icon->setAltIcon(name);
}
int IconInterface::getAltIcon() {
	return m_icon? m_icon->getAltIcon(): 0;
}
void IconInterface::setIconColour(unsigned rgb, float a) {
	if(m_icon) m_icon->setColour(rgb, a);
}

// --------------------------------------------------------------------------------------- //


Button::Button(const Rect& r, Skin* s, const char* c) : Label(r,s,c) {
	m_states = 0x7; // No child tangible
}
void Button::initialise(const Root* root, const PropertyMap& p) {
	Label::initialise(root, p);
	initialiseIcon(this, root, p);
}
void Button::draw() const {
	if(!isVisible()) return;
	drawSkin();
	Label::draw();
}
void Button::onMouseButton(const Point& p, int d, int u) {
	Widget::onMouseButton(p, d, u);
	if(hasFocus() && u==1 && eventPressed && m_rect.contains(p)) eventPressed(this);
}
void Button::setCaption(const char* s) {
	Label* txt = getTemplateWidget("_text")->cast<Label>();
	if(txt) txt->setCaption(s);
	else Label::setCaption(s);
}
const char* Button::getCaption() const {
	Label* txt = getTemplateWidget("_text")->cast<Label>();
	return txt? txt->getCaption(): Label::getCaption();
}


// ===================================================================================== //

void Checkbox::initialise(const Root* root, const PropertyMap& p) {
	Button::initialise(root, p);
	if(p.contains("checked") && atoi(p["checked"])) setChecked(true);
	if(p.contains("drag") && atoi(p["drag"])) m_dragMode = true;
}
Widget* Checkbox::clone(const char* type) const {
	Widget* w = Widget::clone(type);
	Checkbox* c = w->cast<Checkbox>();
	if(c) c->m_dragMode =m_dragMode;
	return w;
}

void Checkbox::onMouseButton(const Point& p, int d, int u) {
	if((hasFocus() && u==1 && !m_dragMode) || (d==1 && m_dragMode)) {
		m_states ^= 0x10; // Selected
		if(eventChanged) eventChanged(this);
	}
	Widget::onMouseButton(p, d, u);
}
void Checkbox::onKey(int code, wchar_t, KeyMask) {
	if(hasFocus() && code==base::KEY_SPACE) {
		m_states ^= 0x10; // Selected
		if(eventChanged) eventChanged(this);
	}
}
void Checkbox::onMouseMove(const Point& last, const Point& pos, int b) {
	Widget::onMouseMove(last, pos, b);
	if(m_dragMode && b && !m_rect.contains(pos)) {
		Widget* parent = getParent();
		while(parent->getParent() && !((int)parent->isTangible()&1)) parent = parent->getParent();
		Checkbox* w = parent->getWidget<Checkbox>(pos);
		if(w && w->isChecked() != isChecked()) {
			w->setChecked(isChecked());
			if(w->eventChanged) w->eventChanged(w);
		}
	}
}

// ===================================================================================== //

void DragHandle::initialise(const Root*, const PropertyMap& p) {
	const char* mode = p.get("mode", 0);
	if(mode && strcmp(mode, "size")) m_mode = SIZE;
	else if(mode && strcmp(mode, "move")) m_mode = MOVE;
	m_clamp = p.getValue("clamp", m_clamp);
}

Widget* DragHandle::clone(const char* t) const {
	Widget* w = Widget::clone(t);
	if(DragHandle* d = w->cast<DragHandle>()) {
		d->m_mode = m_mode;
		d->m_clamp = m_clamp;
	}
	return w;
}

void DragHandle::onMouseButton(const Point& p, int d, int u) {
	Widget::onMouseButton(p,d,u);
	if(d) m_held = p - getAbsolutePosition();
}

void DragHandle::onMouseMove(const Point& last, const Point& pos, int b) {
	if(!b || !hasMouseFocus()) return;
	Point delta = m_held - (pos - getAbsolutePosition());
	Widget* target = getParent();
	const Point& view = target->getParent()->getSize();
	if(m_mode == MOVE) {
		Point p = target->getPosition() + delta;
		if(m_clamp && m_parent) {
			if(p.x + m_rect.width > view.x) p.x = view.x - m_rect.width;
			if(p.y + m_rect.height > view.y) p.x = view.y - m_rect.height;
			if(p.x<0) p.x=0;
			if(p.y<0) p.y=0;
		}
		if(p!=target->getPosition()) target->setPosition(p);
	}
	else {
		Point p = target->getPosition();
		Point size = target->getSize();
		const Rect& targetRect = target->getAbsoluteRect();
		int anchor[2] = { m_anchor&0xf, m_anchor>>4 };
		auto resize = [&](int axis) {
			if(anchor[axis] == 1) { // Size min
				size[axis] -= delta[axis];
				p[axis] += delta[axis];

				int minSize = m_rect.position()[axis] + m_rect.size()[axis] - targetRect.position()[axis];
				int maxSize = p[axis] + size[axis];
				if(size[axis] < minSize) { p[axis] += size[axis] - minSize; size[axis] = minSize; }
				if(m_clamp && size[axis] > maxSize) { p[axis] += size[axis] - maxSize; size[axis] = maxSize; }
			}
			else if(anchor[axis]==4) { // Size max
				size[axis] -= delta[axis];

				int minSize = targetRect.position()[axis] + targetRect.size()[axis] - m_rect.position()[axis];
				int maxSize = view[axis] - p[axis];
				if(size[axis] < minSize) size[axis] = minSize;
				if(m_clamp && size[axis] > maxSize) size[axis] = maxSize;
			}
		};
		resize(0);
		resize(1);

		if(p != target->getPosition()) target->setPosition(p);
		if(size != target->getSize()) target->setSize(size);
	}
}


// ===================================================================================== //

ProgressBar::ProgressBar(const Rect& r, Skin* s, Orientation o) : Widget(r,s), m_min(0), m_max(1), m_value(1), m_borderLow(0), m_borderHigh(0), m_mode(o), m_progress(0) {}
void ProgressBar::setRange(float min, float max) { m_min=min; m_max=max; setValue(m_value); }
float ProgressBar::getRange() const { return m_max; }
float ProgressBar::getValue() const { return m_value; }
void ProgressBar::setValue(float v) {
	m_value = v;
	if(m_progress && m_progress->getParent()) {
		// Update size
		float normalised = (m_value - m_min) / (m_max - m_min);
		Point s = m_progress->getSize();
		Point p = m_progress->getParent()->getSize();
		s[m_mode] = (p[m_mode] - m_borderLow - m_borderHigh) * normalised;
		m_progress->setSize(s.x, s.y);
		// Fix alignment
		int a = m_mode==HORIZONTAL? m_anchor&7: (m_anchor>>4)&7;
		Point pos = m_progress->getPosition();
		if(a==4) {	// Centre aligned
			pos[m_mode] = p[m_mode] - s[m_mode];
			m_progress->setPosition(pos.x, pos.y);
		}
		else if(a==2) {	// Right Aligned
			pos[m_mode] = s[m_mode] - m_borderHigh;
			m_progress->setPosition(pos.x, pos.y);
		}
	}
}
void ProgressBar::setSize(int w, int h) {
	Widget::setSize(w, h);
	setValue(m_value);
}
void ProgressBar::setBarColour(int c) {
	if(m_progress) m_progress->setColour(c);
}
void ProgressBar::initialise(const Root* r, const PropertyMap& p) {
	m_progress = getTemplateWidget("_progress");
	if(p.contains("orientation") && strcmp(p["orientation"], "vertical")==0) m_mode = VERTICAL;
	if(p.contains("min")) m_min = atof(p["min"]);
	if(p.contains("max")) m_min = atof(p["max"]);
	if(p.contains("value")) m_min = atof(p["value"]);
	if(m_progress) {
		char* tmp = 0;
		m_borderLow = m_progress->getPosition()[m_mode];
		m_borderHigh = m_progress->getParent()->getSize()[m_mode] - m_borderLow - m_progress->getSize()[m_mode];
		const char* col = p.get("barcolour", 0);
		if(col && col[0] == '#') m_progress->setColour(strtol(col+1, &tmp, 16));
	}
	setValue(m_value);
}


// ===================================================================================== //

Spinbox::Spinbox(const Rect& r, Skin* s) : Widget(r,s), m_text(0), m_value(0), m_min(0), m_max(100), m_textChanged(false) {}
void Spinbox::initialise(const Root*, const PropertyMap& p) {
	if(p.contains("value")) m_value = atoi(p["value"]);
	if(p.contains("min")) m_min = atoi(p["min"]);
	if(p.contains("max")) m_max = atoi(p["max"]);
	m_text = getTemplateWidget("_text")->cast<Textbox>();
	if(m_text && p.contains("suffix")) m_text->setSuffix(p["suffix"]);
	Button* inc = getTemplateWidget("_inc")->cast<Button>();
	Button* dec = getTemplateWidget("_dec")->cast<Button>();
	if(inc) inc->eventPressed.bind(this, &Spinbox::pressInc);
	if(dec) dec->eventPressed.bind(this, &Spinbox::pressDec);
	if(m_text) {
		m_text->eventChanged.bind(this, &Spinbox::textChanged);
		m_text->eventSubmit.bind(this, &Spinbox::textSubmit);
		m_text->eventLostFocus.bind(this, &Spinbox::textLostFocus);
		m_text->eventMouseWheel.bind(this, &Spinbox::mouseWheel);
		setValue(m_value);
	}
}
int  Spinbox::getValue() const { return m_value; }
void Spinbox::setValue(int v) {
	int value = v<m_min? m_min: v>m_max? m_max: v;
	if(m_text) {
		char buf[16];
		sprintf(buf, "%d", value);
		m_text->setText(buf);
	}
	if(value != m_value && eventChanged) eventChanged(this, value);
	m_value = value;
}
void Spinbox::setSuffix(const char* s)   { if(m_text) m_text->setSuffix(s); }
void Spinbox::setRange(int min, int max) { m_min=min, m_max=max; setValue(m_value); }
void Spinbox::pressInc(Button*)          { setValue(m_value+1); }
void Spinbox::pressDec(Button*)          { setValue(m_value-1); }
void Spinbox::textSubmit(Textbox*)       { parseText(); }
void Spinbox::textLostFocus(Widget*)     { parseText(); }
void Spinbox::mouseWheel(Widget*, int w) { parseText(); setValue( m_value + w); }
void Spinbox::textChanged(Textbox*, const char* text) {
	// remove any non-numbers
	int good = true;
	for(const char* c=text; *c; ++c) {
		if((*c<'0' || *c > '9') && !(*c == '-' && c==text)) good = false;
	}
	// Remove bad characers
	if(!good) {
		char buf[32];
		int i=0;
		for(const char* c=text; *c; ++c) {
			if(!(*c<'0' || *c > '9') || (*c == '-' && c==text)) buf[i++]=*c;
		}
		buf[i] = 0;
		m_text->setText(buf);
	}
	m_textChanged = true;
}
void Spinbox::parseText() {
	if(m_textChanged) {
		char* e;
		const char* text = m_text->getText();
		int v = strtol(text, &e, 10);
		if(e!=text) setValue(v);
		m_textChanged = false;
	}
}

// ===================================================================================== //

SpinboxFloat::SpinboxFloat(const Rect& r, Skin* s) : Widget(r,s), m_text(0), m_value(0), m_min(-1e8f), m_max(1e8f), m_buttonStep(1), m_wheelStep(0.1), m_textChanged(false) {}
void SpinboxFloat::initialise(const Root*, const PropertyMap& p) {
	if(p.contains("value")) m_value = atof(p["value"]);
	if(p.contains("min")) m_min = atof(p["min"]);
	if(p.contains("max")) m_max = atof(p["max"]);
	if(p.contains("step")) m_buttonStep = atof(p["step"]);
	m_text = getTemplateWidget("_text")->cast<Textbox>();
	if(m_text && p.contains("suffix")) m_text->setSuffix(p["suffix"]);
	Button* inc = getTemplateWidget("_inc")->cast<Button>();
	Button* dec = getTemplateWidget("_dec")->cast<Button>();
	if(inc) inc->eventPressed.bind(this, &SpinboxFloat::pressInc);
	if(dec) dec->eventPressed.bind(this, &SpinboxFloat::pressDec);
	if(m_text) {
		m_text->eventChanged.bind(this, &SpinboxFloat::textChanged);
		m_text->eventSubmit.bind(this, &SpinboxFloat::textSubmit);
		m_text->eventLostFocus.bind(this, &SpinboxFloat::textLostFocus);
		m_text->eventMouseWheel.bind(this, &SpinboxFloat::mouseWheel);
		setValue(m_value);
	}
}
float SpinboxFloat::getValue() const { return m_value; }
void SpinboxFloat::setValue(float v) {
	float value = v<m_min? m_min: v>m_max? m_max: v;
	if(m_text) {
		char buf[16];
		sprintf(buf, "%g", value);
		m_text->setText(buf);
	}
	if(value != m_value && eventChanged) eventChanged(this, value);
	m_value = value;
}
void SpinboxFloat::setSuffix(const char* s)   { if(m_text) m_text->setSuffix(s); }
void SpinboxFloat::setRange(float min, float max) { m_min=min, m_max=max; setValue(m_value); }
void SpinboxFloat::setStep(float btn, float w) { m_buttonStep=btn; m_wheelStep=w; }
void SpinboxFloat::pressInc(Button*)          { setValue(m_value+m_buttonStep); }
void SpinboxFloat::pressDec(Button*)          { setValue(m_value-m_buttonStep); }
void SpinboxFloat::textSubmit(Textbox*)       { parseText(); }
void SpinboxFloat::textLostFocus(Widget*)     { parseText(); }
void SpinboxFloat::mouseWheel(Widget*, int w) {
	parseText();
	float value = m_value + w * m_wheelStep;
	value = floor(value*1e6+0.5)/1e6;
	setValue( value );
}
void SpinboxFloat::textChanged(Textbox*, const char* text) {
	// remove any non-numbers
	int good = true;
	bool decimal = false;
	for(const char* c=text; *c; ++c) {
		if((*c<'0' || *c > '9') && !(*c == '-' && c==text) && !(*c=='.'&&!decimal)) good = false;
		if(*c=='.') decimal = true;
	}
	// Remove bad characers
	if(!good) {
		char buf[32];
		int i=0;
		decimal = false;
		for(const char* c=text; *c; ++c) {
			if(!(*c<'0' || *c > '9') || (*c == '-' && c==text) || (!decimal&&*c=='.')) buf[i++]=*c;
			if(*c=='.') decimal = true;
		}
		buf[i] = 0;
		m_text->setText(buf);
	}
	m_textChanged = true;
}
void SpinboxFloat::parseText() {
	if(m_textChanged) {
		char* e;
		const char* text = m_text->getText();
		float v = strtod(text, &e);
		if(e!=text) setValue(v);
		m_textChanged = false;
	}
}

// ===================================================================================== //


Scrollbar::Scrollbar(const Rect& r, Skin* s, int min, int max) : Widget(r,s),
	m_mode(VERTICAL), m_range(min, max), m_value(min), m_step(1), m_block(0) {
}
void Scrollbar::initialise(const Root*, const PropertyMap& p) {
	if(p.contains("min")) m_range.min = atoi(p["min"]);
	if(p.contains("max")) m_range.max = atoi(p["max"]);
	if(m_range.max<m_range.min) m_range.max = m_range.min;
	int val = atoi(p.get("value", "0"));
	m_value = val-1;
	setValue(val);

	// Orientation
	if(p.contains("orientation")) {
		const char* o = p["orientation"];
		if(strcmp(o, "horizontal")==0) m_mode = HORIZONTAL;
	}
	
	// Setup sub object events
	Button* dec = getTemplateWidget<Button>("_dec");
	Button* inc = getTemplateWidget<Button>("_inc");
	m_block = getTemplateWidget<Widget>("_block");

	if(inc) inc->eventMouseDown.bind(this, &Scrollbar::scrollInc);
	if(dec) dec->eventMouseDown.bind(this, &Scrollbar::scrollDec);
	if(m_block) {
		m_block->eventMouseMove.bind(this, &Scrollbar::moveBlock);
		m_block->eventMouseDown.bind(this, &Scrollbar::pressBlock);
	}
}
Widget* Scrollbar::clone(const char* t) const {
	Widget* w = Widget::clone(t);
	Scrollbar* s = w->cast<Scrollbar>();
	s->m_range = m_range;
	s->m_value = m_value;
	s->m_mode = m_mode;
	s->m_step = m_step;
	return w;
}
void Scrollbar::setSize(int w, int h) {
	Widget::setSize(w, h);
	updateBlock();
}

int Scrollbar::getValue() const {
	return m_value;
}
void Scrollbar::setValue(int v) {
	v = v<m_range.min? m_range.min: v>m_range.max? m_range.max: v; // clamp v
	if(v==m_value) return;
	m_value = v;
	updateBlock();
	if(eventChanged) eventChanged(this, m_value);
}
void Scrollbar::onMouseButton(const Point& p, int d, int u) {
	Widget::onMouseButton(p,d,u);
}
bool Scrollbar::onMouseWheel(int w) {
	setValue(m_value - w * m_step);
	return true;
}
inline int Scrollbar::getPixelRange() const {
	if(!m_block) return 0;
	return m_block->getParent()->getSize()[m_mode] - m_block->getSize()[m_mode];
}
void Scrollbar::updateBlock() {
	if(!m_block) return;
	if(m_range.max<=m_range.min) m_block->setPosition(0,0);
	else {
		int range = getPixelRange();
		Point p(0,0);
		p[m_mode] = (m_value - m_range.min) * range / (m_range.max - m_range.min);
		m_block->setPosition(p.x, p.y);
	}
}
void Scrollbar::setRange(int min, int max, int step) {
	if(max < min) max = min;
	m_range.min = min;
	m_range.max = max;
	m_step      = step;
	setValue(m_value);
}
const gui::Range& Scrollbar::getRange() const {
	return m_range;
}
Orientation Scrollbar::getOrientation() const {
	return m_mode;
}
void Scrollbar::setBlockSize(float rel) {
	if(!m_block) return;
	int track = m_block->getParent()->getSize()[m_mode];
	int s = floor(track * rel + 0.5);
	if(s < 16) s = 16;
	if(s > track) s = track;
	Point tmp = m_block->getSize();
	tmp[m_mode] = s;
	m_block->setSize(tmp.x, tmp.y);
	updateBlock();
}
int  Scrollbar::getStep() const { return m_step; }
void Scrollbar::scrollInc(Widget*, const Point&, int) { setValue( m_value+m_step ); }
void Scrollbar::scrollDec(Widget*, const Point&, int) { setValue( m_value-m_step); }
void Scrollbar::pressBlock(Widget*, const Point& p, int b) {
	if(m_range.max > m_range.min) m_held = p;
}
void Scrollbar::moveBlock(Widget* c, const Point& p, int b) {
	if(b && m_held.x>=0) {
		int pixelRange = getPixelRange();
		if(pixelRange == 0) return;
		Point np = p - m_held + c->getPosition();
		setValue( np[m_mode] * (m_range.max - m_range.min) / pixelRange + m_range.min );
	}
	else m_held.x = -1;
}


// ===================================================================================== //

Scrollpane::Scrollpane(const Rect& r, Skin* s) : Widget(r,s), m_alwaysShowScrollbars(true), m_useFullSize(true), m_vScroll(0), m_hScroll(0) {
	m_states |= 0x80; // turn on autosize by default
}
void Scrollpane::initialise(const Root*, const PropertyMap& p) {
	if(p.contains("showscrollbars")) m_alwaysShowScrollbars = atoi(p["showscrollbars"]);
	if(p.contains("fullsize")) m_useFullSize = atoi(p["fullsize"]);
	// sub widgets
	m_client = this;
	m_vScroll = getTemplateWidget<Scrollbar>("_vscroll");
	m_hScroll = getTemplateWidget<Scrollbar>("_hscroll");
	m_client = getTemplateWidget<Widget>("_client");
	if(!m_client) m_client = this;
	
	// Create invisible client for offsetting - need to set as template.
	if(m_client->getTemplateCount()==0) {
		Widget* w = new Widget( Rect(0,0,m_rect.width,m_rect.height), 0);
		w->setAsTemplate();
		while(getWidgetCount()) w->add(getWidget(0)); // move any children
		m_client->add(w);
		m_client = w;
	} 
	else m_client = m_client->getTemplateWidget(0);

	if(m_vScroll) m_vScroll->eventChanged.bind(this, &Scrollpane::scrollChanged);
	if(m_hScroll) m_hScroll->eventChanged.bind(this, &Scrollpane::scrollChanged);
	setPaneSize( m_client->getSize().x, m_client->getSize().y );
}
Widget* Scrollpane::clone(const char* t) const {
	Widget* w = Widget::clone(t);
	w->cast<Scrollpane>()->m_useFullSize = m_useFullSize;
	w->cast<Scrollpane>()->m_alwaysShowScrollbars = m_alwaysShowScrollbars;
	return w;
}
Widget* Scrollpane::getViewWidget() const {
	return m_client->getParent();
}

void Scrollpane::add(Widget* w, unsigned i) {
	Widget::add(w, i);
}

Point Scrollpane::getOffset() const {
	return -m_client->getPosition();
}
void Scrollpane::setOffset(int x, int y, bool force) {
	if(m_vScroll) m_vScroll->setValue(y);
	if(m_hScroll) m_hScroll->setValue(x);
	// Centre if client is smaller than view
	if(!force) {
		Point size = m_client->getSize();
		Point view = m_client->getParent()->getSize();
		if(size.x < view.x) x = size.x/2 - view.x/2;
		if(size.y < view.y) y = size.y/2 - view.y/2;
	}
	m_client->setPosition(-x, -y);
}
void Scrollpane::setSize(int w, int h) {
	Widget::setSize(w, h);
	if(isAutosize()) updateAutosize();
	else setPaneSize( m_client->getSize().x, m_client->getSize().y);
}
const Point& Scrollpane::getPaneSize() const {
	return m_client->getSize();
}

void Scrollpane::updateAutosize() {
	if(this == m_client) return;	// Stop infinite recursion
	// Autosize pane
	if(isAutosize()) {
		// No widgets
		if(getWidgetCount() == 0) {
			setPaneSize(0,0);
		}
		else {
			// Calculate size
			Rect r(0,0,0,0);
			if(m_useFullSize) {
				const Point& s = getViewWidget()->getSize();
				m_client->setSize(s); // Prevent expanding items from stopping client from shrinking
				r.set(0,0,s.x,s.y);
			}
			for(Widget* w: *m_client) {
				if(w->isVisible()) {
					r.include( Rect(w->getPosition(), w->getSize()) );
				}
			}
			setPaneSize(r.width, r.height);
		}
	}
}
void Scrollpane::setPaneSize(int w, int h) {
	const Point& view = getViewWidget()->getSize();
	if(m_useFullSize) {
		if(w < view.x || (!m_hScroll && m_vScroll)) w = view.x;
		if(h < view.y || (!m_vScroll && m_hScroll)) h = view.y;
	}

	Point client(w, h);
	m_client->setSize(w, h);
	
	bool restoreAutoSize = isAutosize();
	Widget* viewport = getViewWidget();
	Scrollbar* scroll[2] = { m_hScroll, m_vScroll };
	for(int i=0; i<2; ++i) {
		if(scroll[i]) {
			bool needed = m_alwaysShowScrollbars || view[i] < client[i];
			if(needed != scroll[i]->isVisible()) {
				setAutosize(false);
				int o = i^1;
				int change = scroll[i]->getSize()[o] * (needed ? -1: 1);
				Point size = view;
				size[o] += change;
				viewport->setSize(size);
				if(scroll[o]) {
					size = scroll[o]->getSize();
					size[o] += change;
					scroll[o]->setSize(size);
				}
				// Move if scrollbar happens to be the other side
				if(scroll[i]->getAbsolutePosition()[o] <= viewport->getAbsolutePosition()[o]) {
					Point pos = viewport->getPosition();
					pos[o] += change;
					viewport->setPosition(pos);
				}
				scroll[i]->setVisible(needed);
				
				// shrink panel size if showing the bar to avoid the bar taking extra scroll space
				// FIXME: May need more conditions involving autosize, fullsize and layout
				if(needed && restoreAutoSize && client[o] == view[o]-change) {
					client[o] += change;
					m_client->setSize(client);
				}
			}
			scroll[i]->setRange(0, client[i] - view[i]);
			scroll[i]->setBlockSize( (float)view[i] / client[i]);
		}
	}
	if(restoreAutoSize && !isAutosize()) setAutosize(true); // Autosize disabled to prevent event chaining
	scrollChanged(0,0);
}
void Scrollpane::useFullSize(bool f) {
	m_useFullSize = f;
	if(isAutosize()) updateAutosize();
	else setPaneSize( getPaneSize().x, getPaneSize().y );
}
bool Scrollpane::onMouseWheel(int w) {
	if(!Widget::onMouseWheel(w)) {
		Point view = getViewWidget()->getSize();
		Point client = m_client->getSize();
		Point pos = m_client->getPosition();
		if(client.y > view.y) {
			if(m_vScroll) m_vScroll->setValue(m_vScroll->getValue() - w*10);
			else m_client->setPosition(pos.x, pos.y + w*10);
		}
		else {
			if(m_hScroll) m_hScroll->setValue(m_hScroll->getValue() - w*10);
			else m_client->setPosition(pos.x + w*10, pos.y);
		}
		scrollChanged(0,0);
	}
	return true;
}
void Scrollpane::scrollChanged(Scrollbar*, int) {
	#define CLAMP(x,a,b) (x<a?a:x>b?b:x)
	int ox=0, oy=0;
	Point p = m_client->getPosition();
	if(m_hScroll && m_vScroll) {
		ox = m_hScroll->getValue();
		oy = m_vScroll->getValue();
	}
	else {
		Point view = getViewWidget()->getSize();
		Point pane = m_client->getSize();
		ox = m_hScroll? m_hScroll->getValue(): CLAMP(-p.x,0,pane.x-view.x);
		oy = m_vScroll? m_vScroll->getValue(): CLAMP(-p.y,0,pane.y-view.y);
	}
	setOffset(ox, oy);
	if(eventChanged) eventChanged(this);
}



// ===================================================================================== //


TabbedPane::TabbedPane(const Rect& r, Skin* s) : Widget(r, s), m_buttonTemplate(0), m_tabStrip(0), m_tabFrame(0), m_currentTab(-1), m_autoSizeTabs(true) {}
void TabbedPane::initialise(const Root* r, const PropertyMap& p) {
	m_tabStrip = getTemplateWidget("_tabstrip");
	m_tabFrame = getTemplateWidget("_client");
	m_buttonTemplate = getTemplateWidget("_tab")->cast<Button>();
	if(m_buttonTemplate) m_buttonTemplate->setVisible(false);
	if(!m_tabFrame) m_tabFrame = this;
	m_client = m_tabFrame;

	// Selected tab can be index or name. Note: Tabs dont exist yet
	if(p.contains("tab")) {
		const char* tab = p["tab"];
		m_currentTab = getTabIndex(tab);
		if(m_currentTab < 0) m_currentTab = atoi(tab);
	}
	else m_currentTab = 0;
}
void TabbedPane::add(Widget* w, unsigned index) {
	if(!m_tabFrame) Widget::add(w, index);
	else addTab(w->getName(), w);
}
Widget* TabbedPane::addTab(const char* name, Widget* frame, int index) {
	// Add frame
	if(index<0) index = getTabCount();
	if(!frame) frame = new Widget(Rect(0,0,100,100), m_tabFrame->getSkin());
	if(m_tabFrame!=this) m_tabFrame->add(frame, index);
	else Widget::add(frame, index);
	frame->setPosition(0,0);
	frame->setSize(m_tabFrame->getSize().x, m_tabFrame->getSize().y);
	frame->setAnchor(0x55);
	frame->setName(name);
	frame->setVisible( frame->getIndex() == m_currentTab );
	// Add button
	if(m_buttonTemplate && m_tabStrip) {
		if(m_buttonTemplate->getParent() == m_tabStrip) ++index;
		Button* b = m_buttonTemplate->clone()->cast<Button>();
		b->setCaption(name);
		b->eventPressed.bind(this, &TabbedPane::onTabButton);
		b->setVisible(true);
		b->setSelected(frame->isVisible());
		m_tabStrip->add(b, index);
		if(m_autoSizeTabs) {
			int size = b->getSkin()->getFontSize();
			int w = b->getSkin()->getFont()->getSize(name, size).x + 6;
			b->setSize(w, b->getSize().y);
		}
		layoutTabs();
	}
	return frame;
}
void TabbedPane::layoutTabs() {
	Point p = m_buttonTemplate->getPosition();
	for(int i=0; i<m_tabStrip->getTemplateCount(); ++i) {
		Widget* w = m_tabStrip->getTemplateWidget(i);
		if(w->isVisible()) {
			w->setPosition(p.x, p.y);
			p.x += w->getSize().x;
		}
	}
}
int     TabbedPane::getTabCount() const { return m_tabFrame->getWidgetCount(); }
Widget* TabbedPane::getTab(int index)   { return m_tabFrame->getWidget(index); }
Widget* TabbedPane::getTab(const char* name) {
	int index = getTabIndex(name);
	if(index>=0) return getTab(index);
	return 0;
}
void TabbedPane::removeTab(const char* name, bool destroy) { removeTab( getTabIndex(name), destroy ); }
void TabbedPane::removeTab(int index, bool destroy) {
	if(index < 0 || index >= getTabCount()) return;
	Widget* tab = getTab(index);
	m_tabFrame->remove( tab );
	if(destroy) delete tab;
	// Delete button
	if(m_buttonTemplate && m_tabStrip) {
		if(m_buttonTemplate->getParent() == m_tabStrip) ++index;
		Widget* b = m_tabStrip->getTemplateWidget(index);
		m_tabStrip->remove(b);
		delete b;
		layoutTabs();
	}
}
void TabbedPane::selectTab(const char* name) { selectTab( getTabIndex(name) ); }
void TabbedPane::selectTab(int index) {
	if(m_currentTab == index || index < 0 || index >= getTabCount()) return;
	Widget* tab = getTab(m_currentTab);
	Widget* button = getTabButton(m_currentTab);
	if(tab) tab->setVisible(false);
	if(button) button->setSelected(false);

	Widget* ntab = getTab(index);
	Widget* nbutton = getTabButton(index);

	if(ntab) ntab->setVisible(true);
	if(nbutton) nbutton->setSelected(true);

	m_currentTab = index;
	m_client = ntab;
	if(eventTabChanged) eventTabChanged(this);
}

Widget* TabbedPane::getTabButton(int index) {
	if(!m_tabStrip || index < 0 || index >= getTabCount()) return 0;
	if(m_buttonTemplate->getParent() == m_tabStrip) ++index;
	return m_tabStrip->getTemplateWidget(index);
}

void TabbedPane::onTabButton(Button* b) {
	Widget* w = b;
	while(w && w->getParent() != m_tabStrip) w = w->getParent();
	int index = w->getTemplateIndex();
	if(m_buttonTemplate->getParent() == w->getParent()) --index;
	selectTab(index);
}

Widget*     TabbedPane::getCurrentTab()            { return getTab(m_currentTab); }
int         TabbedPane::getCurrentTabIndex() const { return m_currentTab; }
const char* TabbedPane::getCurrentTabName() const  { return getTabName(m_currentTab); }

const char* TabbedPane::getTabName(int index) const {
	if(index<0 || index>=getTabCount()) return 0;
	return m_tabFrame->getWidget(index)->getName();
}
int TabbedPane::getTabIndex(const char* name) const {
	for(int i=0; i<m_tabFrame->getWidgetCount(); ++i) {
		if(strcmp(name, m_tabFrame->getWidget(i)->getName()) == 0) return i;
	}
	return -1;
}


// ===================================================================================== //

CollapsePane::CollapsePane(const Rect& r, Skin* s) : Widget(r,s), m_collapsed(false), m_moveable(false) {}
void CollapsePane::initialise(const Root* root, const PropertyMap& p) {
	if(p.contains("moveable")) m_moveable = atoi(p["moveable"]);

	m_client = getTemplateWidget("_client");
	m_header = getTemplateWidget<Button>("_header");
	m_check = getTemplateWidget<Checkbox>("_check");
	if(m_header) m_header->eventPressed.bind(this, &CollapsePane::toggle);
	if(m_check) m_check->eventChanged.bind(this, &CollapsePane::checkChanged);
	if(!m_client) m_client = this;

	const char* caption = p["caption"];
	if(caption) setCaption(caption);
	m_collapsed = p.getValue("collapsed", 0);
	expand(!m_collapsed);
}
Widget* CollapsePane::clone(const char* type) const {
	Widget* w = Widget::clone(type);
	if(CollapsePane* c = w->cast<CollapsePane>()) {
		c->setCaption(getCaption());
		c->expand(isExpanded());
	}
	return w;
}
void CollapsePane::setCaption(const char* c) {
	if(m_header) m_header->setCaption(c);
}
const char* CollapsePane::getCaption() const {
	return m_header? m_header->getCaption(): "";
}
bool CollapsePane::isExpanded() const { return !m_collapsed; }
void CollapsePane::expand(bool e) {
	m_collapsed = !e;
	if(m_client == this) return; // Invalid setup
	
	if(e) {
		m_client->setAnchor(0);
		setSize(getClientRect().bottomRight());
		m_client->setAnchor(0x55);
	}
	else if(m_header) {
		m_client->setAnchor(0);
		setSize(m_header->getSize());
	}
	m_client->setVisible(e);
	
	Widget* state = getTemplateWidget("_state");
	if(state) state->setSelected(e);
	if(Widget* parent = getParent()) parent->refreshLayout();
}
void CollapsePane::updateAutosize() {
	Point s = getSize();
	if(!m_collapsed) Widget::updateAutosize();
	if(s != getSize()) {
		if(Widget* parent = getParent()) parent->refreshLayout();
	}
}
void CollapsePane::toggle(Button*) {
	expand(m_collapsed);
}
void CollapsePane::setChecked(bool c) {
	if(m_check) m_check->setChecked(c);
}
void CollapsePane::checkChanged(Button* b) {
	if(eventChecked) eventChecked(this, b->isSelected());
}
bool CollapsePane::isChecked() const {
	return m_check && m_check->isChecked();
}
void CollapsePane::setMoveable(bool m) {
	if(m_header && m && !m_moveable) {
		m_header->eventMouseDown.bind(this, &CollapsePane::dragStart);
		m_header->eventMouseMove.bind(this, &CollapsePane::dragMove);
	}
	else if(m_header && !m && m_moveable) {
		m_header->eventMouseDown.unbind();
		m_header->eventMouseMove.unbind();
	}
	m_moveable = m;
}
void CollapsePane::dragStart(Widget*, const Point& p, int b) {
	if(b==1) m_held = p;
}
void CollapsePane::dragMove(Widget*, const Point& p, int b) {
	if(b!=1 || m_held.x<0) m_held.x = -1;
	else {
		setPosition(0, p.y - m_held.y);
		// Todo: reorder layout
	}
}


// ===================================================================================== //

SplitPane::SplitPane(const Rect& r, Skin* s, Orientation o) : Widget(r,s), m_mode(o), m_held(-100), m_minSize(0), m_resizeMode(ALL), m_sash(0) {
}
void SplitPane::initialise(const Root* r, const PropertyMap& p) {
	if(p.contains("min")) m_minSize = atoi(p["min"]);
	if(p.contains("resize")) {
		const char* resize = p["resize"];
		if(strcmp(resize, "first")==0) m_resizeMode = FIRST;
		if(strcmp(resize, "last")==0) m_resizeMode = LAST;
	}

	// Orientation
	if(p.contains("orientation")) {
		const char* o = p["orientation"];
		if(strcmp(o, "horizontal")==0) m_mode = HORIZONTAL;
	}

	// Setup sash
	m_sash = getTemplateWidget("_sash");
	if(!m_sash) {
		m_sash = new Widget(Rect(0,0,1,1), m_skin);
		Widget::add(m_sash, 0);
	}
	setupSash(m_sash);
	m_sash->setVisible( getWidgetCount() > 1 );

	// Set initial positions based on child widget sizes
	if(getWidgetCount()>0) {
		Point pos;
		int sashCount = getWidgetCount() - 1;
		for(int i=0; i<sashCount; ++i) {
			Widget* panel = getWidget(i);
			pos[m_mode] = panel->getPosition()[m_mode] + panel->getSize()[m_mode];
			m_children[i]->setPosition(pos);
		}
		validateLayout();
	}
}
Widget* SplitPane::clone(const char* nt) const {
	SplitPane* w = Widget::clone(nt)->cast<SplitPane>();
	w->m_mode = m_mode;
	w->m_resizeMode = m_resizeMode;
	w->m_minSize = m_minSize;
	return w;
}
void SplitPane::setupSash(Widget* sash) {
	sash->eventMouseMove.bind(this, &SplitPane::moveSash);
	sash->eventMouseDown.bind(this, &SplitPane::pressSash);
}
void SplitPane::pressSash(Widget* w, const Point& p, int) {
	m_held = p[m_mode];
	for(size_t i=0; i<m_children.size(); ++i) {
		if(m_children[i]==w) { m_heldIndex=i; break; }
	}
}
void SplitPane::moveSash(Widget* w, const Point& p, int b) {
	if(b && m_held > -100) {
		int sashSize = w->getSize()[m_mode];
		int panes = getWidgetCount();
		
		// Move sash
		Point sp = w->getPosition();
		int shift = p[m_mode] - m_held;
		sp[m_mode] += shift;
		if(shift < 0) {
			int min = sashSize * m_heldIndex + m_minSize * (m_heldIndex + 1);
			if(sp[m_mode] < min) sp[m_mode] = min;
		} else {
			int max = getSize()[m_mode] - (panes-m_heldIndex-1) * (sashSize + m_minSize);
			if(sp[m_mode] > max) sp[m_mode] = max;
		}
		w->setPosition(sp.x, sp.y);

		// Push into others
		if(shift<0) {
			for(int i=m_heldIndex-1; i>=0; --i) {
				Widget* c = m_children[i];
				sp[m_mode] -= sashSize;
				if(c->getPosition()[m_mode] > sp[m_mode]) {
					c->setPosition(sp.x, sp.y);
				}
				else break;
			}
		}
		else {
			for(int i=m_heldIndex+1; i<panes-1; ++i) {
				Widget* c = m_children[i];
				sp[m_mode] += sashSize;
				if(c->getPosition()[m_mode] < sp[m_mode]) {
					c->setPosition(sp.x, sp.y);
				}
				else break;
			}
		}
		// Refresh pane widgets
		refreshLayout();
	} else m_held = -100;
}
void SplitPane::refreshLayout() {
	// Sets size of child widgets to maximum space between sashes
	int space = m_sash->getSize()[m_mode];
	int count = getWidgetCount();
	int first = count - 1;
	if(count<2) return;
	Point p, s = getSize();
	for(int i=0; i<count; ++i) {
		if(i == first) s[m_mode] = getSize()[m_mode] - p[m_mode];
		else s[m_mode] = m_children[i]->getPosition()[m_mode] - p[m_mode];
		m_children[i+first]->setPosition(p.x, p.y);
		m_children[i+first]->setSize(s.x, s.y);
		p[m_mode] += s[m_mode] + space;
	}
}
void SplitPane::validateLayout() {
	// Sets positions of sashes to be within bounds and not overlapping
	int sashCount = getWidgetCount() - 1;
	int sashSize = m_sash->getSize()[m_mode];
	int min=0, max = getSize()[m_mode] - sashCount * sashSize;
	for(int i=0; i<sashCount; ++i) {
		Widget* sash = m_children[i];
		Point p = sash->getPosition();
		if(p[m_mode] < min) p[m_mode] = min;
		else if(p[m_mode] > max) p[m_mode] = max;
		sash->setPosition(p.x, p.y);
		min = p[m_mode] + sashSize;
		max += sashSize;
	}
}

void SplitPane::setSize(int w, int h) {
	int count = getWidgetCount();
	if(m_resizeMode == ALL) {
		float* rel = 0;
		if(count>1) {
			// Calculate relative sizes
			float total = 0;
			rel = new float[count];
			for(int i=0; i<count; ++i) {
				rel[i] = getWidget(i)->getSize()[m_mode];
				total += rel[i];
			}
			if(total>0) for(int i=0; i<count; ++i) rel[i] /= total;
			else for(int i=0; i<count; ++i) rel[i] = 1.f/count;
			for(int i=1;i<count; ++i) rel[i] += rel[i-1];
		}

		Widget::setSize(w, h);

		if(count>1) {
			// Set sizes from previous relative sizes
			--count;
			Point p = m_sash->getPosition();
			int sashSize = m_sash->getSize()[m_mode];
			float size = m_client->getSize()[m_mode] - sashSize * count;
			for(int i=0; i<count; ++i) {
				p[m_mode] = size * rel[i] + sashSize*i;
				m_children[i]->setPosition(p);
			}
			validateLayout();
			delete [] rel;
		}
	}
	else {
		if(m_resizeMode == FIRST) {
			Point shift = Point(w,h) - getSize();
			shift[m_mode^1] = 0;
			for(int i=0; i<count; ++i) {
				Point p = m_children[i]->getPosition() + shift;
				m_children[i]->setPosition(p);
			}
		}
		Widget::setSize(w, h);
		if(count) validateLayout();
	}

	refreshLayout();
}

void SplitPane::add(Widget* w, unsigned index) {
	Widget::add(w,index);
	// Add corresponding sash
	if(m_sash) {
		Widget* s = m_sash;
		if(getWidgetCount() > 2) {
			s = m_sash->clone();
			s->setAsTemplate();
			s->setPosition(m_sash->getPosition().x, m_sash->getPosition().y);
			setupSash(s);
			Widget::add(s, 0);
		}
		if(getWidgetCount()>1) s->setVisible(true);
	}
}
bool SplitPane::remove(Widget* w) {
	// Delete a sash too
	if(!w->isTemplate()) {
		Widget* sash = getTemplateWidget( w->getIndex() - 1 );
		if(sash==m_sash) m_sash->setVisible(false);
		else Widget::remove( sash );
	}
	return Widget::remove(w);
}

Widget* SplitPane::getWidget(const Point& p, int typeMask, bool intangible, bool templates) {
	if(!m_rect.contains(p) || !isVisible()) return 0;

	// Override selecting splits outside their size
	if(templates && typeMask == Widget::staticType()) {
		int hitSize = 4;
		int count = getWidgetCount() - 1;
		for(int i=0; i<count; ++i) {
			const Rect& r = getTemplateWidget(i)->getAbsoluteRect();
			Point s(r.x+r.width/2, r.y+r.height/2);
			if(i<count-1 && p[m_mode] > getTemplateWidget(i+1)->getAbsoluteRect().position()[m_mode]) continue;
			if(abs(s[m_mode]-p[m_mode]) < hitSize) return getTemplateWidget(i);
		}
	}

	return Widget::getWidget(p, typeMask, intangible, templates);
}

// ===================================================================================== //



Window::Window(const Rect& r, Skin* s) : Widget(r,s), m_minSize(80,40) {
}
void Window::initialise(const Root* root, const PropertyMap& p) {
	m_client = this;
	Button* t = getTemplateWidget<Button>("_caption");
	Button* x = getTemplateWidget<Button>("_close");
	Widget* sbr = getTemplateWidget<Widget>("_sizebr");

	m_client = getTemplateWidget<Widget>("_client");
	if(!m_client) m_client = this;
	if(m_client==this) printf("Warning: window has no client\n");
	m_title = t;

	if(p.contains("caption")) setCaption( p["caption"]);
	if(p.contains("minimum")){
		sscanf(p["minimum"], "%d %d", &m_minSize.x, &m_minSize.y);
		setMinimumSize(m_minSize.x, m_minSize.y);
	}

	initialiseIcon(this, root, p);

	// Setup events
	if(t) {
		t->eventMouseDown.bind(this, &Window::grabHandle);
		t->eventMouseMove.bind(this, &Window::dragHandle);
	}
	if(sbr) {
		sbr->eventMouseDown.bind(this, &Window::grabHandle);
		sbr->eventMouseMove.bind(this, &Window::sizeHandle);
	}
	if(x) x->eventPressed.bind(this, &Window::pressClose);
}
void Window::setCaption(const char* c) {
	if(m_title) m_title->setCaption(c);
}
const char* Window::getCaption() const {
	return m_title? m_title->getCaption(): "";
}

void Window::setMinimumSize(int w, int h) {
	m_minSize.set(w, h);
	Point s = getSize();
	if(s.x<w || s.y<h) {
		if(s.x<w) s.x=w;
		if(s.y<h) s.y=h;
		setSize(s.x, s.y);
	}
}

void Window::onMouseButton(const Point& p, int d, int u) {
	raise();
	Widget::onMouseButton(p,d,u);
}

void Window::pressClose(Button*) {
	setVisible(false);
	if(eventClosed) eventClosed(this);
}
void Window::grabHandle(Widget*, const Point& p, int b) {
	raise();
	if(b==1) m_held = p;
}
void Window::dragHandle(Widget* c, const Point& p, int b) {
	if(b && m_held.x>0) {
		Point pos = getPosition();
		pos = pos + p - m_held;
		setPosition(pos.x, pos.y);
	} else m_held.x=-1;
}
void Window::sizeHandle(Widget* c, const Point& p, int b) {
	if(b && m_held.x>0) {
		Point s = getSize();
		s = s + p - m_held;
		// Minimum size	- ToDo: put in window properties
		if(s.x<m_minSize.x) s.x = m_minSize.x;
		if(s.y<m_minSize.y) s.y = m_minSize.y;

		setSize(s.x, s.y);
	} else m_held.x=-1;
}


// ===================================================================================== //


Popup::Popup(const Rect& r, Skin* s) : Widget(r,s), m_owner(0) {
}
void Popup::initialise(const Root*, const PropertyMap& p) {
	setVisible(false);
}
void Popup::popup(Root* root, const Point& abs, Widget* owner) {
	if(m_parent) m_parent->remove(this);
	root->getRootWidget()->add(this, abs.x, abs.y);
	setVisible(true);
	while(owner && !owner->cast<Popup>()) owner = owner->getParent();
	if(owner) {
		owner->cast<Popup>()->m_owned.push_back(this);
		m_owner = owner;
	}
	else {
		eventLostFocus.bind(this, &Popup::lostFocus);
		setFocus();
	}
}
void Popup::popup(Widget* owner, Side side) {
	const Rect& r = owner->getAbsoluteRect();
	Point pos = r.position();
	switch(side) {
	case LEFT: pos.x -= getSize().x; break;
	case RIGHT: pos.x += r.height; break;
	case ABOVE: pos.y -= getSize().y; break;
	case BELOW: pos.y += r.height; break;
	}
	popup(owner->getRoot(), pos, owner);
}

void Popup::hide() {
	setVisible(false);
	for(Popup* w: m_owned) w->hide();
}

void Popup::lostFocus(Widget* w) {
	if(!getRoot()) return;
	Widget* newPopup = getRoot()->getFocusedWidget();
	while(newPopup && !newPopup->cast<Popup>()) newPopup = newPopup->getParent();
	if(!newPopup) hide();
	else {
		// Bind lost focus event to this function
		Widget* newItem = getRoot()->getFocusedWidget();
		newItem->eventLostFocus.bind(this, &Popup::lostFocus);
		for(Popup* w: newPopup->cast<Popup>()->m_owned) w->hide();
	}
}

Widget* Popup::addItem(Root* root, const char* tname, const char* name, const char* text, ButtonDelegate event) {
	// Helper function for adding menu buttons
	Widget* w = root->createWidget<Widget>(tname);
	if(w) {
		int bottom = 0;
		if(getWidgetCount()) bottom = getWidget(getWidgetCount()-1)->getRect().bottom();
		if(w->cast<Label>()) w->cast<Label>()->setCaption(text);
		if(w->cast<Button>()) w->cast<Button>()->eventPressed = event;
		w->setName(name);
		w->setSize(getSize().x, w->getSize().y);
		w->setAnchor("lr");
		add(w, 0, bottom);
		setSize(getSize().x, bottom + w->getSize().y);
	}
	return w;
}
