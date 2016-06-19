#ifndef _BASE_QUATERNION_
#define _BASE_QUATERNION_

#include "matrix.h"

/** Quaternion class */
class Quaternion {
	public:
	float x,y,z,w;

	Quaternion();											/** Construct empty quaternion */
	Quaternion(float x, float y, float z, float w);			/** Construct quaternion with specified value */
	Quaternion(const vec3& euler);							/** Construct quaternion from euler angles (pitch,yaw,roll) */
	Quaternion(const Matrix& m);							/** Construct quaternion from rotation matrix */
	Quaternion(const float* f);								/** Construct quaternion from float array */

	// Accessors
	operator      const float*() const;						/** typecast to float array */
	operator      float*();									/** typecast to float array */
	const float&  operator[](int) const;					/** Access elements by index */
	float&        operator[](int);							/** Access elements by index */
	Quaternion&   set(float x, float y, float z, float w);	/** Set the element values */


	// Conversion
	Quaternion& fromEuler(const vec3&);						/** Set values from euler angle */
	Quaternion& fromMatrix(const Matrix& m);				/** Set values from matrix */
	void        toMatrix(Matrix& m) const;					/** Convert quaternion to matrix in place */
	void        toEuler(vec3& v) const;						/** Convert quaternion to euler angles in place */
	Matrix      getMatrix() const;							/** Return matrix representation of quaternion */
	vec3        getEuler() const;							/** Return euler angled representation of quaternion */
	vec3        getAxis() const;							/** Get the axis represented by this quaternion */
	float       getAngle() const;							/** Get angle represented by this quaternion */
	Quaternion  getInverse() const;							/** Get the inverse of this quaternion */


	// Operators
	bool        operator==(const Quaternion& q) const;		/** Compare two quaternions */
	bool        operator!=(const Quaternion& q) const;		/** Compare quaternions */
	Quaternion& operator*=(const Quaternion& q);
	Quaternion  operator* (const Quaternion& q) const;
	vec3        operator* (const vec3&) const;

	float dot(const Quaternion&) const;
	float getAngle(const Quaternion&) const;
	float length2() const;
	float length() const;

	// Static Functions
	static Quaternion arc(const vec3& a, const vec3& b);	/** Get the quaternion between two direction vectors */
};




// Inline implementation
inline Quaternion::Quaternion() : x(0),y(0),z(0),w(1) {};
inline Quaternion::Quaternion(float x, float y, float z, float w): x(x), y(y), z(z), w(w) {}
inline Quaternion::Quaternion(const float* f) : x(f[0]), y(f[1]), z(f[2]), w(f[3]) {}
inline Quaternion::Quaternion(const vec3& e)                            { fromEuler(e); }
inline Quaternion::Quaternion(const Matrix& m)                          { fromMatrix(m); }
inline Quaternion::operator const float*() const                        { return &x; }
inline Quaternion::operator float*()                                    { return &x; }
inline const float& Quaternion::operator[](int i) const                 { return *((&x)+i); }
inline float&       Quaternion::operator[](int i)                       { return *((&x)+i); }
inline Quaternion&  Quaternion::set(float x, float y, float z, float w) { this->x=x; this->y=y; this->z=z; this->w=w; return *this; }
inline bool         Quaternion::operator==(const Quaternion& q) const   { return x==q.x && y==q.y && z==q.z && w==q.w; }
inline bool         Quaternion::operator!=(const Quaternion& q) const   { return x!=q.x || y!=q.y || z!=q.z || w!=q.w; }
inline vec3         Quaternion::getEuler() const                        { vec3 e; toEuler(e); return e; }
inline Matrix       Quaternion::getMatrix() const                       { Matrix m; toMatrix(m); return m; }
inline float        Quaternion::getAngle() const                        { return 2 * acos(w); }
inline Quaternion   Quaternion::getInverse() const                      { return Quaternion(-x, -y, -z, w); }
inline float        Quaternion::dot(const Quaternion& q) const          { return x*q.x + y*q.y + z*q.z + w*q.w; }
inline float        Quaternion::length2() const                         { return dot(*this); }
inline float        Quaternion::length() const                          { return sqrt( length2() ); }

inline Quaternion& Quaternion::operator*=(const Quaternion& q) {
	set(w*q.x + x*q.w + y*q.z - z*q.y,
	    w*q.y + y*q.w + z*q.x - x*q.z,
	    w*q.z + z*q.w + x*q.y - y*q.x,
	     w*q.w - x*q.x - y*q.y - z*q.z);
	return *this;
}
inline Quaternion Quaternion::operator*(const Quaternion& q) const {
	return Quaternion(w*q.x + x*q.w + y*q.z - z*q.y,
					  w*q.y + y*q.w + z*q.x - x*q.z,
					  w*q.z + z*q.w + x*q.y - y*q.x,
					  w*q.w - x*q.x - y*q.y - z*q.z);
}

inline vec3 Quaternion::getAxis() const {
	float sq = 1.0f - w*w;
	if(sq==0) return vec3(1,0,0);
	else return vec3(x,y,z) / sqrt(sq);
}

inline float Quaternion::getAngle(const Quaternion& q) const {
	float s = sqrt(length2() * q.length2());
	return acos(dot(q)/s);
}

