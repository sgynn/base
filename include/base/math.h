#ifndef _BASE_MATH_
#define _BASE_MATH_

#include <math.h>

typedef unsigned long long	uint64;
typedef unsigned int	uint,	uint32;
typedef unsigned short	ushort,	uint16;
typedef unsigned char	ubyte,	uint8;

class Vec2 {
	public:
	float x,y;
	Vec2() : x(0),y(0){}
	Vec2(float x,float y) : x(x), y(y) {}
	Vec2(const float* v) : x(v[0]), y(v[1]) {}
	
	operator const float*() const { return &x; }
	operator float*() { return &x; }

	Vec2& operator= (const Vec2& v) { x=v.x; y=v.y; return *this; }
	Vec2& operator+=(const Vec2& v) { x+=v.x; y+=v.y; return *this; }
	Vec2& operator-=(const Vec2& v) { x-=v.x; y-=v.y; return *this; }
	Vec2& operator*=(float v) { x*=v; y*=v; return *this; }
	Vec2 operator+ (const Vec2& b) const { return Vec2(x+b.x,y+b.y); }
	Vec2 operator- (const Vec2& b) const { return Vec2(x-b.x,y-b.y); }
	Vec2 operator* (const float s) const { return Vec2(x*s,y*s); }
	Vec2 operator/ (const float s) const { return Vec2(x/s,y/s); }

	bool operator==(const Vec2& v) const { return v.x==x && v.y==y; }
	bool operator!=(const Vec2& v) const { return v.x!=x || v.y!=y; }
	
	float distanceSq(const Vec2& v) const { return (x-v.x)*(x-v.x) + (y-v.y)*(y-v.y); }
	float distance(const Vec2& v) const { return sqrt((x-v.x)*(x-v.x) + (y-v.y)*(y-v.y) ); }
	float length() const { return sqrt(x*x + y*y); }
	Vec2& normalise() {
		if(x==0 && y==0) return *this;
		float l=length();
		x /= l;
		y /= l;
		return *this;
	}
	Vec2 normalised() const {
		if(x==0 && y==0) return *this;
		float l=length();
		return *this * (1/l);
	}
	float dot(const Vec2& v) const {return (x*v.x) + (y*v.y); }
}; 

class Vec3 {
	public:
		float x,y,z;
		Vec3() : x(0),y(0),z(0){}
		Vec3(float x,float y,float z) : x(x), y(y), z(z) {}
		Vec3(const Vec2& v, float z=0) : x(v.x), y(v.y), z(z) {}
		Vec3(const float* v) : x(v[0]), y(v[1]), z(v[2]) {}

		operator const float*() const { return &x; }
		operator float*() { return &x; }
		
		Vec3& operator= (const Vec3& v) { x=v.x; y=v.y; z=v.z; return *this; }
		Vec3& operator+=(const Vec3& v) { x+=v.x; y+=v.y; z+=v.z; return *this; }
		Vec3& operator-=(const Vec3& v) { x-=v.x; y-=v.y; z-=v.z; return *this; }
		Vec3& operator*=(float v) { x*=v; y*=v; z*=v; return *this; }
		Vec3 operator+ (const Vec3& b) const { return Vec3(x+b.x,y+b.y,z+b.z); }
		Vec3 operator- (const Vec3& b) const { return Vec3(x-b.x,y-b.y,z-b.z); }
		Vec3 operator* (const float s) const { return Vec3(x*s,y*s,z*s); }
		Vec3 operator/ (const float s) const { return Vec3(x/s,y/s,z/s); }

		bool operator==(const Vec3& v)  const { return v.x==x && v.y==y && v.z==z; }
		bool operator!=(const Vec3& v)  const { return v.x!=x || v.y!=y || v.z!=z; }
		
		float distanceSq(const Vec3& v) const { return (x-v.x)*(x-v.x) + (y-v.y)*(y-v.y) + (z-v.z)*(z-v.z); }
		float distance(const Vec3& v) const { return sqrt((x-v.x)*(x-v.x) + (y-v.y)*(y-v.y) + (z-v.z)*(z-v.z)); }
		float length() const { return sqrt(x*x + y*y + z*z); }
		Vec3& normalise() {
			if(x==0 && y==0 && z==0) return *this;
			float l=length();
			x /= l;
			y /= l;
			z /= l;
			return *this;
		}
		Vec3 normalised() const {
			if(x==0 && y==0 && z==0) return *this;
			float l=length();
			return *this / l;
		}

