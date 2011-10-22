#include "collision.h"

using namespace base;

#define clamp(v,min,max)	((v)<min?min:(v)>max?max:v)
/** Closest point on a line to a point */
vec3 base::closestPointOnLine(const vec3& point, const vec3& a, const vec3& b) {
	vec3 d = b-a;
	float t = d.dot(point-a) / d.dot(d);
	if(t<0) t=0; if(t>1) t=1;
	return a + d*t;
}

/** Closest point between lines */
float base::closestPointBetweenLines(const vec3& a1, const vec3& b1, const vec3& a2, const vec3& b2, vec3& p1, vec3& p2) {
	vec3 d1 = b1-a1; //Direction vectors
	vec3 d2 = b2-a2;
	vec3 r = a1-a2;
	float l1 = d1.dot(d1); //Squared length of lines
	float l2 = d2.dot(d2);
	float f = d2.dot(r);
	float s, t;
	//Do either lines degenerate into a point?
	if(l1<0.001 && l2<0.001) { //Two points
		p1=a1; p2=a2;
		return r.dot(r);
	}
	if(l1<0.001) {	//Line 1 degenerates
		s = 0; t = clamp(f/l2,0,1);
	} else {
		float c = d1.dot(r);
		if(l2<0.001) { //Line 2 degenerates
			t=0; s=clamp(-c/l1, 0,1);
		} else {
			// General non-degenerate case
			float b = d1.dot(d2);
			float denom = l1*l2 - b*b;
			if(denom==0) s = 0; //Parallel lines
			else s = clamp((b*f - c*l2)/denom, 0,1);
			t = (b*s+f)/l2;
			//if t inside [0,1] we are done, else clamp t and recalculate s
			if(t<0) {
				t=0; s=clamp(-c/l1, 0,1);
			} else if(t>1) {
				t=1; s=clamp((b-c)/l1, 0,1);
			}
		}
	}
	p1 = a1 + d1 * s;
	p2 = a2 + d2 * t;
	return p1.distanceSq(p2);
}

/** Intersection of a ray with Plane (page 177) */
int base::intersectRayPlane(const vec3& p, const vec3& d,  const vec3& normal, float offset, vec3& result) {
	float dn = normal.dot(d);
	if(dn==0) return 0;
	float t = (offset - normal.dot(p)) / dn;
	if(t>0 && t<1) {
		result = p + d * t;
		return 1;
	}
	return 0;
}

/** Intersection of a ray with Sphere (page 178) */
int base::intersectRaySphere(const vec3& p, const vec3& d, const vec3& centre, float radius, vec3& out) {
	vec3 m = p - centre;
	float b = m.dot(d);
	float c = m.dot(m) - radius*radius;
	//Exit if ray starts outside sphere and pointed away
	if(c > 0 && b > 0) return 0;
	float discriminant = b*b-c;
	if(discriminant<0) return 0; //missed
	//calculate smallest intersecion points
	float t = -b - sqrt(discriminant);
	//if t<0, we started inside sphere
	if(t < 0) t=0;
	out = p + d*t;
	return 1;
}
/** Triangle intersection with line */
int base::intersectLineTriangle(const vec3& p, const vec3& q, const vec3& a, const vec3& b, const vec3& c, vec3& out) {
	vec3 pq = q - p;
	vec3 pa = a - p;
	vec3 pb = b - p;
	vec3 pc = c - p;
	
	float u,v,w;
	vec3 m = pq.cross(p);
	u = pq.dot(c.cross(b)) + m.dot(c - b);
	v = pq.dot(a.cross(c)) + m.dot(a - c);
	w = pq.dot(b.cross(a)) + m.dot(b - a);
	if(u+v+w==0) return 0; //parallel to plane
	if((u<0&&v>0) || (v<0&&u>0) || (u<0&&w>0) || (w<0&&u>0) || (v<0&&w>0) || (w<0&&v>0)) return 0;
	
	// Compute the barycentric coordinates (u, v, w) determining the
	// intersection point r, r = u*a + v*b + w*c
	float denom = 1.0f / (u+v+w);
	u *= denom;
	v *= denom;
	w *= denom;
	
	//convert from barycentric coordinates
	out = a*u + b*v + c*w;
	return 1;
}
/** Intersection point between two 2D lines */
int base::intersectLines(const vec2& as, const vec2& ad, const vec2& bs, const vec2& bd, vec2& out) {
	float d = ad.x*bd.y - ad.y*bd.x;
	if(d==0) return 0;
	float t1 = (bd.y*(as.x-bs.x) - bd.x*(as.y-bs.y)) /-d;
	float t2 = (ad.y*(bs.x-as.x) - ad.x*(bs.y-as.y)) / d;
	if(t1>=0 && t1<=1 && t2>=0 && t2<=1) {
		out = as + ad*t1;
		return 1;
	}
	return 0;
}


