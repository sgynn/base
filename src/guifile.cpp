#include <base/gui.h>
#include <base/xml.h>
#include <base/png.h>
#include <base/font.h>
#include <base/material.h>
#include <base/hashmap.h>
#include <cstdio>

typedef base::gui::Style Style;

// Create functions
namespace base {
	namespace gui {
		Control* createContainer(const Loader& e) {
			int width = e.attribute("width", 0);
			int height= e.attribute("height", 0);
			Container* c;
			c = new Container(0, 0, width, height, width&&height);
			return c;
		}
		Control* createFrame(const Loader& e) {
			int width = e.attribute("width", 0);
			int height= e.attribute("height", 0);
			const char* caption = e.attribute("caption", (const char*)0);
			int moveable = e.attribute("moveable", 0);
			// Create object
			Frame* c = new Frame(width, height, caption, moveable, e.style());
			return c;
		}
		Control* createLabel(const Loader& e) {
			const char* caption = e.attribute("caption", (const char*)0);
			Label* c = new Label(caption, e.style());
			return c;
		}
		Control* createButton(const Loader& e) {
			int width = e.attribute("width", 0);
			int height= e.attribute("height", 0);
			const char* caption = e.attribute("caption", (const char*)0);
			Style* style = e.style();
			int event = e.attribute("event", 0);
			// Create object
			Button* c;
			if(width && height) c = new Button(width, height, caption, style, event);
			else c = new Button(caption, style, event);
			return c;
		}
		// ...
		Control* createCheckbox(const Loader& e) {
			int size = e.attribute("size", 16);
			int value = e.attribute("value", 0);
			const char* caption = e.attribute("caption", (const char*)0);
			Style* style = e.style();
			int event = e.attribute("event", 0);
			// Create object
			Checkbox* c = new Checkbox(size, caption, value, style, event);
			return c;
		}
		Control* createSlider(const Loader& e, int o) {
			float min = e.attribute("min", 0.0f);
			float max = e.attribute("max", 1.0f);
			float step = e.attribute("step", .0f);
			float value = e.attribute("value", min);
			int width = e.attribute("width", 0);
			int height= e.attribute("height", 0);
			int event = e.attribute("event", 0);
			if(width==0 && o==Slider::VERTICAL) width=18;
			else if(height==0 && o==Slider::HORIZONTAL) height=18;
			Slider* c = new Slider(o, width, height, e.style(), event);
			c->setRange(min, max, step);
			c->setValue(value);
			return c;
		}
		Control* createSliderV(const Loader& e) { return createSlider(e,Slider::VERTICAL); }
		Control* createSliderH(const Loader& e) { return createSlider(e,Slider::HORIZONTAL); }

		Control* createScrollbar(const Loader& e, int o) {
			int min = e.attribute("min", 0);
			int max = e.attribute("max", 10);
			int value = e.attribute("value", min);
			int width = e.attribute("width", 0);
			int height= e.attribute("height", 0);
			int event = e.attribute("event", 0);
			if(width==0 && o==Scrollbar::VERTICAL) width=16;
			else if(height==0 && o==Scrollbar::HORIZONTAL) height=16;
			Scrollbar* c = new Scrollbar(o, width, height, min, max, e.style(), event);
			c->setValue(value);
			return c;
		}
		Control* createScrollbarV(const Loader& e) { return createScrollbar(e,Scrollbar::VERTICAL); }
		Control* createScrollbarH(const Loader& e) { return createScrollbar(e,Scrollbar::HORIZONTAL); }

		Control* createListbox(const Loader& e) {
			int width = e.attribute("width", 0);
			int height= e.attribute("height", 0);
			int event = e.attribute("event", 0);
			Listbox* list = new Listbox(width, height, e.style(), event);
			// Child items
			for(XML::iterator i=e->begin(); i!=e->end(); ++i) {
				if(strcmp(i->name(), "item")==0) {
					const char* itm = i->text()? i->text(): i->attribute("name");
					list->addItem(itm, i->attribute("selected",0)>0);
				}
			}
			return list;
		}
		Control* createDropList(const Loader& e) {
			int max   = e.attribute("max", 0);
			int width = e.attribute("width", 0);
			int height= e.attribute("height", 0);
			int event = e.attribute("event", 0);
			DropList* list = new DropList(width, height, max, e.style(), event);
			// Child items
			for(XML::iterator i=e->begin(); i!=e->end(); ++i) {
				if(strcmp(i->name(), "item")==0) {
					const char* itm = i->text()? i->text(): i->attribute("name");
					list->addItem(itm, i->attribute("selected",0)>0);
				}
			}
			return list;
		}
		Control* createInput(const Loader& e) {
			const char* text = e.attribute("text", "");
			int width = e.attribute("width", 0);
			int height= e.attribute("height", 0);
			char mask = *e.attribute("mask", "");
			int event = e.attribute("event", 0);
			gui::Input* c = new gui::Input(width, height, text, e.style(), event);
			c->setMask(mask);
			return c;
		}

	}
}

