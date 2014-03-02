#ifndef _BASE_VEC_
#define _BASE_VEC_

#include <math.h>

/** 2d vector */
class vec2 {
	public:
	float x,y;

	vec2();
	vec2(float x, float y);
	vec2(const float*);
	vec2& set(float x, float y);
	
	operator float*();
	operator const float*() const;

	vec2& operator= (const vec2& v);
	vec2& operator+=(const vec2& v);
	vec2& operator-=(const vec2& v);
	vec2& operator*=(const float v);
	vec2& operator/=(const float v);
	vec2  operator- () const;
	vec2  operator+ (const vec2& b) const;
	vec2  operator- (const vec2& b) const;
	vec2  operator* (const vec2& s) const;
	vec2  operator/ (const vec2& s) const;
	vec2  operator+ (const float b) const;
	vec2  operator- (const float b) const;
	vec2  operator* (const float s) const;
	vec2  operator/ (const float s) const;
	bool  operator==(const vec2& v) const;
	bool  operator!=(const vec2& v) const;
	
	float length2() const;
	float length() const;
	vec2& normalise();
	vec2  normalised() const;
	float dot(const vec2& v) const;
	int   size(const vec2& p) const;
	float distance(const vec2& v) const;
	float distance2(const vec2& v) const;
	vec2  approach(const vec2& target, float step) const;
}; 

/** 3d vector */
class vec3 {
	public:
	float x,y,z;

	vec3();
	vec3(float x, float y, float z);
	vec3(const float*);
	vec3& set(float x, float y, float z);

	vec2 xy() const { return vec2(x,y); }
	vec2 xz() const { return vec2(x,z); }
	
	operator float*();
	operator const float*() const;

	vec3& operator= (const vec3& v);
	vec3& operator+=(const vec3& v);
	vec3& operator-=(const vec3& v);
	vec3& operator*=(const float v);
	vec3& operator/=(const float v);
	vec3  operator- () const;
	vec3  operator+ (const vec3& b) const;
	vec3  operator- (const vec3& b) const;
	vec3  operator* (const vec3& s) const;
	vec3  operator/ (const vec3& s) const;
	vec3  operator+ (const float b) const;
	vec3  operator- (const float b) const;
	vec3  operator* (const float s) const;
	vec3  operator/ (const float s) const;
	bool  operator==(const vec3& v) const;
	bool  operator!=(const vec3& v) const;
	
	float length() const;
	float length2() const;
	vec3& normalise();
	vec3  normalised() const;
	float dot(const vec3& v) const;
	vec3  cross(const vec3& v) const;
	float distance(const vec3& v) const;
	float distance2(const vec3& v) const;
	vec3  approach(const vec3& target, float step) const;
}; 


// Implementations
inline vec2::vec2(): x(0), y(0) {}
inline vec2::vec2(float x, float y): x(x), y(y) {}
inline vec2::vec2(const float* p): x(p[0]), y(p[1]) {}
inline vec2::operator float*() { return &x; }
inline vec2::operator const float*() const { return &x; }
inline vec2& vec2::set(float vx, float vy) { x=vx; y=vy; return *this; }

inline vec2& vec2::operator= (const vec2& v) { x=v.x; y=v.y;   return *this; }
inline vec2& vec2::operator+=(const vec2& v) { x+=v.x; y+=v.y; return *this; }
inline vec2& vec2::operator-=(const vec2& v) { x-=v.x; y-=v.y; return *this; }
inline vec2& vec2::operator*=(const float s) { x*=s; y*=s; return *this; }
inline vec2& vec2::operator/=(const float s) { x/=s; y/=s; return *this; }
inline vec2  vec2::operator- () const             { return vec2(-x, -y); }
inline vec2  vec2::operator+ (const vec2& v) const { return vec2(x+v.x, y+v.y); }
inline vec2  vec2::operator- (const vec2& v) const { return vec2(x-v.x, y-v.y); }
inline vec2  vec2::operator* (const vec2& s) const { return vec2(x*s.x, y*s.y); }
inline vec2  vec2::operator/ (const vec2& s) const { return vec2(x/s.x, y/s.y); }
inline vec2  vec2::operator+ (const float v) const { return vec2(x+v, y+v); }
inline vec2  vec2::operator- (const float v) const { return vec2(x-v, y-v); }
inline vec2  vec2::operator* (const float s) const { return vec2(x*s, y*s); }
inline vec2  vec2::operator/ (const float s) const { return vec2(x/s, y/s); }
inline bool  vec2::operator==(const vec2& v) const { return x==v.x && y==v.y; }
inline bool  vec2::operator!=(const vec2& v) const { return x!=v.x || y!=v.y; }
inline vec2  operator+(float s, const vec2& v) { return vec2(s+v.x, s+v.y); }
inline vec2  operator-(float s, const vec2& v) { return vec2(s-v.x, s-v.y); }
inline vec2  operator*(float s, const vec2& v) { return v*s; }
inline vec2  operator/(float s, const vec2& v) { return v/s; }

