#ifndef _GUI_
#define _GUI_

#include "math.h"
#include <list>
#include <vector>

namespace base {
	class Texture;
	class Font;

namespace GUI {	

	class Control;
	class Container;

	/** Perhaps use this as a style
	 *	font
	 *	background { colour, focus, over }
	 *	foreground { colour, focus, over }
	 *	fadeTime - time to fade between colour and over
	 *	border   - border size. Line if no sprite, else sprite border
	 *	sprite   - sprites can have multiple frames
	 *	frame	 - sprite frame
	 *	overFrame
	 *	focusFrame
	 * Use a GUI::Sprite class for the sprite
	 * 	width, height, texture, rows, columns
	 *
	 * Controls:
	 * 	Container	- contains child controls.
	 * 	Frame		- A container graphics which can be dragged
	 * 	Label		- Some text (no interaction)
	 * 	Button		- Changes on over/focus, has click event
	 * 	Listbox		- List of text items. Scrollable
	 * 	DropList	- Dropdown list (uses listbox)
	 * 	Input		- Text Input Field
	 *
	 * 	Checkbox	- Can be a full button, or a label with box. Can be radio button
	 **/

	/** GUI Sprites */
	class Sprite {
		public:
		Sprite(Texture* tex, int rows=1, int cols=1);
		void drawGlyph(int frame, int cx, int cy) const;
		void draw(int frame, int x, int y, int w, int h) const;
		void draw(int frame, int x, int y, int w, int h, int border) const;
		void draw(int frame, int x, int y, int w, int h, int t, int l, int r, int b) const;
		int width() const;
		int height() const;
		int frames() const { return m_rows * m_cols; }
		protected:
		Texture* m_texture;
		int m_rows, m_cols;
	};

	/** Gui style  */
	class Style {
		public:
		Style();
		Style(const Style* s);

		enum TextAlign { LEFT, CENTRE, RIGHT };
		Font* font;
		int align;

		float stime;

		//Colours
		enum ColourCode { BACK=0, BORDER=1, TEXT=2,  BASE=4, FOCUS=8, OVER=12 };
		void setColour(int code, const Colour& colour);
		inline const Colour& getColour(int code) const { return m_colours[(code&3) + (code&12? 3*((code>>2)-1): 0) ]; }

		protected:
		Colour m_colours[9];
	};

	/** The control with focus */
	extern Control* focus;
	extern Style defaultStyle;

	/** Event data */
	struct Event {
		uint command;
		uint value;
		const char* text;
		Control* control;
		Event() : command(0), value(0), text(0), control(0) {}
	};

	/** Base class for all GUI Elements */
	class Control {
		friend class Container;
		public:
		Control(Style* style=0, uint cmd=0);
		virtual ~Control();
		virtual uint update(Event* event=0);
		virtual void draw() = 0;
		void setPosition(int x, int y);	 //Relative positions
		const Point getPosition() const;
		virtual void setAbsolutePosition(int x, int y) { m_position.x=x; m_position.y=y; } //internal position
		virtual const Point& getAbsolutePosition() const { return m_position; }
		virtual void setSize(int width, int height) { m_position.y+=m_size.y-height; m_size.x=width, m_size.y=height; }
		const Point& getSize() const { return m_size; }
		void show() { m_visible=true; }
		void hide() { m_visible=false; }
		void setVisible(bool vis=true) { m_visible=vis; }
		void raise();	//Bring control to the front of the list (draws on top)
		bool visible() const { return m_visible; }
		virtual bool isContainer() const { return false; }
		Container* getParent() const { return m_container; }
		protected:
		Point m_position, m_size;
		Style* m_style;
		uint m_command;
		bool m_visible;
		Container* m_container;

		//Utilities
		float updateState(bool on, float state) const;
		inline bool isOver(int x, int y) const { return isOver(Point(x,y), m_position, m_size); }
		inline virtual bool isOver(const Point& pt, const Point& p, const Point& s) const {
			return pt.x>p.x && pt.x<p.x+s.x && pt.y>p.y && pt.y<p.y+s.y;
		}
		uint setEvent(Event* e, uint value=0, const char* txt=0);
		virtual uint click(Event* e, const Point& p) { return 0; }
		Point textPosition(const char* c, int oa=0, int ob=0); //Get relative text position
		//Drawing
		void drawFrame(const Point& p, const Point& s, const char* title=0, Style* style=0) const;
		void drawArrow(const Point& p, int direction, int size=8) const;
		void drawRect(int x, int y, int w, int h, const Colour& c = white) const;
		Colour blendColour(int type, int state, float value=1) const;
		void scissorPushNew(int x, int y, int w, int h) const;
		void scissorPush(int x, int y, int w, int h) const;
		void scissorPop() const;
	};

