#ifndef _BASE_VEC_
#define _BASE_VEC_

#include <math.h>
class vec3;

/// 2d vector
class vec2 {
	public:
	float x,y;

	vec2();
	vec2(float v);
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
	vec2& operator*=(const vec2& v);
	vec2& operator/=(const vec2& v);
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
	
	bool  isZero() const;
	float length2() const;
	float length() const;
	vec2& normalise();
	vec2  normalised() const;
	float normaliseWithLength(); // normalise vector but return original length
	float dot(const vec2& v) const;
	int   size(const vec2& p) const;
	float distance(const vec2& v) const;
	float distance2(const vec2& v) const;

	vec3  xzy(float z=0) const;	// convert to vec3
	vec3  xyz(float z=0) const;
}; 

/// 3d vector
class vec3 {
	public:
	float x,y,z;

	vec3();
	vec3(float v);
	vec3(float x, float y, float z);
	vec3(const vec2&, float z);
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
	vec3& operator*=(const vec3& v);
	vec3& operator/=(const vec3& v);
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
	
	bool  isZero() const;
	float length() const;
	float length2() const;
	vec3& normalise();
	vec3  normalised() const;
	float normaliseWithLength(); // normalise vector but return original length
	float dot(const vec3& v) const;
	vec3  cross(const vec3& v) const;
	float distance(const vec3& v) const;
	float distance2(const vec3& v) const;
}; 

/// 4d vector - Yes we do need one
class vec4 {
	public:
	float x,y,z,w;
	vec4();
	vec4(float v);
	vec4(float x, float y, float z, float w=1);
	vec4(const vec3& xyz, float w=1);
	vec4(const float*);
	vec4& set(float x, float y, float z, float w);
	vec3 xyz() const;
	operator float*();
	operator const float*() const;

	vec4& operator= (const vec4& v);
	vec4& operator+=(const vec4& v);
	vec4& operator-=(const vec4& v);
	vec4& operator*=(const float v);
	vec4& operator/=(const float v);
	vec4& operator*=(const vec4& v);
	vec4& operator/=(const vec4& v);
	vec4  operator- () const;
	vec4  operator+ (const vec4& b) const;
	vec4  operator- (const vec4& b) const;
	vec4  operator* (const vec4& s) const;
	vec4  operator/ (const vec4& s) const;
	vec4  operator+ (const float b) const;
	vec4  operator- (const float b) const;
	vec4  operator* (const float s) const;
	vec4  operator/ (const float s) const;
	bool  operator==(const vec4& v) const;
	bool  operator!=(const vec4& v) const;
	
	bool  isZero() const;
	float length() const;
	float length2() const;
	vec4& normalise();
	vec4  normalised() const;
	float normaliseWithLength(); // normalise vector but return original length
	float dot(const vec4& v) const;
	float distance(const vec4& v) const;
	float distance2(const vec4& v) const;
};


// Ray class.
class Ray {
	public:
	vec3 start;
	vec3 direction;

	Ray() {}
	Ray(const vec3& p, const vec3& d);
	Ray& set(const vec3& p, const vec3& d);
	vec3 point(float t) const;
};


#define ImplementMutateOp(op) \
	inline vec2& vec2::operator op(const vec2& v) { x op v.x; y op v.y; return *this; } \
	inline vec3& vec3::operator op(const vec3& v) { x op v.x; y op v.y; z op v.z; return *this; } \
	inline vec4& vec4::operator op(const vec4& v) { x op v.x; y op v.y; z op v.z; w op v.w; return *this; } \

#define ImplementMutateOpF(op) \
	inline vec2& vec2::operator op(float v) { x op v; y op v; return *this; } \
	inline vec3& vec3::operator op(float v) { x op v; y op v; z op v; return *this; } \
	inline vec4& vec4::operator op(float v) { x op v; y op v; z op v; w op w; return *this; } \

#define ImplementReturnOp(op) \
	inline vec2 vec2::operator op(const vec2& v) const { return vec2(x op v.x, y op v.y); } \
	inline vec3 vec3::operator op(const vec3& v) const { return vec3(x op v.x, y op v.y, z op v.z); } \
	inline vec4 vec4::operator op(const vec4& v) const { return vec4(x op v.x, y op v.y, z op v.z, w op v.w); } \

