#ifndef _GUI_WIDGETS_
#define _GUI_WIDGETS_

#include "gui.h"

namespace gui {

/** Integer range structure */
struct Range {
	Range(int min=0, int max=0) : min(min), max(max) {}
	int min, max;
};

/** Labels are intangible */
class Label : public Widget {
	WIDGET_TYPE(Label);
	Label(const Rect&, Skin*, const char* c="");
	virtual void draw() const override;
	virtual void setSize(int w, int h) override;
	virtual Point getPreferredSize() const override;
	virtual void setCaption(const char*);
	virtual const char* getCaption() const;
	virtual void setWordWrap(bool w);
	virtual void setFontSize(int s);	// set to 0 to use default size
	virtual void setFontAlign(int a);	// use 0 for skin default
	protected:
	virtual void initialise(const Root*, const PropertyMap&) override;
	virtual Widget* clone(const char*) const override;
	void updateAutosize() override;
	void updateWrap();
	String m_caption;
	int   m_fontSize;	// Override font sise
	int   m_fontAlign;	// Override font align
	bool  m_wordWrap;	// Word wrap option
	std::vector<int> m_wrapValues; // Length of each line

};

/** Icon image */
class Icon : public Widget {
	WIDGET_TYPE(Icon);
	Icon(const Rect& r, Skin*);
	IconList* getIconList() const;							// Get icon set
	void setIcon(IconList* list, int index, int alt=-1);	// Set iconSet and index
	void setIcon(const char* name);							// Set icon index by name
	void setIcon(int index);								// Set icon index
	int  getIcon() const;									// Get icon index
	const char* getIconName() const;
	void setAltIcon(int index);
	void setAltIcon(const char* name);
	int  getAltIcon() const;								// Get icon index for alternate icon (when selected)
	void  setAngle(float a) { m_angle=a; }
	float getAngle() const { return m_angle; }
	void draw() const override;
	void initialise(const Root*, const PropertyMap&) override;
	Point getPreferredSize() const override;
	protected:
	virtual Widget* clone(const char*) const override;
	IconList* m_iconList;
	int       m_iconIndex;
	int       m_iconIndexAlt;
	float     m_angle;
};

/** Interface for accessing an icon subwidget called '_icon' */
class IconInterface {
	public:
	void initialiseIcon(Widget*, const Root*, const PropertyMap&);
	void setIcon(IconList* list, int index, int alt=-1);
	void setIcon(const char* name);
	void setIcon(int index);
	int  getIcon() const;
	const char* getIconName() const;
	void setAltIcon(const char* name);
	void setAltIcon(int index);
	int  getAltIcon() const;
	void setIconColour(unsigned rgb, float a=1);
	private:
	Icon* m_icon = nullptr;
};

/** Image */
class Image : public Widget {
	WIDGET_TYPE(Image);
	Image(const Rect&, Skin*);
	void  setImage(const char* file);	// Set image - uses renderer::addImage();
	void  setImage(int imageID);			// Set image to renderer image index
	int   getImage() const;
	void  setAngle(float a) { m_angle=a; }
	float getAngle() const { return m_angle; }
	void  draw() const override;
	Point getPreferredSize() const override;
	protected:
	virtual void initialise(const Root*, const PropertyMap&) override;
	virtual Widget* clone(const char*) const override;
	int      m_image;
	float    m_angle;
};

/** Basic Button */
class Button : public Label, public IconInterface {
	WIDGET_TYPE(Button);
	public:
	Button(const Rect&, Skin*, const char* c="");
	void setCaption(const char*) override;
	const char* getCaption() const override;
	void draw() const override;
	Point getPreferredSize() const override;
	public: // Events
	Delegate<void(Button*)> eventPressed;
	protected:
	virtual void initialise(const Root*, const PropertyMap&) override;
	void onMouseButton(const Point&, int, int) override;
	void updateAutosize() override;
};

/** Checkbox */
class Checkbox : public Button {
	WIDGET_TYPE(Checkbox);
	Checkbox(const Rect& r, Skin* s, const char* t="") : Button(r, s, t) {}
	bool isChecked() const { return isSelected(); }
	void setChecked(bool c) { setSelected(c); }
	public:
	Delegate<void(Button*)> eventChanged;
	protected:
	virtual void initialise(const Root*, const PropertyMap&) override;
	void onMouseButton(const Point&, int, int) override;
	void onMouseMove(const Point&, const Point&, int) override;
	void onKey(int code, wchar_t chr, KeyMask mask) override;
	Widget* clone(const char*) const override;
	protected:
	bool m_dragMode = false;
};

/** Drag / resize handle component */
class DragHandle : public Widget {
	WIDGET_TYPE(DragHandle);
	enum Mode { MOVE, SIZE };
	DragHandle(const Rect& r, Skin* s, Mode mode=MOVE) : Widget(r,s), m_mode(mode) {}
	void initialise(const Root*, const PropertyMap&) override;
	Widget* clone(const char*) const override;
	protected:
	void onMouseMove(const Point&, const Point&, int) override;
	void onMouseButton(const Point&, int, int) override;
	Mode m_mode;
	bool m_clamp = false;
	Point m_held;
};

/** Progress bar */
class ProgressBar : public Widget {
	WIDGET_TYPE(ProgressBar);
	ProgressBar(const Rect&, Skin*, Orientation o=HORIZONTAL);
	virtual void setSize(int w, int h) override;
	void  setRange(float min, float max);
	void  setValue(float);
	float getRange() const;
	float getValue() const;
	void  setBarColour(int c);
	protected:
	virtual void initialise(const Root*, const PropertyMap&) override;
	protected:
	float m_min, m_max, m_value;
	int m_borderLow, m_borderHigh;
	Orientation m_mode;
	Widget* m_progress;
};

/** Textbox - editable text */
class Textbox : public Widget {
	WIDGET_TYPE(Textbox);
	Textbox(const Rect& r, Skin*);
	~Textbox();
	void draw() const override;
	void setPosition(int x, int y) override;
	void updateAutosize() override;
	const char* getText() const { return m_text; }
	const char* getSelectedText() const;
	int getLineCount() const;
	void setText(const char*);
	void insertText(const char*);
	void select(int start, int len=0, bool shift=false);
	void setMultiLine(bool);
	void setReadOnly(bool r);
	void setPassword(char character);
	void setSuffix(const char*);
	void setHint(const char*);
	public:
	Delegate<void(Textbox*, const char*)> eventChanged;
	Delegate<void(Textbox*)>              eventSubmit;

