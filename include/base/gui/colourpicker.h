#pragma once

#include "gui.h"
#include <base/math.h>
#include <base/colour.h>

namespace gui { class Image; }

class ColourPicker : public gui::Widget {
	WIDGET_TYPE(ColourPicker);
	ColourPicker(const Rect&, gui::Skin*);
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
	gui::Image* m_wheel = 0;
	gui::Image* m_value = 0;
	gui::Image* m_alpha = 0;
	gui::Image* m_wheelMark = 0;
	gui::Image* m_valueMark = 0;
	gui::Image* m_alphaMark = 0;
};

