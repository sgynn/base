#pragma once

#include "gui.h"

namespace gui {
	
class Animator {
	public:
	enum Ease { Linear, Smooth, SmoothIn, SmoothOut, Bounce, Spring, Overshoot };
	enum State { ACTIVE, ENDED };
	Animator(Widget* target, bool transient=true) : m_widget(target), m_transient(transient) {}
	virtual ~Animator() {}
	virtual State update(float time) = 0;
	Widget* getWidget() const { return m_widget; }
	bool isTransient() const { return m_transient; }
	inline static float ease(float t, Ease mode) {
		constexpr float B1 = 1 / 2.75;
		constexpr float B2 = 2 / 2.75;
		constexpr float B3 = 1.5 / 2.75;
		constexpr float B4 = 2.5 / 2.75;
		constexpr float B5 = 2.25 / 2.75;
		constexpr float B6 = 2.625 / 2.75;

		switch(mode) {
		case Linear: return t;
		case Smooth: return 3*t*t-2*t*t*t;
		case SmoothIn: return t*t;
		case SmoothOut: return 1-(1-t)*(1-t);
		case Bounce: 
			if (t < B1) return 7.5625 * t * t;
			if (t < B2) return 7.5625 * (t - B3) * (t - B3) + .75;
			if (t < B4) return 7.5625 * (t - B5) * (t - B5) + .9375;
			return 7.5625 * (t - B6) * (t - B6) + .984375;
		case Spring:
			return 1 - cos(11 * t) * pow(3, -3*t);
		case Overshoot:
			return 1 - cos(4.7124 * t) * pow(3, -3*t);
		}
		return t;
	}
	// Handy interpolation functions
	inline static float lerp(float a, float b, float t) { return a*(1-t) + b*t; }
	inline static Point lerp(const Point& a, const Point& b, float t) {
		return Point(floor(a.x * (1-t) + b.x * t + 0.5), floor(a.y * (1-t) + b.y * t + 0.5));
	}
	inline static unsigned lerp(unsigned a, unsigned b, float t) { // Colour
		float st = 1-t;
		return (unsigned)((a&0xff)*st + (b&0xff)*t)
		     | ((unsigned)((a&0xff00)*st + (b&0xff00)*t) & 0xff00)
		     | ((unsigned)((a&0xff0000)*st + (b&0xff0000)*t) & 0xff0000)
		     | ((unsigned)((a&0xff000000)*st + (b&0xff000000)*t) & 0xff0000);
	}

	protected:
	Widget* m_widget;
	bool m_transient;
};

// Delay the start time of another animator
class Delay : public Animator {
	public:
	Delay(Widget* w, float seconds, Animator* next) : Animator(w), m_remaining(seconds), m_next(next) { }
	~Delay() { if(m_remaining > 0 && m_next->isTransient()) delete m_next; }
	State update(float time) {
		m_remaining -= time;
		if(m_remaining <= 0) {
			m_widget->getRoot()->startAnimator(m_next);
			return ENDED;
		}
		return ACTIVE;
	}
	private:
	float m_remaining;
	Animator* m_next;
};


template<typename T>
class LerpAnimator : public Animator {
	public:
	LerpAnimator(Widget* w, T start, T end, float time=1, Ease ease=Linear) : Animator(w), m_start(start), m_end(end), m_ease(ease), m_step(1/time) {}
	T updateValue(float time) {
		m_value += time * m_step;
		if(m_value >= 1) return m_end;
		return lerp(m_start, m_end, ease(m_value, m_ease));
	}
	public:
	Delegate<void()> eventCompleted;

	protected:
	inline State getState() {
		if(m_value >= 1 && eventCompleted) eventCompleted();
		return m_value<1? ACTIVE: ENDED;
	}
	private:
	T m_start, m_end;
	Ease m_ease;
	float m_step;
	float m_value = 0;
};


class PositionAnimator : public LerpAnimator<Point> {
	public:
	using LerpAnimator::LerpAnimator;
	State update(float time) override {
		m_widget->setPosition(updateValue(time));
		return getState();
	}
};
class SizeAnimator : public LerpAnimator<Point> {
	public:
	using LerpAnimator::LerpAnimator;
	State update(float time) override {
		m_widget->setSize(updateValue(time));
		return getState();
	}
};
class ColourAnimator : public LerpAnimator<unsigned> {
	public:
	using LerpAnimator::LerpAnimator;
	State update(float time) override {
		m_widget->setColourARGB(updateValue(time));
		return getState();
	}
};
class FadeAnimator : public LerpAnimator<float> {
	public:
	using LerpAnimator::LerpAnimator;
	State update(float time) override {
		m_widget->setAlpha(updateValue(time));
		return getState();
	}
};



}

