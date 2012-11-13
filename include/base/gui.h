#ifndef _BASE_GUI_
#define _BASE_GUI_

#include <base/math.h>
#include <base/hashmap.h>
#include <list>
#include <vector>

namespace base {
	class Texture;
	class Font;
	class XMLElement;

namespace gui {	

	class Control;
	class Container;

	/**
	 * Controls:
	 * 	Container	- contains child controls.
	 * 	Frame		- A container graphics which can be dragged
	 * 	Label		- Some text (no interaction)
	 * 	Button		- Changes on over/focus, has click event
	 * 	Checkbox	- Checked toggles on click
	 * 	Listbox		- List of text items. Scrollable
	 * 	DropList	- Dropdown list (uses listbox)
	 * 	Input		- Text Input Field
	 * 	
	 * Input - would like to remove the distributed dependance on base
	 *     - mousePosition	- Cursor location in screen space
	 *     - mouseButton	- bitset of button states
	 *     - mouseClick	- bitset of changed button states
	 *     - key		- key state
	 *     - keyPressed	- key was pressed since last frame
	 *     - lastKey	- the last key to be pressed
	 *     - lastChar	- last inputted character? maybe use keyString like flashpunk?
	 *     - window width	
	 *     - window height
	 *     - frameTime	- time in seconds since the last update call
	 */

	/** GUI Sprites */
	class Sprite {
		public:
		Sprite(): m_texture(0), m_rows(0), m_cols(0) { setBorder(0); }
		Sprite(const Texture& tex, int rows=1, int cols=1);
		void drawGlyph(int frame, int cx, int cy) const;
		void draw(int frame, int x, int y, int w, int h, bool border=true) const;
		void setBorder(int size);
		void setBorder(int top, int left, int right, int bottom);
		int width() const { return m_width; }
		int height() const { return m_height; }
		int frames() const { return m_rows * m_cols; }
		protected:
		uint m_texture;
		int m_width, m_height;
		int m_rows, m_cols;
		vec2 m_one;
		bool m_hasBorder;
		struct { int top, left, right, bottom; } m_border;
	};

	/** Gui style  */
	class Style {
		public:
		Style();
		Style(const Style* s);

		enum TextAlign { LEFT, CENTRE, RIGHT };
		enum Type { BACK=0, BORDER=1, TEXT=2 };
		enum State { BASE=1, OVER=2, FOCUS=4, DISABLED=8 };

		/** Set the font */
		void setFont(Font* f, int align=LEFT)	{ m_font=f; m_align=align; }
		void setAlign(int align)				{ m_align=align; }
		void setFade(float time)				{ m_fadeTime=time; }
		void setSprite(const Sprite& sprite)	{ m_sprite = sprite; }

		/** Set colour (loads to avoid ambiguity) */
		void setColour(int type, uint colour, float alpha=1)            { setColour(type, 0xf, Colour(colour), alpha); }
		void setColour(int type, int state, uint colour, float alpha=1) { setColour(type, state, Colour(colour), alpha); }
		void setColour(int type, const Colour& colour)                  { setColour(type,0xf,colour); }
		void setColour(int type, const Colour& colour, float alpha)     { setColour(type,0xf,colour,alpha); }
		void setColour(int type, int state, const Colour& colour);
		void setColour(int type, int state, const Colour& colour, float alpha);

		/** Set sprite frames */
		void setFrame(int state, int frame);
		void setFrame(int frame) { setFrame(0xf,frame); }

		inline const Colour& getColour(int type, int state=BASE) const { return m_colours[type + 3*code(state) ]; }
		inline int  getFrame(int state=BASE) const { return m_frames[ code(state) ]; }

		protected:
		friend class Control;
		inline static int code(int s) { return s<8? s>>1: 3+(s>>4); }; // Assume s is valid
		void drop();			// Drop reference
		int m_ref;				// Reference count
		Colour m_colours[12];	// Colours for types,states
		int m_frames[4];		// Frames for states
		Font* m_font;			// Font
		int m_align;			// Text alignment
		float m_fadeTime;		// Fade time (unused)
		Sprite m_sprite;		// Image
	};

	/** Event data */
	struct Event {
		uint id;
		uint value;
		const char* text;
		Control* control;
		Event() : id(0), value(0), text(0), control(0) {}
	};


