#pragma once

/** 2D integer point */
struct Point {
	int x, y;
	Point() : x(0), y(0) {};
	explicit Point(int v) : x(v), y(v) {}
	Point(int x, int y) : x(x), y(y) {};
	Point& set(int px, int py) { x=px; y=py; return *this; }
	Point operator-() const { return Point(-x, -y); }
	bool  operator< (const Point& p) const { return x<p.x || (x==p.x && y<p.y);  }
	bool  operator==(const Point& p) const { return x==p.x && y==p.y; }
	bool  operator!=(const Point& p) const { return x!=p.x || y!=p.y; }
	Point operator+ (const Point& b) const { return Point(x+b.x,y+b.y); }
	Point operator- (const Point& b) const { return Point(x-b.x,y-b.y); }
	Point& operator+=(const Point& p)      { x+=p.x; y+=p.y; return *this; }
	Point& operator-=(const Point& p)      { x-=p.x; y-=p.y; return *this; }
	Point& operator+=(int v)      { x+=v; y+=v; return *this; }
	Point& operator-=(int v)      { x-=v; y-=v; return *this; }
	Point operator+(int v)  const { return Point(x+v, y+v); }
	Point operator-(int v)  const { return Point(x-v, y-v); }
	operator const int*() const { return &x; }
	operator int*() { return &x; }
};

/** 3D integer point. Used for indexing 3D grids */
struct Point3 {
	int x, y, z;
	Point3() : x(0), y(0), z(0) {};
	explicit Point3(int v) : x(v), y(v), z(v) {}
	Point3(int x, int y, int z) : x(x), y(y), z(z) {};
	Point3& set(int px, int py, int pz) { x=px; y=py; z=pz; return *this; }
	Point3 operator-() const { return Point3(-x, -y, -z); }
	bool   operator< (const Point3& p) const { return x<p.x || (x==p.x && y<p.y) || (x==p.x && y==p.y && z<p.z); }
	bool   operator==(const Point3& p) const { return x==p.x && y==p.y && z==p.z; }
	bool   operator!=(const Point3& p) const { return x!=p.x || y!=p.y || z!=p.z; }
	Point3 operator+ (const Point3& b) const { return Point3(x+b.x,y+b.y,z+b.z); }
	Point3 operator- (const Point3& b) const { return Point3(x-b.x,y-b.y,z-b.z); }
	Point3& operator+=(const Point3& p)      { x+=p.x; y+=p.y; z+=p.z; return *this; }
	Point3& operator-=(const Point3& p)      { x-=p.x; y-=p.y; z-=p.z; return *this; }
	Point3& operator+=(int v)      { x+=v; y+=v; z+=v; return *this; }
	Point3& operator-=(int v)      { x-=v; y-=v; z-=v; return *this; }
	Point3 operator+(int v)  const { return Point3(x+v, y+v, z+v); }
	Point3 operator-(int v)  const { return Point3(x-v, y-v, z-v); }
	operator const int*() const { return &x; }
	operator int*() { return &x; }
};

/** 2D Rectangle. Bounding shape */
struct Rect {
	int x, y, width, height;

	Rect() {}
	Rect(int x, int y, int w, int h): x(x), y(y), width(w), height(h) {}
	Rect(int x, int y, const Point& size): x(x), y(y), width(size.x), height(size.y) {}
	Rect(const Point& pos, const Point& size): x(pos.x), y(pos.y), width(size.x), height(size.y) {}
	Rect(const Point& pos, int w=0, int h=0): x(pos.x), y(pos.y), width(w), height(h) {}

	Rect& set(int x, int y, int w, int h) { this->x=x; this->y=y; width=w; height=h; return *this; }
	Rect& set(const Point& pos, const Point& size) { x=pos.x; y=pos.y; width=size.x; height=size.y; return *this; }
	Rect& set(const Point& pos, int w, int h) { x=pos.x; y=pos.y; width=w; height=h; return *this; }
	Rect& set(int x, int y, const Point& size) { this->x=x; this->y=y; width=size.x; height=size.y; return *this; }

	bool   operator==(const Rect& r) const { return x==r.x && y==r.y && width==r.width && height==r.height; }
	bool   operator!=(const Rect& r) const { return x!=r.x || y!=r.y || width!=r.width || height!=r.height; }

	void include(int x, int y);
	void include(const Rect& r);
	void include(const Point& p) { include(p.x, p.y); }
	void intersect(const Rect& r);
	Rect intersection(const Rect& r) const;

	Point& position() { return *reinterpret_cast<Point*>(&x); }
	Point& size()     { return *reinterpret_cast<Point*>(&width); }

	const Point& position() const { return *reinterpret_cast<const Point*>(&x); }
	const Point& size() const     { return *reinterpret_cast<const Point*>(&width); }
	Point bottomRight() const     { return Point(x+width, y+height); }

