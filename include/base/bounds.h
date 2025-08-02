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
	bool isValid() const;							/// Is min <= max
	bool isEmpty() const;							/// Is min == max

	T map(float t) const;							/// Map a 0-1 value to this range
	float unmap(const T& t) const;					/// Map a value to a 0-1 range wrt this 

	operator T*() { return &min; }
	operator const T*() const { return &min; }

	bool operator==(const RangeT<T>& r) const;
	bool operator!=(const RangeT<T>& r) const;
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

	bool operator==(const BoundingBox& b) const;
	bool operator!=(const BoundingBox& b) const;
	
	vec3 size      () const;						/// Get the size of the box
	vec3 centre    () const;						/// Get the box centre point
	void include   (const vec3& p);					/// Expand the bounding box to contain a point */
	void include   (const BoundingBox& box);		/// Expand the bounding box to contain another bounding box */
	void expand    (const float v);					/// Expand the range by value in all directions
	void expand    (const vec3& v);					/// Expand bounds per axis
	bool contains  (const vec3& p) const;			/// Is a point within the bounding box */
	bool contains  (const BoundingBox& b) const;	/// Is a box entirely within this bounding box */
	bool intersects(const BoundingBox& b) const;	/// Does a box intersect with this box */
	bool isValid() const;							/// Is the box valid? min <= max
	bool isEmpty() const;							/// Is the box empty? min = max
	vec3 clamp(const vec3&) const;					/// Clamp a vector inside box

	vec3 getCorner(int) const;						/// Get box corner
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

	bool operator==(const BoundingBox2D& b) const;
	bool operator!=(const BoundingBox2D& b) const;
	
	vec2 size      () const;						/// Get the size of the box
	vec2 centre    () const;						/// Get the box centre point
	void include   (const vec2& p);					/// Expand the bounding box to contain a point */
	void include   (const BoundingBox2D& box);		/// Expand the bounding box to contain another bounding box */
	void expand    (const float v);					/// Expand the range by value in all directions
	void expand    (const vec2& v);					/// Expand the range by value in each axis
	bool contains  (const vec2& p) const;			/// Is a point within the bounding box */
	bool contains  (const BoundingBox2D& b) const;	/// Is a box entirely within this bounding box */
	bool intersects(const BoundingBox2D& b) const;	/// Does a box intersect with this box */
	bool isValid() const;							/// Is the box valid? min <= max
	bool isEmpty() const;							/// Is the box empty? min = max
	vec2 clamp(const vec2&) const;					/// Clamp a point inside the box
};


