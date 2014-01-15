#ifndef _POINT_
#define _POINT_

/** 2D integer point */
struct Point {
	int x, y;
	Point() : x(0), y(0) {};
	Point(int x, int y) : x(x), y(y) {};
	bool  operator< (const Point& p) const { return x<p.x || (x==p.x && y<p.y);  }
	bool  operator==(const Point& p) const { return x==p.x && y==p.y; }
	bool  operator!=(const Point& p) const { return x!=p.x || y!=p.y; }
	Point operator+ (const Point& b) const { return Point(x+b.x,y+b.y); }
	Point operator- (const Point& b) const { return Point(x-b.x,y-b.y); }
	operator const int*() const { return &x; }
	operator int*() { return &x; }
};

/** 3D integer point. Used for indexing 3D grids */
struct Point3 {
	int x, y, z;
	Point3() : x(0), y(0), z(0) {};
	Point3(int x, int y, int z) : x(x), y(y), z(z) {};
	bool   operator< (const Point3& p) const { return x<p.x || (x==p.x && y<p.y) || (x==p.x && y==p.y && z<p.z); }
	bool   operator==(const Point3& p) const { return x==p.x && y==p.y && z==p.z; }
	bool   operator!=(const Point3& p) const { return x!=p.x || y!=p.y || z!=p.z; }
	Point3 operator+ (const Point3& b) const { return Point3(x+b.x,y+b.y,z+b.z); }
	Point3 operator- (const Point3& b) const { return Point3(x-b.x,y-b.y,z-b.z); }
	operator const int*() const { return &x; }
	operator int*() { return &x; }
};

/** 2D Rectangle. Bounding shape */
struct Rect {
	int x, y, width, height;

	Rect() {}
	Rect(int x, int y, int w, int h): x(x), y(y), width(w), height(h) {}
	Rect(const Point& pos, const Point& size): x(pos.x), y(pos.y), width(size.x), height(size.y) {}

	Point& position() { return *reinterpret_cast<Point*>(&x); }
	Point& size()     { return *reinterpret_cast<Point*>(&width); }

	const Point& position() const { return *reinterpret_cast<const Point*>(&x); }
	const Point& size() const     { return *reinterpret_cast<const Point*>(&width); }

	bool contains   (int px, int py) const  { return px>=x && px<=x+width && py>=y && py<=y+height; }
	bool contains   (const Point& p) const  { return contains(p.x, p.y); }
	bool intersects (const Rect& o) const   { return x<o.x+o.width && x+width>o.x && y<o.y+o.height && y+height>o.y; }
	int  right      () const                { return x+width; }
	int  bottom     () const                { return y+height; }
};

#endif