		float dot(const Vec3& v) const {return (x*v.x) + (y*v.y) + (z*v.z); }
		Vec3 cross(const Vec3& v) const { return Vec3(y*v.z-z*v.y, z*v.x-x*v.z, x*v.y-y*v.x); }
}; 

typedef Vec2 vec2;
typedef Vec3 vec3;

enum M_AXIS { AXIS_X, AXIS_Y, AXIS_Z };

//very lightweight stripped-down matrix class
class Matrix {
	public:
	float m[16];
	Matrix() { identity(); }
	Matrix(const float* pointer) { for(int i=0; i<16; i++) m[i]=pointer[i]; }
	Matrix(const vec3& left, const vec3& up, const vec3& forward, const vec3& position=vec3()) {
		for(int i=0; i<3; i++) {
			m[i] = left[i];
			m[i+4] = up[i];
			m[i+8] = forward[i];
			m[i+12] = position[i];
		}
		m[3]=m[7]=m[11]=0; m[15]=1;
	}
	Matrix& identity() {
		m[1]=m[2]=m[3]=m[4]=m[6]=m[7]=m[8]=m[9]=m[11]=m[12]=m[13]=m[14]=0;
		m[0]=m[5]=m[10]=m[15]=1;
		return *this;
	}
	operator float*() { return &m[0]; }
	operator const float*() const { return &m[0]; }
	Matrix& operator=(const Matrix& a) {
		for(uint i=0; i<16; i++) m[i]=a.m[i];
		return *this;
	}
	Matrix operator*(const Matrix& mat) const {
		Matrix result;
		result.m[0] = m[0]*mat.m[0] + m[4]*mat.m[1] + m[8]*mat.m[2] + m[12]*mat.m[3];
		result.m[1] = m[1]*mat.m[0] + m[5]*mat.m[1] + m[9]*mat.m[2] + m[13]*mat.m[3];
		result.m[2] = m[2]*mat.m[0] + m[6]*mat.m[1] + m[10]*mat.m[2] + m[14]*mat.m[3];
		result.m[3] = m[3]*mat.m[0] + m[7]*mat.m[1] + m[11]*mat.m[2] + m[15]*mat.m[3];
		
		result.m[4] = m[0]*mat.m[4] + m[4]*mat.m[5] + m[8]*mat.m[6] + m[12]*mat.m[7];
		result.m[5] = m[1]*mat.m[4] + m[5]*mat.m[5] + m[9]*mat.m[6] + m[13]*mat.m[7];
		result.m[6] = m[2]*mat.m[4] + m[6]*mat.m[5] + m[10]*mat.m[6] + m[14]*mat.m[7];
		result.m[7] = m[3]*mat.m[4] + m[7]*mat.m[5] + m[11]*mat.m[6] + m[15]*mat.m[7];
		
		result.m[8] = m[0]*mat.m[8] + m[4]*mat.m[9] + m[8]*mat.m[10] + m[12]*mat.m[11];
		result.m[9] = m[1]*mat.m[8] + m[5]*mat.m[9] + m[9]*mat.m[10] + m[13]*mat.m[11];
		result.m[10] = m[2]*mat.m[8] + m[6]*mat.m[9] + m[10]*mat.m[10] + m[14]*mat.m[11];
		result.m[11] = m[3]*mat.m[8] + m[7]*mat.m[9] + m[11]*mat.m[10] + m[15]*mat.m[11];
		
		result.m[12] = m[0]*mat.m[12] + m[4]*mat.m[13] + m[8]*mat.m[14] + m[12]*mat.m[15];
		result.m[13] = m[1]*mat.m[12] + m[5]*mat.m[13] + m[9]*mat.m[14] + m[13]*mat.m[15];
		result.m[14] = m[2]*mat.m[12] + m[6]*mat.m[13] + m[10]*mat.m[14] + m[14]*mat.m[15];
		result.m[15] = m[3]*mat.m[12] + m[7]*mat.m[13] + m[11]*mat.m[14] + m[15]*mat.m[15];
		return result;
		
	}
	vec3 operator*(const vec3& v) const {
		vec3 result;
		result.x = m[0]*v.x + m[4]*v.y + m[8]*v.z + m[12];
		result.y = m[1]*v.x + m[5]*v.y + m[9]*v.z + m[13];
		result.z = m[2]*v.x + m[6]*v.y + m[10]*v.z + m[14];
		return result;
	}
	Matrix transpose() const {
		Matrix mat;
		mat[0] = m[0];	mat[1] = m[4];	mat[2] = m[8];	mat[3] = m[12];
		mat[4] = m[1];	mat[5] = m[5];	mat[6] = m[9];	mat[7] = m[13];
		mat[8] = m[2];	mat[9] = m[6];	mat[10] = m[10]; mat[11] = m[14];
		mat[12] = m[3];	mat[13] = m[7];	mat[14] = m[11]; mat[15] = m[15];
		return mat;
	}
	/** A simple fast inverse matrix. Limited to translation and rotation matrices */
	static void inverseAffine(Matrix& out, const Matrix& in) {
		//invert rotation part
		out[0] = in[0];	out[1] = in[4];	out[2] = in[8];	out[3] = 0;
		out[4] = in[1];	out[5] = in[5];	out[6] = in[9];	out[7] = 0;
		out[8] = in[2];	out[9] = in[6];	out[10] = in[10]; out[11] = 0;
		//Multiply translation part
		out[12] = out[0]*-in[12] + out[4]*-in[13] + out[8 ]*-in[14];
		out[13] = out[1]*-in[12] + out[5]*-in[13] + out[9 ]*-in[14];
		out[14] = out[2]*-in[12] + out[6]*-in[13] + out[10]*-in[14];
		out[15] = 1;
	}
};