// Test
template<class T> class AabbT {
	public:
	T centre, extent;
	AabbT() : extent(-1) {}
	AabbT(const T& centre, const T& extent) : centre(centre), extent(extent) {}
	AabbT& setInvalid();
	AabbT& set(const T& centre, const T& extent);
	AabbT& setRange(const T& min, const T& max);

	bool operator==(const AabbT& b) const;
	bool operator!=(const AabbT& b) const;

	T size() const;
	bool valid() const;
	bool empty() const;
	AabbT& include(const T&);
	AabbT& include(const AabbT&);
	void expand(const T&);
	bool contains(const T& point) const;
	bool contains(const AabbT& point) const;
	bool intersects(const AabbT& point) const;
	T clamp(const T&) const;
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
template<typename T> bool RangeT<T>::isValid() const           { return  max >= min; }
template<typename T> bool RangeT<T>::isEmpty() const           { return  max == min; }
template<typename T> T    RangeT<T>::map(float t) const        { return  min + t * (max - min); }
template<typename T> float RangeT<T>::unmap(const T& t) const  { return  (t - min) / (max - min); }
template<typename T> bool  RangeT<T>::operator==(const RangeT<T>& o) const  { return  min==o.min && max==o.max; }
template<typename T> bool  RangeT<T>::operator!=(const RangeT<T>& o) const  { return  min!=o.min || max!=o.max; }




// Inline implemnation
inline BoundingBox& BoundingBox::setInvalid()	{ min.x=min.y=min.z=1e37f; max.x=max.y=max.z=-1e37f; return *this; }
inline BoundingBox& BoundingBox::set(const vec3& vmin, const vec3& vmax) { min=vmin; max=vmax; return *this; }
inline BoundingBox& BoundingBox::set(float a, float b, float c, float d, float e, float f) { min.x=a; min.y=b, min.z=c; max.x=d; max.y=e; max.z=f; return *this; }

inline bool BoundingBox::operator==(const BoundingBox& o) const  { return  min==o.min && max==o.max; }
inline bool BoundingBox::operator!=(const BoundingBox& o) const  { return  min!=o.min || max!=o.max; }

inline bool BoundingBox::isValid() const { return max.x>=min.x && max.y>=min.y && max.z>=min.z; }
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
inline void BoundingBox::expand(const vec3& v) {
	min -= v;
	max += v;
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
inline vec3 BoundingBox::clamp(const vec3& p) const {
	if(!isValid()) return p;
	vec3 r = p;
	if     (r.x < min.x) r.x = min.x;
	else if(r.x > max.x) r.x = max.x;
	if     (r.y < min.y) r.y = min.y;
	else if(r.y > max.y) r.y = max.y;
	if     (r.z < min.z) r.z = min.z;
	else if(r.z > max.z) r.z = max.z;
	return r;
}

inline vec3 BoundingBox::getCorner(int i) const {
	const vec3* v = &min;
	return vec3(v[i&1].x, v[i>>1&1].y, v[i>>2&1].z);
}


// 2D version
inline BoundingBox2D& BoundingBox2D::setInvalid()	{ min.x=min.y=1e37f; max.x=max.y=-1e37f; return *this; }
inline BoundingBox2D& BoundingBox2D::set(const vec2& vmin, const vec2& vmax) { min=vmin; max=vmax; return *this; }
inline BoundingBox2D& BoundingBox2D::set(float a, float b, float c, float d) { min.x=a; min.y=b; max.x=c; max.y=d; return *this; }

inline bool BoundingBox2D::operator==(const BoundingBox2D& o) const  { return  min==o.min && max==o.max; }
inline bool BoundingBox2D::operator!=(const BoundingBox2D& o) const  { return  min!=o.min || max!=o.max; }

inline bool BoundingBox2D::isValid() const { return max.x>=min.x && max.y>=min.y; }
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
inline void BoundingBox2D::expand(const vec2& v) {
	min -= v;
	max += v;
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
inline vec2 BoundingBox2D::clamp(const vec2& p) const {
	if(!isValid()) return p;
	vec2 r = p;
	if     (r.x < min.x) r.x = min.x;
	else if(r.x > max.x) r.x = max.x;
	if     (r.y < min.y) r.y = min.y;
	else if(r.y > max.y) r.y = max.y;
	return r;
}



template<class T> AabbT<T>& AabbT<T>::setInvalid() { extent = T(-1); return *this; }
template<class T> AabbT<T>& AabbT<T>::set(const T& ctr, const T& ext) { centre=ctr; extent=ext; return *this; }
template<class T> AabbT<T>& AabbT<T>::setRange(const T& min, const T& max) { centre = (max+min)/2; extent = (max-min)/2; return *this; }
template<class T> T AabbT<T>::size() const { return extent * 2; }
template<class T> void AabbT<T>::expand(const T& s) { extent += s/2; }

/*
template<class T> bool AabbT<T>::valid() const { return extent.x>=0 && extent.y>=0; }
template<class T> bool AabbT<T>::empty() const { return extent.x==0 || extent.y==0; }
template<class T> AabbT<T>& AabbT<T>::include(const T& p) { return *this; }
template<class T> AabbT<T>& AabbT<T>::include(const AabbT&) { return *this; }
template<class T> bool AabbT<T>::contains(const T& point) const {}
template<class T> bool AabbT<T>::contains(const AabbT& point) const {}
template<class T> bool AabbT<T>::intersects(const AabbT& point) const {}
template<class T> T AabbT<T>::clamp(const T&) const {}
*/
template<> inline bool AabbT<vec2>::valid() const { return extent.x>=0 && extent.y>=0; }
template<> inline bool AabbT<vec2>::empty() const { return extent.x==0 || extent.y==0; }
template<> inline bool AabbT<vec3>::valid() const { return extent.x>=0 && extent.y>=0 && extent.z>=0; }
template<> inline bool AabbT<vec3>::empty() const { return extent.x==0 || extent.y==0 || extent.z==0; }
template<> inline bool AabbT<vec2>::contains(const vec2& point) const { return fabs(centre.x-point.x)<=extent.x && fabs(centre.y-point.y)<extent.y; }

template<typename T> bool AabbT<T>::operator==(const AabbT<T>& o) const  { return centre==o.centre && extent==o.extent; }
template<typename T> bool AabbT<T>::operator!=(const AabbT<T>& o) const  { return centre!=o.centre || extent!=o.extent; }


#endif

