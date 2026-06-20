#pragma once

#include <base/gui/gui.h>
#include <base/planevolume.h>

namespace base { class Camera; }

namespace gui {
class SelectionBox : public gui::Widget {
	public:
	SelectionBox();
	void update(int gameStateFlags);
	void cancel();
	bool isActive() const;
	void draw() const override;
	Delegate<void(SelectionBox*)> eventApply;
	base::PlaneVolume getVolume(base::Camera*) const;
	protected:
	int m_state;
	Point m_start;
	Point m_end;
	base::PlaneVolume m_volume;
};
}