#define ImplementReturnOpF(op) \
	inline vec2 vec2::operator op(float v) const { return vec2(x op v, y op v); } \
	inline vec3 vec3::operator op(float v) const { return vec3(x op v, y op v, z op v); } \
	inline vec4 vec4::operator op(float v) const { return vec4(x op v, y op v, z op v, w op v); } \

#define ImplementReturnOpFExt(op) \
	inline vec2 operator op(float f, const vec2& v) { return vec2(f op v.x, f op v.y); } \
	inline vec3 operator op(float f, const vec3& v) { return vec3(f op v.x, f op v.y, f op v.z); } \
	inline vec4 operator op(float f, const vec4& v) { return vec4(f op v.x, f op v.y, f op v.z, f op v.w); } \

#define ImplementFuncOp(func) \
	inline vec2 func(const vec2& v) { return vec2(func(v.x), func(v.y)); } \
	inline vec3 func(const vec3& v) { return vec3(func(v.x), func(v.y), func(v.z)); } \
	inline vec4 func(const vec4& v) { return vec4(func(v.x), func(v.y), func(v.z), func(v.w)); } \

#define ImplementFuncOpT(func, T) \
	inline vec2 func(const vec2& v, T p) { return vec2(func(v.x, p), func(v.y, p)); } \
	inline vec3 func(const vec3& v, T p) { return vec3(func(v.x, p), func(v.y, p), func(v.z, p)); } \
	inline vec4 func(const vec4& v, T p) { return vec4(func(v.x, p), func(v.y, p), func(v.z, p), func(v.w, p)); } \

#define ImplementCommon(name, func, const) \
	inline float vec2::name() const { func; } \
	inline float vec3::name() const { func; } \
	inline float vec4::name() const { func; } \

#define ImplementCommonV(name, func, const) \
	inline float vec2::name(const vec2& v) const { func; } \
	inline float vec3::name(const vec3& v) const { func; } \
	inline float vec4::name(const vec4& v) const { func; } \

#define ImplementCommonExt(name, func) \
	inline vec2 name(const vec2& a, const vec2& b, float t) { func; } \
	inline vec3 name(const vec3& a, const vec3& b, float t) { func; } \
	inline vec4 name(const vec4& a, const vec4& b, float t) { func; } \


// Implementations
inline vec2::vec2(): x(0), y(0) {}
inline vec2::vec2(float v): x(v), y(v) {}
inline vec2::vec2(float x, float y): x(x), y(y) {}
inline vec2::vec2(const float* p): x(p[0]), y(p[1]) {}
inline vec2::operator float*() { return &x; }
inline vec2::operator const float*() const { return &x; }
inline vec2& vec2::set(float vx, float vy) { x=vx; y=vy; return *this; }
inline float vec2::dot(const vec2& v) const { return x*v.x + y*v.y; }
inline bool  vec2::operator==(const vec2& v) const { return x==v.x && y==v.y; }
inline bool  vec2::operator!=(const vec2& v) const { return x!=v.x || y!=v.y; }
inline bool  vec2::isZero() const { return x==0 && y==0; }

inline vec3::vec3(): x(0), y(0), z(0) {}
inline vec3::vec3(float v): x(v), y(v), z(v) {}
inline vec3::vec3(float x, float y, float z): x(x), y(y), z(z) {}
inline vec3::vec3(const vec2& p, float z): x(p.x), y(p.y), z(z) {}
inline vec3::vec3(const float* p): x(p[0]), y(p[1]), z(p[2]) {}
inline vec3::operator float*() { return &x; }
inline vec3::operator const float*() const { return &x; }
inline vec3& vec3::set(float vx, float vy, float vz) { x=vx; y=vy; z=vz; return *this; }
inline float vec3::dot(const vec3& v) const { return x*v.x + y*v.y + z*v.z; }
inline bool  vec3::operator==(const vec3& v) const { return x==v.x && y==v.y && z==v.z; }
inline bool  vec3::operator!=(const vec3& v) const { return x!=v.x || y!=v.y || z!=v.z; }
inline bool  vec3::isZero() const { return x==0 && y==0 && z==0; }