	/** GUI manager. All elements contain a link to this. 
	 * can me implicit, not directly created. The root control will create it.
	 */
	class Root {
		public:
		~Root();
		int update(Event& e);     // Update gui events
		void draw();              // Draw gui
		void merge(Root* r); // Copy named controls from r
		Control* getControl(const char* name) const;
		private:
		friend class Control;
		friend class Loader;
		Root(Control*);			// Only Control and Loader can create instances
		void drop();			// Drop reference
		int m_ref;				// Reference counter
		Control* m_control;		// Root control
		Control* m_lastFocus;	// Control focused in last update
		Control* m_focus;		// Focused control
		Control* m_over;		// Control under cursor
		Point m_lastMouse;		// Last mouse point for mouseMove
		HashMap<Control*> m_names; // Map of named elements
	};

	/** GUI Loader - used to load fromm XML */
	class Loader {
		public:
		/** Register a widget construction function with the loader */
		static void addType(const char* name, Control*(*c)(const Loader&));
		static Container* parse(const char* file);
		static Container* load(const char* file);
		
		// These functions are called from construction functions //
		Style* style() const;
		const char* attribute(const char* name, const char* d) const;
		float attribute(const char* name, float d) const;
		int attribute(const char* name, int d) const;
		// Reference to the parent control
		const Control* parent() const { return m_parent; }
		// If the element has custom children, need to access XML element directly
		const XMLElement* operator->() const;
		// Additional functions that may be useful
		static uint parseHex(const char*);
		private:
		Loader();
		Style* readStyle(const XMLElement&) const;
		Colour readColour(const XMLElement&) const;
		int addControls(const XMLElement&, Container*);
		const Control* m_parent;
		const XMLElement* m_item;
		HashMap<Style*> m_styles;
		static HashMap<Control*(*)(const Loader&)> s_types;
	};


	/** Base class for all GUI Elements */
	class Control {
		friend class Container;
		friend class Root;
		friend class Loader;
		public:
		Control(Style* style=0, uint cmd=0);	// Constructor
		virtual ~Control();						// Destructor
		virtual void draw() = 0;				// Draw control (and children)
		uint update();							// Calls Root::update()
		uint update(Event& event);				// Calls Root::update()
		void setPosition(int x, int y);	 		// Set relative positions
		const Point getPosition() const;		// Get relative position
		virtual void setAbsolutePosition(int x, int y) { m_position.x=x; m_position.y=y; } //internal position
		virtual const Point& getAbsolutePosition() const { return m_position; }
		virtual void setSize(int width, int height) { m_position.y+=m_size.y-height; m_size.x=width, m_size.y=height; }
		const Point& getSize() const { return m_size; }
		virtual void show() { m_visible=true; }
		virtual void hide() { m_visible=false; }
		inline void setVisible(bool vis=true) { vis?show():hide(); }
		inline void setEnabled(bool e=true) { m_enabled=e; }
		void raise();			// Bring control to the front of the list (draws on top)
		void setFocus();		// Set the focus to this
		bool hasFocus() const;	// Is the focus here?
		bool visible() const { return m_visible; }				// Is this control drawn
		bool enabled() const { return m_enabled && m_visible; }	// Do events affect this
		virtual bool isContainer() const { return false; }		// Does this class inherit Container
		Container* getParent() const { return m_parent; }		// Get parent control
		Control* getControl(const char* name) const;			// Get named control in this heirachy (Root)
		protected:
		int getState() const;		// Get control state (uses Root focus and over)
		Point m_position, m_size;	// Control dimensions
		Style* m_style;				// Style object
		uint m_command;				// Event code
		bool m_visible;				// Control is drawn
		bool m_enabled;				// Control in enabled state
		Container* m_parent;		// Parent control
		Root* m_root;				// Root object

		// Internal update events -  use these instead of click() and update();
		virtual uint mouseEvent( Event&, const Point&, int)               { return 0; }
		virtual uint mouseMove(  Event&, const Point&, const Point&, int) { return 0; }
		virtual uint mouseWheel( Event&, const Point&, int)               { return 0; }
		virtual uint mouseEnter( Event&)                                  { return 0; }
		virtual uint mouseExit(  Event&)                                  { return 0; }
		virtual uint keyEvent(   Event&, uint, uint16)                    { return 0; }
		virtual uint gainFocus(  Event&)                                  { return 0; }
		virtual uint loseFocus(  Event&)                                  { return 0; }

