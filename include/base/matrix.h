#pragma once

#include "vec.h"
#include <initializer_list>

/** Inline lightweight matrix class */
class Matrix {
	public:
	float m[16];

	Matrix();
	Matrix(const float* p);
	Matrix(std::initializer_list<float> v) { int i=-1; for(float f : v) if(++i<16) m[i]=f; else break; }
	Matrix(const vec3& x, const vec3& y, const vec3& z, const vec3& p=vec3());

	Matrix& setRotation(const Matrix& m);	// Update rotation part of matrix
	Matrix& setTranslation(const vec3& t);	// Update translation part
	Matrix& set(const vec3& t, const vec3& b, const vec3& n, const vec3& p=vec3());

	operator float*();
	operator const float*() const;
	Matrix&  operator=  (const Matrix&) = default;
	Matrix&  operator*= (const Matrix& m);
	Matrix   operator*  (const Matrix&) const;
	vec4     operator*  (const vec4&) const;
	vec3     operator*  (const vec3&) const;
	vec3     rotate     (const vec3&) const;	// Multiply vector by rotation part of matrix
	vec3     transform  (const vec3&) const;	// Transform a vector - same as matrix * vector
	vec3     unrotate   (const vec3&) const; 	// Multiply vector by inverse rotation part
	vec3     untransform(const vec3&) const;	// Multiply vector by inverse affine matrix
	Ray      transform(const Ray&) const;
	Ray      untransform(const Ray&) const;

	Matrix&  setIdentity();			// Set this matrix to the identity matrix
	Matrix&  transpose();			// Transpose this matrix
	Matrix   transposed() const;	// Get a transposed copy of this matrix

	Matrix&  translate(const vec3& t);	// Translate this matrix by t
	Matrix&  scale(const vec3& s);		// scale this matrix by s
	Matrix&  scale(float s);			// scale this matrix by s

	bool isAffine() const;			// Is this an affine matrix

	static void inverseAffine(Matrix& out, const Matrix& in);
};


// Implementation //
inline Matrix::Matrix() { setIdentity(); }
inline Matrix::Matrix(const float* p) { for(int i=0; i<16; i++) m[i]=p[i]; }
inline Matrix::Matrix(const vec3& x, const vec3& y, const vec3& z, const vec3& p) {
	m[0]=x[0];  m[1]=x[1];  m[2]=x[2];  m[3]=0;
	m[4]=y[0];  m[5]=y[1];  m[6]=y[2];  m[7]=0;
	m[8]=z[0];  m[9]=z[1];  m[10]=z[2]; m[11]=0;
	m[12]=p[0]; m[13]=p[1]; m[14]=p[2]; m[15]=1;
}
inline Matrix& Matrix::setIdentity() {
		m[1]=m[2]=m[3]=m[4]=m[6]=m[7]=m[8]=m[9]=m[11]=m[12]=m[13]=m[14]=0;
		m[0]=m[5]=m[10]=m[15]=1;
		return *this;
}
inline Matrix& Matrix::setRotation(const Matrix& r) {
	m[0]=r[0]; m[1]=r[1]; m[2]=r[2];
	m[4]=r[4]; m[5]=r[5]; m[6]=r[6];
	m[8]=r[8]; m[9]=r[9]; m[10]=r[10];
	return *this;
}
inline Matrix& Matrix::setTranslation(const vec3& t) {
	m[12]=t[0]; m[13]=t[1]; m[14]=t[2];
	return *this;
}
inline Matrix& Matrix::set(const vec3& t, const vec3& b, const vec3& n, const vec3& p) {
	m[0]=t[0];  m[1]=t[1];  m[2]=t[2];  m[3]=0;
	m[4]=b[0];  m[5]=b[1];  m[6]=b[2];  m[7]=0;
	m[8]=n[0];  m[9]=n[1];  m[10]=n[2]; m[11]=0;
	m[12]=p[0]; m[13]=p[1]; m[14]=p[2]; m[15]=1;
	return *this;
}

