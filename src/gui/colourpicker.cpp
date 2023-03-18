#include <base/gui/colourpicker.h>
#include <base/gui/skin.h>
#include <base/gui/renderer.h>
#include <base/gui/widgets.h>
#include <base/colour.h>

using namespace gui;

ColourPicker::ColourPicker(const Rect& r, Skin* s) : Widget(r, s) {
}

ColourPicker::~ColourPicker() {
}

void ColourPicker::initialise(const Root* root, const PropertyMap& p) {
	// Generate subwidgets
	if(root && getWidgetCount() == 0) {
		IconList* images = root->getIconList("colourpicker");
		if(!images) images = createImages(const_cast<Root*>(root), 128);

		int size = getSize().y;
		int bar = size / 8 + 4;
		m_wheel = new Icon(Rect(0,0,size,size), m_skin);
		m_value = new Icon(Rect(size+4,0,bar,size), m_skin);
		m_alpha = new Icon(Rect(0,0,bar,size), m_skin);
		Icon* checker = new Icon(Rect(size+bar+8,0,bar,size), m_skin);

		m_wheelMark = new Icon(Rect(0,0,3,3), m_skin);
		m_valueMark = new Icon(Rect(0,0,bar,1), m_skin);
		m_alphaMark = new Icon(Rect(0,0,bar,1), m_skin);

		m_wheel->setIcon(images, 0);
		m_wheel->setAnchor("tlrb");
		m_wheel->setTangible(Tangible::ALL);
		m_wheel->setAsTemplate();
		m_value->setIcon(images, 1);
		m_value->setAnchor("trb");
		m_value->setTangible(Tangible::ALL);
		m_value->setAsTemplate();
		m_alpha->setIcon(images, 2);
		m_alpha->setAnchor("tlrb");
		m_alpha->setTangible(Tangible::ALL);
		checker->setIcon(images, 3);
		checker->setAnchor("trb");
		checker->setAsTemplate();
		checker->setTangible(Tangible::ALL);
		m_wheelMark->setIcon(images, 4);
		m_valueMark->setIcon(images, 4);
		m_alphaMark->setIcon(images, 4);

		add(m_wheel);
		add(m_value);
		m_wheel->add(m_wheelMark);
		m_value->add(m_valueMark);
		if(m_alpha) {
			add(checker);
			checker->add(m_alpha);
			m_alpha->add(m_alphaMark);
		}

		m_wheel->eventMouseDown.bind(this, &ColourPicker::changeWheel);
		m_wheel->eventMouseMove.bind(this, &ColourPicker::changeWheel);
		m_value->eventMouseDown.bind(this, &ColourPicker::changeValue);
		m_value->eventMouseMove.bind(this, &ColourPicker::changeValue);
		if(m_alpha) {
			m_alpha->eventMouseDown.bind(this, &ColourPicker::changeAlpha);
			m_alpha->eventMouseMove.bind(this, &ColourPicker::changeAlpha);
		}
		setColour(0xffffff);
	}
}

