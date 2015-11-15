#ifndef _BASE_AABB_
#define _BASE_AABB_

#undef min
#undef max

#include "vec.h"


/** Range template class */
template<typename T>
class RangeT {
	public:
	T min, max;
	RangeT(T value=0);								/// Construct a range with min and max the same
	RangeT(T min, T max);							/// Construct a range with defined values
	void setInvalid();								/// Set to a negative range
	void set(T value);								/// Reset the range to a value
	void set(T min, T max);							/// Set the range to value
	bool contains(const T& value) const;			/// Does this range contain a value
	bool contains(const RangeT<T>& value) const;	/// Does this range fully contain another range
	bool intersects(const RangeT<T>& value) const;	/// Does this range intersect a range
	void include(const RangeT<T>& value) const;		/// Expand range to include another range
	void include(const T& value);					/// Expand the range to include value
	void expand(const T& value);					/// Expand the range by value in both directions
	T    clamp(const T& v) const;					/// Clamp a value within this range
	T    wrap(const T& v) const;					/// Wrap a value witin this range
	T    size() const;								/// Return the size of the range
	bool isValid() const;
	bool isEmpty() const;
};

typedef RangeT<float> Rangef, Range;
typedef RangeT<int>   Rangei;


/** Axis aligned bounding box class */
class BoundingBox {
	public:
	vec3 min;
	vec3 max;
	
	BoundingBox() {};
	BoundingBox(const vec3& p) : min(p), max(p) {};
	BoundingBox(const vec3& min, const vec3& max) : min(min), max(max) {};
	BoundingBox(float ax, float ay, float az, float bx, float by, float bz) : min(ax,ay,az), max(bx,by,bz) {};

	BoundingBox& setInvalid();
	BoundingBox& set(const vec3& min, const vec3& max);
	BoundingBox& set(float ax, float ay, float az, float bx, float by, float bz);
	
	BoundingBox& operator= (const BoundingBox& b) { min=b.min; max=b.max; return *this; }
	BoundingBox  operator+ (const vec3& v) const { return BoundingBox(min+v, max+v); }
	BoundingBox  operator- (const vec3& v) const { return BoundingBox(min-v, max-v); }
	BoundingBox& operator+=(const vec3& v) { min+=v, max+=v; return *this;  }
	BoundingBox& operator-=(const vec3& v) { min-=v, max-=v; return *this;  }
	
	vec3 size      () const;						/// Get the size of the box
	vec3 centre    () const;						/// Get the box centre point
	void include   (const vec3& p);					/// Expand the bounding box to contain a point */
	void include   (const BoundingBox& box);		/// Expand the bounding box to contain another bounding box */
	void expand    (const float v);					/// Expand the range by value in all directions
	bool contains  (const vec3& p) const;			/// Is a point within the bounding box */
	bool contains  (const BoundingBox& b) const;	/// Is a box entirely within this bounding box */
	bool intersects(const BoundingBox& b) const;	/// Does a box intersect with this box */
	bool isValid() const;							/// Is the box valid? min < max
	bool isEmpty() const;							/// Is the box empty? min = max
	
};


/** 2D Bounding box */
class BoundingBox2D {
	public:
	vec2 min;
	vec2 max;
	
	BoundingBox2D() {};
	BoundingBox2D(const vec2& p) : min(p), max(p) {};
	BoundingBox2D(const vec2& min, const vec2& max) : min(min), max(max) {};
	BoundingBox2D(float ax, float ay, float bx, float by) : min(ax,ay), max(bx,by) {};

	BoundingBox2D& setInvalid();
	BoundingBox2D& set(const vec2& min, const vec2& max);
	BoundingBox2D& set(float ax, float ay, float bx, float by);
	
	BoundingBox2D& operator= (const BoundingBox2D& b) { min=b.min; max=b.max; return *this; }
	BoundingBox2D  operator+ (const vec2& v) const { return BoundingBox2D(min+v, max+v); }
	BoundingBox2D  operator- (const vec2& v) const { return BoundingBox2D(min-v, max-v); }
	BoundingBox2D& operator+=(const vec2& v) { min+=v, max+=v; return *this;  }
	BoundingBox2D& operator-=(const vec2& v) { min-=v, max-=v; return *this;  }
	
	vec2 size      () const;						/// Get the size of the box
	vec2 centre    () const;						/// Get the box centre point
	void include   (const vec2& p);					/// Expand the bounding box to contain a point */
	void include   (const BoundingBox2D& box);		/// Expand the bounding box to contain another bounding box */
	void expand    (const float v);					/// Expand the range by value in all directions
	bool contains  (const vec2& p) const;			/// Is a point within the bounding box */
	bool contains  (const BoundingBox2D& b) const;	/// Is a box entirely within this bounding box */
	bool intersects(const BoundingBox2D& b) const;	/// Does a box intersect with this box */
	bool isValid() const;							/// Is the box valid? min < max
	bool isEmpty() const;							/// Is the box empty? min = max
};

