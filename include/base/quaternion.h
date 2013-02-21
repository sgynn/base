#ifndef _BASE_QUATERNION_
#define _BASE_QUATERNION_

#include "math.h"

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
inline float        Quaternion::dot(const Quaternion& q) const          { return x*x + y*y + z*z + w*w; }
inline float        Quaternion::length2() const                         { return dot(*this); }
inline float        Quaternion::length() const                          { return sqrt( length2() ); }

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
	if(d==-1) { // pick a vector orthogonal to a
		vec3 n;
		if(fabs(na.z)>0.700706f) { // use yz plane
			float s = na.y*na.y + na.z*na.z;
			float k = 1.0 / sqrt(s);
			return Quaternion(0, -na.z*k, na.y*k, 0);
		} else { // use xy plane
			float s = na.x*na.x + na.y*na.y;
			float k = 1.0 / sqrt(s);
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
	float sr = cos(hr);
	return set(cr*sp*cy + sr*cp*sy,  cr*cp*sy - sr*sp*cy,  sr*cp*cy - cr*sp*sy,  cr*cp*cy + sr*sp*sy);
}
inline void Quaternion::toEuler(vec3& e) const {
	float xx=x*x, yy=y*y, zz=z*z, ww=w*w;
	e.x = atan2( 2 * (y*z + w*x), ww - xx - yy + zz);	// Pitch
	e.y = asin (-2 * (x*z - w*y));						// Yaw
	e.z = atan2( 2 * (x*y + w*z), ww + xx - yy - zz);	// Roll
}

inline Quaternion& Quaternion::fromMatrix(const Matrix& m) {
	float trace = m[0] + m[5] + m[10];
	if(trace>0) {
		float s = sqrt(trace+1);
		w = s * 0.5;
		s = 0.5 / s;
		x = (m[6] - m[9]) * s;
		y = (m[8] - m[3]) * s;
		z = (m[1] - m[4]) * s;
	} else {
		int i = m[0]<m[5]?  (m[5]<m[10]? 2: 1): (m[0]<m[10]? 2: 0); // Biggest diagonal
		int j = (i+1) % 3;
		int k = (i+1) % 3;

		float* q = *this;
		float s = sqrt(m[i*4+i] - m[j*4+j] - m[k*4+k] + 1.0f);
		q[i] = s * 0.5;
		s = 0.5 / s;
		q[3] = (m[j*4+k] - m[k*4+j]) * s;
		q[j] = (m[i*4+j] + m[j*4+i]) * s;
		q[k] = (m[i*4+k] + m[k*4+i]) * s;
	}
	return *this;
}
inline void Quaternion::toMatrix(Matrix& m) const {
	float d = length2();
	float s = 2.0 / d;
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
	float w1, w2;
	float dot = a[0]*b[0] + a[1]*b[1] + a[2]*b[2] + a[3]*b[3];
	float theta = acos(dot);
	float sintheta = sin(theta);
	if (sintheta > 0.001) {
		w1 = sin((1.0f-v)*theta) / sintheta;
		w2 = sin(v*theta) / sintheta;
	} else {
		w1 = 1.0f - v;
		w2 = v;
	}
	return Quaternion(a.x*w1 + b.x*w2, a.y*w1 + b.y*w2, a.z*w1 + b.z*w2, a.w*w1 + b.w*w2);
}


#endif