		//Utilities
		virtual bool isOver(int x, int y) const { return x>=m_position.x && y>=m_position.y && x<=m_position.x+m_size.x && y<=m_position.y+m_size.y; }
		uint setEvent(Event& e, uint value=0, const char* txt=0); // Set the event
		Point textPosition(const char* c) const;	// Get relative aligned text position
		Point textSize(const char* text) const;		// Get the size of a text string
		float fadeValue(float v, int dir) const;	// Fade a value using style::m_fadeTime;
		void setRoot(Root*);						// Add reference to root (recursive)
		void dropRoot();							// Drop reference to root (recursive)
		//Drawing
		void drawFrame(const Point& p, const Point& s, const char* title=0, int state=Style::BASE) const;
		void drawArrow(const Point& p, int direction, int size=8, int state=0) const;
		void drawText(int x, int y, const char* text, int state=0) const;
		void drawText(const Point&, const char* text, int state=0) const;
		static void drawRect(int x, int y, int w, int h, const Colour& c = white, bool fill=true);
		static void drawCircle(int x, int y, float r, const Colour& col=white, bool fill=true, int s=16);
		static void scissorPushNew(int x, int y, int w, int h);
		static void scissorPush(int x, int y, int w, int h);
		static void scissorPop();
	};

	/** Container class. Contains other controls */
	class Container : public Control {
		friend class Control;
		public:
		Container();
		Container(int x, int y, int w, int h, bool clip=false, Style* style=0);
		virtual ~Container() { clear(); }
		virtual void draw();
		Control* add( Control* c, int x, int y );
		Control* add( const char* caption, Control* c, int x, int y, int cw=120);
		Control* remove( Control* c );
		void clear();
		uint count() const { return m_contents.size(); }
		Control* getControl(uint index);
		Control* getControl(const Point&, bool recursive=true);
		virtual bool isContainer() const { return true; }
		protected:
		void setAbsolutePosition(int x, int y); // move all contained controls
		void moveContents(int dx, int dy);
		std::list<Control*> m_contents;
		bool m_clip;
	};

	/** Frame, A basic container with a border that can be dragged around */
	class Frame : public Container {
		public:
		Frame(int x, int y, int width, int height, const char* caption=0, bool moveable=0, Style* style=0);
		Frame(int width, int height, const char* caption=0, bool moveable=0, Style* style=0);
		virtual ~Frame() { setCaption(0); }
		const char* getCaption() const { return m_caption; }
		void setCaption(const char* c);
		void draw();
		protected:
		// Events
		virtual uint mouseEvent(Event&, const Point&, int);
		virtual uint mouseMove(Event&, const Point&, const Point&, int);

		char* m_caption;	// Frame Caption
		bool m_moveable;	// Can you drag the frame around
		bool m_held;		// Is the frame held
	};

	/** Your basic label */
	class Label : public Control {
		public:
		Label(const char* caption, Style* style=0);
		void draw();
		void setCaption(const char* caption);
		const char* getCaption() const { return m_caption; }
		protected:
		inline virtual bool isOver(int x, int y) const { return false; }
		char m_caption[128];
	};


	/** A button */
	class Button : public Control {
		public:
		Button(const char* caption, Style* style=0, uint command=0);
		Button(int width, int height, const char* caption=0, Style* style=0, uint command=0);
		void setCaption(const char* c);
		const char* getCaption() const { return m_caption; }
		virtual void draw();
		protected:
		virtual uint mouseEvent(Event&, const Point&, int);
		char m_caption[64];
		Point m_textPos;
	};

	class Checkbox : public Button {
		public:
		Checkbox(int size, const char* caption, bool value=false, Style* style=0, uint command=0);
		bool getValue() const { return m_value; }
		void setValue(bool value) { m_value = value; }
		virtual void draw();
		protected:
		virtual uint mouseEvent(Event&, const Point&, int);
		int m_boxSize;
		bool m_value;
	};