IconList* ColourPicker::createImages(Root* root, int size) {
	int barSize = size / 8;
	int line = size + barSize * 3;
	// Circle
	Colour colour;
	float centre = size / 2.f;
	uint* imageData = new uint[size*line];
	for(int x=0; x<size; ++x) {
		for(int y=0; y<size; ++y) {
			uint& out = imageData[x+y*line];
			float dx = x - centre;
			float dy = y - centre;
			float det = dx*dx + dy*dy;
			if(det > centre*centre) out = 0;
			else {
				float sat = sqrt(det) / centre;
				float hue = atan2(dx, dy) / TWOPI;
				colour.fromHSV(hue, fmin(sat,1), 1);
				colour = Colour(colour.b, colour.g, colour.r); // Needs ABGR
				out = colour.toARGB();
			}
		}
	}

	// value bar - white to black
	colour.a = 1;
	uint* barData = imageData + size;
	for(int i=0; i<size; ++i) {
		colour.r = colour.g = colour.b = 1 - (float)i/size;
		uint c = colour.toARGB();
		for(int j=0; j<barSize; ++j) barData[j + i*line] = c;
	}

	// Alpha bar - white to alpha
	colour.r = colour.g = colour.b = 1;
	barData = imageData + size + barSize;
	for(int i=0; i<size; ++i) {
		colour.a = 1 - (float)i/size;
		uint c = colour.toARGB();
		for(int j=0; j<barSize; ++j) barData[j + i*line] = c;
	}

	// Checker bar - alpha background
	uint check[2] = { 0xff606060, 0xffa0a0a0 };
	barData = imageData + size + barSize * 2;
	for(int i=0; i<size; ++i) {
		for(int j=0; j<barSize; ++j) {
			barData[j + i*line] = check[((i>>2)&1) ^ ((j>>2)&1)];
		}
	}
	// Create the skins
	int tex = root->getRenderer()->addImage("colourpicker", line, size, 4, imageData);
	IconList* images = new IconList();
	images->setImageIndex(tex);
	Rect bar(size+1, 1, barSize-2, size-2);
	images->addIcon("wheel", Rect(0,0,size,size));
	images->addIcon("value", bar); bar.x += barSize;
	images->addIcon("alpha", bar); bar.x += barSize;
	images->addIcon("checker", bar);
	images->addIcon("dot", Rect(size+1, 1, 1, 1));
	root->addIconList("colourpicker", images);
	return images;
}

void ColourPicker::setColour(const Colour& c) {
	m_colour = c;
	HSV hsv = c.toHSV();
	if(m_wheel) m_wheel->setColour(Colour(hsv.value, hsv.value, hsv.value));
	if(m_value)	m_value->setColour(Colour().fromHSV(hsv.hue, hsv.saturation, 1));
	if(m_alpha) m_alpha->setColour(m_colour.toRGB());
	if(m_alpha) m_alpha->setColour(m_colour.toRGB());

	if(m_valueMark) {
		m_valueMark->setPosition(0, (1-hsv.value) * m_value->getSize().y);
		m_valueMark->setColour(hsv.value<0.5? 0xffffff: 0x000000);
	}
	if(m_alphaMark) {
		m_alphaMark->setPosition(0, (1-c.a) * m_alpha->getSize().y);
		m_alphaMark->setColour(hsv.value<0.5? 0xffffff: 0x000000);
	}
	if(m_wheelMark) {
		const Point& size = m_wheel->getSize();
		float x = sin(hsv.hue*TWOPI) * hsv.saturation * size.x/2.f + size.x/2.f;
		float y = cos(hsv.hue*TWOPI) * hsv.saturation * size.y/2.f + size.y/2.f;;
		m_wheelMark->setPosition((int)x-1, (int)y-1);
		m_wheelMark->setColour(hsv.value<0.5? 0xffffff: 0x000000);
	}
}

const Colour& ColourPicker::getColour() const {
	return m_colour;
}

void ColourPicker::changeWheel(Widget* w, const Point& p, int b) {
	if(b==1) {
		Point size = w->getSize();
		float aspect = (float)size.x / size.y;
		float offset = size.x / 2.f;
		float dx = p.x - offset;
		float dy = (p.y - offset) * aspect;
		float sat = sqrt(dx*dx + dy*dy) / offset;
		float hue = atan2(dx, dy) / TWOPI;
		float value = m_colour.toHSV().value;
		m_colour.fromHSV(hue, fmin(1,sat), value>0? value: 0.01, m_colour.a);
		setColour(m_colour);
		if(eventChanged) eventChanged(m_colour);
	}
}

void ColourPicker::changeValue(Widget* w, const Point& p, int b) {
	if(b==1) {
		float value = 1 - (float)p.y / w->getSize().y;
		value = value<0? 0: value>1?1: value;
		HSV hsv = m_colour.toHSV();
		m_colour.fromHSV(hsv.hue, hsv.saturation, value, m_colour.a);
		setColour(m_colour);
		if(eventChanged) eventChanged(m_colour);
	}
}

void ColourPicker::changeAlpha(Widget* w, const Point& p, int b) {
	if(b==1) {
		float alpha = 1 - (float)p.y / w->getSize().y;
		m_colour.a= alpha<0? 0: alpha>1?1: alpha;
		setColour(m_colour);
		if(eventChanged) eventChanged(m_colour);
	}
}