	bool valid      () const { return width >= 0 && height >= 0; }
	bool empty      () const { return width == 0 && height == 0; }
	bool contains   (int px, int py) const  { return px>=x && px<x+width && py>=y && py<y+height; }
	bool contains   (const Point& p) const  { return contains(p.x, p.y); }
	bool contains   (const Rect& o) const   { return o.x>=x && o.x+o.width<=x+width && o.y>=y && o.y+o.height<=y+height; }
	bool intersects (const Rect& o) const   { return o.x<x+width && o.x+o.width>x && o.y<y+height && o.y+o.height>y; }
	int  right      () const                { return x+width; }
	int  bottom     () const                { return y+height; }
	int  top        () const                { return y; }
	int  left       () const                { return x; }
};

inline void Rect::include(int px, int py) {
	if(px<x) width  += x-px, x = px;
	if(py<y) height += y-py, y = py;
	if(px>=x+width) width = px-x+1;
	if(py>=y+height) height = py-y+1;
}
inline void Rect::include(const Rect& r) {
	if(r.width<=0 || r.height<=0) return;
	include(r.x, r.y);
	include(r.x+r.width-1, r.y+r.height-1);
}
inline void Rect::intersect(const Rect& r) {
	int x2 = right()<r.right()? right(): r.right();
	int y2 = bottom()<r.bottom()? bottom(): r.bottom();
	if(x<r.x) x = r.x;
	if(y<r.y) y = r.y;
	width = x2 - x;
	height = y2 - y;
}
inline Rect Rect::intersection(const Rect& r) const {
	Rect result = r;
	result.intersect(*this);
	return result; 
}



// ------------------------ Range iterators --------------------------------- //

// Point range iterator - yeilds all points in a box. { min <= x < max }
class PointRangeIterator {
	public:
	PointRangeIterator(Point start, Point end, Point value) : m_start(start), m_end(end), m_value(value) {
		if(start.x>=end.x || start.y>=end.y) m_end = start;
	}
	PointRangeIterator operator++() { PointRangeIterator tmp=*this; operator++(0); return tmp; }
	PointRangeIterator operator++(int) { ++m_value.x; if(m_value.x>=m_end.x) m_value.set(m_start.x, m_value.y+1); return *this; }
	bool operator!=(const PointRangeIterator& o) const { return m_value != o.m_value; }
	const Point& operator*() const { return m_value; }
	private:
	Point m_start, m_end, m_value;
};
struct PointRangeIteratorWrapper {
	Point a, b;
	PointRangeIterator begin() const { return PointRangeIterator(a,b,a); }
	PointRangeIterator end() const { return PointRangeIterator(a,b, Point(a.x, a.x<b.x&&a.y<b.y? b.y: a.y)); }
};
inline PointRangeIteratorWrapper range(const Point& a, const Point& b) { return PointRangeIteratorWrapper{a, b}; }
inline PointRangeIteratorWrapper range(const Point& r) { return PointRangeIteratorWrapper{Point(), r}; }
inline PointRangeIteratorWrapper range(const Rect& r) { return PointRangeIteratorWrapper{r.position(), r.bottomRight()}; }


// 3D version - yeilds all points in a cube. { min <= x <= max }
class Point3RangeIterator {
	public:
	Point3RangeIterator(Point3 start, Point3 end, Point3 value) : m_start(start), m_end(end), m_value(value) {
		if(start.x>=end.x || start.y>=end.y || start.z>=end.z) m_end = start;
	}
	Point3RangeIterator operator++() { Point3RangeIterator tmp=*this; operator++(0); return tmp; }
	Point3RangeIterator operator++(int) {
		if(++m_value.x>=m_end.x) {
			m_value.x = m_start.x;
			if(++m_value.y>=m_end.y) m_value.set(m_start.x, m_start.y, m_value.z+1);
		}
		return *this;
	}
	bool operator!=(const Point3RangeIterator& o) const { return m_value != o.m_value; }
	const Point3& operator*() const { return m_value; }
	private:
	Point3 m_start, m_end, m_value;
};
struct Point3RangeIteratorWrapper {
	Point3 a, b;
	Point3RangeIterator begin() const { return Point3RangeIterator(a,b,a); }
	Point3RangeIterator end() const { return Point3RangeIterator(a,b,Point3(a.x,a.y, a.x<b.x&&a.y<b.y&&a.z<b.z? b.z: a.z)); }
};
inline Point3RangeIteratorWrapper range(const Point3& a, const Point3& b) { return Point3RangeIteratorWrapper{a, b}; }
inline Point3RangeIteratorWrapper range(const Point3& r) { return Point3RangeIteratorWrapper{Point3(), r}; }




