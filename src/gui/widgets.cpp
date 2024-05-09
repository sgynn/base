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


Label::Label(const Rect& r, Skin* s, const char* c) : Widget(r,s), m_caption(c), m_fontSize(0), m_fontAlign(0), m_wordWrap(0) {
	m_states = 0x3; // intangible
}
void Label::initialise(const Root*, const PropertyMap& map) {
	if(map.contains("align")) m_fontAlign = atoi(map["align"]);
	if(map.contains("wrap")) setWordWrap( atoi(map["wrap"] ) );
	if(map.contains("fontsize")) m_fontSize = atoi(map["fontsize"]);
	if(map.contains("caption")) setCaption( map["caption"] );
}
void Label::copyData(const Widget* from) {
	if(const Label* label = cast<Label>(from)) {
		m_fontAlign = label->m_fontAlign;
		m_wordWrap = label->m_wordWrap;
		m_fontSize = label->m_fontSize;
		m_caption = label->m_caption;
	}
}
void Label::setCaption( const char* c) {
	m_caption = c;
	if(m_wordWrap) updateWrap();
	updateAutosize();
}
void Label::setSize(int w, int h) {
	Widget::setSize(w, h);
	if(m_wordWrap) updateWrap();
}
Point Label::getPreferredSize(const Point& hint) const {
	if(!isAutosize() || !m_skin || !m_skin->getFont()) return getSize();
	const char* t = m_caption? m_caption.str(): "XX";
	int fontSize = m_fontSize? m_fontSize: m_skin->getFontSize();
	Point size;
	if(m_wrapValues.empty()) size = m_skin->getFont()->getSize(t, fontSize);
	else {
		char* cap = const_cast<char*>((const char*)m_caption); // nasty
		for(int w: m_wrapValues) cap[w] = '\n';
		size = m_skin->getFont()->getSize(t, fontSize);
		for(int w: m_wrapValues) cap[w] = ' ';
	}
	Point& offset = m_skin->getState(0).textPos; // Approximate - could have weird effects
	size += offset + offset;
	assert(size.x<5000 && size.y<5000);
	return size;
}
void Label::updateAutosize() {
	if(isAutosize()) {
		//Point lastSize = getSize();
		Point newSize = getPreferredSize();
		setSizeAnchored(newSize);
		
		// Font alignment dictates resize direction - *conflicts with widget anchor*
		/*
		int a = m_fontAlign? m_fontAlign: m_skin->getFontAlign();
		if(a&10) {
			Point p = getPosition();
			if((a&3)==ALIGN_RIGHT) p.x -= getSize().x - lastSize.x;
			else if((a&3)==ALIGN_CENTER) p.x -= (getSize().x - lastSize.x)/2;
			if((a&12)==ALIGN_BOTTOM) p.y -= getSize().y - lastSize.y;
			else if((a&12)==ALIGN_MIDDLE) p.y -= (getSize().y - lastSize.y)/2;
			setPosition(p);
		}
		*/
	}
}
void Label::setWordWrap(bool w) {
	m_wordWrap = w;
	if(w) updateWrap();
	else m_wrapValues.clear();
}
void Label::updateWrap() {
	m_wrapValues.clear();
	if(!m_caption || !m_skin->getFont()) return;
	int size = m_fontSize? m_fontSize: m_skin->getFontSize();
	int length = m_caption.length();
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
	return m_caption.str();
}
void Label::draw() const {
	if(!isVisible()) return;
	if(m_caption && m_skin->getFont()) {
		Skin::State& s = m_skin->getState( getState() );
		static Skin tempSkin(1);
		int align = m_fontAlign? m_fontAlign: m_skin->getFontAlign();
		int size = m_fontSize? m_fontSize: m_skin->getFontSize();
		tempSkin.setFont(m_skin->getFont(), size, align);
		tempSkin.getState(0).foreColour = deriveColour(s.foreColour, m_colour, m_states);
		tempSkin.getState(0).textPos = s.textPos;
		if(m_wrapValues.empty()) {
			m_root->getRenderer()->drawText(m_rect, m_caption, 0, &tempSkin, 0);
		}
		else {
			// FIXME  - do this properly
			char* caption = const_cast<char*>(m_caption.str());
			for(int w: m_wrapValues) caption[w] = '\n';
			m_root->getRenderer()->drawText(m_rect, caption, 0, &tempSkin, 0);
			for(int w: m_wrapValues) caption[w] = ' ';
		}
	}
	drawChildren();
}


