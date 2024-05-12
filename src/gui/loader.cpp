#include <base/gui/gui.h>
#include <base/gui/skin.h>
#include <base/gui/font.h>
#include <base/gui/widgets.h>
#include <base/gui/layouts.h>
#include <base/gui/renderer.h>
#include <base/xml.h>
#include <cstdlib>
#include <cstdio>

using namespace base;
using namespace gui;


namespace gui {
	template<typename T> T parseValue(const char* s, char** e, int r=0) { return strtol(s,e,r); }
	template<> float parseValue<float>(const char* s, char** e, int r) { return strtof(s,e); }
	template<typename T>
	int parseValues(const char* s, int n, T* v) {
		if(!s||!*s) return 0;
		char* e;
		for(int i=0; i<n; ++i) {
			v[i] = parseValue<T>(s, &e);
			if(s==e) return i;
			s = e;
		}
		return n;
	}
	Rect parseRect(const char* s, const Rect& defaultValue) {
		Rect r = defaultValue;
		if(!parseValues(s, 4, &r.x)) r = defaultValue;
		return r;
	}
	Skin::Border parseBorder(const char* s, const Skin::Border& defaultValue) {
		Skin::Border r = defaultValue;
		int n = parseValues(s, 4, &r.left);
		if(n==1) r.right = r.bottom = r.top = r.left;
		else if(n==2) { r.right=r.left; r.bottom=r.top; }
		return r;
	}
	Point parsePoint(const char* s, const Point& defaultValue=Point()) {
		Point r = defaultValue;
		int n = parseValues(s, 2, &r.x);
		if(n==1) r.y = r.x;
		return r;
	}

	template<int N>
	int parseEnum(const char* s, const char* const (&array)[N], int defaultvalue=-1) {
		for(int i=0; i<N; ++i) if(strcmp(s, array[i])==0) return i;
		return defaultvalue;
	}

	int parseSkinState(const char* s) {
		const char* names[] = { "normal", "highlight", "pressed", "disabled" };
		int state = -1;
		for(int i=0; i<4; ++i) {
			if(strstr(s, names[i])) { state = i; break; }
		}
		if(strstr(s, "selected")) state = state<0? 4: state|4;
		return state;
	}

	uint parseColour(const char* data, uint fallback=0xffffffff) {
		if(!data[0]) return fallback;
		char* end;
		if(data[0]=='#') {
			uint v = strtoul(data+1, &end, 16);
			if(end <= data + 7) v |= 0xff000000; // Full opacity if alpha omitted
			return v;
		}
		// rgba
		int rgba[4] = { 0,0,0,255 };
		if(parseValues(data, 4, rgba) > 2) {
			return rgba[0] | rgba[1]<<8 | rgba[2]<<16 | rgba[3]<<24;
		}
		return fallback;
	}

	inline int operator~(LoadFlags f) { return ~(int)f; }
	inline bool operator&(int a, LoadFlags b) { return a&(int)b; }
}


void Root::addFont(const char* name, Font* font)         { m_fonts[name] = font; }
void Root::addSkin(const char* name, Skin* skin)         { m_skins[name] = skin; }
void Root::addIconList(const char* name, IconList* list) { m_iconLists[name] = list; }

Widget* Root::load(const char* file, Widget* root, LoadFlags flags) {
	printf("Loading gui %s\n", file);
	XML xml = XML::load(file);
	return load(xml.getRoot(), root, flags);
}
Widget* Root::parse(const char* string, Widget* root, LoadFlags flags) {
	printf("Loading gui\n");
	XML xml = XML::parse(string);
	return load(xml.getRoot(), root, flags);
}