#undef min
#undef max
class aabb {
	public:
	Vec3 min;
	Vec3 max;
	
	aabb() {};
	aabb(const vec3& p) : min(p), max(p) {};
	aabb(const vec3& min, const vec3& max) : min(min), max(max) {};
	aabb(float ax, float ay, float az, float bx, float by, float bz) : min(ax,ay,az), max(bx,by,bz) {};
	
	aabb& operator=(const aabb& b) { min=b.min; max=b.max; return *this; }
	aabb operator+(const vec3& v) const { return aabb(min+v, max+v); }
	aabb operator-(const vec3& v) const { return aabb(min-v, max-v); }
	aabb& operator+=(const vec3& v) { min+=v, max+=v; return *this;  }
	aabb& operator-=(const vec3& v) { min-=v, max-=v; return *this;  }
	
	/** Get the size of the box */
	vec3 size() const { return vec3(max-min); }
	/** Get the box centre point */
	vec3 centre() const { return (min+max)*0.5f; }
	
	/** Expand the bounding box to contain a point */
	void addPoint(const vec3& p) {
		if(p.x<min.x) min.x=p.x;
		if(p.y<min.y) min.y=p.y;
		if(p.z<min.z) min.z=p.z;
		if(p.x>max.x) max.x=p.x;
		if(p.y>max.y) max.y=p.y;
		if(p.z>max.z) max.z=p.z;
	}
	/** Expand the bounding box to contain another bounding box */
	void addBox(const aabb& box) {
		addPoint(box.min);
		addPoint(box.max);
	}
	/** Is a point within the bounding box */
	bool contains(const vec3& p) const {
		return p.x>=min.x && p.x<=max.x && p.y>=min.y && p.y<=max.y && p.z>=min.z && p.z<=max.z;
	}
	/** Is a box entirely within this bounding box */
	bool contains(const aabb& b) const {
		return b.min.x>=min.x && b.min.y>=min.y && b.min.z>=min.z && b.max.x<=max.x && b.max.y<=max.y && b.max.z<=max.z;
	}
	/** Does a box intersect with this box */
	bool intersects(const aabb& b) const {
		return b.max.x>min.x && b.min.x<max.x && b.max.y>min.y && b.min.y<max.y && b.max.z>min.z && b.min.z<max.z;
	}
};

