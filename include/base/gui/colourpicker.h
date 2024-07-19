#pragma once

#include "gui.h"
#include <base/math.h>
#include <base/colour.h>

namespace gui {
class Image;

class ColourPicker : public gui::Widget {
	WIDGET_TYPE(ColourPicker);
	ColourPicker();
	ColourPicker(Root*, int width, int height);
	~ColourPicker();
	void initialise(const gui::Root*, const gui::PropertyMap&) override;
	Point getPreferredSize(const Point& hint) const override;
	void refreshLayout() override;
	
	void setHasAlpha(bool);
	void setColour(const Colour& c);
	const Colour& getColour() const;

	public:
	Delegate<void(const Colour&)> eventChanged;

	private:
	void changeWheel(gui::Widget*, const Point&, int);
	void changeValue(gui::Widget*, const Point&, int);
	void changeAlpha(gui::Widget*, const Point&, int);
	Colour m_colour;

	static gui::IconList* createImages(gui::Root*, int size);

	private:
	Image* m_wheel = 0;
	Image* m_value = 0;
	Image* m_alpha = 0;
	Image* m_wheelMark = 0;
	Image* m_valueMark = 0;
	Image* m_alphaMark = 0;
};
}

