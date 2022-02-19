#ifndef _GUI_SKIN_
#define _GUI_SKIN_

#include <base/point.h>
#include <vector>

namespace gui {

class Font;

/** Text alignment */
enum TextAlign { ALIGN_LEFT=1, ALIGN_RIGHT=2, ALIGN_CENTER=3, ALIGN_TOP=4, ALIGN_BOTTOM=8, ALIGN_MIDDLE=12 };

class Skin {
	public:
	/** State names. SELECTED is used as a bitflag */
	enum StateName { NORMAL=0, OVER=1, PRESSED=2, DISABLED=3, SELECTED=4 };

	/** Image borders */
	struct Border {
		Border(short b=0) : left(b), top(b), right(b), bottom(b) {}
		Border(short t, short l, short r, short b) : left(l), top(t), right(r), bottom(b) {}
		short left, top, right, bottom;
	};

	/** State info */
	struct State {
		Rect     rect;		// Source rect on image (pixels)
		Border   border;	// Border info
		unsigned foreColour;// Text/Icon colour
		unsigned backColour;// Widget colour
		Point    textPos;	// Text position offset
	};

	public:
	Skin(int states=0);
	~Skin();
	Skin(const Skin&);

	State& getState(int s) const {
		return m_stateCount==1? m_states[0]: m_stateCount==4? m_states[s&3]: m_states[s];
	}
	Font* getFont() const		{ return m_font; }
	int   getFontSize() const	{ return m_fontSize; }
	int   getFontAlign() const  { return m_fontAlign; }
	int   getImage() const		{ return m_image; }
	int   getStateCount() const { return m_stateCount; }

	void  setImage(int index) { m_image = index; }
	void  setFontSize(int s) { m_fontSize = s; }
	void  setFontAlign(int a) { m_fontAlign = a; }
	void  setFont(Font* font, int size, int align=5) { m_font=font; m_fontSize=size; m_fontAlign=align; }
	void  setState(int state, const Rect& rect, const Border& border=0, unsigned foreColour=0xffffffff, unsigned backColour=0xffffffff, const Point& textPos=Point());
	void  setState(int state, const State& data);
	void  setStateCount(int count); // 1, 4, 8;

	protected:
	State*    m_states;
	int       m_stateCount; // 1, 4, 8;
	int       m_definedStates;
	Font*     m_font;
	int       m_fontSize;
	int       m_fontAlign;
	int       m_image;
};

/** List of icons */
class IconList {
	public:
	int addIcon(const char* name, const Rect& r);	// Add an icon rect
	int getIconIndex(const char* name) const;		// Get index of named icon
	int getImageIndex() const;						// Get image index, same as in Skin
	int size() const;								// Number of icons
	const char* getIconName(int index) const;		// Get the name of an icon
	const Rect& getIconRect(int index) const;		// Get icon source rectangle
	void setImageIndex(int index);					// Set the image. Index is defined in renderer
	void setIconRect(int index, const Rect& rect);	// Update icon rect
	void setIconName(int index, const char* name);	// Update icon name
	void deleteIcon(int index);						// Delete an icon from the set
	void clear();									// Delete all icons
	protected:
	struct Icon { char name[64]; Rect rect; };
	std::vector<Icon> m_icons;
	int m_image;
};

}

#endif