using namespace base;
using namespace gui;

//// Static types map ////
HashMap<Control*(*)(const Loader&)> Loader::s_types;
void Loader::addType(const char* name, Control*(*c)(const Loader&)) { s_types[name] = c; }

//// Accessor functions ////
const char* Loader::attribute(const char* name, const char* d) const { return m_item->attribute(name, d); }
float       Loader::attribute(const char* name, float d) const       { return m_item->attribute(name, d); }
int         Loader::attribute(const char* name, int d) const         { return m_item->attribute(name, d); }
const XML::Element* Loader::operator->() const                       { return m_item; }
Style* Loader::style() const {
	// Find Style as a child object
	for(XML::iterator i=m_item->begin(); i!=m_item->end(); ++i) {
		if(i->name() && strcmp("style", i->name())==0) return const_cast<Loader*>(this)->readStyle(*i);
	}
	// Find style by 'style' attribute
	const char* name = attribute("style", (const char*)0);
	if(name) return getStyle(name);
	else if(m_parent) return m_parent->getStyle();
	return 0;
}
Style* Loader::getStyle(const char* name) const {
	if(m_styles.contains(name)) return m_styles[name];
	else return 0;
}
Control* Loader::getControl(const char* name) const {
	return m_parent->getControl(name);
}

/** Parse a hex value from string */
uint Loader::parseHex(const char* s) {
	if(*s=='#') ++s; // Allow html notation
	if(s[0]=='0' && s[1]=='x') s+=2; // Allow 0x00 notation
	int hex=0;
	for(const char* c=s; *c; ++c) {
		if(*c>='0' && *c<='9') hex = (hex<<4) + *c-'0';
		else if(*c>='a' && *c<='f') hex = (hex<<4) + *c-'a'+10;
		else if(*c>='A' && *c<='F') hex = (hex<<4) + *c-'A'+10;
		else break;
	}
	return hex;
}


/** Loader constructor */
Loader::Loader() {
	static bool initialised = false;
	if(!initialised) {	
		// Register construct functions
		addType("container", base::gui::createContainer);
		addType("frame",     base::gui::createFrame);
		addType("label",     base::gui::createLabel);
		addType("button",    base::gui::createButton);
		addType("checkbox",  base::gui::createCheckbox);
		addType("slider",    base::gui::createSliderH);
		addType("hslider",   base::gui::createSliderH);
		addType("vslider",   base::gui::createSliderV);
		addType("hscroll",   base::gui::createScrollbarH);
		addType("vscroll",   base::gui::createScrollbarV);
		addType("listbox",   base::gui::createListbox);
		addType("droplist",  base::gui::createDropList);
		addType("input",     base::gui::createInput);
		initialised = true;
	}
}



// Get the array index of a string //
inline int stringIndex(const char* s, const char** list, int max) {
	for(int i=0; i<max; ++i) if(strcmp(s,list[i])==0) return i;
	return -1;
}
// Get array bitmask of a string //
int stringMask(const char* s, const char** list, int max) {
	int mask = 0;
	const char* c=s;
	while(++c) {
		if(c>s && (*c==',' || *c==0)) {
			int m=0, len = c-s-1;
			for(m=0; m<max; ++m) if(strncmp(s, list[m], len)==0) break;
			if(m<max) mask |= 1<<m;
			if(*c==0) break; else s=c+1;
		}
	}
	return mask;
}

// Read a hex + alpha colour from xml //
Colour Loader::readColour(const XML::Element& e) const {
	const char* s = e.attribute("colour", "ffffff");
	Colour c = Loader::parseHex(s);
	c.a = e.attribute("alpha", 1.0f);
	return c;
}