template<typename T>
struct RangeT {
	T min, max;
	RangeT(T r=0): min(r), max(r) {}
	RangeT(T min, T max) : min(min), max(max) {}
	void addPoint(T v) { if(v<min) min=v; if(v>max) max=v; }
	bool contains(T p) const { return p>=min && p<=max; }
	T range() const { return max - min; }
	T clamp(T v) const { return v<min? min: v>max?max: v; }
	T wrap(T v) const { return v - floor((v+min)/range())*range(); }
};
typedef RangeT<float> Range;



struct Colour {
	float r,g,b,a;
	Colour() : r(1),g(1),b(1),a(1){}
	Colour(float r,float g,float b,float a=1.0) : r(r),g(g),b(b),a(a){}
	Colour(uint c, float a=1.0) : r(((c&0xff0000)>>16)/255.0f), g(((c&0xff00)>>8)/255.0f), b((c&0xff)/255.0f), a(a) {}
	operator const float*() const { return &r; }
	operator uint() const { return ((uint)(r*255)<<16) + ((uint)(g*255)<<8) + ((uint)(b*255)); }
	bool operator==(const Colour&c) const { return c.r==r&&c.g==g&&c.b==b&&c.a==a; }
	static Colour lerp(const Colour& a, const Colour& b, float t) { return Colour(a.r+(b.r-a.r)*t, a.g+(b.g-a.g)*t, a.b+(b.b-a.b)*t, a.a+(b.a-a.a)*t); }
};

const Colour black   = Colour(0,0,0);
const Colour white   = Colour(1,1,1);
const Colour red     = Colour(1,0,0);
const Colour blue    = Colour(0,0,1);
const Colour green   = Colour(0,1,0);
const Colour cyan    = Colour(0,1,1);
const Colour magenta = Colour(1,0,1);
const Colour yellow  = Colour(1,1,0);
const Colour orange  = Colour(1,0.5f,0);
const Colour purple  = Colour(1,0,0.5f);
const Colour grey    = Colour(0.5f,0.5f,0.5f);
const Colour transparent = Colour(0,0,0,0);

enum EDirection { NORTH=0x1, EAST=0x2, SOUTH=0x4, WEST=0x8 };

struct Point {
	int x, y;
	Point() : x(0), y(0) {};
	Point(int x, int y) : x(x), y(y) {};
	bool operator<(const Point& p) const { return x<p.x || (x==p.x && y<p.y);  }
	bool operator==(const Point& p) const { return x==p.x && y==p.y; }
	Point operator+ (const Point& b) const { return Point(x+b.x,y+b.y); }
	Point operator- (const Point& b) const { return Point(x-b.x,y-b.y); }
};

struct Point3 {
	int x, y, z;
	Point3() : x(0), y(0), z(0) {};
	Point3(int x, int y, int z) : x(x), y(y), z(z) {};
	bool operator<(const Point3& p) const { return x<p.x || (x==p.x && y<p.y) || (x==p.x && y==p.y && z<p.z); }
	bool operator==(const Point3& p) const { return x==p.x && y==p.y && z==p.z; }
	Point3 operator+ (const Point3& b) const { return Point3(x+b.x,y+b.y,z+b.z); }
	Point3 operator- (const Point3& b) const { return Point3(x-b.x,y-b.y,z-b.z); }
};

struct Rect {
	int x, y, width, height;
	Rect(int x, int y, int w, int h): x(x), y(y), width(w), height(h) {}
	Rect(const Point& pos, const Point& size): x(pos.x), y(pos.y), width(size.x), height(size.y) {}
	bool contains(int px, int py) const { return px>=x && px<=x+width && py>=y && py<=y+height; }
	bool contains(const Point& p) const { return contains(p.x, p.y); }
	bool intersects(const Rect& o) const { return x<o.x+o.width && x+width>o.x && y<o.y+o.height && y+height>o.y; }
};

#ifndef PI
#define PI 3.14159265359
#define TWOPI 6.283185307
#endif

#endif

