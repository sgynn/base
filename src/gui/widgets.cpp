#include <base/gui/widgets.h>
#include <base/gui/renderer.h>
#include <base/gui/skin.h>
#include <base/gui/font.h>
#include <base/gui/layouts.h>

#include <cstdio>
#include <base/input.h>
#include <base/assert.h>

using namespace gui;

inline unsigned parseColour(const PropertyMap& p, const char* key, unsigned d=0xffffff) {
	char* e;
	const char* c = p.get(key, 0);
	if(!c || c[0]!='#') return d;
	unsigned r = strtol(c+1, &e, 16);
	return e>c? r: d;
}

// ===================================================================================== //


Label::Label(const char* c) : m_caption(c), m_fontSize(0), m_fontAlign(0), m_wordWrap(0) {
	m_states = 0x3; // intangible
	m_rect.set(0,0,32, 16);
}
Label::Label(const char* c, Font* font, int size) : Label(c) {
	setFont(font, size);
}
void Label::initialise(const Root* r, const PropertyMap& map) {
	if(map.contains("font")) m_font = r->getFont(map["font"]);
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
		m_font = label->m_font;
	}
}
void Label::setSkin(Skin* s) {
	Super::setSkin(s);
	updateWrap();
}
void Label::setCaption( const char* c) {
	m_caption = c;
	updateWrap();
	updateAutosize();
}
void Label::setSize(int w, int h) {
	Widget::setSize(w, h);
	if(m_wordWrap) updateWrap();
}
Point Label::getPreferredSize(const Point& hint) const {
	if(!isAutosize()) return getSize();
	Font* font = m_font? m_font: m_skin->getFont();
	if(!font) return getSize();
	const char* t = m_caption? m_caption.str(): "XX";
	int fontSize = m_fontSize? m_fontSize: m_skin? m_skin->getFontSize(): 16;
	Point size;
	if(m_lines.empty()) size = font->getSize(t, fontSize);
	else {
		size.x = 0;
		for(const Line& line: m_lines) size.x = std::max(size.x, (int)line.width);
		size.y = m_lines.size() * font->getLineHeight(fontSize);
	}
	if(m_skin) {
		Point& offset = m_skin->getState(0).textPos; // Approximate - could have weird effects
		size += offset + offset;
	}
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
	updateWrap();
}
void Label::updateWrap() {
	m_lines.clear();
	Font* font = m_font? m_font: m_skin? m_skin->getFont(): nullptr;
	if(!m_caption || !font) return;
	int size = m_fontSize? m_fontSize: m_skin? m_skin->getFontSize(): 16;

	if(m_wordWrap) {
		int length = m_caption.length();
		Point full = font->getSize(m_caption, size, length);
		if(full.x > m_rect.width) {
			int start=0, end=0;
			const char* e = m_caption;
			while(end < length) {
				while(*e && *e!=' ' && *e!='\n') ++e;
				Point s = font->getSize(m_caption+start, size, e-m_caption-start);
				if((s.x > m_rect.width && e > m_caption + start && end > start)) {
					m_lines.push_back({(unsigned short)start, (unsigned short)s.x});
					start = end + 1;
				}
				else if(!*e || *e=='\n') {
					m_lines.push_back({(unsigned short)start, (unsigned short)s.x});
					if(!*e) return;
					start = ++e - m_caption;
				}
				else end = e++ - m_caption;
			}
			return;
		}
	}

	// set wrap values on line breaks to handle alignment properly
	const char* br = strchr(m_caption, '\n');
	if(!br || !br[1]) return; // Single line - simple version
	const char* start = m_caption;
	while(br) {
		m_lines.push_back({(unsigned short)(start-m_caption), (unsigned short)font->getSize(start, size, br-start).x});
		start = br + 1;
		br = strchr(start, '\n');
	}
	m_lines.push_back({(unsigned short)(start-m_caption), (unsigned short)font->getSize(start, size, br-start).x});
}
void Label::setFont(Font* font, int size) {
	m_font = font;
	setFontSize(size);
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
	Font* font = m_font? m_font: m_skin->getFont();
	if(font && m_caption) {
		static const Skin::State fallback;
		const Skin::State& s = m_skin? m_skin->getState(getState()): fallback;
		static Skin tempSkin(1);
		int align = m_fontAlign? m_fontAlign: m_skin? m_skin->getFontAlign(): 0;
		int size = m_fontSize? m_fontSize: m_skin? m_skin->getFontSize(): 16;
		unsigned colour = deriveColour(s.foreColour, m_colour, m_states);
		if(m_lines.empty()) {
			tempSkin.setFont(font, size, align);
			tempSkin.getState(0).foreColour = colour;
			tempSkin.getState(0).textPos = s.textPos;
			m_root->getRenderer()->drawText(m_rect, m_caption, 0, &tempSkin, 0);
		}
		else {
			Point pos = m_rect.position() + s.textPos;
			if((align&0xc)==ALIGN_BOTTOM) pos.y += m_rect.height - m_lines.size() * font->getLineHeight(size);
			else if((align&0xc)==ALIGN_MIDDLE) pos.y += (m_rect.height - m_lines.size() * font->getLineHeight(size)) / 2;
			int h = align&3;
			Line line = m_lines[0];
			for(size_t i=1; i<m_lines.size(); ++i) {
				pos.x = m_rect.x + s.textPos.x;
				if(h == ALIGN_RIGHT) pos.x += m_rect.width - line.width;
				else if(h == ALIGN_CENTER) pos.x += (m_rect.width - line.width) / 2;
				int limit = m_lines[i].start - line.start;
				m_root->getRenderer()->drawText(pos, m_caption+line.start, limit, font, size, colour);
				line = m_lines[i];
				pos.y += m_skin->getFont()->getLineHeight(size);
			}
			pos.x = m_rect.x + s.textPos.x;
			if(h == ALIGN_RIGHT) pos.x += m_rect.width - line.width;
			else if(h == ALIGN_CENTER) pos.x += (m_rect.width - line.width) / 2;
			m_root->getRenderer()->drawText(pos, m_caption+line.start, -1, font, size, colour);
		}
	}
	drawChildren();
}