// Read style from xml //
Style* Loader::readStyle(const XML::Element& e) {
	Style* style = 0;
	// Get style base
	const char* extend = e.attribute("extend");
	if(m_styles.contains(extend)) style = m_styles[extend];
	// Add attributes
	if(e.size()>0) {
		if(style) style = new Style(style);
		else style = new Style();
		int type;
		const char* s;
		static const char* values[] = { "sprite", "colour", "frame", "font", "tween" };
		static const char* ctypes[] = { "back", "border", "text" };
		static const char* cmodes[] = { "base", "over", "focus", "disabled" };
		static const char* aligns[] = { "left", "centre", "right" };
		for(XML::iterator i=e.begin(); i!=e.end(); ++i) {
			type = stringIndex(i->name(), values, 4);
			switch(type) {
			case 0: // Sprite
				s = i->attribute("file");
				if(s) {
					Texture* t = loadTexture(s);
					if(!t) break;
					int rows = i->attribute("rows", 1);
					int cols = i->attribute("cols", 1);
					int border = i->attribute("border", 0);
					Sprite sprite(*t, rows, cols);
					if(border>0) sprite.setBorder(border);
					style->setSprite( sprite );
				}
				break;
			case 1: // Colour
				type = stringIndex(i->attribute("type"), ctypes, 3);
				if(type>=0) {
					Colour colour = readColour(*i);
					// What mode or modes?
					s = i->attribute("state","base,over,focus,disabled");
					int states = stringMask(s, cmodes, 4);
					if(!states) printf("GUI Error: invalid states '%s'\n", s);
					if(states) style->setColour(type, states, colour);
				} else printf("Invalid colour type %s\n", i->attribute("type"));
				break;
			case 2: // Frame
				// Similar to colour
				s = i->attribute("state","base,over,focus,disabled");
				type = stringMask(s, cmodes, 4);
				if(!type) printf("GUI Error: invalid states '%s'\n", s);
				if(type) style->setFrame(type, i->attribute("value", 0));
				break;
			case 3: // Font
				type = stringIndex(i->attribute("align", "left"), aligns, 3);
				if(i->hasAttribute("file")) {
					Font* font = loadFont( i->attribute("file") );
					style->setFont( font, type);
				} else style->setAlign(type);
				break;
			case 4: // tween
				style->setFade( i->attribute("time", 0.0f) );
				break;
			}
		}
	}
	return style;
}

// Load sprite texture from file //
Texture* Loader::loadTexture(const char* file) {
	static const int fmt[] = { 0, Texture::LUMINANCE, Texture::LUMINANCE_ALPHA, Texture::RGB, Texture::RGBA };
	HashMap<Texture*>::iterator it = m_textures.find(file);
	if(it==m_textures.end()) {
		PNG png = PNG::load(file);
		if(!png.data) return 0;
		Texture* t = new Texture( Texture::create(png.width, png.height, fmt[png.bpp/8], png.data) );
		m_textures[file] = t;
		return t;
	} else return *it;
}
// Load a font //
Font* Loader::loadFont(const char* file) {
	HashMap<Font*>::iterator it = m_fonts.find(file);
	if(it==m_fonts.end()) {
		Font* f = new Font(file);
		m_fonts[file] = f;
		return f;
	} else return *it;
}

// Build controls //
int Loader::addControls(const base::XML::Element& e, base::gui::Container* parent) {
	int count = 0;
	for(XML::iterator i = e.begin(); i!=e.end(); ++i) {
		// Read controls
		if(i->type() == XML::COMMENT) {
		} else if(strcmp(i->name(), "style")==0) {
			// Only read style if in root node
			if(parent->getParent()==0) {
				Style* style = readStyle(*i);
				if(i->hasAttribute("name")) m_styles.insert(i->attribute("name"), style);
			}
		} else if(s_types.contains(i->name())) {
			m_item = &(*i);
			m_parent = parent;
			Control* c = s_types[i->name()](*this);
			// Set general values
			if(!attribute("visible", 1)) c->hide();
			Point cp = c->getAbsolutePosition(); // generally (0,0)
			int x = attribute("x", cp.x);
			int y = attribute("y", cp.y);
			parent->add( c, x, y);
			++count;
			// Add to map
			if(i->hasAttribute("name")) parent->m_root->m_names[i->attribute("name")] = c;
			// Recurse to children
			if(c->isContainer()) {
				count += addControls(*i, static_cast<Container*>(c));
			}
		} else {
			printf("Error: No such control type '%s'\n", i->name());
		}
	}
	return count;
}


Container* Loader::load(const char* file) {
	Loader loader;
	base::XML xml = base::XML::load(file);
	Container* base = new Container();
	base->setRoot( new Root(base) );
	int c = loader.addControls(xml.getRoot(), base);
	if(c) return base;
	else {
		delete base;
		return 0;
	}
}
Container* Loader::parse(const char* string) {
	Loader loader;
	base::XML xml = base::XML::parse(string);
	Container* base = new Container();
	base->setRoot( new Root(base) );
	int c = loader.addControls(xml.getRoot(), base);
	if(c) return base;
	else {
		delete base;
		return 0;
	}

}