inline float vec2::length2() const    { return x*x + y*y; }
inline float vec2::length() const     { return sqrt(x*x+y*y); }
inline vec2& vec2::normalise()        { if(x!=0||y!=0) { float l=length(); x/=l; y/=l; } return *this; }
inline vec2  vec2::normalised() const { return *this/length(); }
inline float vec2::dot(const vec2& v) const       { return x*v.x + y*v.y; }
inline float vec2::distance2(const vec2& v) const { return (*this-v).length2(); }
inline float vec2::distance (const vec2& v) const { return (*this-v).length(); }

inline vec2 fabs (const vec2& v) { return vec2(fabs(v.x), fabs(v.y)); }
inline vec2 floor(const vec2& v) { return vec2(floor(v.x), floor(v.y)); }
inline vec2 ceil (const vec2& v) { return vec2(ceil(v.x), ceil(v.y)); }
inline vec2 lerp(const vec2& a, const vec2& b, float t) { 
	return vec2(a.x+(b.x-a.x)*t, a.y+(b.y-a.y)*t);
}
inline vec2 vec2::approach(const vec2& t, float step) const {
	float d = distance(t);
	if(d<step) return t;
	else return *this + step / d * (t-*this);
}





inline vec3::vec3(): x(0), y(0), z(0) {}
inline vec3::vec3(float x, float y, float z): x(x), y(y), z(z) {}
inline vec3::vec3(const float* p): x(p[0]), y(p[1]), z(p[2]) {}
inline vec3::operator float*() { return &x; }
inline vec3::operator const float*() const { return &x; }
inline vec3& vec3::set(float vx, float vy, float vz) { x=vx; y=vy; z=vz; return *this; }

inline vec3& vec3::operator= (const vec3& v) { x=v.x;  y=v.y;  z=v.z;  return *this; }
inline vec3& vec3::operator+=(const vec3& v) { x+=v.x; y+=v.y; z+=v.z; return *this; }
inline vec3& vec3::operator-=(const vec3& v) { x-=v.x; y-=v.y; z-=v.z; return *this; }
inline vec3& vec3::operator*=(const float s) { x*=s; y*=s; z*=s; return *this; }
inline vec3& vec3::operator/=(const float s) { x/=s; y/=s; z/=s; return *this; }
inline vec3  vec3::operator- () const        { return vec3(-x, -y, -z); }
inline vec3  vec3::operator+ (const vec3& v) const { return vec3(x+v.x, y+v.y, z+v.z); }
inline vec3  vec3::operator- (const vec3& v) const { return vec3(x-v.x, y-v.y, z-v.z); }
inline vec3  vec3::operator* (const vec3& s) const { return vec3(x*s.x, y*s.y, z*s.z); }
inline vec3  vec3::operator/ (const vec3& s) const { return vec3(x/s.x, y/s.y, z/s.z); }
inline vec3  vec3::operator+ (const float v) const { return vec3(x+v, y+v, z+v); }
inline vec3  vec3::operator- (const float v) const { return vec3(x-v, y-v, z-v); }
inline vec3  vec3::operator* (const float s) const { return vec3(x*s, y*s, z*s); }
inline vec3  vec3::operator/ (const float s) const { return vec3(x/s, y/s, z/s); }
inline bool  vec3::operator==(const vec3& v) const { return x==v.x && y==v.y && z==v.z; }
inline bool  vec3::operator!=(const vec3& v) const { return x!=v.x || y!=v.y || z!=v.z; }
inline vec3  operator+(float s, const vec3& v) { return vec3(s+v.x, s+v.y, s+v.z); }
inline vec3  operator-(float s, const vec3& v) { return vec3(s-v.x, s-v.y, s-v.z); }
inline vec3  operator*(float s, const vec3& v) { return v*s; }
inline vec3  operator/(float s, const vec3& v) { return v/s; }

inline float vec3::length2() const    { return x*x + y*y + z*z; }
inline float vec3::length() const     { return sqrt(x*x+y*y+z*z); }
inline vec3& vec3::normalise()        { if(x!=0||y!=0||z!=0) { float l=length(); x/=l; y/=l; z/=l; } return *this; }
inline vec3  vec3::normalised() const { return *this/length(); }
inline float vec3::dot(const vec3& v) const       { return x*v.x + y*v.y + z*v.z; }
inline vec3  vec3::cross(const vec3& v) const     { return vec3(y*v.z-z*v.y, z*v.x-x*v.z, x*v.y-y*v.x);  }
inline float vec3::distance2(const vec3& v) const { return (*this-v).length2(); }
inline float vec3::distance (const vec3& v) const { return (*this-v).length(); }

inline vec3 fabs (const vec3& v) { return vec3(fabs(v.x), fabs(v.y), fabs(v.z)); }
inline vec3 floor(const vec3& v) { return vec3(floor(v.x), floor(v.y), floor(v.z)); }
inline vec3 ceil (const vec3& v) { return vec3(ceil(v.x), ceil(v.y), ceil(v.z)); }
inline vec3 lerp (const vec3& a, const vec3& b, float t) { 
	return vec3(a.x+(b.x-a.x)*t, a.y+(b.y-a.y)*t, a.z+(b.z-a.z)*t);
}
inline vec3 vec3::approach(const vec3& t, float step) const {
	float d = distance(t);
	if(d<step) return t;
	else return *this + step / d * (t-*this);
}



#endif