	/** Slider */
	class Slider : public Control {
		public:
		enum Orientation { VERTICAL, HORIZONTAL };
		Slider(int orientation=VERTICAL, int width=16, int height=120, Style* style=0, uint command=0);
		virtual void draw();
		void setValue(float value);
		float getValue() const { return m_value; }
		float getMin() const { return m_min; }
		float getMax() const { return m_max; }
		void setRange(float min=0, float max=1, float step=0) { m_min=min, m_max=max; m_step=step; setValue(m_value); }
		protected:
		virtual uint mouseEvent(Event&, const Point&, int);
		virtual uint mouseMove(Event&, const Point&, const Point&, int);
		float m_min, m_max;	// Value limits
		float m_value;		// Current value
		float m_step;		// Rounding
		int m_orientation;	// Horizontal or Vertical
		int m_held;			// Is the block held
	};

	/** Scrollar */
	class Scrollbar : public Control {
		friend class ListBase;
		public:
		enum Orientation { VERTICAL, HORIZONTAL };
		Scrollbar(int orientation=VERTICAL, int width=16, int height=120, int min=0, int max=100, Style* style=0, uint command=0);
		virtual void draw();
		void setValue(int value) { m_value=value<m_min?m_min: value>m_max? m_max: value; }
		int getValue() const { return m_value; }
		int getMin() const { return m_min; }
		int getMax() const { return m_max; }
		void setRange(int min=0,int max=100) { m_min=min, m_max=max; setValue(m_value); }
		protected:
		virtual uint mouseEvent(Event&, const Point&, int);
		virtual uint mouseMove(Event&, const Point&, const Point&, int);
		int m_min, m_max;	// Value limits
		int m_value;		// Current value
		int m_step;			// Value step for buttons or mousewheel
		int m_orientation;	// Horizontal or Vertical
		int m_held;			// Is the block held
		int m_blockSize;	// Size of block
	};

	/** Base class for generic listbox with vertical scrollbar */
	class ListBase: public Control {
		public:
		ListBase(int width, int height, Style* style, uint command);
		virtual void setAbsolutePosition(int x, int y);
		virtual void setSize(int width, int height);
		virtual void draw();
		virtual uint size() const = 0;
		void scrollTo(uint index);
		void selectItem(uint index) { m_selected=index; }
		uint selected() const       { return m_selected; }
		protected:
		virtual void drawItem(uint index, int x, int y, int width, int height) = 0;
		virtual bool mouseEvent(Event& e, uint index, const Point&, int b);
		// Events: needs all of them
		virtual uint mouseEvent( Event&, const Point&, int);
		virtual uint mouseMove(  Event&, const Point&, const Point&, int);
		virtual uint mouseWheel( Event&, const Point&, int);
		void updateBounds(); // Call this when list changes
		uint m_itemHeight;
		uint m_selected;
		uint m_hover;
		Scrollbar m_scroll;
	};

	/** Basic text listbox */
	class Listbox : public ListBase {
		public:
		Listbox(int width, int height, Style* style, uint command);
		void addItem(const char* item, bool sel=false);
		void addItem(const char* item, uint index, bool sel=false);
		void removeItem(uint index);
		void clearItems();
		const char* getItem(uint index) const;
		const char* selectedItem() const { return getItem(m_selected); }
		uint size() const { return m_items.size(); }
		protected:
		virtual void drawItem(uint index, int x, int y, int width, int height);
		struct Item { char* text; };
		std::vector<Item> m_items;
	};
	
	/** A Dropdown List */
	class DropList: public Listbox {
		public:
		DropList(int width, int height, int maxSize=0, Style* style=0, uint command=0);
		virtual void draw();
		protected:
		virtual uint loseFocus(  Event&) { closeList(); return 0; }
		virtual uint mouseEvent( Event&, const Point&, int);
		void openList();
		void closeList();
		int m_max;	//Maximum height of popup list
		bool m_open;	//Open state of the list
		float m_state;
	};

	/** A control for text input */
	class Input: public Control {
		public:
		Input(int width, int height, const char* text=0, Style* style=0, uint command=0);
		virtual void draw();
		void setMask(char mask='*') { m_mask=mask; }
		void setText(const char* text);
		const char* getText() const { return m_text; }
		protected:
		virtual uint mouseEvent( Event&, const Point&, int);
		virtual uint keyEvent(   Event&, uint, uint16);
		char m_mask;
		char m_text[128];
		int m_loc, m_len;
		float m_state;
	};

};
};

#endif

