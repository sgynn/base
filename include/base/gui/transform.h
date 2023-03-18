#pragma once

#include <cmath>

namespace gui {

class Transform {
	float m[6] = { 1,0,0,1,0,0 };
	inline static int round(float f) { return (int)floor(f+0.5); }
	public:
	struct Pos { float x, y; };
	float& operator[](int i) { return m[i]; }

	template<typename T>
	inline Pos transformf(T x, T y) const {
		return {
			m[0]*x + m[2]*y + m[4],
			m[1]*x + m[3]*y + m[5],
		};
	}

	inline Point transform(int x, int y) const {
		Pos p = transformf(x, y);
		return { round(p.x), round(p.y) };
	}
	inline Point transform(const Point& p) const {
		return transform(p.x, p.y);
	}

	inline Rect transform(const Rect& r) const {
		Pos a = transformf(r.x, r.y);
		Pos b = transformf(r.right(), r.bottom());
		if(m[1]==0 && m[2]==0) {
			Point s((int)floor(a.x), (int)floor(a.y));
			return Rect(s, ceil(b.x)-s.x, ceil(b.y-s.y));
		}
		else {
			Rect r(a.x, a.y, 0, 0);
			r.include(a.x, b.y);
			r.include(b.x, a.y);
			r.include(b.x, b.y);
			return r;
		}
	}

	template<class P>
	inline Pos untransformf(const P& p) const {
		Pos r;
		r.x = (p.x * m[3] - m[3] * m[4] - p.y * m[2] + m[2] * m[5]) / (m[0] * m[3] - m[2] * m[1]);
		r.y = (p.y - m[5] - r.x * m[1]) / m[3];
		return r;
	}

	template<class P>
	inline Point untransform(const P& p) const {
		Pos r = untransformf(p);
		return { round(r.x), round(r.y) };
	}

	void translate(float x, float y) {
		m[4] += m[0]*x + m[2]*y;
		m[5] += m[1]*x + m[3]*y;
	}
	void scale(float s) {
		m[0] *= s;
		m[1] *= s;
		m[2] *= s;
		m[3] *= s;
	}
	void rotate(float angle) {
		float sa = sin(angle);
		float ca = cos(angle);
		// Minimal matrix mult
		float m0 = m[0], m1 = m[1];
		m[0] = m[2]*sa + m0*ca;
		m[1] = m[3]*sa + m1*ca;
		m[2] = m[2]*ca - m0*sa;
		m[3] = m[3]*ca - m1*sa;
	}
	void rotate(float angle, float cx, float cy) {
		translate(cx, cy);
		rotate(angle);
		translate(-cx, -cy);
	}
};


}