// ===================================================================================== //

Icon::Icon(const Rect& r, Skin* s) : Widget(r, s), m_iconList(0), m_iconIndex(0), m_angle(0) {
	m_states = 0x43; // intangible, inherit state
}
IconList* Icon::getIconList() const { return m_iconList; }
int Icon::getIcon() const { return m_iconIndex; }
void Icon::setIcon(IconList* list, const char* name) {
	int icon = list->getIconIndex(name);
	setIcon(list, icon);
}
void Icon::setIcon(IconList* list, int index) {
	m_iconList = list;
	m_iconIndex = index;
	updateAutosize();
}
void Icon::setIcon(int index) {
	m_iconIndex = index;
	updateAutosize();
}
void Icon::setIcon(const char* name) {
	if(m_iconList) {
		m_iconIndex = m_iconList->getIconIndex(name);
		if(m_iconIndex<0 && name[0]) printf("Error: Icon %s not found\n", name);
		updateAutosize();
	}
}
const char* Icon::getIconName() const {
	return m_iconList? m_iconList->getIconName(m_iconIndex): 0;
}
void Icon::initialise(const Root* root, const PropertyMap& p) {
	if(root && p.contains("iconlist")) m_iconList = root->getIconList( p["iconlist"]);
	char* e;
	if(const char* icon = p.get("icon", 0)) {
		m_iconIndex = strtol(icon, &e, 10);
		if(e==icon) setIcon(icon);
	}
	if(p.contains("angle")) m_angle = atof(p["angle"]) * 0.0174532;
	updateAutosize();
}
void Icon::copyData(const Widget* from) {
	if(const Icon* icon = cast<Icon>(from)) {
		m_iconList = icon->m_iconList;
		m_iconIndex = icon->m_iconIndex;
	}
}
Point Icon::getPreferredSize(const Point& hint) const {
	if(isAutosize() && m_iconList && m_iconIndex>=0 && m_iconIndex < m_iconList->size()) {
		if(m_anchor!=0x33) {
			if((m_anchor&0xf) == 3) {
				Point s = m_iconList->getIconRect(m_iconIndex).size();
				s.y = s.y * m_rect.width / s.x;
				return s;
			}
			if((m_anchor>>4) == 3) {
				Point s = m_iconList->getIconRect(m_iconIndex).size();
				s.x = s.x * m_rect.height / s.y;
				return s;
			}
		}
		return m_iconList->getIconRect(m_iconIndex).size();
	}
	return getSize();
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
	m_root->getRenderer()->drawIcon(m_iconList, m_iconIndex, r, m_angle, colour);
	drawChildren();
}

// ===================================================================================== //