Widget* Root::load(const XMLElement& xmlRoot, Widget* root, LoadFlags flags) {
	Widget* first = 0;
	if(!root) root = m_root;

	Widget* rescale = 0;
	Point baseSize = root->getSize();
	baseSize.x = xmlRoot.attribute("width", baseSize.x);
	baseSize.y = xmlRoot.attribute("height", baseSize.y);
	if(baseSize != root->getSize()) {
		rescale = root;
		root = new Widget( Rect(0,0,baseSize.x,baseSize.y), 0);
	}

	for(XML::iterator i = xmlRoot.begin(); i!=xmlRoot.end(); ++i) {
		if(*i == "font") {
			if(~flags&LoadFlags::SKINS) continue;
			// ToDo: Bitmap font support, glyph lists
			const char* name = i->attribute("name");
			if((~flags&LoadFlags::REPLACE) && getFont(name)) continue;

			Font* font = new Font();
			auto loadFontFace = [font](const XMLElement& face, const char* name) {
				int size = face.attribute("size", 16);
				const char* src = face.attribute("source", name);
				if(!src || size<=0) return false;
				FontLoader* loader = nullptr;
				if(strstr(src, ".png")) loader = new BitmapFont(src, face.attribute("characters", nullptr));
				else if(strstr(src, ".ttf")) loader = new FreeTypeFont(src);
				else loader = new SystemFont(src);
				// Code point ranges
				Point range = parsePoint(face.attribute("range"));
				if(range.y>0) loader->addRange(range.x, range.y);
				// Additional ranges
				for(const XMLElement& r: face) {
					if(r == "range") loader->addRange(r.attribute("start", 0), r.attribute("end", 0));
				}
				// default range if none set
				if(loader->countGlyphs() == 0) loader->addRange(32, 126);
				bool success = font->addFace(*loader, size);
				delete loader;
				return success;
			};

			bool valid = false;
			if(i->size() == 0) valid = loadFontFace(*i, name);
			else for(auto& face: *i) {
				if(face == "face") valid |= loadFontFace(face, 0);
			}

			if(!valid) printf("Error: Font '%s' has no valid faces\n", name);
			if(valid) addFont(name, font);
			else delete font;
		}
		else if(*i == "skin") {
			if(~flags&LoadFlags::SKINS) continue;
			const char* name = i->attribute("name");
			Skin* exists = getSkin(name);
			if((~flags&LoadFlags::REPLACE) && exists) continue;
			if(exists) printf("Replacing skin %s\n", name);
			Skin* skin = 0;
			// Inherit existing skin
			const char* inherit = i->attribute("inherit");
			if(inherit[0]) {
				skin = getSkin(inherit);
				if(skin) skin = new Skin(*skin);
				else {
					printf("Skin '%s' not found when creating '%s'\n", inherit, name);
					skin = new Skin();
				}
			}
			// Overwrite existing skin - keep pointer
			else if(Skin* s = getSkin(name)) {
				s->setStateCount(0);
				s->setImage(-1);
				skin = s;
			}
			else skin = new Skin();

			for(XML::iterator j = i->begin(); j!=i->end(); ++j) {
				if(*j=="state") {
					// Get type
					int state = parseSkinState(j->attribute("name", "normal"));
					if(state<0) printf("Error: Invalid skin state: %s\n", j->attribute("name"));
					else {
						// Make sure state 0 exists
						if(skin->getStateCount() == 0) skin->setState(0, Skin::State());
						Skin::State s = skin->getState(state);
						s.rect    = parseRect(j->attribute("coord"), s.rect);
						s.textPos = parsePoint(j->attribute("offset"), s.textPos);
						s.border  = parseBorder(j->attribute("border"), s.border);
						s.foreColour  = parseColour(j->attribute("colour"), state? s.foreColour: 0xffffffff);
						s.foreColour  = parseColour(j->attribute("forecolour"), s.foreColour);
						s.backColour  = parseColour(j->attribute("backcolour"), state? s.backColour: 0xffffffff);
						skin->setState(state, s);
					}
				}
				// Font information
				else if(*j=="font") {
					Font* font = getFont( j->attribute("name") );
					int size = j->attribute("size", 18);
					int align = ALIGN_TOP | ALIGN_LEFT;
					const char* alignNames[] = { "left", "right", "centre", "top", "bottom", "middle" };
					const char* alignText = j->attribute("align");
					for(int i=0; i<3; ++i) {
						if(strstr(alignText, alignNames[i]))   align = (align&0xc)|(i+1);
						if(strstr(alignText, alignNames[i+3])) align = (align&0x3)|((i+1)<<2);
					}
					skin->setFont(font, size, align);
				}
				// Image
				else if(*j=="image") {
					const char* file = j->attribute("file");
					int id = getRenderer()->getImage(file);
					if(id==-1) id = getRenderer()->addImage(file);
					if(id==-1) printf("Failed to load image '%s' for skin '%s'\n", file, i->attribute("name"));
					skin->setImage( id );
				}
				else if(*j=="generate") {
					printf("Error: Old gui image generator was removed\n");
				}
			}

			// Validate skin
			if(skin->getStateCount()==0) skin->setState(0, Rect(0,0,0,0));
			// Needs font too

			// Add to map
			m_skins[name] = skin;
		}
		else if(*i=="genimage") {
			// Image generator
			ImageGenerator gen;
			const char* name = i->attribute("name", "generated");
			if(i->hasAttribute("glyphs")) {
				gen.type = ImageGenerator::Glyphs;
				gen.radius = i->attribute("glyphs", 16);
				if(i->size()) printf("Error: Generated glyph image cannot also define states");
			}
			else {
				gen.radius = i->attribute("radius", 0);
				gen.core = parsePoint(i->attribute("core", "2 2"));
				gen.lineWidth = i->attribute("line", 1);
				static const char* corners[] = { "tl", "tr", "bl", "br" };
				int stateMask = 0;
				for(const XMLElement& e: *i) {
					if(e=="corner") {
						const char* which = e.attribute("name");
						const char* style = e.attribute("style");
						for(int i=0; i<4; ++i) {
							if(strstr(which, corners[i])) {
								gen.corner[i] = (ImageGenerator::Corner) parseEnum(style, {"square", "round", "chamfer"}, 0);
							}
						}
					}
					else if(e=="state") {
						int state = parseSkinState(e.attribute("name", "normal"));
						if(state>=0) {
							gen.colours[state].back = parseColour(e.attribute("back"), 0xff000000); 
							gen.colours[state].line = parseColour(e.attribute("line"), 0xffffffff); 
							stateMask |= 1<<state;
						}
					}
				}

				// Copy colours to any missing states
				const int fromIndex[] = { 0, 0, 1, 0, 0, 4, 5, 3 };
				for(int i=0; i<8; ++i) if(~stateMask&(1<<i)) gen.colours[i] = gen.colours[fromIndex[i]];
				if(stateMask>=16) gen.type = ImageGenerator::Eight;
				else if(stateMask>1) gen.type = ImageGenerator::Four;
				else gen.type = ImageGenerator::Single;
			}
			getRenderer()->generateImage(name, gen);
		}
		else if(*i=="iconlist") {
			if(~flags&LoadFlags::ICONS) continue;
			const char* name = i->attribute("name");
			if((~flags&LoadFlags::REPLACE) && getIconList(name)) continue;
			IconList* list = getIconList(name);
			bool exists = list;
			if(!list) list = new IconList();
			for(XML::iterator j=i->begin(); j!=i->end(); ++j) {
				if(*j=="icon") {
					const char* iconName = j->attribute("name");
					int index = exists? list->getIconIndex(iconName): -1;
					Rect rect = gui::parseRect(j->attribute("rect"), Rect(0,0,16,16));
					if(index<0 || !iconName[0]) list->addIcon(iconName, rect);
					else list->setIconRect(index, rect);
				}
				else if(*j=="image"){
					const char* file = j->attribute("file");
					int id = getRenderer()->getImage(file);
					if(id == -1) id = getRenderer()->addImage(file);
					if(id == -1) printf("Failed to load image '%s' for icon set '%s'\n", file, name);
					list->setImageIndex(id);
				}
			}
			m_iconLists[name] = list;
		}
		else if(*i=="template") {
			if(~flags&LoadFlags::TEMPLATES) continue;
			const char* name = i->attribute("name");
			if((~flags&LoadFlags::REPLACE) && findInMap(m_templates, name)) continue;
			Widget* w = loadWidget(*i, true);
			if(w) {
				if(w->getName()) {
					w->m_states &= ~0x20; // Clear template flag on root widget
					if(m_templates.contains(w->m_name.str())) {
						printf("Replacing template %s\n", w->m_name.str());
						delete m_templates[w->m_name.str()];
					}
					m_templates[w->m_name] = w;
				}
				else {
					printf("Error: Template has no name\n");
					delete w;
				}
			}
			else printf("Failed to load template %s\n", i->attribute("name"));
		}
		else if(*i=="widget") {
			if(~flags&LoadFlags::WIDGETS) continue;
			Widget* w = loadWidget(*i, false);
			if(w) {
				if(w->m_name) m_widgets[ w->m_name ] = w;
				if(!first) first = w;
				root->add(w);
			}
		}
	}
	
	// rescale
	if(rescale) {
		root->setSize(rescale->getSize());
		while(root->getWidgetCount()) rescale->add(root->getWidget(0));
		delete root;
	}

	return first;
}