	/** Container class. Contains other controls */
	class Container : public Control {
		friend class Control;
		public:
		Container();
		Container(int x, int y, int w, int h, bool clip=false, Style* style=0);
		virtual ~Container() { clear(); }
		virtual uint update(Event* event=0);
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
		virtual uint click(Event* e, const Point& p);
		std::list<Control*> m_contents;
		bool m_clip;
	};

	/** Frame, A basic container with a border that can be dragged around */
	class Frame : public Container {
		public:
		Frame(int x, int y, int width, int height, const char* caption=0, bool moveable=0, Style* style=0);
		Frame(int width, int height, const char* caption=0, bool moveable=0, Style* style=0);
		virtual uint update(Event* event=0);
		void draw();
		protected:
		virtual uint click(Event* e, const Point& p);
		const char* m_caption;	// Frame Caption
		bool m_moveable;	// Can you drag the frame around
		Point m_held;		// Coordinates the frame is held at
	};

	/** Your basic label */
	class Label : public Control {
		public:
		Label(const char* caption, Style* style=0);
		void draw();
		void setCaption(const char* caption);
		const char* getCaption() const { return m_caption; }
		protected:
		inline virtual bool isOver(const Point& pt, const Point& p, const Point& s) const { return false; }
		const char* m_caption;
		int m_offset; //offset for alignment
	};


	/** A button */
	class Button : public Control {
		public:
		Button(const char* caption, Style* style=0, uint command=0);
		Button(int width, int height, const char* caption=0, Style* style=0, uint command=0);
		const char* getCaption() const { return m_caption; }
		virtual uint update(Event* event=0);
		virtual void draw();
		protected:
		virtual uint click(Event* e, const Point& p);
		const char* m_caption;
		float m_state;
		Point m_textPos;
	};

	class Checkbox : public Button {
		public:
		Checkbox(const char* caption, bool value=false, Style* style=0, uint command=0);
		bool getValue() const { return m_value; }
		void setValue(bool value) { m_value = value; }
		virtual uint update(Event* event=0);
		virtual void draw();
		protected:
		virtual uint click(Event* e, const Point& p);
		bool m_value;
	};

	/** Scrollar */
	class Scrollbar : public Control {
		friend class Listbox;
		public:
		Scrollbar(int orientation=0, int width=16, int height=120, int min=0, int max=100, Style* style=0, uint command=0); // 0:vertical, 1:horizontal 
		virtual uint update(Event* event=0);
		virtual void draw();
		void setValue(int value) { m_value=value<m_min?m_min: value>m_max? m_max: value; }
		int getValue() const { return m_value; }
		int getMin() const { return m_min; }
		int getMax() const { return m_max; }
		void setRange(int min=0,int max=100) { m_min=min, m_max=max; setValue(m_value); }
		protected:
		virtual uint click(Event* e, const Point& p);
		int m_min, m_max;
		int m_value;
		int m_orientation;
		int m_held;
		int m_blockSize;
		float m_repeat;
	};
	
	/** A scrollable listbox */
	class Listbox : public Control {
		public:
		Listbox(int width, int height, Style* style=0, uint command=0);
		virtual uint update(Event* event=0);
		virtual void draw();
		virtual void setSize(int width, int height); // resize scrollbar
		virtual void drawItem(uint index, const char* item, int x, int y, int width, int height, float state);
		virtual bool clickItem(uint index, const char* item, int x, int y) { return index!=m_item; }
		uint addItem(const char* item, bool selected=false);
		uint removeItem(uint index);
		uint count() const { return m_items.size(); }
		const char* getItem(uint index) const { return index<count()? m_items[index].name: 0; }
		const char* selectedItem() const { return getItem(m_item); }
		void clearItems();
		void scrollTo(uint index);
		void selectItem(uint index);
		protected:
		virtual void setAbsolutePosition(int x, int y); // move internal scrollbar
		virtual uint click(Event* e, const Point& p);
		struct ListItem { const char* name; float state; };
		std::vector<ListItem> m_items;
		uint m_itemHeight;	// Height of each item
		uint m_item;		// Selected Item
		uint m_hover;		// Hovered item
		Scrollbar m_scroll;	// Scrollbar
	};

	/** A Dropdown List */
	class DropList: public Listbox {
		public:
		DropList(int width, int height, int maxSize=0, Style* style=0, uint command=0);
		virtual uint update(Event* event=0);
		virtual void draw();
		protected:
		virtual uint click(Event* e, const Point& p);
		void openList();
		void closeList();
		int m_max;	//Maximum height of popup list
		bool m_open;	//Open state of the list
		float m_state;
	};

	/** A control for text input */
	class Input: public Control {
		public:
		Input(int length, const char* text=0, Style* style=0, uint command=0);
		virtual uint update(Event* event=0);
		virtual void draw();
		void setMask(char mask='*') { m_mask=mask; }
		void setText(const char* text);
		const char* getText() const { return m_text; }
		protected:
		virtual uint click(Event* e, const Point& p);
		char m_mask;
		char m_text[128];
		int m_loc, m_len;
		float m_state;
	};

};
};

#endif