Image::Image(const Rect& r, Skin* s) : Widget(r, s), m_image(-1), m_angle(0) {
	m_states |= 0x40; // add inherit state
}
void Image::initialise(const Root* root, const PropertyMap& p) {
	if(p.contains("angle")) m_angle = atof(p["angle"]) * 0.0174532;
	if(root && p.contains("image")) {
		const char* file = p["image"];
		int index = root->getRenderer()->getImage(file);
		if(index<0) index = root->getRenderer()->addImage(file);
		setImage(index);
		// This is here because m_root may be null in updateAutosize()
		if(isAutosize() && m_image>=0) setSizeAnchored(root->getRenderer()->getImageSize(index));
	}
}
void Image::copyData(const Widget* from) {
	if(const Image* img = cast<Image>(from)) {
		m_image = img->m_image;
		m_angle = img->m_angle;
	}
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

Point Image::getPreferredSize(const Point& hint) const {
	if(isAutosize() && m_image>=0 && m_root) {
		// Maintain aspect ratio
		if(m_anchor!=0x33) {
			if((m_anchor&0xf) == 3) {
				Point s = m_root->getRenderer()->getImageSize(m_image);
				s.y = s.y * m_rect.width / s.x;
				return s;
			}
			if((m_anchor>>4) == 3) {
				Point s = m_root->getRenderer()->getImageSize(m_image);
				s.x = s.x * m_rect.height / s.y;
				return s;
			}
		}

		return m_root->getRenderer()->getImageSize(m_image);
	}
	return getSize();
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
	m_icon = w->getTemplateWidget<Icon>("_icon");
	if(m_icon) m_icon->initialise(root, p);
}
void IconInterface::setIcon(IconList* list, int index) {
	if(m_icon) m_icon->setIcon(list, index);
}
void IconInterface::setIcon(IconList* list, const char* name) {
	if(m_icon) m_icon->setIcon(list, name);
}
void IconInterface::setIcon(int index) {
	if(m_icon) m_icon->setIcon(index);
}
void IconInterface::setIcon(const char* name) {
	if(m_icon) m_icon->setIcon(name);
}
int IconInterface::getIcon() const {
	return m_icon? m_icon->getIcon(): 0;
}
const char* IconInterface::getIconName() const {
	return m_icon? m_icon->getIconName(): 0;
}
IconList* IconInterface::getIconList() const {
	return m_icon? m_icon->getIconList(): 0;
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
	if(hasFocus() && u==1 && eventPressed && contains(p)) eventPressed(this);
}
Point Button::getPreferredSize(const Point& hint) const {
	if(m_caption) return Label::getPreferredSize(hint);
	else return Widget::getPreferredSize(hint);
}
void Button::updateAutosize() {
	if(m_caption) Label::updateAutosize();
	else Widget::updateAutosize();
}
void Button::setCaption(const char* s) {
	Label* txt = getTemplateWidget<Label>("_text");
	if(txt) txt->setCaption(s);
	else Label::setCaption(s);
}
const char* Button::getCaption() const {
	Label* txt = getTemplateWidget<Label>("_text");
	return txt? txt->getCaption(): Label::getCaption();
}


// ===================================================================================== //

void Checkbox::initialise(const Root* root, const PropertyMap& p) {
	Button::initialise(root, p);
	p.readValue("icon", m_checkedIcon);
	p.readValue("nicon", m_uncheckedIcon);
	p.readValue("drag", m_dragMode);
	setChecked(p.getValue("checked", isChecked()));
}
void Checkbox::copyData(const Widget* from) {
	Super::copyData(from);
	if(const Checkbox* c = cast<Checkbox>(from)) {
		m_dragMode = c->m_dragMode;
		m_checkedIcon = c->m_checkedIcon;
		m_uncheckedIcon = c->m_uncheckedIcon;
	}
}

void Checkbox::setSelected(bool s) {
	Button::setIcon(s? m_checkedIcon: m_uncheckedIcon);
	Button::setSelected(s);
}

void Checkbox::setIcon(IconList* list, int checked, int unchecked) {
	Button::setIcon(list, isSelected()? checked: unchecked);
	m_checkedIcon = checked;
	m_uncheckedIcon = unchecked;
}
void Checkbox::setIcon(IconList* list, const char* checked, const char* unchecked) {
	if(list) {
		m_checkedIcon = list->getIconIndex(checked);
		m_uncheckedIcon = list->getIconIndex(unchecked);
	}
	Button::setIcon(list, isSelected()? checked: unchecked);
}

void Checkbox::onMouseButton(const Point& p, int d, int u) {
	if((hasFocus() && u==1 && !m_dragMode && contains(p)) || (d==1 && m_dragMode)) {
		setChecked(!isChecked());
		if(eventChanged) eventChanged(this);
	}
	Widget::onMouseButton(p, d, u);
}
void Checkbox::onKey(int code, wchar_t, KeyMask) {
	if(hasFocus() && code==base::KEY_SPACE) {
		setChecked(!isChecked());
		if(eventChanged) eventChanged(this);
	}
}
void Checkbox::onMouseMove(const Point& last, const Point& pos, int b) {
	Widget::onMouseMove(last, pos, b);
	if(m_dragMode && b && !contains(pos)) {
		Point global = m_derivedTransform.transform(pos);
		if(Checkbox* target = getRoot()->getRootWidget()->getWidget<Checkbox>(global, false, true)) {
			if(target->isChecked() != isChecked()) {
				Widget* p0 = m_parent;
				Widget* p1 = target->getParent();
				if(p0 != p1) { p0=p0->getParent(); p1=p1->getParent(); }
				if(p0 == p1) {
					target->setChecked(isChecked());
					if(target->eventChanged) target->eventChanged(target);
				}
			}
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

void DragHandle::copyData(const Widget* from) {
	if(const DragHandle* handle = cast<DragHandle>(from)) {
		m_mode = handle->m_mode;
		m_clamp = handle->m_clamp;
	}
}

void DragHandle::onMouseButton(const Point& p, int d, int u) {
	Widget::onMouseButton(p,d,u);
	if(d) m_held = p;
}

void DragHandle::onMouseMove(const Point& last, const Point& pos, int b) {
	if(!b || !hasMouseFocus()) return;
	Point delta = m_held - pos;
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
		const Rect& targetRect = target->getRect();
		int anchor[2] = { m_anchor&0xf, m_anchor>>4 };
		auto resize = [&](int axis) {
			if(anchor[axis] == 0) { // Size min
				size[axis] -= delta[axis];
				p[axis] += delta[axis];
				int minSize = m_rect.position()[axis] + m_rect.size()[axis];
				int maxSize = targetRect.position()[axis] + targetRect.size()[axis];
				if(size[axis] < minSize) { p[axis] += size[axis] - minSize; size[axis] = minSize; }
				if(m_clamp && size[axis] > maxSize) { p[axis] += size[axis] - maxSize; size[axis] = maxSize; }
			}
			else if(anchor[axis]==1) { // Size max
				size[axis] -= delta[axis];
				int minSize = targetRect.size()[axis] - getPosition()[axis];
				int maxSize = view[axis] - targetRect.position()[axis];
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
		Point barSize = m_progress->getSize();
		Point viewSize = m_progress->getParent()->getSize();
		barSize[m_mode] = (viewSize[m_mode] - m_borderLow - m_borderHigh) * normalised;
		m_progress->setSize(barSize);
		// Fix alignment
		int a = m_mode==HORIZONTAL? m_anchor&3: (m_anchor>>4)&3;
		Point pos = m_progress->getPosition();
		if(a==2) {	// Centre aligned
			pos[m_mode] = viewSize[m_mode] / 2 - barSize[m_mode]/2 + (m_borderHigh - m_borderLow);
			m_progress->setPosition(pos);
		}
		else if(a==1) {	// Right Aligned
			pos[m_mode] = viewSize[m_mode] - barSize[m_mode] - m_borderHigh;
			m_progress->setPosition(pos);
		}
		else {	// Left Aligned
			pos[m_mode] = m_borderLow;
			m_progress->setPosition(pos);
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
	if(m_progress) {
		if(m_value == m_max) {
			m_borderLow = m_progress->getPosition()[m_mode];
			m_borderHigh = m_progress->getParent()->getSize()[m_mode] - m_borderLow - m_progress->getSize()[m_mode];
		}
		else setValue(m_max);
		uint colour;
		if(p.readValue("barcolour", colour)) m_progress->setColour(colour);
	}
	p.readValue("min", m_min);
	p.readValue("max", m_max);
	p.readValue("value", m_value);
	setValue(m_value);
}

void ProgressBar::copyData(const Widget* from) {
	if(const ProgressBar* o = cast<ProgressBar>(from)) {
		m_mode = o->m_mode;
		m_min = o->m_min;
		m_max = o->m_max;
		m_value = o->m_value;
		m_borderLow = o->m_borderLow;
		m_borderHigh = o->m_borderHigh;
	}
}


// ===================================================================================== //

template<typename T> SpinboxT<T>::SpinboxT(const Rect& r, Skin* s, const char* format) 
	: Widget(r,s), m_text(0), m_value(0), m_min(0), m_max(0), m_buttonStep(1), m_wheelStep(1), m_textChanged(false), m_format(format) {
}
template<typename T> void SpinboxT<T>::initialise(const Root*, const PropertyMap& p) {
	if(p.contains("value")) m_value = atof(p["value"]);
	if(p.contains("min")) m_min = atof(p["min"]);
	if(p.contains("max")) m_max = atof(p["max"]);
	if(p.contains("step")) m_buttonStep = m_wheelStep = atof(p["step"]);
	m_text = getTemplateWidget<Textbox>("_text");
	if(m_text && p.contains("suffix")) m_text->setSuffix(p["suffix"]);
	Button* inc = getTemplateWidget<Button>("_inc");
	Button* dec = getTemplateWidget<Button>("_dec");
	if(inc) inc->eventPressed.bind(this, &SpinboxT<T>::pressInc);
	if(dec) dec->eventPressed.bind(this, &SpinboxT<T>::pressDec);
	if(m_text) {
		m_text->eventChanged.bind(this, &SpinboxT<T>::textChanged);
		m_text->eventSubmit.bind(this, &SpinboxT<T>::textSubmit);
		m_text->eventLostFocus.bind(this, &SpinboxT<T>::textLostFocus);
		m_text->eventMouseWheel.bind(this, &SpinboxT<T>::mouseWheel);
		setValue(m_value);
	}
}

template<typename T> void SpinboxT<T>::copyData(const Widget* from) {
	if(const SpinboxT<T>* s = dynamic_cast<const SpinboxT<T>*>(from)) {
		m_min = s->m_min;
		m_value = s->m_value;
		m_wheelStep = s->m_wheelStep;
		m_buttonStep = s->m_buttonStep;
	}
}
template<typename T> void SpinboxT<T>::setStep(T b, T w) { m_buttonStep=b; m_wheelStep=w; }
template<typename T> T SpinboxT<T>::getValue() const { return m_value; }
template<typename T> void SpinboxT<T>::setValue(T v, bool event) {
	T value = v<m_min? m_min: v>m_max? m_max: v;
	if(m_text) {
		int n = 0;
		char buf[16];
		sprintf(buf, m_format, value, &n);
		m_text->setText(buf);
	}
	if(value != m_value) {
		m_value = value;
		if(event) fireChanged();
	}
}
template<typename T> void SpinboxT<T>::setSuffix(const char* s)   { if(m_text) m_text->setSuffix(s); }
template<typename T> void SpinboxT<T>::setRange(T min, T max)     { m_min=min, m_max=max; setValue(m_value, true); }
template<typename T> void SpinboxT<T>::pressInc(Button*)          { setValue(m_value+m_buttonStep, true); }
template<typename T> void SpinboxT<T>::pressDec(Button*)          { setValue(m_value-m_buttonStep, true); }
template<typename T> void SpinboxT<T>::textSubmit(Textbox*)       { parseText(true); }
template<typename T> void SpinboxT<T>::textLostFocus(Widget*)     { parseText(true); }
template<typename T> void SpinboxT<T>::mouseWheel(Widget*, int w) { parseText(false); setValue(m_value + w * m_wheelStep, true); }
template<typename T> void SpinboxT<T>::textChanged(Textbox*, const char* text) {
	// undo if invalid
	T temp;
	int len = 0;
	sscanf(text, m_format, &temp, &len);
	int tlen = strlen(text);
	bool valid = false;
	if(len == tlen) valid = true;
	else if(tlen<3) { 	// Allow it if appending 0 makes it a valid number
		char test[4] = {0,0,0,0};
		strcpy(test, text);
		test[tlen] = '0';
		sscanf(test, m_format, &temp, &len);
		if(len == tlen+1) valid = true;
	}
	if(valid) {
		m_textChanged = true;
		m_previous = text;
	}
	else m_text->setText(m_previous);
}
template<typename T> void SpinboxT<T>::parseText(bool event) {
	if(m_textChanged) {
		T v = 0;
		sscanf(m_text->getText(), m_format, &v);
		setValue(v, event);
		m_textChanged = false;
	}
}
template<> void SpinboxT<float>::mouseWheel(Widget*, int w) {
	parseText(false);
	float v = m_value + w * m_wheelStep;
	if(fabs(v) < 1e-6f) v=0;
	setValue(v, true); 
}

// Make sure these functions actually get created
namespace gui {
template class SpinboxT<int>;
template class SpinboxT<float>;
}
Spinbox::Spinbox(const Rect& r, Skin* s) : SpinboxT(r,s, "%d%n") { setRange(-100000, 100000); }
SpinboxFloat::SpinboxFloat(const Rect& r, Skin* s) : SpinboxT(r,s, "%g%n") { setRange(-1e6, 1e6); }
void Spinbox::fireChanged() { if(eventChanged) eventChanged(this, m_value); }
void SpinboxFloat::fireChanged() { if(eventChanged) eventChanged(this, m_value); }


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
void Scrollbar::copyData(const Widget* from) {
	if(const Scrollbar* s = cast<Scrollbar>(from)) {
		m_range = s->m_range;
		m_value = s->m_value;
		m_mode = s->m_mode;
		m_step = s->m_step;
	}
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
	m_vScroll = getTemplateWidget<Scrollbar>("_vscroll");
	m_hScroll = getTemplateWidget<Scrollbar>("_hscroll");
	m_client = getTemplateWidget<Widget>("_client");
	if(!m_client) m_client = this;
	
	// Create invisible client for offsetting - need to set as template.
	if(m_client->getTemplateCount()==0) {
		Widget* w = new Widget( Rect(0,0,m_rect.width,m_rect.height), 0);
		w->setAsTemplate();
		w->setTangible(isTangible());
		while(getWidgetCount()) w->add(getWidget(0)); // move any children
		w->setLayout(m_client->getLayout());
		m_client->setLayout(nullptr);
		m_client->add(w);
		m_client = w;
	} 
	else m_client = m_client->getTemplateWidget(0);

	if(m_vScroll) m_vScroll->eventChanged.bind(this, &Scrollpane::scrollChanged);
	if(m_hScroll) m_hScroll->eventChanged.bind(this, &Scrollpane::scrollChanged);
	setPaneSize( m_client->getSize().x, m_client->getSize().y );
}
void Scrollpane::copyData(const Widget* from) {
	if(const Scrollpane* s = cast<Scrollpane>(from)) {
		m_useFullSize = s->m_useFullSize;
		m_alwaysShowScrollbars = s->m_alwaysShowScrollbars;
	}
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

void Scrollpane::ensureVisible(const Point& p) {
	Point view = getViewWidget()->getSize();
	printf("EnsureVisible %d,%d (%dx%d)\n", p.x, p.y, view.x, view.y);
	Point min(p - view);
	Point max = p;
	Point c = getOffset();
	if(c.x < min.x) c.x = min.x;
	else if(c.x > max.x) c.x = max.x;
	if(c.y < min.y) c.y = min.y;
	else if(c.y > max.y) c.y = max.y;
	setOffset(c.x, c.y);
	printf("-> Offset %d,%d\n", getOffset().x, getOffset().y);
}

Point Scrollpane::getPreferredSize(const Point& hint) const {
	return getSize();
}

void Scrollpane::updateAutosize() {
	if(this == m_client) return;	// Stop infinite recursion
	if(isAutosize()) {
		m_client->pauseLayout();
		m_client->setAutosize(true);
		Point s = m_client->getPreferredSize(getViewWidget()->getSize());
		m_client->setAutosize(false);
		setPaneSize(s.x, s.y);
		m_client->resumeLayout();
	}
}
void Scrollpane::setPaneSize(int w, int h) {
	const Point& view = getViewWidget()->getSize();
	if(m_useFullSize) {
		if(w < view.x || (!m_hScroll && m_vScroll)) w = view.x;
		if(h < view.y || (!m_vScroll && m_hScroll)) h = view.y;
	}

	Point client(w, h);
	Point oldSize = m_client->getSize();
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
				if(scroll[i]->getPosition()[o] <= viewport->getPosition()[o]) {
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
	if(oldSize != m_client->getSize()) scrollChanged(0,0);
}
void Scrollpane::useFullSize(bool f) {
	m_useFullSize = f;
	if(isAutosize()) updateAutosize();
	else setPaneSize( getPaneSize().x, getPaneSize().y );
}

void Scrollpane::alwaysShowScrollbars(bool a) {
	m_alwaysShowScrollbars = a;
	useFullSize(m_useFullSize);
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
	m_buttonTemplate = getTemplateWidget<Button>("_tab");
	if(m_buttonTemplate) m_buttonTemplate->setVisible(false);
	if(!m_tabFrame) m_tabFrame = this;
	m_client = m_tabFrame;

	// Selected tab can be index or name. Note: Tabs dont exist yet
	if(p.contains("tab")) {
		const char* tab = p["tab"];
		int tabIndex = getTabIndex(tab);
		if(tabIndex<0) tabIndex = atoi(tab);
		selectTab(tabIndex);
	}
	else m_currentTab = 0;
	
	// connect tabs created with clone()
	if(m_tabStrip) {
		int num = m_tabStrip->getTemplateCount();
		for(int i=0; i<num; ++i) {
			Button* tabButton = cast<Button>(m_tabStrip->getTemplateWidget(i));
			tabButton->eventPressed.bind(this, &TabbedPane::onTabButton);
		}
	}
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
	frame->setAnchor(0x33);
	frame->setName(name);
	frame->setVisible( frame->getIndex() == m_currentTab );
	// Add button
	if(m_buttonTemplate && m_tabStrip) {
		if(m_buttonTemplate->getParent() == m_tabStrip) ++index;
		Button* b = cast<Button>(m_buttonTemplate->clone());
		b->setCaption(name);
		b->eventPressed.bind(this, &TabbedPane::onTabButton);
		b->setVisible(true);
		b->setSelected(frame->isVisible());
		m_tabStrip->add(b, index);
		if(m_autoSizeTabs) {
			int size = b->getSkin()->getFontSize();
			int w = b->getSkin()->getFont()->getSize(name? name: "X", size).x + 6;
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

CollapsePane::CollapsePane(const Rect& r, Skin* s) : Widget(r,s), m_collapsed(false), m_moveable(false), m_expandAnchor(0) {}
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
	m_expandAnchor = m_anchor;
	expand(!m_collapsed);
}
void CollapsePane::copyData(const Widget* from) {
	if(const CollapsePane* c = cast<CollapsePane>(from)) {
		m_expandAnchor = c->m_expandAnchor;
		m_collapsed = c->m_collapsed;
		m_moveable = c->m_moveable;
	}
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
	m_client->pauseLayout();
	
	if(e) {
		m_client->setAnchor(0);
		Point expandedSize = getClientRect().bottomRight();
		if((m_expandAnchor&0xf)==3) expandedSize.x = getSize().x;
		if((m_expandAnchor>>4)==3) expandedSize.y = getSize().y;
		setSize(expandedSize);
		m_client->setAnchor(0x33);
		if(m_expandAnchor) m_anchor = m_expandAnchor;
	}
	else if(m_header) {
		m_client->setAnchor(0);
		Point collapsedSize = m_header->getSize() + m_header->getPosition();
		m_expandAnchor = m_anchor;
		// Manage anchor if set to expand
		if(m_anchor == 0x33) m_anchor = 0x03; // tlrb is invalid when collapsed. Assumes vertical !
		if((m_anchor&0x0f) == 0x03) { collapsedSize.x = getSize().x; m_client->setAnchor(0x03); }
		if((m_anchor&0xf0) == 0x30) { collapsedSize.y = getSize().y; m_client->setAnchor(0x30); }
		setSize(collapsedSize);
	}
	m_client->setVisible(e);
	m_client->resumeLayout();
	
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
void SplitPane::copyData(const Widget* from) {
	if(const SplitPane* w = cast<SplitPane>(from)) {
		m_mode = w->m_mode;
		m_minSize = w->m_minSize;
		m_resizeMode = w->m_resizeMode;
	}
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

Orientation SplitPane::getOrientation() const {
	return m_mode;
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

Widget* SplitPane::getWidget(const Point& p, int typeMask, bool intangible, bool templates, bool clip) {
	if(clip && !contains(p)) return nullptr;
	if(m_children.empty()) return nullptr;

	// Override selecting splits outside their size
	if(templates && typeMask == Widget::staticType()) {
		int hitSize = 4;
		int count = getWidgetCount() - 1;
		for(int i=0; i<count; ++i) {
			const Rect& r = getTemplateWidget(i)->getRect();
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
	if(!m_title) grabHandle(this, p ,d);
}

void Window::onMouseMove(const Point& l, const Point& p, int b) {
	Widget::onMouseMove(l,p,b);
	if(!m_title) dragHandle(this, p, b);
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
		if(eventResized) eventResized(this);
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
	while(owner && !cast<Popup>(owner)) owner = owner->getParent();
	if(owner) {
		cast<Popup>(owner)->m_owned.push_back(this);
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
	case RIGHT: pos.x += r.width; break;
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
	while(newPopup && !cast<Popup>(newPopup)) newPopup = newPopup->getParent();
	if(!newPopup) hide();
	else {
		// Bind lost focus event to this function
		Widget* newItem = getRoot()->getFocusedWidget();
		newItem->eventLostFocus.bind(this, &Popup::lostFocus);
		for(Popup* w: cast<Popup>(newPopup)->m_owned) w->hide();
	}
}

Widget* Popup::addItem(Root* root, const char* tname, const char* name, const char* text, ButtonDelegate event) {
	// Helper function for adding menu buttons
	Widget* w = root->createWidget<Widget>(tname);
	if(w) {
		int bottom = 0;
		if(getWidgetCount()) bottom = getWidget(getWidgetCount()-1)->getRect().bottom();
		if(cast<Label>(w)) cast<Label>(w)->setCaption(text);
		if(cast<Button>(w)) cast<Button>(w)->eventPressed = event;
		w->setName(name);
		w->setSize(getSize().x, w->getSize().y);
		w->setAnchor("lr");
		add(w, 0, bottom);
		setSize(getSize().x, bottom + w->getSize().y);
	}
	return w;
}


// ===================================================================================== //

ScaleBox::ScaleBox(const Rect& r, Skin* s) : Widget(r, s) {
	Widget* client = new Widget(r, nullptr);
	client->setAsTemplate();
	client->setTangible(Tangible::CHILDREN);
	add(client, 0, 0);
	m_client = client;
}

void ScaleBox::initialise(const Root* root, const PropertyMap& p) {
	if(p.contains("scale")) setScale(atof(p["scale"]));
}

void ScaleBox::updateTransforms() {
	if(!m_parent) return;
	m_derivedTransform = m_parent->getDerivedTransform();
	m_derivedTransform.translate(m_rect.x, m_rect.y);
	m_derivedTransform.scale(m_scale);
	updateChildTransforms();
}

void ScaleBox::setScale(float scale) {
	m_scale = scale;
	m_client->setSize(m_rect.width / scale, m_rect.height / scale);
	updateAutosize();
	updateTransforms();
}

void ScaleBox::setPosition(int x, int y) {
	Widget::setPosition(x, y);
	updateTransforms();
}

void ScaleBox::setSize(int w, int h) {
	Widget::setSize(w, h);
	m_client->setSize(m_rect.width / m_scale, m_rect.height / m_scale);
}

Widget* ScaleBox::getWidget(const Point& p, int typeMask, bool tg, bool tm, bool clip) {
	if((clip && !m_rect.contains(p)) || !isVisible()) return 0;
	// Widget::getWidget assumes transform is merely translation
	
	Point local = p;
	local.x /= m_scale;
	local.y /= m_scale;
	return Widget::getWidget(local, typeMask, tg, tm, false);
}

void ScaleBox::updateAutosize() {
	m_client->setAutosize(isAutosize());
	if(!isAutosize()) return;
	Point s = m_client->getPreferredSize();
	setSizeAnchored(Point(s.x * m_scale, s.y * m_scale));
	//setScale(fmin((float)m_rect.width / s.x, (float)m_rect.height / s.y)); // ToDo: AutoScale
}