// ===================================================================================== //

Image::Image() : m_group(nullptr), m_image(0), m_angle(0) {
	setInheritState(true);
}
Image::Image(int w, int h) : Image() {
	setSize(w, h);
}
Image::Image(Root* root, const char* image) : Image() {
	setRoot(root);
	setImage(image);
	setAutosize(true);
	setAutosize(false);
}
Image::Image(IconList* group, const char* image) : Image() {
	setImage(group, image);
	setAutosize(true);
	setAutosize(false);
}

int Image::findImage(const Root* root, IconList* list, const char* name) {
	if(!name || !name[0]) return -1;
	char* end;
	int id = strtol(name, &end, 10);
	if(end > name) return id;
	if(list) return list->getIconIndex(name);
	id = root->getRenderer()->getImage(name);
	if(id<0) id = root->getRenderer()->addImage(name);
	return id;
}

void Image::initialise(const Root* root, const PropertyMap& p) {
	if(p.contains("angle")) m_angle = atof(p["angle"]) * 0.0174532;
	if(root) {
		if(p.contains("group")) m_group = root->getIconList( p["group"]);
		if(p.contains("image")) setImage(findImage(root, m_group, p["image"]));
		// This is here because m_root may be null in updateAutosize()
		if(isAutosize() && m_image>=0) {
			Root* tmp = m_root;
			m_root = const_cast<Root*>(root);
			setSizeAnchored(getPreferredSize());
			m_root = tmp;
		}
	}
}
void Image::copyData(const Widget* from) {
	if(const Image* img = cast<Image>(from)) {
		m_group = img->m_group;
		m_image = img->m_image;
		m_angle = img->m_angle;
	}
}
void Image::setImage(const char* name) { setImage(m_root, name); }
void Image::setImage(Root* root, const char* name) {
	if(m_group) {
		setImage(m_group->getIconIndex(name));
	}
	else if(root) {
		int index = root->getRenderer()->getImage(name);
		if(index<0) index = root->getRenderer()->addImage(name);
		setImage(index);
	}
	else printf("Error: Trying to set image with no root set\n");
}
void Image::setImage(int image) {
	m_image = image;
	updateAutosize();
}

