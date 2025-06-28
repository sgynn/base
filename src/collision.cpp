#include "base/collision.h"

using namespace base;

#define EPSILON 0.001
#define clamp(v,min,max)	((v)<min?min:(v)>max?max:v)
/** Closest point on a line to a point */
vec3 base::closestPointOnLine(const vec3& point, const vec3& a, const vec3& b) {
	vec3 d = b-a;
	float t = d.dot(point-a) / d.dot(d);
	if(t<0) t=0;
	if(t>1) t=1;
	return a + d*t;
}

/** Closest point between lines */
float base::closestPointBetweenLines(const vec3& a1, const vec3& b1, const vec3& a2, const vec3& b2, vec3& p1, vec3& p2) {
	float s, t;
	float r = closestPointBetweenLines(a1,b1, a2,b2, s,t);
	p1 = a1 + (b1-a1) * s;
	p2 = a2 + (b2-a2) * t;
	return r;
}
float base::closestPointBetweenLines(const vec3& a1, const vec3& b1, const vec3& a2, const vec3& b2, float& s, float& t) {
	vec3 d1 = b1-a1; //Direction vectors
	vec3 d2 = b2-a2;
	vec3 r = a1-a2;
	float l1 = d1.dot(d1); //Squared length of lines
	float l2 = d2.dot(d2);
	float f = d2.dot(r);
	//Do either lines degenerate into a point?
	if(l1<0.001 && l2<0.001) { //Two points
		s = t = 0;
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
	return (a1+d1*s).distance2( a2+d2*t );
}

/** Intersection of a ray with Plane (page 177) */
int base::intersectRayPlane(const vec3& p, const vec3& d,  const vec3& normal, float offset, vec3& result) {
	float t = 0;
	int r = intersectRayPlane(p,d, normal,offset, t);
	result = p + d * t;
	return r;
}
int base::intersectRayPlane(const vec3& p, const vec3& d,  const vec3& normal, float offset, float& t) {
	float dn = normal.dot(d);
	if(dn==0) return 0;
	t = (offset - normal.dot(p)) / dn;
	return t>=0? 1: 0;
}

/** Intersection of a ray with Sphere (page 178) */
int base::intersectRaySphere(const vec3& p, const vec3& d, const vec3& centre, float radius, vec3& out) {
	float t;
	int r = intersectRaySphere(p,d, centre,radius, t);
	out = p +d*t;
	return r;
}
int base::intersectRaySphere(const vec3& p, const vec3& d, const vec3& centre, float radius, float& t) {
	// Note: d must be unit length
	vec3 m = p - centre;
	float b = m.dot(d);
	float c = m.dot(m) - radius*radius;
	//Exit if ray starts outside sphere and pointed away
	if(c > 0 && b > 0) return 0;
	float discriminant = b*b-c;
	if(discriminant<0) return 0; //missed
	//calculate smallest intersecion points
	t = -b - sqrt(discriminant);
	//if t<0, we started inside sphere
	if(t < 0) t=0;
	return 1;
}

/** Intersection of a ray and capsule (page 197) */
int base::intersectRayCapsule(const vec3& p, const vec3& d, const vec3& a, const vec3&b, float radius, float& t) {
	// Ray Cylinder
	vec3 l = b-a, m=p-a;
	float md = m.dot(l);
	float nd = d.dot(l);
	float dd = l.dot(l);
	// Fully outside cylinder endcaps
	//if(md<0 && md+nd < 0) return intersectRaySphere(p,d,a,radius,t); // Outside a
	//if(md>dd && md+nd > dd) return intersectRaySphere(p,d,b,radius,t); // Outside b
	float nn = d.dot(d);
	float mn = m.dot(d);
	float va = dd * nn - nd * nd;
	float vk = m.dot(m) - radius * radius;
	float vc = dd * vk - md * md;
	// Segment parallel to axis
	if(fabs(va) < EPSILON) {
		if(vc>0) return 0; // Outside cylinder
		if(md < 0) return intersectRaySphere(p,d,a,radius,t);
		else if(md > dd) return intersectRaySphere(p,d,b,radius,t);
		else t = 0;
		return 1;
	}

	float vb = dd * mn - nd * md;
	float discr = vb*vb - va * vc;
	if(discr < 0) return 0; // No real roots - miss
	t = (-vb - sqrt(discr)) / va;
	if(md + t * nd < 0) { // Outside a
		return intersectRaySphere(p,d,a,radius,t);
	}
	if(md + t * nd > dd) { // Outside b
		return intersectRaySphere(p,d,b,radius,t);
	}
	if(t<0) return 0;
	return 1;
}



/** Intersection of a ray with an axis aligned bounding box */
int base::intersectRayAABB(const vec3& p, const vec3& d, const vec3& c, const vec3& h, vec3& out) {
	float t;
	int r = intersectRayAABB(p,d,c,h,t);
	if(r) out = p + d * t;
	return r;
}
int base::intersectRayAABB(const vec3& p, const vec3& d, const vec3& c, const vec3& h, float& t) {
	float tmin=0, tmax=1e30f;
	for(int i=0; i<3; ++i) {
		if(fabs(d[i]) < EPSILON) {
			// Parallel
			if(fabs(p[i] - c[i]) > h[i]) return 0;
		}
		else {
			// Plane intersections
			float ood = 1.0f / d[i];
			float t1 = (c[i] - h[i] - p[i]) * ood;
			float t2 = (c[i] + h[i] - p[i]) * ood;
			if(t1 > t2) ood=t1, t1=t2, t2=ood;	// Swap
			if(t1>tmin) tmin=t1;
			if(t2<tmax) tmax=t2;
			if(tmin > tmax) return 0;
		}
	}
	t = tmin;
	return 1;
}



/** Triangle intersection with line */
int base::intersectLineTriangle(const vec3& p, const vec3& q, const vec3& a, const vec3& b, const vec3& c, vec3& out) {
	float bary[3];
	int r = intersectLineTriangleb(p,q,a,b,c,bary);
	if(r) out = a*bary[0] + b*bary[1] + c*bary[2];
	return r;
}
/** Triangle intersection with line - line distance version */
int base::intersectLineTriangle(const vec3& p, const vec3& q, const vec3& a, const vec3& b, const vec3& c, float& t) {
	float bary[3];
	int r = intersectLineTriangleb(p,q,a,b,c,bary);
	if(r){
		vec3 hit = a*bary[0] + b*bary[1] + c*bary[2];
		vec3 d = q - p;
		t = d.dot(hit - p) / d.dot(d);
	}
	return r;
}

/** Minor optimisation for specific case */
int base::intersectRayTriangle(const vec3& p, const vec3& d, const vec3& a, const vec3& b, const vec3& c, float& t) {
	float u,v,w;
	vec3 m = d.cross(p);
	u = d.dot(c.cross(b)) + m.dot(c - b);
	v = d.dot(a.cross(c)) + m.dot(a - c);
	w = d.dot(b.cross(a)) + m.dot(b - a);
	if(u+v+w==0) return 0; //parallel to plane
	if((u<0&&v>0) || (v<0&&u>0) || (u<0&&w>0) || (w<0&&u>0) || (v<0&&w>0) || (w<0&&v>0)) return 0;

	float denom = 1.0f / (u+v+w);
	vec3 hit = u*denom*a + v*denom*b + w*denom*c;
	t = d.dot(hit-p) / d.dot(d);
	return t >= 0;
}

/** Triangle intersection with line - Barycentric version */
int base::intersectLineTriangleb(const vec3& p, const vec3& q, const vec3& a, const vec3& b, const vec3& c, float* bary) {
	vec3 pq = q - p;
	float u,v,w;
	vec3 m = pq.cross(p);
	u = pq.dot(c.cross(b)) + m.dot(c - b);
	v = pq.dot(a.cross(c)) + m.dot(a - c);
	w = pq.dot(b.cross(a)) + m.dot(b - a);
	if(u+v+w==0) return 0; //parallel to plane
	if((u<0&&v>0) || (v<0&&u>0) || (u<0&&w>0) || (w<0&&u>0) || (v<0&&w>0) || (w<0&&v>0)) return 0;
	
	// Compute the barycentric coordinates (u, v, w) determining the
	float denom = 1.0f / (u+v+w);
	bary[0] = u * denom;
	bary[1] = v * denom;
	bary[2] = w * denom;
	return 1;
}


/** Intersection point between two 2D lines */
int base::intersectLines(const vec2& as, const vec2& ae, const vec2& bs, const vec2& be, vec2& out) {
	float u, v;
	int r = intersectLines(as,ae, bs,be, u,v);
	out = as + (ae-as)*u;
	return r;
}
int base::intersectLines(const vec2& as, const vec2& ae, const vec2& bs, const vec2& be, float& t1, float& t2) {
	vec2 ad = ae-as;
	vec2 bd = be-bs;
	float d = ad.x*bd.y - ad.y*bd.x;
	if(d==0) return 0;
	t1 = (bd.y*(as.x-bs.x) - bd.x*(as.y-bs.y)) /-d;
	t2 = (ad.y*(bs.x-as.x) - ad.x*(bs.y-as.y)) / d;
	return t1>=0 && t1<=1 && t2>=0 && t2<=1 ? 1: 0;
}



// Closest point on a triangle (Page 141)
int base::closestPointOnTriangle(const vec3& p, const vec3& a, const vec3& b, const vec3& c, vec3& out) {
	// Region A
	vec3 ab = b - a;
	vec3 ac = c - a;
	vec3 ap = p - a;
	float d1 = ab.dot(ap);
	float d2 = ac.dot(ap);
	if(d1<=0 && d2<=0) { out= a; return 1; };	// barycentric(1,0,0)

	// Region B
	vec3 bp = p - b;
	float d3 = ab.dot(bp);
	float d4 = ac.dot(bp);
	if(d3>=0 && d4<=d3) { out=b; return 2; };	// barycentric(0,1,0)

	// Edge region AB
	float vc = d1*d4 - d3*d2;
	if(vc <= 0 && d1 >= 0 && d3 <= 0) {
		float v = d1 / (d1-d3);
		out = a + v * ab;			// barycentric(1-v,v,0)
		return 4;
	}

	// Region C
	vec3 cp = p - c;
	float d5 = ab.dot(cp);
	float d6 = ac.dot(cp);
	if(d6 >=0 && d5 <= d6) { out=c; return 3; };	// barycentric(0,0,1)

	// Region AC
	float vb = d5*d2 - d1*d6;
	if(vb <= 0 && d2 >= 0 && d6 <= 0) {
		float w = d2 / (d2 - d6);
		out = a + w * ac;			// barycentric(1-w,0,w);
		return 6;
	}

	// Region BC
	float va = d3*d6 - d5*d4;
	if(va <= 0 && (d4 - d3) >= 0 && (d5 - d6) >= 0) {
		float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
		out = b + w * (c-b);		// barycentric(0,1-w,w)
		return 5;
	}

	// Inside face region
	float denom = 1 / (va + vb + vc);
	float v = vb * denom;
	float w = vc * denom;
	out = a + ab * v + ac * w;		// barycentric(1-v-w, v, w) or (va*denom,v,w)
	return 0;
}

int base::intersectRayOBB(const vec3& p, const vec3& d, const vec3& centre, const vec3& halfSize, const Quaternion& orientation, float& t) {
	Quaternion inverse = orientation.getInverse();
	vec3 lp = inverse * (p - centre);
	vec3 ld = inverse * d;
	return intersectRayAABB(lp, ld, vec3(), halfSize, t);
}

int base::intersectBoxes(const vec3& ac, const vec3& ae, const Quaternion& aq, const vec3& bc, const vec3& be, const Quaternion bq) {
	// Get transform of B relative to A
	Quaternion r = bq * aq.getInverse();
	vec3 t = aq.getInverse() * (bc - ac);
	vec3 mat[3]  = { r.xAxis(), r.yAxis(), r.zAxis() };
	vec3 amat[3] = { fabs(mat[0]), fabs(mat[1]), fabs(mat[2]) };
	float ra, rb;

	// Test axes on A
	for(int i=0; i<3; ++i) {
		ra = ae[i];
		rb = be.x * amat[0][i] + be.y * amat[1][i] + be.z * amat[2][i];
		if(fabs(t[i]) > ra + rb) return 0;
	}

	// Test axes on B
	for(int i=0; i<3; ++i) {
		ra = ae.x * amat[i][0] + ae.y * amat[i][1] + ae.z * amat[i][2];
		rb = be[i];
		if(fabs(t.x * mat[i][0] + t.y * mat[i][1] + t.z * mat[i][2]) > ra + rb) return 0;
	}
	
	// Test axis AX x BX
	ra = ae.y * amat[0][2] + ae.z * amat[0][1];
	rb = be.y * amat[2][0] + be.z * amat[1][0];
	if(fabs(t.z * mat[0][1] - t.y * mat[0][2]) > ra + rb) return 0;

	// Test axis AX x BY
	ra = ae.y * amat[1][2] + ae.z * amat[1][1];
	rb = be.x * amat[2][0] + be.z * amat[0][0];
	if(fabs(t.z * mat[1][1] - t.y * mat[1][2]) > ra + rb) return 0;

	// Test axis AX x BZ
	ra = ae.y * amat[2][2] + ae.z * amat[2][1];
	rb = be.x * amat[1][0] + be.y * amat[0][0];
	if(fabs(t.z * mat[2][1] - t.y * mat[2][2]) > ra + rb) return 0;

	// Test axis AY x BX
	ra = ae.x * amat[0][2] + ae.z * amat[0][0];
	rb = be.y * amat[2][1] + be.z * amat[1][1];
	if(fabs(t.x * mat[0][2] - t.z * mat[0][0]) > ra + rb) return 0;
	
	// Test axis AY x BY
	ra = ae.x * amat[1][2] + ae.z * amat[1][0];
	rb = be.x * amat[2][1] + be.z * amat[0][1];
	if(fabs(t.x * mat[1][2] - t.z * mat[1][0]) > ra + rb) return 0;

	// Test axis AY x BZ
	ra = ae.x * amat[2][2] + ae.z * amat[2][0];
	rb = be.x * amat[1][1] + be.y * amat[0][1];
	if(fabs(t.x * mat[2][2] - t.z * mat[2][0]) > ra + rb) return 0;

	// Test axis AZ x BX
	ra = ae.x * amat[0][1] + ae.y * amat[0][0];
	rb = be.y * amat[2][2] + be.z * amat[1][2];
	if(fabs(t.y * mat[0][0] - t.x * mat[0][1]) > ra + rb) return 0;

	// Test axis AZ x BY
	ra = ae.x * amat[1][1] + ae.y * amat[1][0];
	rb = be.x * amat[2][2] + be.z * amat[0][2];
	if(fabs(t.y * mat[1][0] - t.x * mat[1][1]) > ra + rb) return 0;

	// Test axis AZ x BZ
	ra = ae.x * amat[2][1] + ae.y * amat[2][0];
	rb = be.x * amat[1][2] + be.y * amat[0][2];
	if(fabs(t.y * mat[2][0] - t.x * mat[2][1]) > ra + rb) return 0;

	return 1;
}