inline Matrix::operator float*() { return &m[0]; }
inline Matrix::operator const float*() const { return &m[0]; }
inline Matrix Matrix::operator*(const Matrix& mat) const {
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
inline Matrix& Matrix::operator*=(const Matrix& m) {
	*this = *this * m;
	return *this;
}
inline vec3 Matrix::operator*(const vec3& v) const {
	vec3 result;
	result.x = m[0]*v.x + m[4]*v.y + m[8]*v.z + m[12];
	result.y = m[1]*v.x + m[5]*v.y + m[9]*v.z + m[13];
	result.z = m[2]*v.x + m[6]*v.y + m[10]*v.z + m[14];
	return result;
}
inline vec4 Matrix::operator*(const vec4& v) const {
	vec4 result;
	result.x = m[0]*v.x + m[4]*v.y + m[8]*v.z + m[12]*v.w;
	result.y = m[1]*v.x + m[5]*v.y + m[9]*v.z + m[13]*v.w;
	result.z = m[2]*v.x + m[6]*v.y + m[10]*v.z + m[14]*v.w;
	result.w = m[3]*v.x + m[7]*v.y + m[11]*v.z + m[15]*v.w;
	return result;
}
inline vec3 Matrix::rotate(const vec3& v) const {
	vec3 result;
	result.x = m[0]*v.x + m[4]*v.y + m[8]*v.z;
	result.y = m[1]*v.x + m[5]*v.y + m[9]*v.z;
	result.z = m[2]*v.x + m[6]*v.y + m[10]*v.z;
	return result;
}
inline vec3 Matrix::transform(const vec3& v) const {
	return *this * v;
}
inline vec3 Matrix::unrotate(const vec3& v) const {
	vec3 result;
	result.x = m[0]*v.x + m[1]*v.y + m[2]*v.z;
	result.y = m[4]*v.x + m[5]*v.y + m[6]*v.z;
	result.z = m[8]*v.x + m[9]*v.y + m[10]*v.z;
	return result;
}
inline vec3 Matrix::untransform(const vec3& v) const {
	vec3 result, t = v - vec3(&m[12]);
	result.x = m[0]*t.x + m[1]*t.y + m[2]*t.z;
	result.y = m[4]*t.x + m[5]*t.y + m[6]*t.z;
	result.z = m[8]*t.x + m[9]*t.y + m[10]*t.z;
	return result;
}
inline Ray Matrix::transform(const Ray& ray) const {
	Ray result;
	result.start = transform(ray.start);
	result.direction = rotate(ray.direction);
	return result;
}
inline Ray Matrix::untransform(const Ray& ray) const {
	Ray result;
	result.start = untransform(ray.start);
	result.direction = unrotate(ray.direction);
	return result;
}
inline Matrix& Matrix::transpose() {
	float tmp;
	#define swap(a,b) { tmp=m[a]; m[a]=m[b]; m[b]=tmp; }
	swap(1,4); swap(2,8); swap(3,12);
	swap(6,9); swap(7,13); swap(11,14);
	#undef swap
	return *this;
}
inline Matrix Matrix::transposed() const {
	Matrix mat;
	mat[0] = m[0];	mat[1] = m[4];	mat[2] = m[8];	mat[3] = m[12];
	mat[4] = m[1];	mat[5] = m[5];	mat[6] = m[9];	mat[7] = m[13];
	mat[8] = m[2];	mat[9] = m[6];	mat[10] = m[10]; mat[11] = m[14];
	mat[12] = m[3];	mat[13] = m[7];	mat[14] = m[11]; mat[15] = m[15];
	return mat;
}

inline Matrix& Matrix::translate(const vec3& t) {
	m[12] += t.x;
	m[13] += t.y;
	m[14] += t.z;
	return *this;
}

inline Matrix& Matrix::scale(const vec3& s) {
	m[0] *= s.x; m[1] *= s.x; m[2] *= s.x;
	m[4] *= s.y; m[5] *= s.y; m[6] *= s.y;
	m[8] *= s.z; m[9] *= s.z; m[10] *= s.z;
	return *this;
}

inline Matrix& Matrix::scale(float s) {
	m[0] *= s; m[1] *= s; m[2] *= s;
	m[4] *= s; m[5] *= s; m[6] *= s;
	m[8] *= s; m[9] *= s; m[10] *= s;
	return *this;
}

/** A simple fast inverse matrix. Limited to translation and rotation matrices */
inline void Matrix::inverseAffine(Matrix& out, const Matrix& in) {
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

inline bool Matrix::isAffine() const {
	const float ep = 0.00001;
	vec3 a(m), b(m+4), c(m+8);
	return a.dot(b)<ep && a.dot(c)<ep && b.dot(c)<ep;

}