inline Quaternion Quaternion::arc(const vec3& a, const vec3& b) {
	vec3  na = a.normalised();
	vec3  nb = b.normalised();
	vec3  c = na.cross(nb);
	float d = na.dot(nb);
	if(d<-0.9999f) { // pick a vector orthogonal to a
		vec3 n;
		if(fabs(na.z)>0.700706f) { // use yz plane
			float s = na.y*na.y + na.z*na.z;
			float k = 1.0f / sqrt(s);
			return Quaternion(0, -na.z*k, na.y*k, 0);
		} else { // use xy plane
			float s = na.x*na.x + na.y*na.y;
			float k = 1.0f / sqrt(s);
			return Quaternion(-na.y*k, na.x*k, 0, 0);
		}
	}
	float s  = sqrt((1.0f+d) * 2.0f);
	float rs = 1.0f / s;
	return Quaternion(c.x*rs, c.y*rs, c.z*rs, s*0.5f);
}

inline Quaternion& Quaternion::fromEuler(const vec3& e) {
	float hy = e.y * 0.5f;
	float hp = e.x * 0.5f;
	float hr = e.z * 0.5f;
	float cy = cos(hy);
	float sy = sin(hy);
	float cp = cos(hp);
	float sp = sin(hp);
	float cr = cos(hr);
	float sr = sin(hr);

	x = cr*sp*cy + sr*cp*sy;  
	y = cr*cp*sy - sr*sp*cy;  
	z = sr*cp*cy - cr*sp*sy;
	w = cr*cp*cy + sr*sp*sy;
	return *this;

}
inline void Quaternion::toEuler(vec3& e) const {
//	float xx=x*x, yy=y*y, zz=z*z, ww=w*w;
//	e.x = atan2( 2 * (y*z + w*x), ww - xx - yy + zz);	// Pitch
//	e.y = asin (-2 * (x*z - w*y));						// Yaw
//	e.z = atan2( 2 * (x*y + w*z), ww + xx - yy - zz);	// Roll

	// My version - use matrix forward for pitch,yaw
	float fx = 2 * (x*z + w*y);
	float fy = 2 * (y*z - w*x);
	float fz = 1 - 2 * (x*x + y*y);
	float up = 1 - 2 * (x*x + z*z);
	float ly = 2 * (x*y + w*z);		// matrix left.y

	e.y = atan2(fx, fz);
	e.x = -atan2(fy, sqrt(fx*fx + fz*fz));
	if(up<0) { e.y -= PI; e.x=-PI-e.x; } // Flip values if pointing down.
	e.z = asin(ly / cos(e.x) );
	// Other version: PI-pitch, yaw+PI, roll+PI
}

inline Quaternion& Quaternion::fromMatrix(const Matrix& m) {
	float trace = m[0] + m[5] + m[10];
	if(trace>0) {
		float s = sqrt(trace+1);
		w = s * 0.5f;
		s = 0.5f / s;
		x = (m[6] - m[9]) * s;
		y = (m[8] - m[2]) * s;
		z = (m[1] - m[4]) * s;
	} else {
		int i = m[0]<m[5]?  (m[5]<m[10]? 2: 1): (m[0]<m[10]? 2: 0); // Biggest diagonal
		int j = (i+1) % 3;
		int k = (i+2) % 3;

		float* q = *this;
		float s = sqrt(m[i*4+i] - m[j*4+j] - m[k*4+k] + 1.0f);
		q[i] = s * 0.5f;
		s = 0.5f / s;
		q[3] = (m[j*4+k] - m[k*4+j]) * s;
		q[j] = (m[i*4+j] + m[j*4+i]) * s;
		q[k] = (m[i*4+k] + m[k*4+i]) * s;
	}
	return *this;
}
inline void Quaternion::toMatrix(Matrix& m) const {
	float d = length2();
	float s = 2.0f / d;
	float xs=x*s,  ys=y*s,  zs=z*s;
	float wx=w*xs, wy=w*ys, wz=w*zs;
	float xx=x*xs, xy=x*ys, xz=x*zs;
	float yy=y*ys, yz=y*zs, zz=z*zs;
	m[0] = 1-(yy+zz);
	m[1] = xy+wz;
	m[2] = xz-wy;
	m[4] = xy-wz;
	m[5] = 1-(xx+zz);
	m[6] = yz+wx;
	m[8] = xz+wy;
	m[9] = yz-wx;
	m[10] = 1-(xx+yy);
	m[14] = 1;
	m[3]=m[7]=m[11]=m[12]=m[13]=m[14]=0;
}

inline Quaternion slerp(const Quaternion& a, const Quaternion& b, float v) {
	float dot = a[0]*b[0] + a[1]*b[1] + a[2]*b[2] + a[3]*b[3];
	if(dot<=-1 || dot >= 1) return a;
	float flip = dot<0? -1: 1;
	float theta = acos(dot * flip);
	if(theta!=0.0f) {
		float d = 1 / sin(theta);
		float s = sin((1.0f-v)*theta) * d;
		float t = sin(v*theta) * d * flip;
		return Quaternion(a.x*s+b.x*t, a.y*s+b.y*t, a.z*s+b.z*t, a.w*s+b.w*t);
	} else return a;
}

// vector multiplication (nVidia SDK implementation)
inline vec3 Quaternion::operator*(const vec3& v) const {
	vec3 uv, uuv;
	vec3 qvec(x, y, z);
	uv = qvec.cross(v);
	uuv = qvec.cross(uv);
	uv *= (2.0f * w);
	uuv *= 2.0f;
	return v + uv + uuv;
}
inline vec3 operator*(const vec3& v, const Quaternion& q) {
	return q * v;
}


#endif