void Image::setImage(IconList* group, int index) {
	m_group = group;
	setImage(index);
}
void Image::setImage(IconList* group, const char* name) {
	m_group = group;
	setImage(name);
}

int Image::getImageIndex() const {
	return m_image;
}
const char* Image::getImageName() const {
	if(m_image < 0) return "";
	if(m_group) return m_group->getIconName(m_image);
	if(m_root) return m_root->getRenderer()->getImageName(m_image);
	return nullptr;
}
IconList* Image::getGroup() const {
	return m_group;
}

Point Image::getPreferredSize(const Point& hint) const {
	if(isAutosize() && m_image>=0) {
		if(!m_root && !m_group) return getSize();
		Point size = m_group? m_group->getIconRect(m_image).size(): m_root->getRenderer()->getImageSize(m_image);

		// Maintain aspect ratio
		if(m_anchor!=0x33) {
			if((m_anchor&0xf) == 3) size.y = size.y * m_rect.width / size.x;
			if((m_anchor>>4) == 3)  size.x = size.x * m_rect.height / size.y;
		}

		return size;
	}
	return getSize();
}

void Image::draw() const {
	if(!isVisible()) return;
	if(m_image >= 0) {
		Rect r = m_rect;
		unsigned colour = m_colour;
		int stateIndex = getState();
		if(m_skin) {
			const Skin::State& state = m_skin->getState(stateIndex);
			r.position() += state.textPos;
			colour = deriveColour(state.foreColour, m_colour, m_states);
		}
		if(m_group) m_root->getRenderer()->drawIcon(m_group, m_image, r, m_angle, colour);
		else m_root->getRenderer()->drawImage(m_image, r, m_angle, colour, (m_colour>>24)/255.0);
	}
	drawChildren();
}

// ===================================================================================== //

void IconInterface::initialiseIcon(Widget* w, const Root* root, const PropertyMap& p) {
	m_icon = w->getTemplateWidget<Image>("_icon");
	if(m_icon) static_cast<Widget*>(m_icon)->initialise(root, p);
}
void IconInterface::setIcon(IconList* list, int index) {
	if(m_icon) m_icon->setImage(list, index);
}
void IconInterface::setIcon(IconList* list, const char* name) {
	if(m_icon) m_icon->setImage(list, name);
}
void IconInterface::setIcon(int index) {
	if(m_icon) m_icon->setImage(index);
}
void IconInterface::setIcon(const char* name) {
	if(m_icon) m_icon->setImage(name);
}
int IconInterface::getIconIndex() const {
	return m_icon? m_icon->getImageIndex(): 0;
}
const char* IconInterface::getIconName() const {
	return m_icon? m_icon->getImageName(): 0;
}
IconList* IconInterface::getIconGroup() const {
	return m_icon? m_icon->getGroup(): 0;
}
void IconInterface::setIconColour(unsigned rgb, float a) {
	if(m_icon) m_icon->setColour(rgb, a);
}

// --------------------------------------------------------------------------------------- //


Button::Button(const char* c) : Label(c) {
	setTangible(Tangible::SELF);
}
void Button::initialise(const Root* root, const PropertyMap& p) {
	Label::initialise(root, p);
	initialiseIcon(this, root, p);
	m_textWidget = getTemplateWidget<Label>("_text");
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
	if(m_caption && getTemplateCount()==0) return Label::getPreferredSize(hint);
	else return Widget::getPreferredSize(hint);
}
void Button::setCaption(const char* s) {
	if(m_textWidget) m_textWidget->setCaption(s);
	else Label::setCaption(s);
}
const char* Button::getCaption() const {
	return m_textWidget? m_textWidget->getCaption(): Label::getCaption();
}


