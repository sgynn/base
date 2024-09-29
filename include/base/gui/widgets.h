#pragma once

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
	Label(const char* c="");
	Label(const char* caption, Font* font, int size=0);
	virtual void draw() const override;
	virtual void setSize(int w, int h) override;
	virtual Point getPreferredSize(const Point& hint=Point()=Point()) const override;
	virtual void setCaption(const char*);
	virtual const char* getCaption() const;
	virtual void setWordWrap(bool w);
	virtual void setFontSize(int s);	// set to 0 to use default size
	virtual void setFontAlign(int a);	// use 0 for skin default
	virtual void setFont(Font*, int size=0);
	protected:
	void initialise(const Root*, const PropertyMap&) override;
	void copyData(const Widget*) override;
	void updateAutosize() override;
	void updateWrap();
	String m_caption;
	Font* m_font = nullptr; // Override font
	int   m_fontSize;	// Override font sise
	int   m_fontAlign;	// Override font align
	bool  m_wordWrap;	// Word wrap option
	std::vector<int> m_wrapValues; // Length of each line

};

/** Image */
class Image : public Widget {
	WIDGET_TYPE(Image);
	Image();
	Image(int width, int height);
	Image(Root*, const char*);
	Image(IconList*, const char*);
	void  setImage(int key);
	void  setImage(const char* name);
	void  setImage(Root*, const char* name);
	void  setImage(IconList* group, int index);
	void  setImage(IconList* group, const char* name);
	int   getImageIndex() const;
	const char* getImageName() const;
	IconList* getGroup() const;

	void  setAngle(float a) { m_angle=a; }
	float getAngle() const { return m_angle; }
	void  draw() const override;
	Point getPreferredSize(const Point& hint=Point()) const override;

	protected:
	friend class IconInterface;
	static int findImage(const Root*, IconList* list, const char* property);
	void initialise(const Root*, const PropertyMap&) override;
	void copyData(const Widget*) override;
	IconList* m_group;
	int       m_image;
	float     m_angle;
};

// Accessor interface for widgets that contain an icon
class IconInterface {
	public:
	void  setIcon(int key);
	void  setIcon(const char* name);
	void  setIcon(IconList* group, int index);
	void  setIcon(IconList* group, const char* name);
	void  setIconColour(unsigned rgb, float a=1);
	int   getIconIndex() const;
	Image*      getIcon() const { return m_icon; }
	IconList*   getIconGroup() const;
	const char* getIconName() const;
	protected:
	IconInterface() {}
	void initialiseIcon(Widget*, const Root*, const PropertyMap&);
	static int findImage(const Root* r, IconList* list, const char* property) { return Image::findImage(r,list,property); }
	private:
	Image* m_icon = nullptr;
};

/** Basic Button */
class Button : public Label, public IconInterface {
	WIDGET_TYPE(Button);
	public:
	Button(const char* caption="");
	void setCaption(const char*) override;
	const char* getCaption() const override;
	void draw() const override;
	Point getPreferredSize(const Point& hint=Point()) const override;
	public: // Events
	Delegate<void(Button*)> eventPressed;
	protected:
	virtual void initialise(const Root*, const PropertyMap&) override;
	void onMouseButton(const Point&, int, int) override;
	protected:
	Label* m_textWidget = nullptr;
};

/** Checkbox */
class Checkbox : public Button {
	WIDGET_TYPE(Checkbox);
	Checkbox(const char* text="") : Button(text) {}
	void setSelected(bool) override;
	bool isChecked() const { return isSelected(); }
	void setChecked(bool c) { setSelected(c); }
	void setDragSelect(int s) { m_dragMode = s; }
	void setIcon(IconList* list, int checked, int unchecked=-1);
	void setIcon(IconList* list, const char* checked, const char* unchecked=0);
	void setIcon(int checked, int unchecked=-1) { setIcon(getIconGroup(), checked, unchecked); }
	void setIcon(const char* checked, const char* unchecked=0) { setIcon(getIconGroup(), checked, unchecked); }
	public:
	Delegate<void(Button*)> eventChanged;
	protected:
	void initialise(const Root*, const PropertyMap&) override;
	void onMouseButton(const Point&, int, int) override;
	void onMouseMove(const Point&, const Point&, int) override;
	void onKey(int code, wchar_t chr, KeyMask mask) override;
	void copyData(const Widget*) override;
	protected:
	int m_dragMode = 0;
	int m_checkedIcon = -1;
	int m_uncheckedIcon = -1;
};