Widget* Root::loadWidget(const char* file, const char* widget) {
	XML xml = XML::load(file);
	const XMLElement& e = xml.getRoot().find("widget");
	if(e.name()[0]) {
		return loadWidget(e, false);
	}
	return 0;
}

Widget* Root::loadWidget(const base::XMLElement& e, bool isTemplate) const {
	static const char* empty = 0;
	const char* type = e.attribute("class", "Widget");
	const char* skin = e.attribute("skin", "default");
	const char* temp = e.attribute("template");
	Rect r = gui::parseRect(e.attribute("rect"), Rect(0,0,42,20));
	bool invalidType = !s_constuct.contains(type);
	
	Widget* w = 0;
	if(temp[0]) {
		Widget* templateWidget = m_templates.contains(temp)? m_templates[temp]: 0;
		if(!templateWidget) return printf("Error: Can't find template '%s'\n", temp), w;
		if(invalidType) printf("Error: Widget type %s not registered. Using %s as fallback\n", type, templateWidget->getTypeName());
		if(!e.hasAttribute("class") || invalidType) type = templateWidget->getTypeName();
		if(strcmp(type, templateWidget->getTypeName()))
			printf("Changing type %s -> %s\n", templateWidget->getTypeName(), type);

		// Clone template
		w = templateWidget->clone(type);
		w->pauseLayout();
		w->setPosition(r.x, r.y);

		if(e.hasAttribute("rect"))
			w->setSize(r.width, r.height);

		// Change skin?
		if(e.hasAttribute("skin")) {
			Skin* s = getSkin(skin);
			if(s) w->setSkin(s);
		}

		if(isTemplate) w->m_states |= 0x20;
		else w->m_states &= ~0x20;
	}
	else {
		// Create new widget
		Skin* s = getSkin(skin);
		if(!s) printf("Error: Skin %s undefined for %s '%s'\n", skin, type, e.attribute("name"));
		if(invalidType) printf("Error: Widget type %s not registered. Using %s as fallback\n", type, "Widget");
		if(invalidType) w = new Widget(r,s);
		else w = s_constuct[type](r, s);
		if(isTemplate) w->m_states |= 0x20;
	}

	// Set name
	w->m_name = e.attribute("name");

	// Anchor property
	const char* anchor = e.attribute("anchor", empty);
	if(anchor) w->setAnchor(anchor);
	if(e.attribute("relative", 0)) w->useRelativePositioning(true);

	// Other general properties
	if(e.attribute("inheritstate", 0)) w->setInheritState(true);
	if(!e.attribute("visible", 1)) w->setVisible(false);
	if(!e.attribute("enabled", 1)) w->setEnabled(false);
	if(e.hasAttribute("tangible")) w->setTangible((Tangible)e.attribute("tangible", 3));
	if(e.hasAttribute("colour")) w->setColour(e.attribute("colour", 0xffffff));
	if(e.hasAttribute("alpha")) w->setAlpha(e.attribute("alpha", 1.0f));
	w->setToolTip(e.attribute("tip"));


	// Create properties
	PropertyMap properties;
	for(const auto& i: e.attributes()) {
		properties[i.key] = i.value;
	}

	// Additional properties
	char buffer[6];
	int itemCount=0;
	for(XML::iterator i=e.begin(); i!=e.end(); ++i) {
		if(*i=="property") {
			for(const auto& j: i->attributes()) {
				properties[j.key] = j.value;
			}
		}
		else if(*i=="item") {
			const char* value = i->text();
			if(!value) value   = i->attribute("text");
			sprintf(buffer, "%d", itemCount++);
			properties[ buffer ] = value;
			properties[ "count" ] = buffer;
		}
	}

	// Load children
	w->pauseLayout();
	for(XML::iterator i=e.begin(); i!=e.end(); ++i) {
		if(*i=="widget") {
			Widget* c = loadWidget(*i, isTemplate);
			if(c) w->add(c);
		}
	}

	// Initialise widget
	w->initialise(this, properties);

	// Automatic layout
	if(e.attribute("autosize", 0)) w->setAutosize(true);
	const char* layout = e.attribute("layout", empty);
	if(layout) {
		int spacing = e.attribute("spacing", 0);
		int margin = e.attribute("margin", 0);
		Layout* lay = Root::createLayout(layout, margin, spacing);
		if(!lay) printf("Error: Layout '%s' undefined\n", layout);
		w->setLayout(lay);
	}
	w->resumeLayout();

	//printf("Created %swidget %s '%s'\n", isTemplate? "template ": "", type, e.attribute("name"));

	if(!w->m_skin && w->getType() != Widget::staticType()) {
		printf("Error: widget '%s' [%s] has no skin\n", w->m_name.str(), w->getTypeName());
	}

	return w;
}