inline vec4::vec4(): x(0), y(0), z(0), w(0) {}
inline vec4::vec4(float v): x(v), y(v), z(v), w(v) {}
inline vec4::vec4(float x, float y, float z, float w): x(x), y(y), z(z), w(w) {}
inline vec4::vec4(const vec3& p, float w): x(p.x), y(p.y), z(p.z), w(w) {}
inline vec4::vec4(const float* p): x(p[0]), y(p[1]), z(p[2]), w(p[3]) {}
inline vec4::operator float*() { return &x; }
inline vec4::operator const float*() const { return &x; }
inline vec4& vec4::set(float vx, float vy, float vz, float vw) { x=vx; y=vy; z=vz; w=vw; return *this; }
inline float vec4::dot(const vec4& v) const { return x*v.x + y*v.y + z*v.z + w*v.w; }
inline bool  vec4::operator==(const vec4& v) const { return x==v.x && y==v.y && z==v.z && w==v.w; }
inline bool  vec4::operator!=(const vec4& v) const { return x!=v.x || y!=v.y || z!=v.z || w!=v.w; }
inline bool  vec4::isZero() const { return x==0 && y==0 && z==0 && w==0; }


ImplementMutateOp(=);
ImplementMutateOp(+=);
ImplementMutateOp(-=);
ImplementMutateOp(*=);
ImplementMutateOp(/=);
ImplementMutateOpF(*=);
ImplementMutateOpF(/=);

ImplementReturnOp(-);
ImplementReturnOp(+);
ImplementReturnOp(*);
ImplementReturnOp(/);
ImplementReturnOpF(+);
ImplementReturnOpF(-);
ImplementReturnOpF(*);
ImplementReturnOpF(/);

ImplementReturnOpFExt(-);
ImplementReturnOpFExt(+);
ImplementReturnOpFExt(*);
ImplementReturnOpFExt(/);

ImplementCommon(length2,    return dot(*this), const);
ImplementCommon(length,     return sqrt(length2()), const);
ImplementCommonV(distance2, return (v-*this).length2(), const );
ImplementCommonV(distance,  return (v-*this).length(), const );
ImplementCommon(normaliseWithLength, float l=length(); if(l>0) *this/=l; return l, );

ImplementFuncOp(fabs);
ImplementFuncOp(ceil);
ImplementFuncOp(floor);
ImplementFuncOpT(pow, float);

ImplementCommonExt(lerp, return a*(1-t) + b*t );
ImplementCommonExt(step, float d=a.distance(b); return d<t? b: a + (b-a) * (t / d) );

inline vec2 vec2::operator-() const { return vec2(-x,-y); }
inline vec3 vec3::operator-() const { return vec3(-x,-y,-z); }
inline vec4 vec4::operator-() const { return vec4(-x,-y,-z,-w); }

inline vec2& vec2::normalise() { if(!isZero()) *this/=length(); return *this; }
inline vec3& vec3::normalise() { if(!isZero()) *this/=length(); return *this; }
inline vec4& vec4::normalise() { if(!isZero()) *this/=length(); return *this; }
inline vec2 vec2::normalised() const { return vec2(*this).normalise(); }
inline vec3 vec3::normalised() const { return vec3(*this).normalise(); }
inline vec4 vec4::normalised() const { return vec4(*this).normalise(); }

inline vec3 vec3::cross(const vec3& v) const { return vec3(y*v.z-z*v.y, z*v.x-x*v.z, x*v.y-y*v.x);  }

inline vec3 vec2::xyz(float z) const { return vec3(x,y,z); }
inline vec3 vec2::xzy(float z) const { return vec3(x,z,y); }
inline vec3 vec4::xyz() const { return vec3(x,y,z); }


inline Ray::Ray(const vec3& p, const vec3& d) : start(p), direction(d.normalised()) {}
inline Ray& Ray::set(const vec3& p, const vec3& d) { start = p, direction = d.normalised(); return *this; }
inline vec3 Ray::point(float t) const { return start + direction * t; }


#endif