/** Drag / resize handle component */
class DragHandle : public Widget {
	WIDGET_TYPE(DragHandle);
	enum Mode { MOVE, SIZE, ORDER };
	DragHandle(Mode mode=MOVE) : m_mode(mode) {}
	void initialise(const Root*, const PropertyMap&) override;
	void copyData(const Widget*) override;
	Mode getMode() const { return m_mode; }
	Delegate<void(Widget*, Mode mode)> eventEndDrag; // Widget is target
	protected:
	void onMouseMove(const Point&, const Point&, int) override;
	void onMouseButton(const Point&, int, int) override;
	Widget* m_target = nullptr;
	Mode m_mode;
	bool m_clamp = false;
	Point m_held;
	int m_index;
};

/** Progress bar */
class ProgressBar : public Widget {
	WIDGET_TYPE(ProgressBar);
	ProgressBar(Orientation o=HORIZONTAL);
	void setSize(int w, int h) override;
	void copyData(const Widget*) override;
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
	enum SubmitOption { KeepFocus, ClearFocus, SubmitOnLoseFocus };
	Textbox();
	~Textbox();
	void draw() const override;
	void setPosition(int x, int y) override;
	Point getPreferredSize(const Point& hint=Point()) const override;
	const char* getText() const { return m_text; }
	const char* getSelectedText() const;
	int getLineCount() const;
	int getLineNumber(uint characterIndex) const;
	void setText(const char*);
	void insertText(const char*);
	void select(int start, int len=0, bool shift=false);
	void setMultiLine(bool);
	void setReadOnly(bool r);
	void setPassword(char character);
	void setSuffix(const char*);
	void setHint(const char*);
	void setSubmitAction(SubmitOption);
	public:
	Delegate<void(Textbox*, const char*)> eventChanged;
	Delegate<void(Textbox*)>              eventSubmit;

	protected:
	void copyData(const Widget*) override;
	void initialise(const Root*, const PropertyMap&) override;
	void onKey(int code, wchar_t key, KeyMask mask) override;
	void onMouseButton(const Point&, int, int) override;
	void onMouseMove(const Point&, const Point&, int) override;
	void onLoseFocus() override;
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
	SubmitOption m_submitAction = KeepFocus;
	String m_hint;
	mutable char* m_selection;
	std::vector<int> m_lines; // Start of each line
};

/** Spinbox - Editable number with up/down buttons */
template<typename T>
class SpinboxT : public Widget {
	public:
	SpinboxT(const char* format);
	void initialise(const Root*, const PropertyMap&) override;
	T  getValue() const;
	void setValue(T value, bool fireChangeEvent=false);
	void setRange(T min, T max);
	void setSuffix(const char* s);
	void setStep(T button, T wheel);
	protected:
	void copyData(const Widget*) override;
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
	Spinbox();
	Delegate<void(Spinbox*,int)> eventChanged;
	void fireChanged() override;
};
class SpinboxFloat : public SpinboxT<float> {
	WIDGET_TYPE(SpinboxFloat);
	SpinboxFloat();
	Delegate<void(SpinboxFloat*,float)> eventChanged;
	void fireChanged() override;
};


