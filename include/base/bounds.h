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
	void set(T value);								/// Reset the range to a value
	void set(T min, T max);							/// Set the range to value
	bool contains(const T& value) const;			/// Does this range contain a value
	bool intersects(const RangeT<T>& value) const;	/// Does this range intersect another
	void expand(const T& value);					/// Expand the range to include value
	T    clamp(const T& v) const;					/// Clamp a value within this range
	T    wrap(const T& v) const;					/// Wrap a value witin this range
	T    size() const;								/// Return the size of the range
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
	
	BoundingBox& operator= (const BoundingBox& b) { min=b.min; max=b.max; return *this; }
	BoundingBox  operator+ (const vec3& v) const { return BoundingBox(min+v, max+v); }
	BoundingBox  operator- (const vec3& v) const { return BoundingBox(min-v, max-v); }
	BoundingBox& operator+=(const vec3& v) { min+=v, max+=v; return *this;  }
	BoundingBox& operator-=(const vec3& v) { min-=v, max-=v; return *this;  }
	
	vec3 size      () const;						/// Get the size of the box
	vec3 centre    () const;						/// Get the box centre point
	void add       (const vec3& p);					/// Expand the bounding box to contain a point */
	void add       (const BoundingBox& box);		/// Expand the bounding box to contain another bounding box */
	bool contains  (const vec3& p) const;			/// Is a point within the bounding box */
	bool contains  (const BoundingBox& b) const;	/// Is a box entirely within this bounding box */
	bool intersects(const BoundingBox& b) const;	/// Does a box intersect with this box */
};


// Range implementation
template<typename T> RangeT<T>::RangeT(T v):        min(v), max(v) {}
template<typename T> RangeT<T>::RangeT(T a, T b):   min(a), max(b) {}
template<typename T> void RangeT<T>::set(T v)       { min = max = v; }
template<typename T> void RangeT<T>::set(T a, T b)  { min = a; max = b; }
template<typename T> bool RangeT<T>::contains(const T& v) const           { return v>=min && v<=max; }
template<typename T> bool RangeT<T>::intersects(const RangeT<T>& v) const { return v.max>=min && v.min<=max; }
template<typename T> void RangeT<T>::expand(const T& v)                   { if(v<min) min=v; if(v>max) max=v;  }
template<typename T> T    RangeT<T>::clamp(const T& v)  const  { return v<min? min: v>max? max: v; }
template<typename T> T    RangeT<T>::wrap(const T& v)  const   { return  v - floor((v-min)/(max-min))*(max-min); }
template<typename T> T    RangeT<T>::size() const              { return  max - min; }




// Inline implemnation
inline vec3 BoundingBox::size()   const { return max-min; }
inline vec3 BoundingBox::centre() const { return (min+max)*0.5; }
inline void BoundingBox::add(const vec3& p) {
	if(p.x<min.x) min.x=p.x;
	if(p.y<min.y) min.y=p.y;
	if(p.z<min.z) min.z=p.z;
	if(p.x>max.x) max.x=p.x;
	if(p.y>max.y) max.y=p.y;
	if(p.z>max.z) max.z=p.z;
}
inline void BoundingBox::add(const BoundingBox& b) {
	add(b.min);
	add(b.max);
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



#endif

