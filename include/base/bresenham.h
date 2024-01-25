#pragma once
#include <base/math.h>

namespace base {

// Callback signiture: function(Point cell, float intersectStart, float intersectEnd);
// callback returns whether to continue iteration (true = continue)
// function returns ray distance to blocking cell (intersectStart)
template<typename F>
float bresenhamLineT(const vec2& p, const vec2& d, float cellSize, float limit, const F& callback) {
	// get start cell
	Point cell( floor(p.x/cellSize), floor(p.y/cellSize));
	const Point cd( d.x>0? 1:-1, d.y>0? 1:-1);

	// Step x major
	if(fabs(d.x) >= fabs(d.y)) {
		float vx = (cell.x*cellSize + (d.x>0?cellSize:0) - p.x) / d.x;	// first offset
		float slope = fabs(d.y / d.x) * cellSize;
		float step = fabs(cellSize/d.x);
		float side = d.y>0? cellSize: 0;
		float c = (p.y + vx*d.y) - cell.y * cellSize;
		if(d.y<0) c = cellSize - c;
		float t, u=0, v=vx;

		while(u<limit) {
			if(c>cellSize) {
				c -= cellSize;
				t = (cell.y*cellSize + side - p.y) / d.y;
				if(!callback(cell, u, t)) return u;
				cell.y += cd.y;
				u = t;
			}
			if(!callback(cell, u, v)) return u;
			cell.x += cd.x;
			u = v;
			v += step;
			c += slope;
		}
	}

	// Step y major
	else {
		float vy = (cell.y*cellSize + (d.y>0?cellSize:0) - p.y) / d.y;	// First offset
		float slope = fabs(d.x / d.y) * cellSize;;
		float step = fabs(cellSize/d.y);
		float side = d.x>0? cellSize: 0;
		float c = (p.x + vy*d.x) - cell.x * cellSize;
		if(d.x<0) c = cellSize - c;
		float t, u=0, v=vy;

		while(u<limit) {
			if(c>cellSize) {
				c -= cellSize;
				t = (cell.x*cellSize + side - p.x) / d.x;
				if(!callback(cell, u, t)) return u;
				cell.x += cd.x;
				u = t;
			}
			if(!callback(cell, u, v)) return u;
			cell.y += cd.y;
			u = v;
			v += step;
			c += slope;
		}
	}

	return limit;
}

}