/** Scrollbar - Can also be used for sliders */
class Scrollbar : public Widget {
	WIDGET_TYPE(Scrollbar);
	Scrollbar(Orientation orientation=VERTICAL, int min=0, int max=100);
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
	void copyData(const Widget*) override;
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
	Scrollpane();
	void initialise(const Root*, const PropertyMap&) override;
	Point getOffset() const;					// Get scroll offset
	void setOffset(int x, int y, bool force=false);			// Set scroll offset
	void setOffset(const Point& o) { setOffset(o.x,o.y); } 	// Set scroll offset
	const Point& getPaneSize() const;			// Get size of scrollable panel
	void setPaneSize(int width, int height);	// Set size of scrollable panel
	void useFullSize(bool m);					// Minimum size of panel is widget size. Also locks size if no scrollbar
	void alwaysShowScrollbars(bool s);				// Always show scrollbars. False only shows them when needed.
	void copyData(const Widget*) override;
	Point getPreferredSize(const Point& hint=Point()) const override;
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

/** Slider - For float value input. Can have two blocks for min and max */
class Slider : public Widget {
	WIDGET_TYPE(Slider);
	struct Range { float min, max; operator float() const { return min; } operator float*() { return &min; } };
	Slider();
	void setValue(float value, bool triggerEvent=false);
	void setValue(float min, float max, bool triggerEvent=false);
	void setRange(float min, float max);
	void setSize(int width, int height) override;
	void setQuantise(float step);
	float getQuantise() const { return m_quantise; }
	const Range& getValue() const { return m_value; }
	const Range& getRange() const { return m_range; }
	Orientation getOrientation() const { return m_orientation; }
	using Widget::setSize;
	public:
	Delegate<void(Slider*, const Range&)> eventChanged;
	private:
	void initialise(const gui::Root*, const gui::PropertyMap&) override;
	void copyData(const Widget*) override;
	bool onMouseWheel(int w) override;
	void pressBlock(Widget*, const Point&, int);
	void moveBlock(Widget*, const Point&, int);
	void updateBlock();
	int  getPixelRange() const;
	protected:
	Orientation m_orientation;
	Widget* m_block[2];
	int     m_held = -1;
	Range   m_range = {0,1};
	Range   m_value = {0,1};
	float   m_quantise=0;
};



/** TabbedPane */
class TabbedPane : public Widget {
	WIDGET_TYPE(TabbedPane);
	TabbedPane();
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
	CollapsePane();
	void initialise(const Root*, const PropertyMap&) override;
	void copyData(const Widget*) override;
	bool isExpanded() const;
	bool isChecked() const;
	void expand(bool);
	void setCaption(const char*);
	void setChecked(bool);
	const char* getCaption() const;
	public:
	Delegate<void(CollapsePane*, int)> eventReordered;
	Delegate<void(CollapsePane*, bool)> eventExpanded;
	Delegate<void(CollapsePane*, bool)> eventChecked;
	protected:
	void toggle(Button*);
	void checkChanged(Button*);
	void updateAutosize() override;
	void onChildChanged(Widget*) override;
	protected:
	Button* m_header;
	Checkbox* m_check;
	Widget* m_stateWidget;
	bool m_collapsed;
	char m_expandAnchor;
};

/** SplitPane */
class SplitPane : public Widget {
	WIDGET_TYPE(SplitPane);
	enum ResizeMode { ALL, FIRST, LAST };
	SplitPane(Orientation orientation=VERTICAL);
	void initialise(const Root*, const PropertyMap&) override;
	void copyData(const Widget*) override;
	void add(Widget* w, unsigned index) override;
	void setSize(int w, int h) override;
	bool remove(Widget* w) override; 
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
	Widget* getWidget(const Point& p, int, bool, bool, bool) override;
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
	Window();
	void setCaption(const char*);
	const char* getCaption() const;
	void setMinimumSize(int w, int h);
	public:
	Delegate<void(Window*)> eventClosed;
	Delegate<void(Window*)> eventResized;
	protected:
	void initialise(const Root*, const PropertyMap&) override;
	void onMouseButton(const Point&, int, int) override;
	void onMouseMove(const Point&, const Point&, int) override;
	void onChildMouseDown(Widget* c, int b) override;
	Button* m_title;
	private:
	void pressClose(Button*);
	void grabHandle(Widget*, const Point&, int);
	void dragHandle(Widget*, const Point&, int);
	void sizeHandle(Widget*, const Point&, int);
	Point m_held;
	Point m_minSize;
};

// Popup menu
class Popup : public Widget {
	WIDGET_TYPE(Popup);
	enum Side { LEFT, RIGHT, ABOVE, BELOW };
	typedef Delegate<void(Button*)> ButtonDelegate;
	public:
	Popup();
	Popup(Widget* contents, bool destroyOnClose=false);
	Popup(int width, int height, bool destroyOnClose=false);
	~Popup();
	void popup(Widget* owner, Side side=BELOW);
	void popup(Root* root, const Point& absolutePosition, Widget* owner=0);
	Widget* addItem(Root* root, const char* button, const char* name="", const char* caption="", ButtonDelegate event=ButtonDelegate());
	template<class F> Widget* addItem(Root* r, const char* type, const char* caption, F&& func) {
		Widget* w = addItem(r, type, "", caption, {});
		if(Button* b = cast<Button>(w)) b->eventPressed.bind(func);
		return w;
	}
	Widget* getOwner() { return m_owner; }
	void addOwnedPopup(Popup*);
	void hideOwnedPopups();
	void hide();
	protected:
	void initialise(const Root*, const PropertyMap&) override;
	void lostFocus(Widget*);
	protected:
	bool m_destroyOnClose = false;
	Widget* m_owner = nullptr;
	std::vector<Popup*> m_owned; // Used for child popups
	Delegate<void(Widget*)> ownerLoseFocus;
};

// A container that can scale its contents
class ScaleBox : public Widget {
	WIDGET_TYPE(ScaleBox);
	public:
	ScaleBox();
	void initialise(const Root*, const PropertyMap&) override;
	Point getPreferredSize(const Point& hint=Point()) const override { return getSize(); }
	void setPosition(int x, int y) override;
	void setSize(int w, int h) override;
	void updateAutosize() override;
	void setScale(float scale);
	float getScale() const { return m_scale; }
	protected:
	Widget* getWidget(const Point&, int mask, bool intangible, bool templates, bool clip) override;
	void updateTransforms() override;
	float m_scale = 1;
};


}