	protected:
	Widget* clone(const char*) const override;
	void initialise(const Root*, const PropertyMap&) override;
	void onKey(int code, wchar_t key, KeyMask mask) override;
	void onMouseButton(const Point&, int, int) override;
	void onMouseMove(const Point&, const Point&, int) override;
	void drawText(Point& p, const char* t, uint len, uint col) const;
	int  indexAt(const Point&) const;
	void updateOffset(bool end);
	void updateLineData();
	private:
	char*  m_text;
	int    m_buffer;
	int    m_length;
	int    m_cursor;
	int    m_selectLength;
	Rect   m_selectRect;
	uint   m_selectColour;
	bool   m_multiline;
	bool   m_readOnly;
	char   m_password;
	String m_suffix;
	int    m_held;
	int    m_offset;
	String m_hint;
	mutable char* m_selection;
	std::vector<int> m_lines; // Start of each line
};

/** Spinbox - Editable number with up/down buttons */
template<typename T>
class SpinboxT : public Widget {
	public:
	SpinboxT(const Rect&, Skin*, const char* format);
	void initialise(const Root*, const PropertyMap&) override;
	T  getValue() const;
	void setValue(T value, bool fireChangeEvent=false);
	void setRange(T min, T max);
	void setSuffix(const char* s);
	void setStep(T button, T wheel);
	protected:
	void mouseWheel(Widget*, int w);
	void textChanged(Textbox*, const char*);
	void textSubmit(Textbox*);
	void textLostFocus(Widget*);
	void pressInc(Button*);
	void pressDec(Button*);
	void parseText(bool evt);
	virtual void fireChanged() = 0;
	protected:
	Textbox* m_text;
	T        m_value;
	T        m_min;
	T        m_max;
	T        m_buttonStep;
	T        m_wheelStep;
	bool     m_textChanged;
	String   m_previous;
	const char* m_format;
};
class Spinbox : public SpinboxT<int> {
	WIDGET_TYPE(Spinbox);
	Spinbox(const Rect&, Skin*);
	Delegate<void(Spinbox*,int)> eventChanged;
	void fireChanged() override;
};
class SpinboxFloat : public SpinboxT<float> {
	WIDGET_TYPE(SpinboxFloat);
	SpinboxFloat(const Rect&, Skin*);
	Delegate<void(SpinboxFloat*,float)> eventChanged;
	void fireChanged() override;
};


/** Scrollbar - Can also be used for sliders */
class Scrollbar : public Widget {
	WIDGET_TYPE(Scrollbar);
	Scrollbar(const Rect& r, Skin*, int min=0, int max=100);
	int   getValue() const;
	void  setValue(int value);
	void  setRange(int min, int max, int step=1);
	void  setSize(int width, int height) override;
	const Range& getRange() const;
	int   getStep() const;
	Orientation getOrientation() const;
	void setBlockSize(float rel);
	using Widget::setSize;
	public:
	Delegate<void(Scrollbar*, int)> eventChanged;
	protected:
	void initialise(const Root*, const PropertyMap&) override;
	Widget* clone(const char*) const override;
	protected:
	void onMouseButton(const Point&, int, int) override;
	bool onMouseWheel(int w) override;
	void scrollInc(Widget*, const Point&, int);
	void scrollDec(Widget*, const Point&, int);
	void pressBlock(Widget*, const Point&, int);
	void moveBlock(Widget*, const Point&, int);
	void updateBlock();
	int  getPixelRange() const;
	protected:
	Orientation m_mode;
	Range       m_range;
	int         m_value;
	int         m_step;
	Point       m_held;
	Widget*     m_block;
};

/** Scrollpane - sets up events from template. Needs _client, _vscroll, _hscroll */
class Scrollpane : public Widget {
	WIDGET_TYPE(Scrollpane);
	Scrollpane(const Rect& r, Skin*);
	void initialise(const Root*, const PropertyMap&) override;
	Point getOffset() const;					// Get scroll offset
	void setOffset(int x, int y, bool force=false);			// Set scroll offset
	void setOffset(const Point& o) { setOffset(o.x,o.y); } 	// Set scroll offset
	const Point& getPaneSize() const;			// Get size of scrollable panel
	void setPaneSize(int width, int height);	// Set size of scrollable panel
	void useFullSize(bool m);					// Minimum size of panel is widget size. Also locks size if no scrollbar
	Widget* clone(const char*) const override;
	Point getPreferredSize() const override;
	using Widget::add;							// import add funtions from widget
	void add(Widget* w, unsigned index) override;// Adding child widgets resizes pane if in autosize mode
	void setSize(int w, int h) override;
	Widget* getViewWidget() const;				// Get viewport widget - mey need for events if client is smaller
	void ensureVisible(const Point& pos);		// Set scrollbars so a point is within view
	public:
	Delegate<void(Scrollpane*)> eventChanged;	// Scroll viewport changed
	private:
	void updateAutosize() override;
	void scrollChanged(Scrollbar*, int);
	bool onMouseWheel(int) override;
	bool m_alwaysShowScrollbars;
	bool m_useFullSize;
	Scrollbar* m_vScroll;
	Scrollbar* m_hScroll;
};

/** TabbedPane */
class TabbedPane : public Widget {
	WIDGET_TYPE(TabbedPane);
	TabbedPane(const Rect& r, Skin*);
	void        initialise(const Root*, const PropertyMap&) override;
	void        add(Widget* w, unsigned index) override;
	Widget*     addTab(const char* name, Widget* frame=0, int index=-1);
	Widget*     getTab(const char* name);
	Widget*     getTab(int index);
	int         getTabCount() const;
	void        removeTab(const char* name, bool destroy=true);
	void        removeTab(int index, bool destroy=true);
	void        selectTab(const char* name);
	void        selectTab(int index);
	Widget*     getCurrentTab();
	int         getCurrentTabIndex() const;
	const char* getCurrentTabName() const;
	const char* getTabName(int index) const;
	int         getTabIndex(const char* name) const;
	public:
	Delegate<void(TabbedPane*)> eventTabChanged;
	protected:
	Widget* getTabButton(int index);
	void    onTabButton(Button*);
	void    layoutTabs();
	Widget* m_buttonTemplate;
	Widget* m_tabStrip;
	Widget* m_tabFrame;
	int     m_currentTab;
	bool    m_autoSizeTabs;
};

/** Collapseable pane */
class CollapsePane : public Widget {
	WIDGET_TYPE(CollapsePane);
	CollapsePane(const Rect&, Skin*);
	void initialise(const Root*, const PropertyMap&) override;
	Widget* clone(const char*) const override;
	bool isExpanded() const;
	void expand(bool);
	void setCaption(const char*);
	void setChecked(bool c);
	void setMoveable(bool);
	bool isChecked() const;
	const char* getCaption() const;
	public:
	Delegate<void(CollapsePane*, int)> eventReordered;
	Delegate<void(CollapsePane*, bool)> eventExpanded;
	Delegate<void(CollapsePane*, bool)> eventChecked;
	protected:
	void toggle(Button*);
	void checkChanged(Button*);
	void dragStart(Widget*, const Point&, int);
	void dragMove(Widget*, const Point&, int);
	void updateAutosize() override;
	protected:
	Button* m_header;
	Checkbox* m_check;
	bool m_collapsed;
	bool m_moveable;
	Point m_held;
};

/** SplitPane */
class SplitPane : public Widget {
	WIDGET_TYPE(SplitPane);
	enum ResizeMode { ALL, FIRST, LAST };
	SplitPane(const Rect& r, Skin*, Orientation orientation=VERTICAL);
	virtual void initialise(const Root*, const PropertyMap&) override;
	virtual Widget* clone(const char*) const override;
	virtual void add(Widget* w, unsigned index) override;
	virtual void setSize(int w, int h) override;
	virtual bool remove(Widget* w) override; 
	void setResizeMode(ResizeMode);	// Affects how children are resized
	void setSplit(int split);	// Set split in pixels
	void setSplit(float split);	// Set split percentage { 0 - 1 }
	Orientation getOrientation() const;
	using Widget::setSize;
	protected:
	void updateAutosize() override {}
	void setupSash(Widget*);
	void validateLayout();
	void refreshLayout() override;
	void pressSash(Widget*, const Point&, int);
	void moveSash(Widget*, const Point&, int);
	using Widget::getWidget;
	protected:
	Widget* getWidget(const Point& p, int, bool, bool) override;
	protected:
	Orientation m_mode;
	int         m_held;
	int         m_heldIndex;
	int         m_minSize;
	ResizeMode  m_resizeMode;
	Widget*     m_sash;
};


/** Window - dragable, resizable, X */
class Window : public Widget, public IconInterface {
	WIDGET_TYPE(Window);
	Window(const Rect& r, Skin*);
	void setCaption(const char*);
	const char* getCaption() const;
	void setMinimumSize(int w, int h);
	public:
	Delegate<void(Window*)> eventClosed;
	protected:
	void initialise(const Root*, const PropertyMap&) override;
	void onMouseButton(const Point&, int, int) override;
	Button* m_title;
	private:
	void pressClose(Button*);
	void grabHandle(Widget*, const Point&, int);
	void dragHandle(Widget*, const Point&, int);
	void sizeHandle(Widget*, const Point&, int);
	Point m_held;
	Point m_minSize;
};

class Popup : public Widget {
	WIDGET_TYPE(Popup);
	enum Side { LEFT, RIGHT, ABOVE, BELOW };
	typedef Delegate<void(Button*)> ButtonDelegate;
	public:
	Popup(const Rect& r, Skin*);
	void popup(Widget* owner, Side side=BELOW);
	void popup(Root* root, const Point& absolutePosition, Widget* owner=0);
	Widget* addItem(Root* root, const char* button, const char* name="", const char* caption="", ButtonDelegate event=ButtonDelegate());
	void hide();
	protected:
	void initialise(const Root*, const PropertyMap&) override;
	void lostFocus(Widget*);
	protected:
	Widget* m_owner;
	std::vector<Popup*> m_owned; // Used for child popups
	Delegate<void(Widget*)> ownerLoseFocus;
};


}

#endif