// ===================================================================================== //

void Checkbox::initialise(const Root* root, const PropertyMap& p) {
	Button::initialise(root, p);
	if(getIcon()) {
		if(p.contains("icon")) m_checkedIcon = findImage(root, getIconGroup(), p["icon"]);
		if(p.contains("nicon")) m_uncheckedIcon = findImage(root, getIconGroup(), p["nicon"]);
	}
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
				Widget* p0 = this;
				Widget* p1 = target;
				for(int i=0; i<m_dragMode; ++i) {
					p0 = p0->getParent();
					p1 = p1->getParent();
					if(p0 == p1) {
						target->setChecked(isChecked());
						if(target->eventChanged) target->eventChanged(target);
						break;
					}
				}
			}
		}
	}
}

// ===================================================================================== //

void DragHandle::initialise(const Root*, const PropertyMap& p) {
	m_mode = p.getEnum("mode", {"move", "size", "order"}, MOVE);
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
	if(d) {
		m_held = p;
		m_target = getParent();
		while(m_target->isTemplate()) m_target = m_target->getParent();
		if(m_mode==ORDER && !m_target->getParent()->getLayout()) m_mode = MOVE;
		if(m_mode==MOVE && m_target->getParent()->getLayout()) m_mode = ORDER;
		m_index = m_target->getIndex();
	}
	if(u && m_target) {
		if(m_mode == ORDER) m_target->getParent(true)->add(m_target, m_index);
		m_target->getParent()->resumeLayout();
		if(eventEndDrag) eventEndDrag(m_target, m_mode);
		m_target = nullptr;
	}
}

void DragHandle::onMouseMove(const Point& last, const Point& pos, int b) {
	if(!b || !hasMouseFocus() || !m_target) return;
	Point delta = pos - m_held;
	Widget* parent = m_target->getParent(true);
	const Point& view = parent->getSize();
	if(m_mode == MOVE) {
		Point p = m_target->getPosition() + delta;
		if(m_clamp) p = Rect(0, 0, view - m_target->getSize()).clamp(p);
		if(p!=m_target->getPosition()) m_target->setPosition(p);
	}
	else if(m_mode == ORDER) {
		m_target->getParent()->pauseLayout();
		m_target->raise();
		Point p = m_target->getPosition() + delta;
		int m = parent->getLayout()->getMargin();
		
		// Figure out our new index
		int index = 0;
		Rect t(p, m_target->getSize());
		for(Widget* w: parent) {
			const Rect& r = w->getRect();
			if(r.bottom() <= t.bottom()) ++index;
			else if(r.right() <= t.right() && r.top() <= t.top()) ++index;
			else break;
		}
		if(index != m_index) {
			m_index = index;
			parent->add(m_target, index);
			parent->refreshLayout();
			setMouseFocus();
			m_target->getParent(true)->pauseLayout();
			m_target->raise();
		}

		if(m_clamp) p = Rect(m, m, view - m_target->getSize() - Point(m*2, m*2)).clamp(p);
		if(p!=m_target->getPosition()) m_target->setPosition(p);
	}
	else if(m_mode == SIZE) {
		Point p = m_target->getPosition();
		Point size = m_target->getSize();
		const Rect& targetRect = m_target->getRect();
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
				size[axis] += delta[axis];
				int minSize = targetRect.size()[axis] - getPosition()[axis];
				int maxSize = view[axis] - targetRect.position()[axis];
				if(size[axis] < minSize) size[axis] = minSize;
				if(m_clamp && size[axis] > maxSize) size[axis] = maxSize;
			}
		};
		resize(0);
		resize(1);

		if(p != m_target->getPosition()) m_target->setPosition(p);
		if(size != m_target->getSize()) m_target->setSize(size);
	}
}