// Range implementation
template<typename T> RangeT<T>::RangeT(T v):        min(v), max(v) {}
template<typename T> RangeT<T>::RangeT(T a, T b):   min(a), max(b) {}
template<typename T> void RangeT<T>::set(T v)       { min = max = v; }
template<typename T> void RangeT<T>::set(T a, T b)  { min = a; max = b; }
template<typename T> bool RangeT<T>::contains(const T& v) const           { return v>=min && v<=max; }
template<typename T> bool RangeT<T>::contains(const RangeT<T>& v) const   { return v.min>=min && v.max<=max; }
template<typename T> bool RangeT<T>::intersects(const RangeT<T>& v) const { return v.max>=min && v.min<=max; }
template<typename T> void RangeT<T>::include(const RangeT<T>& v) const     { if(v.min<min) min=v.min; if(v.max>max) max=v.max; }
template<typename T> void RangeT<T>::include(const T& v)                   { if(v<min) min=v; if(v>max) max=v;  }
template<typename T> void RangeT<T>::expand(const T& v)                   { min -= v; max += v;  }
template<typename T> T    RangeT<T>::clamp(const T& v)  const  { return v<min? min: v>max? max: v; }
template<typename T> T    RangeT<T>::wrap(const T& v)  const   { return  v - floor((v-min)/(max-min))*(max-min); }
template<typename T> T    RangeT<T>::size() const              { return  max - min; }




// Inline implemnation
inline BoundingBox& BoundingBox::setInvalid()	{ min.x=min.y=min.z=1e37f; max.x=max.y=max.z=-1e37f; return *this; }
inline BoundingBox& BoundingBox::set(const vec3& vmin, const vec3& vmax) { min=vmin; max=vmax; return *this; }
inline BoundingBox& BoundingBox::set(float a, float b, float c, float d, float e, float f) { min.x=a; min.y=b, min.z=c; max.x=d; max.y=e; max.z=f; return *this; }

inline bool BoundingBox::isValid() const { return max.x>min.x && max.y>min.y && max.z>min.z; }
inline bool BoundingBox::isEmpty() const { return max.x==min.x && max.y==min.y && max.z==min.z; }
inline vec3 BoundingBox::size()   const { return max-min; }
inline vec3 BoundingBox::centre() const { return (min+max)*0.5; }
inline void BoundingBox::include(const vec3& p) {
	if(p.x<min.x) min.x=p.x;
	if(p.y<min.y) min.y=p.y;
	if(p.z<min.z) min.z=p.z;
	if(p.x>max.x) max.x=p.x;
	if(p.y>max.y) max.y=p.y;
	if(p.z>max.z) max.z=p.z;
}
inline void BoundingBox::include(const BoundingBox& b) {
	if(!b.isValid()) return;
	include(b.min);
	include(b.max);
}
inline void BoundingBox::expand(float v) {
	min.x-=v; min.y-=v; min.z-=v;
	max.x+=v; max.y+=v; max.z+=v;
}
inline bool BoundingBox::contains(const vec3& p) const {
	return p.x>=min.x && p.x<=max.x && p.y>=min.y && p.y<=max.y && p.z>=min.z && p.z<=max.z;
}
inline bool BoundingBox::contains(const BoundingBox& b) const {
	return b.min.x>=min.x && b.min.y>=min.y && b.min.z>=min.z && b.max.x<=max.x && b.max.y<=max.y && b.max.z<=max.z;
}
inline bool BoundingBox::intersects(const BoundingBox& b) const {
	return b.max.x>min.x && b.min.x<max.x && b.max.y>min.y && b.min.y<max.y && b.max.z>min.z && b.min.z<max.z;
}



// 2D version
inline BoundingBox2D& BoundingBox2D::setInvalid()	{ min.x=min.y=1e37f; max.x=max.y=-1e37f; return *this; }
inline BoundingBox2D& BoundingBox2D::set(const vec2& vmin, const vec2& vmax) { min=vmin; max=vmax; return *this; }
inline BoundingBox2D& BoundingBox2D::set(float a, float b, float c, float d) { min.x=a; min.y=b; max.x=c; max.y=d; return *this; }

inline bool BoundingBox2D::isValid() const { return max.x>min.x && max.y>min.y; }
inline bool BoundingBox2D::isEmpty() const { return max.x==min.x && max.y==min.y; }
inline vec2 BoundingBox2D::size()   const { return max-min; }
inline vec2 BoundingBox2D::centre() const { return (min+max)*0.5; }
inline void BoundingBox2D::include(const vec2& p) {
	if(p.x<min.x) min.x=p.x;
	if(p.y<min.y) min.y=p.y;
	if(p.x>max.x) max.x=p.x;
	if(p.y>max.y) max.y=p.y;
}
inline void BoundingBox2D::include(const BoundingBox2D& b) {
	include(b.min);
	include(b.max);
}
inline void BoundingBox2D::expand(float v) {
	min.x-=v; min.y-=v;
	max.x+=v; max.y+=v;
}
inline bool BoundingBox2D::contains(const vec2& p) const {
	return p.x>=min.x && p.x<=max.x && p.y>=min.y && p.y<=max.y;
}
inline bool BoundingBox2D::contains(const BoundingBox2D& b) const {
	return b.min.x>=min.x && b.min.y>=min.y && b.max.x<=max.x && b.max.y<=max.y;
}
inline bool BoundingBox2D::intersects(const BoundingBox2D& b) const {
	return b.max.x>min.x && b.min.x<max.x && b.max.y>min.y && b.min.y<max.y;
}

#endif

