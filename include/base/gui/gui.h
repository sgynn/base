#ifndef _GUI_
#define _GUI_

#include <base/hashmap.h>
#include <base/point.h>
#include "delegate.h"
#include <vector>

#include <cstdio>
#include <typeinfo>

namespace base { class XMLElement; }

namespace gui {

class Font;
class Skin;
class IconList;
class Widget;
class Root;
class Renderer;
class Layout;

#ifndef LINUX
typedef unsigned int uint;
#endif

enum Orientation { HORIZONTAL=0, VERTICAL=1 };
enum class Tangible  { NONE, SELF, CHILDREN, ALL };
enum class KeyMask   { None=0, Ctrl=1, Shift=2, Alt=4, Meta=8 };
enum class LoadFlags { WIDGETS=1, TEMPLATES=2, SKINS=4, ICONS=8, REPLACE=16, ALL=0xf };

inline int operator&(KeyMask a, KeyMask b) { return (int)a & (int)b; }
inline KeyMask operator|(KeyMask a, KeyMask b) { return (KeyMask)((int)a | (int)b); }
inline LoadFlags operator|(LoadFlags a, LoadFlags b) { return (LoadFlags)((int)a|(int)b); }


// RTTI stuff - gcc 6.2.1 has warnings. Add -Wno-nonnull-compare to hide them. ( Fixed with nasty hack. The whole thisng is a nasty hack anyway )
#define RTTI_TYPE(name) \
	static int staticType() { static int cid = typeid(name).hash_code(); return cid; } \
	static const char* staticName() { return #name; } \

#define RTTI_BASE(name) public: RTTI_TYPE(name); \
	typedef name BaseType; \
	virtual bool isType(int t) const { return t==staticType(); } \
	virtual int getType() const { return staticType(); } \
	virtual const char* getTypeName() const { return staticName(); } \
	template<class T> T* cast() { size_t ptr=(size_t)this; return ptr && isType(T::staticType())? static_cast<T*>(this): 0; } \
	template<class T> const T* cast() const { size_t ptr=(size_t)this; return ptr && isType(T::staticType())? static_cast<const T*>(this): 0; }

#define RTTI_DERIVED(name) public: RTTI_TYPE(name); \
	typedef BaseType Base; \
	typedef name BaseType; \
	virtual bool isType(int t) const override { return t==staticType() || Base::isType(t); }; \
	virtual int getType() const override { return staticType(); } \
	virtual const char* getTypeName() const override { return staticName(); } \

#define WIDGET_TYPE(name) RTTI_DERIVED(name);





/** Gui string class - basically a cstring wrapper */
class String {
	public:
	String(const char* s = 0) : m_data(0)		{ set(s); }
	String(const char* s, size_t len):m_data(0) { if(s&&s[0]&&len) { size_t l=strlen(s); if(len<l)l=len; m_data=(char*)malloc(l+1); memcpy(m_data,s,l); m_data[l]=0; } }
	String(String&& s) : m_data(s.m_data)       { s.m_data=0; }
	String(const String& s)	: m_data(0)			{ set(s); }
	const String& operator=(const char* s)		{ set(s); return *this;}
	const String& operator=(const String& s)	{ set(s); return *this; }
	const String& operator=(String&& s)	        { if(m_data) free(m_data); m_data=s.m_data; s.m_data=0; return *this; }

	bool operator==(const char* s) const		{ return m_data==s || (!m_data&&!*s) || (m_data && strcmp(m_data, s)==0); }
	bool operator==(const String& s) const		{ return m_data==s.m_data || (m_data && strcmp(m_data, s.m_data)==0); }
	bool operator!=(const char* s) const		{ return m_data!=s && (m_data||*s) && (!m_data || strcmp(m_data, s)); }
	bool operator!=(const String& s) const		{ return m_data!=s.m_data && (!m_data || strcmp(m_data, s.m_data)); }
	friend bool operator==(const char* a, const String& s) { return s == a; }
	friend bool operator!=(const char* a, const String& s) { return s != a; }

	~String()									{ set(0); }
	String& swap(String& other)					{ char* t=m_data; m_data=other.m_data; other.m_data=t; return *this;}
	operator const char*() const				{ return m_data; }
	const char* str() const                     { return m_data? m_data: ""; }
	size_t length() const						{ return m_data? strlen(m_data): 0; }
	bool empty() const							{ return !m_data || !m_data[0]; }
	void clear()                                { set(0); }

	String& operator+=(const char* s)           { if(s&&s[0]) *this = *this + s; return *this; }
	String& operator+=(const String& s)         { if(!s.empty()) *this = *this + s; return *this; }
	String operator+(const char* s)             { if(empty()) return String(s); if(!s||!s[0]) return *this; String r; r.m_data=(char*)malloc(length() + strlen(s) + 1); sprintf(r.m_data, "%s%s", m_data,s); return r; }
	String operator+(const String& s)           { if(empty()) return s; if(s.empty()) return *this; String r; r.m_data=(char*)malloc(length() + s.length() + 1); sprintf(r.m_data, "%s%s", m_data,s.m_data); return r; }
	friend String operator+(const char* a, const String& s) { if(!a&&!a[0]) return s; if(s.empty()) return String(a); String r; r.m_data=(char*)malloc(strlen(a)+s.length()+1); sprintf(r.m_data, "%s%s", a, s.m_data); return r; }
	private:
	void set(const char* s)                     { if(m_data==s) return; if(m_data) free(m_data); m_data = s&&s[0]? strdup(s): 0; }
	char* m_data;
};


/** Additional properties from external data - basically extra accessors for a string hashmap */
class PropertyMap : public base::HashMap<const char*> {
	public:
	bool readValue(const char* key, int& value) const     { if(const char* s=get(key,0)) { value = atoi(s); return true; } return false; }
	bool readValue(const char* key, float& value) const   { if(const char* s=get(key,0)) { value = atof(s); return true; } return false; }
	int   getValue(const char* key, int fallback) const   { if(const char* s=get(key,0)) return atoi(s); return fallback; }
	float getValue(const char* key, float fallback) const { if(const char* s=get(key,0)) return atof(s); return fallback; }
};


/** Gui widget base class */
class Widget {
	friend class Root;
	RTTI_BASE(Widget);

	public:
	Widget(const Rect&, Skin*);
	virtual ~Widget();
	virtual void draw() const;
	virtual void setPosition(int x, int y);
	virtual void setSize(int w, int h);
	virtual void setEnabled(bool);
	virtual void setVisible(bool);
	virtual void setSelected(bool);
	virtual void setTangible(Tangible);
	virtual void setInheritState(bool);
	inline void setPosition(const Point& p) { setPosition(p.x, p.y); }
	inline void setSize(const Point& s)     { setSize(s.x, s.y); }
	void setSizeAnchored(const Point& s);	// Set size, and adjust position based on anchor. Skips if anchored to both sides
	void setPositionFloat(float x, float y, float w, float h);
	void useRelativePositioning(bool);

	Point getPosition() const;
	const Point& getSize() const;

	void setAnchor(int code);			// Set anchor code
	void setAnchor(const char*);		// Set anchor string
	int  getAnchor(char* out=0) const;	// Get anchor code and string
	void setAutosize(bool);				// Set autosize from contents
	bool isAutosize() const;			// Get autosize setting
	bool isRelative() const;			// Do we use relative positioning

	// Widget colour. Note: ARGB==0 to use skin default
	void     setColourARGB(unsigned argb)         { m_colour=argb; m_states|=0x300; }
	int      getColour() const                    { return m_colour&0xffffff; }
	unsigned getColourARGB() const                { return m_colour; }
	void     setColour(unsigned rgb, float a=1.0) { m_colour=rgb; setAlpha(a); m_states|=0x100; }
	void     setColour(float r, float g, float b, float a=1.0) { setColour((int(r*255)&0xff)<<16 | (int(g*255)&0xff)<<8 | (int(b*255)&0xff), a); }
	void     resetColour()                        { m_colour = 0xffffffff; m_states &= ~0x300; }
	void     setAlpha(float a)                    { m_colour = (m_colour&0xffffff) | int(a*255)<<24; m_states|=0x200; }
	float    getAlpha() const                     { return (m_colour>>24)/255.0; }


	void raise();				// Move this widget to the front within its container
	void setFocus();			// Set keyboard focus to this widget
	bool hasFocus() const;		// Does this widget have focus
	void setMouseFocus();		// Set mouse focus to this widget
	bool hasMouseFocus() const;	// Does this widget have mouse focus
	bool isVisible() const;		// Is this widget visible
	bool isEnabled() const;		// Is this widget enabled
	Tangible isTangible() const;	// Do mouse clicks register on this widget, or go to the one behind
	bool isParentEnabled() const; // False if any parent is disabled
	bool isSelected() const;	// Is this widget's state selected
	bool isTemplate() const;	// Is this a template sub-widget
	void setAsTemplate();		// Flag this widget as part of a template

	int     getWidgetCount() const { return m_client->m_children.size() - m_client->m_skipTemplate; }
	int     getIndex() const;
	int     getTemplateIndex() const;
	int     getTemplateCount() const { return m_skipTemplate; }
	Widget* getTemplateWidget(size_t index) { return index<(size_t)m_skipTemplate? m_children[index]: 0; }
	Widget* getParent() const;
	Widget* getWidget(size_t index) const;
	template<class T=Widget> T* getWidget(size_t index) const {
		Widget* w = getWidget(index); return w? w->cast<T>(): 0;
	}
	template<class T=Widget> T* getWidget(const Point& pos, bool intangible=false, bool templates=false) {
		Widget* w = getWidget(pos, T::staticType(), intangible, templates); return w? w->cast<T>(): 0;
	}
	template<class W=Widget> W* getWidget(const char* name) const {
		Widget* w = findChildWidget(name); return w? w->cast<W>(): 0;
	}
	template<class W=Widget> W* getTemplateWidget(const char* name) const {
		Widget* w = findTemplateWidget(name); return w? w->cast<W>(): 0;
	}
	std::vector<Widget*>::iterator begin() const { return m_client->m_children.begin() + m_client->m_skipTemplate; }
	std::vector<Widget*>::iterator end() const   { return m_client->m_children.end(); }

	virtual Widget* clone(const char* newType=0) const;				// Clone widget and children. newType can override widget class
	virtual void    initialise(const Root*, const PropertyMap&);
	const Point&    getAbsolutePosition() const { return m_rect.position(); }
	const Rect&     getAbsoluteRect() const { return m_rect; }
	const Rect&     getAbsoluteClientRect() const { return m_client->m_rect; }
	Rect            getClientRect() const { return Rect(m_client->getAbsolutePosition()-getAbsolutePosition(), m_client->getSize()); }
	Rect            getRect() const { return Rect( getPosition(), getSize() ); }
	const Widget*	getClientWidget() const { return m_client; }
	const String&   getName() const { return m_name; }
	Skin*           getSkin() const;
	void            setSkin(Skin*);
	void            setName(const char* name);

	// Child widgets
	void add(Widget* w);
	void add(Widget* w, int x, int y);
	virtual void add(Widget* w, unsigned index);
	virtual bool remove(Widget* w); /// Does not delete widget
	virtual int  deleteChildWidgets();  /// Delete all child widgets
	void removeFromParent();

	template<class T> T* createChild(const char* type, const char* name=0); // Implemented below

	Layout* getLayout() const;
	void setLayout(Layout*);
	void pauseLayout();
	void resumeLayout();
	bool isLayoutPaused() const;
	virtual void refreshLayout();

	// Get system root
	Root* getRoot() const { return m_root; }

	public:	// Events
	Delegate<void(Widget*, const Point&, int)>     eventMouseDown;	// point relative to widget
	Delegate<void(Widget*, const Point&, int)>     eventMouseUp;
	Delegate<void(Widget*, const Point&, int)>     eventMouseMove;
	Delegate<void(Widget*, int)>                   eventMouseWheel;
	Delegate<void(Widget*)>                        eventLostFocus;
	Delegate<void(Widget*)>                        eventGainedFocus;
	Delegate<void(Widget*, int, wchar_t, KeyMask)> eventKeyPress;
	Delegate<void(Widget*)>                        eventResized;
	Delegate<void(Widget*)>                        eventMouseEnter;
	Delegate<void(Widget*)>                        eventMouseExit;

	protected:
	Rect     m_rect;			// Position and size (absolute)
	Skin*    m_skin;			// Skin - used for rendering
	unsigned m_colour;			// Widget colour ARGB
	char     m_anchor;			// left, centre, right,  top, middle, bottom. for automatic resizing.
	Layout*  m_layout;			// Automatic layouts
	float*   m_relative;		// alternative coordinates as multiple of parent
	short    m_states;			// Visible, Enabled, Tangible[2], Selected, Template, InheritState, Autosize, OverrideColour[2], LayoutPaused
	char     m_skipTemplate;	// Templates in client widget to skip
	Widget*  m_parent;			// Parent widget
	Widget*  m_client;			// Client widget
	Root*    m_root;			// Gui manager class
	String   m_name;

	std::vector<Widget*>  m_children;

	virtual void onMouseButton(const Point&, int dn, int up);	// Point is absolute value
	virtual void onMouseMove(const Point& last, const Point&, int);
	virtual bool onMouseWheel(int w);
	virtual void onMouseEnter();
	virtual void onMouseExit();
	virtual void onKey(int code, wchar_t chr, KeyMask mask) {}
	virtual void onGainFocus() {}
	virtual void onLoseFocus() {}
	virtual void onChildChanged(Widget*);	// Child widget moved, resized added or removed
	virtual void onAdded() {}

	virtual void updateAutosize();

	int getState() const;
	void drawSkin() const;
	void drawChildren() const;
	void setRoot(Root*);
	void notifyChange();
	void updateRelativeFromRect();
	static unsigned deriveColour(unsigned base, unsigned custom, short flags);

	Widget* findChildWidget(const char*) const;
	Widget* findTemplateWidget(const char*) const;
	virtual Widget* getWidget(const Point&, int mask, bool intangible=false, bool templates=false);
};


/** Gui root object. Manages a gui instance */
class Root {
	friend class Widget;
	public:
	Root(int width, int height, Renderer* r=0);
	Root(const Point& size, Renderer* r=0) : Root(size.x, size.y, r) {}
	~Root();

	void    mouseEvent(const Point& position, int buttonState, int wheel);
	void    keyEvent(int keycode, wchar_t chr);
	void    setKeyMask(KeyMask m);
	void    setKeyMask(bool ctrl, bool shift, bool alt, bool meta=false);

	void    resize(const Point& s) { resize(s.x, s.y); }
	void    resize(int width, int height);
	void    update();
	void    draw() const;
	Widget* parse(const char* xml, Widget* root=0, LoadFlags flags=LoadFlags::ALL|LoadFlags::REPLACE);	// Load all from string
	Widget* load(const char* file, Widget* root=0, LoadFlags flags=LoadFlags::ALL|LoadFlags::REPLACE);	// Load all from file
	Widget* loadWidget(const char* file, const char* widget);	// Only load a single widget
	Widget* load(const base::XMLElement& xmlRoot, Widget* parent=0, LoadFlags flags=LoadFlags::ALL);
	Widget* loadWidget(const base::XMLElement&, bool isTemplate) const;

	void    addFont(const char* name, Font* font);
	void    addSkin(const char* name, Skin* skin);
	void    addIconList(const char* name, IconList* icons);

	typedef std::vector<const char*> StringList;
	StringList getFonts() const;
	StringList getSkins() const;
	StringList getIconLists() const;
	StringList getTemplates() const;
	StringList getWidgetTypes() const;

	Font*     getFont(const char* name) const { return findInMap(m_fonts, name); }
	Skin*     getSkin(const char* name) const { return findInMap(m_skins, name); }
	IconList* getIconList(const char* name) const { return findInMap(m_iconLists, name); }
	Widget*   getRootWidget() const { return m_root; }
	const Point& getSize() const { return m_root->getSize(); }

	Widget*  getFocusedWidget() const { return m_focus; }
	Widget*  getWidgetUnderMouse() const { return m_mouseFocus; }
	bool     getWheelEventConsumed() const { return m_wheelUsed; }	// Did a widget consume the last mouse wheel event

	// Widget creation. skin can be a skin or template name
	template<typename T=Widget> T* createWidget(const Rect& r, const char* skin, const char* name=0) const {
		Widget* w = createWidget(skin, T::staticName(), name);
		if(!w) return nullptr;
		w->setPosition(r.x, r.y);
		w->setSize(r.width, r.height);
		return w->cast<T>();
	}
	template<typename T=Widget> T* createWidget(const float* r, const char* skin, const char* name=0) const {
		Widget* w = createWidget(skin, T::staticName(), name);
		if(!w) return nullptr;
		w->setPositionFloat(r[0], r[1], r[2], r[3]);
		return w->cast<T>();
	}
	template<typename T=Widget> T* createWidget(const char* skin, const char* name=0) const {
		bool forceType = T::staticType() != Widget::staticType();
		Widget* w = createWidget(skin, T::staticName(), name, forceType);
		return w->cast<T>();
	}

	// Widget creation - Use templated versions instead. 
	Widget* createWidget(const char* skin, const char* type, const char* name, bool forceType=false) const;

	// Create a template from a widget and children. Stores a copy.
	void createTemplate(const char* name, Widget*, const char* type=0);
	void addTemplate(const char* name, Widget*);
	const Widget* getTemplate(const char* name) const;
	static Layout* createLayout(const char* name, int margin=0, int spacing=0);

	// get widget
	template<class T=Widget> T* getWidget(const char* name) const {
		return findInMap(m_widgets, name)->cast<T>();
	}

	Renderer* getRenderer() const { return m_renderer; }
	void setRenderer(Renderer* r);

	// Input state - for 
	const Point& getMousePos() const { return m_mousePos; }
	int getMouseState() const { return m_mouseState; }
	KeyMask getKeyMask() const { return m_keyMask; }

	protected:
	Widget* m_focus;		// Widget with key focus
	Widget* m_mouseFocus;	// Widget with mouse focus
	Widget* m_root;			// Root widget
	
	KeyMask m_keyMask;
	Point   m_mousePos;
	int     m_mouseState;
	bool    m_wheelUsed;
	bool    m_changed;

	protected:
	base::HashMap<Font*>     m_fonts;		// Font list
	base::HashMap<Skin*>     m_skins;		// Skin list
	base::HashMap<IconList*> m_iconLists;	// Icons
	base::HashMap<Widget*>   m_templates;	// Template list
	base::HashMap<Widget*>   m_widgets;		// Named widgets
	Renderer*                m_renderer;	// Overrideable renderer class

	template<typename T> static T* findInMap(const base::HashMap<T*>& map, const char* key) {
		typename base::HashMap<T*>::const_iterator i = map.find(key);
		return i==map.end()? 0: i->value;
	}


	// Widget factory
	static base::HashMap<Widget*(*)(const Rect&, Skin*)> s_constuct;
	static base::HashMap<Layout*(*)(int,int)> s_layouts;
	template<class T> static Widget* constructFunc(const Rect& r, Skin* s) { return new T(r,s); }
	template<class T> static Layout* layoutFunc(int m, int s) { return new T(m,s); }
	public:
	/// Register a widget class
	template<class T> static void registerClass() { s_constuct[ T::staticName() ] = &Root::constructFunc<T>; };
	template<class T> static void registerLayout(){ s_layouts[ T::staticName() ] = &Root::layoutFunc<T>; };
};

template<class T> inline T* Widget::createChild(const char* type, const char* name) { 
	if(!m_root) return 0; // Error
	T* w = m_root->createWidget<T>(type, name);
	if(w) add(w);
	return w;
}

}

#endif

