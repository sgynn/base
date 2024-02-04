#pragma once

#include "gui.h"
#include <base/math.h>
#include <base/colour.h>

namespace gui { class Icon; }

class ColourPicker : public gui::Widget {
	WIDGET_TYPE(ColourPicker);
	ColourPicker(const Rect&, gui::Skin*);
	~ColourPicker();
	void initialise(const gui::Root*, const gui::PropertyMap&) override;
	Point getPreferredSize() const override;
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
	gui::Icon* m_wheel = 0;
	gui::Icon* m_value = 0;
	gui::Icon* m_alpha = 0;
	gui::Icon* m_wheelMark = 0;
	gui::Icon* m_valueMark = 0;
	gui::Icon* m_alphaMark = 0;
};