// ===================================================================================== //

ProgressBar::ProgressBar(Orientation o) : m_min(0), m_max(1), m_value(1), m_borderLow(0), m_borderHigh(0), m_mode(o), m_progress(0) {}
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

template<typename T> SpinboxT<T>::SpinboxT(const char* format) 
	: m_text(0), m_value(0), m_min(0), m_max(100), m_buttonStep(1), m_wheelStep(1), m_textChanged(false), m_format(format) {
}
template<typename T> void SpinboxT<T>::initialise(const Root*, const PropertyMap& p) {
	p.readValue("value", m_value);
	p.readValue("min", m_min);
	p.readValue("max", m_max);
	if(p.readValue("step", m_buttonStep)) m_wheelStep = m_buttonStep;

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
		m_max = s->m_max;
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
Spinbox::Spinbox() : SpinboxT("%d%n") { }
SpinboxFloat::SpinboxFloat() : SpinboxT("%g%n") { }
void Spinbox::fireChanged() { if(eventChanged) eventChanged(this, m_value); }
void SpinboxFloat::fireChanged() { if(eventChanged) eventChanged(this, m_value); }


// ===================================================================================== //


Scrollbar::Scrollbar(Orientation orientation, int min, int max) 
	: m_mode(orientation), m_range(min, max), m_value(min), m_step(1), m_block(0)
{
	if(orientation==VERTICAL) m_rect.set(0,0,16,64);
	else m_rect.set(0,0,64,16);
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

Scrollpane::Scrollpane() : m_alwaysShowScrollbars(true), m_useFullSize(true), m_vScroll(0), m_hScroll(0) {
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
		Widget* w = new Widget();
		w->setSize(m_rect.size());
		w->setAsTemplate();
		w->setTangible(getTangible());
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

Slider::Slider() {
}

void Slider::initialise(const Root*, const PropertyMap& p) {
	p.readValue("min", m_range.min);
	p.readValue("max", m_range.max);
	m_block[0] = getTemplateWidget("_block");
	m_block[1] = getTemplateWidget("_block2");
	if(m_block[0] && m_block[1]) m_block[2] = getTemplateWidget("_fill");
	m_orientation = p.getEnum("orientaton", {"horizontal", "vertical"}, HORIZONTAL);
	for(Widget* w: m_block) {
		if(w) {
			w->eventMouseDown.bind(this, &Slider::pressBlock);
			w->eventMouseMove.bind(this, &Slider::moveBlock);
		}
	}
	if(p.contains("value")) {
		float v = p.getValue("value", m_value.min);
		setValue(v, p.getValue("value2", v));
	}
}
void Slider::copyData(const Widget* from) {
	if(const Slider* s = cast<Slider>(from)) {
		m_orientation = s->m_orientation;
		m_quantise = s->m_quantise;
		m_range = s->m_range;
		m_value = s->m_value;
	}
}
void Slider::setSize(int w, int h) {
	Super::setSize(w, h);
	updateBlock();
}
void Slider::setRange(float min, float max) {
	m_range.min = min;
	m_range.max = fmax(min, max);
	updateBlock();
}
void Slider::setQuantise(float step) {
	m_quantise = fmax(0, step);
	setValue(m_value);
}
void Slider::setValue(float min, float max, bool trigger) {
	auto clamp = [this](float v) { return v<m_range.min? m_range.min: v>m_range.max? m_range.max: v; };
	min = clamp(min);
	max = clamp(max);
	if(max < min || !m_block[1]) max = min;
	if(m_quantise>0) {
		min = floor(min/m_quantise+0.5)*m_quantise;
		max = floor(max/m_quantise+0.5)*m_quantise;
	}
	if(min==m_value.min && max==m_value.max) return;
	m_value.min = min;
	m_value.max = max;
	updateBlock();
	if(trigger && eventChanged) eventChanged(this, m_value);
}
void Slider::setValue(float v, bool trigger) { setValue(v,v,trigger); }

bool Slider::onMouseWheel(int w) {
	float diff = m_quantise? w*m_quantise: w * (m_range.max - m_range.min) * 0.05;
	setValue(m_value.min+diff, m_value.max+diff, true);
	return true;
}

void Slider::pressBlock(Widget*, const Point& p, int b) {
	if(m_range.max == m_range.min) return;
	m_held = p[m_orientation];
}

void Slider::moveBlock(Widget* c, const Point& p, int b) {
	if(b && m_held>=0) {
		int pmin=0, pmax=getSize()[m_orientation];
		if(c!=m_block[0]) pmin += m_block[0]->getSize()[m_orientation];
		if(m_block[1] && c!=m_block[1]) pmax -= m_block[1]->getSize()[m_orientation];
		if(pmax <= pmin) return; // error

		int np = p[m_orientation] - m_held + c->getPosition()[m_orientation];
		float normalisedValue = (float)(np - pmin) / (pmax-pmin);
		float value = m_range.min + normalisedValue * (m_range.max-m_range.min);
		if(value<m_range.min) value = m_range.min;
		if(value>m_range.max) value = m_range.max;

		if(c==m_block[0]) setValue(value, fmax(value, m_value.max), true);
		else if(c==m_block[1]) setValue(fmin(value, m_value.min), value, true);
		else if(c==m_block[2]) {
			float separation = m_value.max - m_value.min;
			if(value > m_range.max - separation) value = m_range.max - separation;
			setValue(value, value+separation, true);
		}
	}
	else m_held = -1;
}

void Slider::updateBlock() {
	int w = getSize()[m_orientation];
	if(m_block[0]) {
		int pmax = w - m_block[0]->getSize()[m_orientation];
		if(m_block[1]) pmax -= m_block[1]->getSize()[m_orientation];
		float p = (m_value.min - m_range.min) / (m_range.max - m_range.min);
		Point pos = m_block[0]->getPosition();
		pos[m_orientation] = p * pmax;
		m_block[0]->setPosition(pos);
	}

	if(m_block[1]) {
		int pmin = m_block[0]? m_block[0]->getSize()[m_orientation]: 0;
		int pmax = w - m_block[1]->getSize()[m_orientation];
		float p = (m_value.max - m_range.min) / (m_range.max - m_range.min);
		Point pos = m_block[1]->getPosition();
		pos[m_orientation] = pmin + p * (pmax-pmin);
		m_block[1]->setPosition(pos);
	}

	if(m_block[2]) {
		Point s = m_block[2]->getSize();
		Point p = m_block[2]->getPosition();
		p[m_orientation] = m_block[0]->getRect().bottomRight()[m_orientation];
		s[m_orientation] = m_block[1]->getPosition()[m_orientation] - p[m_orientation];
		m_block[2]->setPosition(p);
		m_block[2]->setSize(s);
	}
}



// ===================================================================================== //


TabbedPane::TabbedPane() : m_buttonTemplate(0), m_tabStrip(0), m_tabFrame(0), m_currentTab(-1), m_autoSizeTabs(true) {}
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
	if(!frame) frame = new Widget();
	if(m_tabFrame != this) m_tabFrame->add(frame, index);
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

CollapsePane::CollapsePane() : m_collapsed(false), m_expandAnchor(0) {}
void CollapsePane::initialise(const Root* root, const PropertyMap& p) {
	m_client = getTemplateWidget("_client");
	m_header = getTemplateWidget<Button>("_header");
	m_check = getTemplateWidget<Checkbox>("_check");
	m_stateWidget = getTemplateWidget("_state");
	if(cast<Checkbox>(m_stateWidget) && m_stateWidget->isTangible()) cast<Checkbox>(m_stateWidget)->eventPressed.bind(this, &CollapsePane::toggle);
	else if(Checkbox* c = cast<Checkbox>(m_header)) c->eventChanged.bind(this, &CollapsePane::toggle);
	else if(m_header) m_header->eventPressed.bind(this, &CollapsePane::toggle);
	if(m_check) m_check->eventChanged.bind(this, &CollapsePane::checkChanged);
	if(!m_client) m_client = this;

	const char* caption = p["caption"];
	if(caption) setCaption(caption);
	m_collapsed = p.getValue("collapsed", 0);
	m_expandAnchor = m_anchor;
	m_clientBorder = Rect(m_client->getPosition(), getSize() - m_client->getSize());
	expand(!m_collapsed);
}
void CollapsePane::copyData(const Widget* from) {
	if(const CollapsePane* c = cast<CollapsePane>(from)) {
		m_expandAnchor = c->m_expandAnchor;
		m_collapsed = c->m_collapsed;
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
		m_client->setRect(m_clientBorder.x, m_clientBorder.y, m_rect.width - m_clientBorder.width, m_rect.height - m_clientBorder.height);
		if(m_expandAnchor) m_anchor = m_expandAnchor;
	}
	else if(m_header) {
		m_clientBorder = Rect(m_client->getPosition(), getSize() - m_client->getSize());
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
	
	if(m_stateWidget) m_stateWidget->setSelected(e);
	if(Widget* parent = getParent()) parent->refreshLayout();
}
void CollapsePane::updateAutosize() {
	Point s = getSize();
	if(!m_collapsed) Widget::updateAutosize();
	if(s != getSize()) {
		if(Widget* parent = getParent()) if(!parent->isLayoutPaused()) parent->refreshLayout();
	}
}
void CollapsePane::onChildChanged(Widget* w) {
	if(isExpanded()) Super::onChildChanged(w);
	else if(!isLayoutPaused()) refreshLayout();
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

// ===================================================================================== //

SplitPane::SplitPane(Orientation o) : m_mode(o), m_held(-100), m_minSize(0), m_resizeMode(ALL), m_sash(0) {
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
		m_sash = new Widget();
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



Window::Window() : m_minSize(80,40) {
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

void Window::onChildMouseDown(Widget* c, int b) {
	if(b==1) raise();
	Super::onChildMouseDown(c, b);
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


Popup::Popup() {
}
Popup::Popup(int width, int height, bool destroyOnClose) : Widget(width, height), m_destroyOnClose(destroyOnClose) {
}
Popup::Popup(Widget* child, bool destroyOnClose) : m_destroyOnClose(destroyOnClose) {
	setSize(child->getSize());
	add(child);
}
Popup::~Popup() {
	for(Widget* w: m_owned) delete w;
}
void Popup::initialise(const Root*, const PropertyMap& p) {
	setVisible(false);
}
void Popup::popup(Root* root, const Point& abs, Widget* owner) {
	if(m_parent == root->getRootWidget()) setPosition(abs);
	else {
		if(m_parent) m_parent->remove(this);
		root->getRootWidget()->add(this, abs.x, abs.y);
	}
	setVisible(true);
	raise();
	Widget* ownerPopup = owner;
	while(ownerPopup && !cast<Popup>(ownerPopup)) ownerPopup = ownerPopup->getParent();
	if(ownerPopup) {
		if(m_owner != ownerPopup) {
			assert(!m_owner); // FIXME Do we need to handle submenus popping up elsewhere
			cast<Popup>(ownerPopup)->m_owned.push_back(this);
		}
		m_owner = ownerPopup;
	}
	else {
		assert(!m_owner);
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

void Popup::addOwnedPopup(Popup* p) {
	assert(!p->m_owner);
	p->m_owner = this;
	m_owned.push_back(p);
}

void Popup::hideOwnedPopups() {
	for(Popup* w: m_owned) w->hide();
}

void Popup::hide() {
	hideOwnedPopups();
	setVisible(false);
	if(m_destroyOnClose) delete this;
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
		for(Popup* w: m_owned) if(w!=newPopup) w->hide();
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

ScaleBox::ScaleBox() {
	Widget* client = new Widget();
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

