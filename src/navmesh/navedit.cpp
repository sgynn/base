#include <base/navmesh.h>
#include <base/collision.h>
#include <base/assert.h>
#include <algorithm>
#include <cstring>
#include <cstdio>

#define EPSILON 1e-6f

// Note: Some nice algotithms at http://paulbourke.net/geometry/polygonmesh


// Debug options
uint debugFlags = 0u;
#ifdef DEBUGGER
#define printf(...) if(debugFlags&0x10) printf(__VA_ARGS__);
#else
#define printf(...)
#endif

using namespace base;

/** These are all the navmesh edit subfunctions needed by NavMesh::carve() */
namespace nav {

	/** Intersection structure and comparitor for carve() */
	struct NavSect { int s; const NavPoly* p[2]; uint e[2]; float u[2]; vec3 point; };
	struct NavSectCmp { int i, d;
		NavSectCmp(int i, bool forward=true): i(i), d(forward?1:-1) {}
		bool operator()(const NavSect& a, const NavSect& b) const { 
			float fa = (a.e[i] + a.u[i]) * d;
			float fb = (b.e[i] + b.u[i]) * d;
			return fa<fb-EPSILON || (fa<fb+EPSILON && a.s*d < b.s*d); // FIXME: probably still wrong when fa==fb
		}
	};


	/** Temporary structure for building polygons */
	struct TempPoly {
		std::vector<vec3>     points;
		std::vector<NavLink*> edges;
	};

	typedef  std::vector<NavPoly*> PList;
	typedef  std::vector<NavSect>  IList;
	enum { INSIDE=1, OUTSIDE, INTERSECTS, TOUCHING };

	int      side      (const vec3& a, const vec3& b, const vec3& p);	// Which side of line ab is point p
	bool     isConvex  (const NavPoly* p);								// Is a polygon convex
	bool     isConvex  (const NavPoly* p, int i);						// Is a point convex
	bool     inverted  (const NavPoly* p);								// Is a polygon inverted (winding)
	int      inside    (const NavPoly* p, const vec2& point);			// Is a point inside a polygon (can be concave)
	int      intersect (const NavPoly* a, const NavPoly* b);			// Do polygons intersect?
	int      insideConvex (const NavPoly* p, const vec2& point);		// Is a point inside a convex polygon
	void     updateCentre (NavPoly* p);									// Calculate polygon centre point

	NavLink* createLink (NavPoly* a, int u, NavPoly* b, int v);					// Link polygons
	void     updateLink (NavLink* link);										// Calculate link additional values
	bool     changeLink (NavLink* l, const NavPoly* from, int f, NavPoly* to, int e);// Change a link target
	void     calculateTraversal(const NavPoly* poly, float limit);

	void     getLinkPoints(NavLink*, vec3& a, vec3& b);
	vec3     getLinkCentre(NavLink*);
	void     getEdgePoints(const NavPoly* p, int edge, vec3& a, vec3& b);
	vec3     getEdgeCentre(const NavPoly* p, int edge);

	NavPoly* create       (const NavPoly* o, int size, const vec3* pt=0);		// Create polygon
	void     removePoint  (NavPoly* p, int i);									// Delete a point from a polygon
	int      cleanPolygon (NavPoly* p);											// Remove unnesesary points
	void     invert       (NavPoly* p);											// Invert polygin winding
	void     deletePoly   (NavPoly*& p);										// delete polygon

	int      merge      (PList& list);											// Merge polygons on list
	NavPoly* merge      (const NavPoly* a, const NavPoly* b, int ea, int eb);	// Merge two polygons along edge e
	NavPoly* joinHoles  (PList& list);											// Holes are seperate inverted polygons inside a polygon. Fix it.

	void     split      (const NavPoly* p, int a, int b, NavPoly** out);		// Split polygon p between vertices ab
	bool     canSplit   (const NavPoly* p, int a, int b);						// Can a polygon be split along ab
	int      makeConvex (const NavPoly* p, PList& out);							// Recursively split polygon p into linked convex polygons
	int      makeConvex (PList& list);											// Make all polygons in list convex
	int      testLine   (const NavPoly* p, const vec3& a, const vec3& b);		// Is a potential edge inside a polygon @deprecated
	int      testSplit(const NavPoly* p, int a, int b);							// Is vector p[a]-p[b] within the polygon
	int      testSplit(const NavPoly* p, int a, const vec3& target);			// Is vector p[a]-target within the polygon
	bool     isVectorInsidePoint(const NavPoly* p, int b, const vec3& target);	// Support function for testSplit - is vector p[b]->target within the point

	int      collect    (const PList& mesh, const NavPoly* brush, PList& out);	// Get all polygons in mesh that intersect brush
	int      expand     (PList& list, int mask);								// Expand selection along links
	int		 getIntersections (const NavPoly* b, const NavPoly* p, IList& out);	// Get all intersections

	int      construct(IList&, PList&, int brushType, int precidence);			// Construct polygons from intersection list
	int      follow(const NavPoly* p, float s, float e, bool f, NavPoly* np, TempPoly& data, bool isBrush);	// Follow polygon edge from s to e, adding vertices to output
	NavPoly* drill(const NavPoly* a, NavPoly* b, bool connect);					// Drill a hole in polygon a with polygon b
	
	inline bool eq(float a, float b, float epsilon=EPSILON) { return fabs(a-b)<epsilon; }
	inline bool eq(const vec3& a, const vec3& b, float epsilon=EPSILON) { return a.distance2(b) < epsilon; }
	inline bool eqSplit(const vec3& a, const vec3& b, float h=EPSILON, float epsilon=EPSILON) { return fabs(a.x-b.x)<epsilon && fabs(a.z-b.z)<epsilon && fabs(a.y-b.y)<h; }
	inline int previousIndex(const NavPoly* p, int i) { return (i-1+p->size) % p->size; }
	inline int nextIndex(const NavPoly* p, int i) { return (i+1) % p->size; }

	bool intersectLines(const vec3& a0, const vec3& a1, const vec3& b1, const vec3& b2, float& u, float& v);

	bool validateLinks(const NavPoly* poly, bool removeInvalid);
	void validateAllLinks(const NavPolyList& mesh);
	bool logPolygon(const NavMesh* mesh, const NavPoly& p, int precidence, bool add, bool clean=false);	// Log polygon to file to load in test debugger

	inline float dot2d(const vec3& a, const vec3& b) { return a.x*b.x + a.z*b.z; }
	inline float dot2d(const vec3& a, const vec2& b) { return a.x*b.x + a.z*b.y; }
	inline float dot2d(const vec2& a, const vec3& b) { return a.x*b.x + a.y*b.z; }
}

extern void validateLinkMemory();

//// //// //// //// //// //// //// //// //// //// //// //// //// //// //// //// //// //// ////


using namespace nav;


/** Which side of line ab is p */
inline int nav::side(const vec3& a, const vec3& b, const vec3& p) {
	vec2 n(b.z-a.z, a.x-b.x);
	float d = dot2d(n, p-a);
	return d<0? -1: d>0? 1: 0;
}


/** Is a polygon convex - note: all crossproducts of adjacent adges will have the same sign  */
bool nav::isConvex(const NavPoly* p) {
	for(int i=p->size-2, j=p->size-1,k=0; k<p->size; i=j, j=k, ++k) {
		if(side(p->points[i], p->points[k], p->points[j])<0) return false;
	}
	return true;
}


/** Is a point convex */
bool nav::isConvex(const NavPoly* p, int e) {
	int a = e? e-1: p->size-1;
	int b = e+1==p->size? 0: e+1;
	return side(p->points[a], p->points[b], p->points[e])>0;
}

/** Is a point inside polygon p. polygon can be concave */
int nav::inside(const NavPoly* a, const vec2& p) {
	vec2 q(p.x+3, p.y+5);
	float u, v;
	int count=0;
	for(int i=a->size-1, j=0; j<a->size; i=j, ++j) {
		v = -1; // in case intersect lines failes to to being parallel
		base::intersectLines(p,q, a->points[i].xz(), a->points[j].xz(), u, v);
		if(v>=0 && v<=1) {
			if(fabs(u)<EPSILON) return 2; // Touching
			if(u>0) ++count;
		}
	}
	return count&1;
}
/** Is a point inside polygon p. polygon must be convex */
int nav::insideConvex(const NavPoly* poly, const vec2& p) {
	int a=1, b=poly->size-1;
	const vec3* vx = poly->points;
	vec2 rel = p - vx[0].xz();
	vec2 n;
	while(a + 1 < b) {
		int c = (a+b)/2;
		n.set(vx[c].z-vx[0].z, vx[0].x-vx[c].x);
		if(n.dot(rel)<0) a=c;
		else b=c;
	}
	// Just test this triangle
	n.set(vx[a].z-vx[b].z, vx[b].x-vx[a].x);
	if(n.dot(p - vx[a]) < 0) return 0;
	if(a==1) {
		n.set(vx[0].z-vx[a].z, vx[a].x-vx[0].x);
		if(n.dot(p - vx[a].xz()) < 0) return 0;
	}
	if(b==poly->size-1) {
		n.set(vx[b].z-vx[0].z, vx[0].x-vx[b].x);
		if(n.dot(p - vx[0].xz()) < 0) return 0;
	}
	return 1;
}


/** Do two polygons intersect - maybe use GJK intersection */
int nav::intersect(const NavPoly* a, const NavPoly* b) {
	static constexpr float low = 1e-4, high=1-1e-4;

	auto checkSpike = [a,b](float t, int i, int j, int u, int v) {
		if(t < low || t > high) {
			int tu, tv, tw;
			if(t > 0.5) { tu=u; tv=v; tw=(v+1)%b->size; }
			else { tu = (u-1+b->size)%b->size; tv=u; tw=v; }
			vec2 n(a->points[j].z - a->points[i].z, a->points[i].x - a->points[j].x);
			if(fsign(dot2d(n, b->points[tu]-b->points[tv]), 1e-3) == fsign(dot2d(n, b->points[tw]-b->points[tv]), 1e-3)) {
				printf("Ignore spike at (%g,%g) on %u\n", b->points[tv].x, b->points[tv].z, a->id);
				return true;
			}
		}
		return false;
	};

	float s,t;
	bool touch = false; // touches via a spike, but not connected
	for(int i=a->size-1,j=0; j<a->size; i=j, ++j) {
		for(int u=b->size-1, v=0; v<b->size; u=v, ++v) {
			bool eiv = eqSplit(a->points[i], b->points[v], 0.1);
			bool eju = eqSplit(a->points[j], b->points[u], 0.1);
			if(eiv && eju) return 4; // Connected
			if(eiv || eju) touch = true;

			if(nav::intersectLines(a->points[i], a->points[j], b->points[u], b->points[v], s, t)) {
				if(s>0 && s<1 && t>0 && t<1) {
					// Check the next point to skip spikes just touching the edge
					if(!checkSpike(t, i,j,u,v)) return 1;
					else touch = true;
				}
				// FIXME: Need to check the next points to see if they actually cross
				if((s<=0 || s>=1) && t>low && t<high) {
					if(!checkSpike(s, i,j,u,v))	return 1;
					else touch = true;
				}
				if((t<=0 || t>=1)  && s>low && s<high) {
					if(!checkSpike(t, i,j,u,v)) return 1;
					else touch = true;
				}
			}
		}
	}
	// Test of one is inside the other
	if(inside(b, (a->points[0] + a->points[1]).xz() * 0.5f)) return 2;
	if(inside(a, (b->points[0] + b->points[1]).xz() * 0.5f)) return 3;
	return touch? 5: 0;
}
/** Polygon centre */
void nav::updateCentre(NavPoly* p) {
	// mean centre
	//vec2 c;
	//for(int i=0; i<p->size; ++i) c += p->points[i];
	//return c / p->size;
	
	/* // median centre
	vec2 low, high;
	low = high = p->points[0];
	for(int i=1; i<p->size; ++i) {
		vec2& c = p->points[i];
		if(c.x<low.x) low.x = c.x;
		if(c.x>high.x) high.x = c.x;
		if(c.y<low.y) low.y = c.y;
		if(c.y>high.y) high.y = c.y;
	}
	return (low+high) * 0.5;
	*/
	
	// Centroid from triangle decomposition
	vec3 c(0,0,0);
	float det=0, temp=0;
	for(int i=p->size-1, j=0; j<p->size; i=j,++j) {
		temp = p->points[i].x * p->points[j].z - p->points[i].z * p->points[j].x;
		temp = fabs(temp);
		c += (p->points[i] + p->points[j]) * temp;
		det += temp;
	}
	p->centre = c / (det*2);

	// Update extents
	p->extents.set(0,0,0);
	for(int i=0; i<p->size; ++i) {
		vec3 e = p->points[i] - p->centre;
		p->extents.x = fmax(p->extents.x, fabs(e.x));
		p->extents.y = fmax(p->extents.y, fabs(e.y));
		p->extents.z = fmax(p->extents.z, fabs(e.z));
	}
}


inline bool nav::intersectLines(const vec3& a0, const vec3& a1, const vec3& b0, const vec3& b1, float& u, float& v) {
	if((a0==b0&&a1==b1) || (a0==b1&&a1==b0)) return false; // Coincident
	if(a0==b0) return u=v=0, true;
	if(a1==b1) return u=v=1, true;
	if(a0==b1) return u=0,v=1, true;
	if(a1==b0) return u=1,v=0, true;
	//return base::intersectLines(a0, a1, b0, b1, u, v);
	{
		float epsilon = 1e-4;
		vec2 ad(a1.x-a0.x, a1.z-a0.z); // a1-a0;
		vec2 bd(b1.x-b0.x, b1.z-b0.z); // b1-b0
		float d = ad.x*bd.y - ad.y*bd.x;
		if(fabs(d)<1e-4) return 0;
		u = (bd.y*(a0.x-b0.x) - bd.x*(a0.z-b0.z)) /-d;
		v = (ad.y*(b0.x-a0.x) - ad.x*(b0.z-a0.z)) / d;
		return u>=-epsilon && u<=1+epsilon && v>=-epsilon && v<=1+epsilon ? 1: 0;
	}
}



//// //// //// //// //// //// //// //// //// //// //// //// //// //// //// //// //// //// ////


NavLink* nav::createLink(NavPoly* a, int u, NavPoly* b, int v) {
	NavLink* link = new NavLink;
	link->poly[0] = a; link->edge[0] = u;
	link->poly[1] = b; link->edge[1] = v;
	if(a) a->links[u] = link;
	if(b) b->links[v] = link;
	return link;
}


/** Calculate link values */
void nav::updateLink(NavLink* l) {
	if(!l) return;
	if(!l->poly[0] || !l->poly[1]) return; // Hanging link
	l->distance = l->poly[0]->centre.distance(l->poly[1]->centre);
	vec2 a = l->poly[0]->points[ l->edge[0] ].xz();
	vec2 b = l->poly[0]->points[ (l->edge[0]+1)%l->poly[0]->size ].xz();
	l->width = a.distance(b);
}

// Generates 2D lines through the polygon that agents with a radius above threshold can't cross
void nav::calculateTraversal(const NavPoly* poly, float maxRadius) {
	if(!poly) return;
	poly->traversal.clear();
	maxRadius *= maxRadius;
	auto processPoly = [](const NavPoly* poly, const NavPoly* look, uint64 skipMask, const vec2& from, float limit, const auto& recurse)->bool {
		for(NavMesh::EdgeInfo& e: look) {
			if(skipMask & (1<<e.a)) continue;
			vec2 ea = e.pointA().xz();
			vec2 ed = e.pointB().xz() - ea;
			float t = ed.dot(from - ea) / ed.dot(ed);
			if(t>0 && t<1) {
				vec2 p = ea + ed * t;
				float dist = p.distance2(from);
				if(dist < limit) {
					if(e.link()) recurse(poly, e.connected(), 1ull<<e.oppositeEdge(), from, limit, recurse);
					else {
						vec2 normal(p.y-from.y, from.x-p.x);
						poly->traversal.push_back({sqrt(dist), normal, normal.dot(from)});
					}
				}
			}
		}
		return false;
	};
	// FIXME- this only adds them to this polygon if they cross into another
	
	for(NavMesh::EdgeInfo& edge: poly) {
		if(!edge.link()) continue;
		const vec2 a = edge.poly.points[edge.a].xz();
		const vec2 b = edge.poly.points[edge.b].xz();
		float width2 = a.distance2(b);

		// Closest edge to edge.a
		for(NavMesh::EdgeInfo& e: poly) {
			uint64 skip = 1ull<<e.a | 1ull<<((e.a - 1 + poly->size) % poly->size);
			processPoly(poly, poly, skip, a, width2, processPoly);
		}
		
		// closest edge to edge.b
		for(NavMesh::EdgeInfo& e: poly) {
			uint64 skip = 1ull<<e.a | 1ull<<e.b;
			processPoly(poly, poly, skip, b, width2, processPoly);
		}
	}
	poly->traversalCalculated = true;

}

/** Change a link target */
bool nav::changeLink(NavLink* link, const NavPoly* from, int fe, NavPoly* to, int e) {
	if(!link) return true;
	int k = 2;
	if(link->poly[0]==from && (fe<0 || link->edge[0]==fe)) k = 0;
	if(link->poly[1]==from && (fe<0 || link->edge[1]==fe)) k = 1;
	assert(k<2);
	if(k<2) {
		if(to) {
			if(to->links[e]!=link) delete to->links[e];
			to->links[e] = link;
		}
		if(link->poly[k]) link->poly[k]->links[ link->edge[k] ] = 0;
		link->poly[k] = to;
		link->edge[k] = e;

		// Fix crashing on invalid links
		bool same = link->poly[0]==link->poly[1] && link->edge[0]==link->edge[1];
		if(same) link->poly[k^1] = to, link->edge[k^1] = e;
	}
	// Validate
	assert(!link->poly[0] || link->poly[0]->links[link->edge[0]]==link);
	assert(!link->poly[1] || link->poly[1]->links[link->edge[1]]==link);
//	assert(l->poly[0]!=l->poly[1] || l->edge[0] != l->edge[1]);
	return k<2;
}

inline void nav::getLinkPoints(NavLink* link, vec3& a, vec3& b) {
	int k = link->poly[0]? 0: 1;
	a = link->poly[k]->points[link->edge[k]];
	b = link->poly[k]->points[(link->edge[k]+1) % link->poly[k]->size];
}

inline vec3 nav::getLinkCentre(NavLink* link) {
	vec3 a, b;
	getLinkPoints(link, a, b);
	return (a+b)/2;
}

inline void nav::getEdgePoints(const NavPoly* poly, int edge, vec3& a, vec3& b) {
	a = poly->points[edge%poly->size];
	b = poly->points[(edge+1)%poly->size];
}

inline vec3 nav::getEdgeCentre(const NavPoly* poly, int edge) {
	vec3 a, b;
	getEdgePoints(poly, edge, a, b);
	return (a+b)/2;
}


//// //// //// //// //// //// //// //// //// //// //// //// //// //// //// //// //// //// ////


/** Create polygon from another */
NavPoly* nav::create(const NavPoly* o, int size, const vec3* pt) {
	NavPoly* p = new NavPoly(0, size, pt);
	p->typeIndex = o->typeIndex;
	p->tag = o->tag;
	// Any other copy stuff here
	return p;
}


/** Delete a point from a polygon */
void nav::removePoint(NavPoly* p, int e) {
	assert(e < p->size);
	if(p->links[e]) delete p->links[e];
	int c = p->size - e - 1;
	if(c>0) {
		for(int i=e; i<e+c; ++i) {
			p->points[i] = p->points[i+1];
			p->links[i] = p->links[i+1];
		}
		// Shift link edge indices
		for(int i=e; i<p->size-1; ++i) {
			if(p->links[i]) {
				if(p->links[i]->poly[0]==p && p->links[i]->edge[0]==i+1) --p->links[i]->edge[0];
				if(p->links[i]->poly[1]==p && p->links[i]->edge[1]==i+1) --p->links[i]->edge[1];
			}
		}
	}
	--p->size;
}
/** Remove unnesesary points from a polygon */
int nav::cleanPolygon(NavPoly* p) {
	// Remove spikes and internal points
	int start = p->size;
	if(p->points[0].distance2(p->points[p->size-1])<EPSILON) removePoint(p, p->size-1);
	for(int i=p->size-1, j=0; j<p->size;) {
		if(j==0) i=p->size-1;
		else if(i>=p->size) break;
		int k = (j+1)%p->size;

		NavLink* la = p->links[i];
		NavLink* lb = p->links[j];
		if(la && lb) {
			// Detect spike or internal points
			if(la == lb) { // Remove spike
				if(!eq(p->points[i], p->points[k])) {
					printf("ERROR: Link corruption\n");
					#ifdef DEBUGGER
					vec3 a = p->points[i], b=p->points[j], c=p->points[k];
					printf(" -> (%g, %g) \n", (a.x+b.x)/2, (a.z+b.z)/2);
					printf(" -> (%g, %g) \n", (c.x+b.x)/2, (c.z+b.z)/2);
					#endif
					break;
				}
				printf("Cleanup: Remove spike %d at (%g, %g)\n", j, p->points[j].x, p->points[j].z);
				printf(" -> (%g, %g)\n", p->points[i].x, p->points[i].z);
				removePoint(p, i<j? j: i);
				removePoint(p, i<j? i: j);
				if(j>p->size) break;
				continue;
			}
			else if(la->poly[0]==la->poly[1] && lb->poly[0]==lb->poly[1]) { // Remove internal point
				// FIXME: get other from link
				int other=-1; for(other=j+1; other<p->size; ++other) if(p->points[j]==p->points[other]) break;
				printf("Cleanup: Remove internal %d, %d\n", j, other);
				p->links[i] = lb; // Swap links around
				p->links[j] = la; 
				removePoint(p, other);
				removePoint(p, j);
				if(j>p->size) break;
				continue;
			}
		}

		// Unlinked spikes
		if(!la && !lb && p->points[i] == p->points[k]) {
			printf("Cleanup: remove spike %d (unlinked)\n", j);
			removePoint(p, j);
			continue;
		}



		// Remove points on a straight line with matching links
		vec2 n(p->points[k].z-p->points[i].z, p->points[i].x-p->points[k].x);
		if(abs(dot2d(n, p->points[j] - p->points[i])) < 1e-4) {
			if(la && lb && ((la->poly[0]==lb->poly[0] && la->poly[1]==lb->poly[1]) || (la->poly[0]==lb->poly[1] && la->poly[1]==lb->poly[0])) ) {
				printf("Remove internal straight (%g, %g)?\n", p->points[j].x, p->points[j].z);
				int o = la->poly[0]==p? 1: 0;
				NavPoly* oPoly = la->poly[o];
				int oEdge = la->edge[o];

				removePoint(p, j);
				removePoint(oPoly, oEdge);
				createLink(p, (j+p->size-1)%p->size, oPoly, (oEdge+oPoly->size-1)%oPoly->size);
				validateLinks(p, false);
				validateLinks(oPoly, false);
				continue;
			}
			else if(!la && !lb) {
				printf("Remove straight %d (%g,%g)\n", j, p->points[j].x, p->points[j].z);
				removePoint(p, j);
				continue;
			}
		}

		// Remove super short edges
		if(p->points[i].distance2(p->points[j]) < EPSILON) {
			printf("Cleanup: Delete near point %d (%g,%g)\n", j, p->points[j].x, p->points[j].z);
			// which one to delete ?
			if(!p->links[j]) removePoint(p, j);
			else removePoint(p, i);
			continue;
		}


		i=j; ++j; // Next point if nothing changed
	}
	if(p->size<start) printf("Cleanup: Deleted %d points (from %d)\n", start-p->size, start);
	return start - p->size;
}

void nav::invert(NavPoly* p) {
	int s = p->size;
	vec3*     pt = p->points;
	NavLink** lk = p->links;
	p->points = new vec3[s];
	p->links  = new NavLink*[s];
	for(int i=0; i<p->size; ++i) {
		p->points[(s-i)%s] = pt[i];
		p->links[s-i-1]    = lk[i];
		if(lk[i]) { // Update link edge
			int k = lk[i]->poly[0]==p? 0: 1;
			lk[i]->edge[k] = s-i-1;
		}
	}
	delete [] pt;
	delete [] lk;
}

bool nav::inverted(const NavPoly* p) {
	// Use shoelace formula for calculating polygon area
	float sum = 0;
	float shift = p->points[0].z * 2; // avoid numbers getting too large, math should be the same
	for(int j=p->size-1, i=0; i<p->size; j=i, ++i) {
		sum += (p->points[i].x - p->points[j].x) * (p->points[i].z + p->points[j].z - shift);
	}
	return sum > 0;
}

void nav::deletePoly(NavPoly*& p) {
	assert(validateLinks(p, true));
	delete p;
	p = 0;
}

//// //// //// //// //// //// //// //// //// //// //// //// //// //// //// //// //// //// ////




/** Merge two polygons */
NavPoly* nav::merge (const NavPoly* a, const NavPoly* b, int sa, int sb) {
	//Build new polygon
	NavPoly* n = create(a, a->size + b->size - 2);
	// Follow a
	for(int i=0; i<a->size-1; ++i) {
		int k = (sa+i+1) % a->size;
		n->points[i] = a->points[k];
		changeLink(a->links[k], a,k, n,i);
	}
	// Follow b
	for(int i=0; i<b->size-1; ++i) {
		int k = (sb+i+1) % b->size;
		int p = i + a->size-1;
		n->points[p] = b->points[k];
		changeLink(b->links[k], b,k, n,p);
	}

	// Remove unnessesary points
	cleanPolygon(n);
	return n;
}
/** Merge a list of polygons */
int nav::merge(PList& list) {
	int count = list.size();
	for(uint i=0; i<list.size(); ++i) {
		if(list[i]) for(uint j=i+1; j<list.size(); ++j) if(list[j]) {
			// Make sure they have the same type
			if(list[i]->typeIndex == list[j]->typeIndex) { // FIXME: May need to stop some merges
				// Get the edge to merge
				const NavLink* link = 0; // The link to merge over
				for(int l=0; l<list[i]->size; ++l) {
					link = list[i]->links[l];
					if(link && (link->poly[0]==list[j] || link->poly[1]==list[j])) break;
					link = 0;
				}

				// Merge polygons
				if(link) {
					int a = link->poly[0]==list[i]? link->edge[0]: link->edge[1];
					int b = link->poly[0]==list[j]? link->edge[0]: link->edge[1];
					NavPoly* merged = merge(list[i], list[j], a, b);
					deletePoly( list[i] );
					deletePoly( list[j] );
					list.push_back( merged );
					--count;
					break;
				}
			}
		}
	}
	// TODO: Rewrite this function to do everything at once.
	// Current implementation is slow as it creates intermediate polygons
	// Also has the issue of holes not being connected from the optimal edge
	//  - Make a list of internal edges to be erased
	//  - Make a bitset of all edges
	//  - Pick an edge, follow it around constructing a polygon, tag done edges
	//  - handle holes somehow
	//  	- create temp poly, if inverted, is a hole and defer if containing poly is not added yet
	
	return count;
}



//// //// //// //// //// //// //// //// //// //// //// //// //// //// //// //// //// //// ////


// @deprecated - use testSplit
int nav::testLine(const NavPoly* p, const vec3& a, const vec3& b) {
	// Check if line ab intersects any edges
	float u,v;
	int r = 0;
	for(int i=p->size-1,j=0; j<p->size; i=j, ++j) {
		if(nav::intersectLines(p->points[i], p->points[j], a,b, u,v)) {
			if(v>EPSILON && v<1-EPSILON) return INTERSECTS; // Intersects
			else if(v<EPSILON && u>1-EPSILON) {	// Check side
				int k = (j+1)%p->size;
				bool c = side(p->points[i], p->points[k], p->points[j]); // concave point
				bool sa = side(p->points[i], p->points[j], b)>0; // outside ij
				bool sb = side(p->points[j], p->points[k], b)>0; // outside jk
				r = (c && sa && sb) || (!c && (sa || sb))? OUTSIDE: INSIDE;
			}
		}
	}
	return r;
}

inline bool nav::isVectorInsidePoint(const NavPoly* p, int b, const vec3& target) {
	int a = (b-1+p->size) % p->size;
	int c = (b+1) % p->size;
	bool convex = side(p->points[a], p->points[c], p->points[b]) > 0;
	bool sa = side(p->points[a], p->points[b], target) < 0; // inside ab
	bool sb = side(p->points[b], p->points[c], target) < 0; // inside bc
	return (convex && sa && sb) || (!convex && (sa || sb));
}
int nav::testSplit(const NavPoly* p, int a, int b) {
	if(!isVectorInsidePoint(p, a, p->points[b])) return OUTSIDE;
	if(!isVectorInsidePoint(p, b, p->points[a])) return OUTSIDE;
	if(isConvex(p)) return INSIDE;
	// Check intersections
	float u, v;
	for(int i=p->size-1,j=0; j<p->size; i=j, ++j) {
		if(i!=a && i!=b && j!=a && j!=b && nav::intersectLines(p->points[i], p->points[j], p->points[a], p->points[b], u,v)) {
			if((u>0 && u<1) && (v>0 && v<1))
				return INTERSECTS;
		}
	}
	return INSIDE;
}
int nav::testSplit(const NavPoly* p, int point, const vec3& target) {
	if(!isVectorInsidePoint(p, point, target)) return OUTSIDE;
	if(isConvex(p)) return INSIDE;
	// Check intersections
	float u, v;
	for(int i=p->size-1,j=0; j<p->size; i=j, ++j) {
		if(i!=point && j!=point && nav::intersectLines(p->points[i], p->points[j], p->points[point], target, u,v)) {
			return INTERSECTS;
		}
	}
	return INSIDE;
}


/** Much faster makeConvex algorithm */
int nav::makeConvex(const NavPoly* p, PList& out) {
	validateLinkMemory();
	
	// Tag concave points
	int concaveCount = 0;
	std::vector<bool> concave(p->size);
	for(int i=0; i<p->size; ++i) {
		concave[i] = !isConvex(p, i);
		if(concave[i]) ++concaveCount;
	}
	assert(concaveCount);

	// create lookup table of same points - assume no 0 length edges
	std::vector<int> same(p->size, -2);
	for(int i=0; i<p->size; ++i) {
		for(int j=i+2; j<p->size; ++j) {
			if(eq(p->points[i], p->points[j])) { same[i]=j; same[j]=i; }
		}
	}
	

	// generate all possible ways to divide the polygon with weight values
	struct Split { int a, b; int f; float d; };
	std::vector<Split> splits;
	auto calculateSplit = [p, &concave](int i, int j) {
		Split split = { i, j, 0, 0 };
		// f is greater if split removes concaveness
		const int s = p->size;
		if(concave[i]) {
			int a = side( p->points[(i+s-1)%s], p->points[i], p->points[j] );
			int b = side( p->points[(i+1)%s],   p->points[i], p->points[j] );
			split.f += a<0 && b>0? 3: 2;
		}
		if(concave[j]) {
			int a = side( p->points[(j+s-1)%s], p->points[j], p->points[i] );
			int b = side( p->points[(j+1)%s],   p->points[j], p->points[i] );
			split.f += a<0 && b>0? 3: 2;
		}
		split.d = p->points[i].distance2(p->points[j]);
		return split;
	};
	for(int i=0; i<p->size; ++i) {
		for(int j=i+2; j<p->size; ++j) {
			if(eq(p->points[i], p->points[j])) {
				if(!concave[i] && !concave[j]) continue; // nope if both convex
				if(!isVectorInsidePoint(p, i, p->points[nextIndex(p, j)])) continue;
				if(!isVectorInsidePoint(p, j, p->points[nextIndex(p, i)])) continue;
				splits.push_back({i, j, 5, 0});
			}
			else if(abs(i-same[j])==1 || abs(same[i]-j)==1 || abs(same[i]-same[j])==1) {
				// split would create 2-point polys
			}
			else if((concave[i] || concave[j]) && testSplit(p, i, j)==INSIDE) {
				splits.push_back(calculateSplit(i, j));
			}
		}
	}
	std::sort(splits.begin(), splits.end(), [](const Split&a, const Split& b) { return a.f>b.f || (a.f==b.f && a.d<b.d); });

	// Invalidate splits 
	float s, t;
	size_t truncate = splits.size();
	for(size_t i=0; i<truncate; ++i) {
		Split& split = splits[i];
		if(!concave[split.a] && !concave[split.b]) { split.f = 0; continue; }

		// Handle zero length splits - these will be first
		if(split.d == 0) {
			for(Split& s: splits) {
				if(s.d > 0 && (s.a==split.b || s.b == split.b)) s.f = 0;
			}
			// Check if this makes them convex
			const vec3& c = p->points[split.a];
			if(side(p->points[previousIndex(p,split.a)], p->points[nextIndex(p, split.b)], c) > 0
				&& side(p->points[previousIndex(p,split.b)], p->points[nextIndex(p, split.a)], c) > 0)
					concave[split.a] = concave[split.b] = false;

			// TODO: below section needs to handle zero length splits
			// at least one side will always be convex by definition
			// need to skip splits on the convex side
			continue;
		}


		// Check intersections with confirmed splits
		for(size_t j=0; j<i; ++j) {
			if(splits[j].f && intersectLines(p->points[split.a], p->points[split.b], p->points[splits[j].a], p->points[splits[j].b], s, t)) {
				if((s>0&&s<1) || (t>0 && t<1)) {
					split.f = 0; // flag invalid
					break;
				}
			}
		}

		if(split.f) {
			// See if this split makes ends convex
			// Need edges to this vertex. They will all be valid
			// Convex if both boundary edge half-spaces contains a split vector
			// TODO: handle case where split connects to a coincident point (same[ix]>=0)
			for(int ix : { split.a, split.b }) {
				assert(ix == split.a || ix == split.b);
				if(!concave[ix]) continue;
				vec3 c = p->points[ix];
				vec3 a = p->points[(ix-1+p->size) % p->size];
				vec3 b = p->points[(ix+1) % p->size];
				a = vec3(a.z-c.z, 0, c.x-a.x); // These are edge normals
				b = vec3(c.z-b.z, 0, b.x-c.x);
				for(size_t j=0; j<=i; ++j) {
					if(splits[j].f && (splits[j].a==ix || splits[j].b==ix)) {
						vec3 d = p->points[splits[j].a==ix? splits[j].b: splits[j].a] - c;
						int good = 0;
						if(d.dot(a)>0) good |= 1;
						if(d.dot(b)>0) good |= 2;
						switch(good) {
						default: break;
						case 1: a.set(d.z, 0, -d.x); break;
						case 2: b.set(-d.z, 0, d.x); break;
						case 3:
							concave[ix] = false;
							if(--concaveCount==0) truncate = i + 1;
							j = i;
							break;
						}
					}
				}
			}
		}
	}

	// Construct new polygons. Note: p is already out[0]
	// Each split connects 2 polygons
	struct SplitLink { NavPoly* poly[2]; int edge[2]; };
	std::vector<SplitLink> splitLinks(truncate, {{0,0}});
	std::vector<int> tmp(16);
	constexpr int splitMask = 0xfffff;
	constexpr int splitA = 1<<20;
	constexpr int splitB = 2<<20;

	for(size_t i=0; i<truncate; ++i) {
		if(splits[i].f==0) continue;
		printf("Split: (%g,%g)(%g,%g)\n", p->points[splits[i].a].x, p->points[splits[i].a].z, p->points[splits[i].b].x, p->points[splits[i].b].z);
		for(int start : {i|splitA, i|splitB}) {
			if(splitLinks[start&splitMask].poly[start>>21]) continue; // Already done
			
			tmp.clear();
			tmp.push_back(start);
			int endIndex = start&splitA? splits[start&splitMask].a: splits[start&splitMask].b;
			while(true) {
				// Get edge point and reverse direction
				int k, e = tmp.back();
				size_t ignoreSplit = -1;
				if(e&splitA) {
					k = splits[e&splitMask].b;
					ignoreSplit = e&splitMask;
				}
				else if(e&splitB) {
					k = splits[e&splitMask].a;
					ignoreSplit = e&splitMask;
				}
				else {
					k = (e + 1) % p->size;
				}
				if(k == endIndex) break;

				// Find next edge, any from splits, or follow polygon
				int next = k, nextIndex = k;
				for(size_t j=0; j<truncate; ++j) {
					const Split& s = splits[j];
					if(j!=ignoreSplit && s.f && (s.a==k || s.b==k)) {
						const int e = s.a==k? s.b: s.a;
						if((nextIndex<endIndex && e>nextIndex && e<=endIndex)
							|| (nextIndex>endIndex && (e>nextIndex || e<=endIndex)))
						{
							next = j | (s.a==k? splitA: splitB);
							nextIndex = e;
						}
					}
				}
				assert(next<splitA || !splitLinks[next&splitMask].poly[next>>21]); // Already used
				if(next>=splitA && splitLinks[next&splitMask].poly[next>>21]) break; // avoid crash in debugger
				assert(next != tmp.back()); // broken
				assert(tmp.size() < (size_t)p->size); // Endless loop
				tmp.push_back(next);
			}

			// Skip zero length splits
			int skip = 0;
			for(int e: tmp) if(e>=splitA && splits[e&splitMask].d==0) ++skip;
			assert(tmp.size() - skip >= 3);
			if(tmp.size()-skip<3) continue;

			// Build this polygon
			NavPoly* np = create(p, tmp.size() - skip);
			int ix = 0;
			for(int e : tmp) {
				if(e < splitA) {
					np->points[ix] = p->points[e];
					if(NavLink* link = p->links[e]) {
						changeLink(link, p, e, np, ix);
					}
					++ix;
				}
				else {
					int side = e >> 21;
					int s = e & splitMask;
					if(splits[s].d > 0) {
						np->points[ix] = p->points[ e&splitA? splits[s].a: splits[s].b ];

						splitLinks[s].poly[side] = np;
						splitLinks[s].edge[side] = ix;
						if(splitLinks[s].poly[side^1]) {
							createLink(np, ix, splitLinks[s].poly[side^1], splitLinks[s].edge[side^1]);
						}
						++ix;
					}
					else splitLinks[s].poly[side] = (NavPoly*)0x1;
				}
			}
			if(!isConvex(np)) {
				printf("ERROR: MakeConvex created a non-convex polygon\n");
				assert(false);
			}
			out.push_back(np);
		}
	}

	return out.size();
}


// Make all convex
int nav::makeConvex(PList& list) {
	uint s = list.size();
	for(uint i=0; i<s; ++i) {
		if(list[i] && !isConvex(list[i])) {
			makeConvex(list[i], list);
			deletePoly( list[i] );
		}
	}
	return list.size() - s;
}


//// //// //// //// //// //// //// //// //// //// //// //// //// //// //// //// //// //// ////



/** Create polygon a-b where a contains b
 * @param a 		Source polygon
 * @param b			Brush polygon
 * @param connect	Create links between polygon b and the new polygon
 * @return			Resulting polygon with hole
 * */
NavPoly* nav::drill(const NavPoly* a, NavPoly* b, bool connect) {
	// Get closest pair of points
	int as=0, bs=0;
	float best=1e8;
	for(int i=0; i<a->size; ++i) {
		for(int j=0; j<b->size; ++j) {
			float d = a->points[i].distance2(b->points[j]);
			if(d<best && testLine(a, a->points[i], b->points[j])==INSIDE
			          && testLine(b, b->points[j], a->points[i])==OUTSIDE ) {
				best = d;
				as = i;
				bs = j;
			}
		}
	}

	// Build polygon
	NavPoly* n = create(a, a->size+b->size+2);
	// Follow polygon a
	for(int i=0; i<=a->size; ++i) {
		int k = (as + i) % a->size;
		n->points[i] = a->points[k];
		changeLink(a->links[k], a,k, n,i);
	}
	int p = a->size+1;
	// Follow polygon b
	for(int i=0; i<=b->size; ++i) {
		int k = (bs - i + b->size) % b->size;
		n->points[i+p] = b->points[k];
		n->links[i+p] = 0;
		if(connect && i<b->size) {
			int bi = (k+b->size-1)%b->size;
			delete b->links[bi];
			createLink(n, i+p, b, bi); // Link brush
		}
	}
	// Create internal link
	createLink(n, a->size,  n, n->size-1);
	cleanPolygon(n);
	return n;
}


/** Construct polygons from intersection data
 *	@param list		List of all intersections between brush and mesh polygons
 *	@param out		List to add created polygons to
 *	@return			Number of polygons created
 * */
int nav::construct(IList& list, PList& out, int brushType, int precidence) {
	// Construct polygons from intersections - intersections sorted by positon along brush
	// Note: intersection index0 = brush, index1 = polygon
	// ToDo - use unsorted list and two sorted lookup maps for brush and polygon.
	PList holes;
	enum FollowMode { FORWARD_POLYGON, REVERSE_POLYGON, FORWARD_BRUSH, REVERSE_BRUSH, INVALID };
	struct Connect { NavPoly* p[2]; int index[2]; };
	std::vector<Connect> linkStart(list.size(), Connect{});

	// Loop until we have used all intersections
	uint count=0, ci=0, ni=0;
	while(true) {
		// Pick an unprocessed intersection
		for(ci=0; ci<list.size(); ++ci) {
			if(~list[ci].s&0xc) break;
		}
		if(ci==list.size()) break; // No intersections left

		uint start = ci;
		int side = list[ci].s&4? 0: 1;
		int  type  = list[ci].p[side]->typeIndex;
		TempPoly data;
		NavPoly* newPoly = create(list[ci].p[1], 0);
		printf("\nConstructing polygon %s\n", NavMesh::getTypeName(type));

		// Follow edges to build polygon
		int loop = 32;
		do {
			if(--loop==0) return -1;
			NavSect& cur = list[ci];

			// Add intersection point to polygon
			if(data.points.empty() || cur.point.distance2(data.points.back())>EPSILON) {
				data.points.push_back( cur.point );
				data.edges.push_back(0);
			}

			// Mark intersection as used
			if(cur.p[1]->typeIndex == type) cur.s |= 4;
			if(cur.p[0]->typeIndex == type || cur.p[0]->typeIndex<0) cur.s |= 8;

			// Determine follow action
			FollowMode op = INVALID; 
			if(side == 0 && cur.p[1]->typeIndex != type) {
				assert(type == brushType);
				op = FORWARD_BRUSH;
			}
			else if(type==brushType) {
				if(cur.p[1]->typeIndex == type) op = cur.s&1? FORWARD_POLYGON: FORWARD_BRUSH;
				else op = FORWARD_BRUSH;
			}
			else {
				op = cur.s&1? FORWARD_POLYGON: REVERSE_BRUSH;
			}

			// Precidence override
			if(NavMesh::getPrecidence(cur.p[1]->typeIndex) > precidence) {
				printf("*");
				if(op==FORWARD_BRUSH) op = cur.s&1? REVERSE_POLYGON: FORWARD_BRUSH;
				else op = FORWARD_POLYGON;
			}

			printf("From %d: ", ci);
			float ev, sv;
			NavSect* ts;
			switch(op) {
			case FORWARD_POLYGON:
				linkStart[ci].p[1] = newPoly;
				linkStart[ci].index[1] = data.points.size()-1;
				ev = -1; sv = cur.e[1] + cur.u[1];
				for(uint i=0; i<list.size(); ++i) {
					if(i!=ci && list[i].p[1] == cur.p[1] && (i==start || !(list[i].s&4))) {
						float t = list[i].e[1] + list[i].u[1];
						if(ev<0 || (ev<sv && (t<ev || t>sv)) || (ev>sv && t>=sv && t<ev)) { ev = t; ni=i; }
					}
				}
				printf("FP %g-%g\n", sv, ev);
				follow(cur.p[1], sv, ev, true, newPoly, data, false);
				ci = ni;
				break;
			case REVERSE_POLYGON:
				linkStart[ci].p[0] = newPoly;
				linkStart[ci].index[0] = data.points.size()-1;
				ev = -1; sv = cur.e[1] + cur.u[1];
				for(uint i=0; i<list.size(); ++i) {
					if(i!=ci && list[i].p[1] == cur.p[1] && (i==start || !(list[i].s&4))) {
						float t = list[i].e[1] + list[i].u[1];
						if(ev<0 || (ev<sv && t>ev && t<=sv) || (ev>sv && (t>ev || t<sv))) { ev = t; ni=i; }
					}
				}
				printf("RP %g-%g\n", sv, ev);
				follow(cur.p[1], sv, ev, false, newPoly, data, false);
				ci = ni;
				break;
			case FORWARD_BRUSH:
				linkStart[ci].p[0] = newPoly;
				linkStart[ci].index[0] = data.points.size() - 1;
				ci = (ci+1)%list.size();
				ts = &list[ci];
				printf("FB %g-%g\n", cur.e[0]+cur.u[0], ts->e[0]+ts->u[0]);
				follow(cur.p[0], cur.e[0]+cur.u[0], ts->e[0]+ts->u[0], true, newPoly, data, true);
				break;
			case REVERSE_BRUSH:
				linkStart[ci].p[1] = newPoly;
				linkStart[ci].index[1] = data.points.size() - 1;
				ci = ci>0? ci-1: list.size()-1;
				ts = &list[ci];
				printf("RB %g-%g\n", cur.e[0]+cur.u[0], ts->e[0]+ts->u[0]);
				follow(cur.p[0], cur.e[0]+cur.u[0], ts->e[0]+ts->u[0], false, newPoly, data, true);
				break;
			default:
				printf("Polygon error %d\n", op);
				return count;
			}

		} while(ci!=start);
		printf("\n");


		// Finalise polygon
		if(data.points.empty()) {
			newPoly->links = 0;
			delete newPoly;
		}
		else {
			newPoly->size = data.points.size();
			newPoly->points = new vec3[newPoly->size];
			newPoly->links = new NavLink*[newPoly->size];
			newPoly->typeIndex = type;
			memcpy((char*)newPoly->points, &data.points[0], newPoly->size * sizeof(vec3));
			memcpy(newPoly->links, &data.edges[0], newPoly->size * sizeof(NavLink*));

			assert(newPoly->size>2);
			if(newPoly->size>2)  {
				if(inverted(newPoly)) holes.push_back(newPoly);
				else out.push_back(newPoly), ++count;
			}
		}
	}

	// Get all partial links
	vec3 a,b,c,d;
	struct FLink { NavPoly* poly; int edge; NavLink* link; };
	std::vector<FLink> partialLinks;
	uint start = out.size() - count;
	for(uint i=start; i<out.size(); ++i) {
		for(int j=0; j<out[i]->size; ++j) {
			NavLink* link = out[i]->links[j];
			if(link && link->poly[0]!=out[i] && link->poly[1]!=out[i]) {
				partialLinks.push_back(FLink{out[i], j, link});
				out[i]->links[j] = 0;
			}
		}
	}

	// Resolve partial links
	for(uint i=0; i<partialLinks.size(); ++i) {
		if(!partialLinks[i].link) continue;
		getEdgePoints(partialLinks[i].poly, partialLinks[i].edge, a, b);
		for(uint j=i+1; j<partialLinks.size(); ++j) {
			if(partialLinks[i].link == partialLinks[j].link) {
				getEdgePoints(partialLinks[j].poly, partialLinks[j].edge, c, d);
				if(eq(a,d) && eq(b,c)) {
					printf("Connect partial (%g, %g)\n", (a.x+b.x)/2, (a.z+b.z)/2);
					createLink(partialLinks[i].poly, partialLinks[i].edge, partialLinks[j].poly, partialLinks[j].edge);
					partialLinks[i].link = 0;
					partialLinks[j].link = 0;
				}
			}
		}
		if(partialLinks[i].link) {
			printf("Failed to resolve partial (%g, %g)\n", (a.x+b.x)/2, (a.z+b.z)/2);
		}
	}

	// Zip up internal links
	for(const Connect& k: linkStart) {
		if(k.p[0] && k.p[1]) {
			if(k.p[0]->links[k.index[0]]) continue;
			// Folow 0 forwards and the 1 backwards. Either all edges should be linked, or none of them
			printf("Zip (%g, %g) B%d - P%d\n", k.p[0]->points[k.index[0]].x, k.p[0]->points[k.index[0]].z, k.index[0], k.index[1]);
			int a[2] = { k.index[0], k.index[1] };
			while(true) {
				int b[2] = { nextIndex(k.p[0], a[0]), previousIndex(k.p[1], a[1]) };
				if(k.p[0]->points[b[0]] != k.p[1]->points[b[1]]) break;
				createLink(k.p[0], a[0], k.p[1], b[1]);
				a[0] = b[0];
				a[1] = b[1];
			}
		}
	}
	
	// Clean polygons
	if(~debugFlags&8)
	for(uint i=out.size()-count; i<out.size(); ++i) cleanPolygon(out[i]);

	// Fix holes. If there are holes, they will all be in the same polygon ...
	if(!holes.empty()) {
		// Find parent
		for(uint i=out.size()-count; i<out.size(); ++i) {
			if(inside(out[i], holes.front()->points[0].xz())) {
				NavPoly* parent = out[i];
				holes.push_back( parent );
				out[i] = joinHoles(holes);
				holes.clear();
				break;
			}
		}
		if(!holes.empty()) printf("Error: Failed to find hole parent\n");
	}



	// DEBUG - clear any invalid links
	for(uint i=0; i<out.size(); ++i) {
		validateLinks(out[i], true);
	}
	
	validateLinkMemory();


	return count;
}


int nav::follow(const NavPoly* p, float s, float e, bool forward, NavPoly* np, TempPoly& data, bool isBrush) {
	// Trivial case - no points
	if(floor(s)==floor(e) && ((forward && e>=s) || (!forward && e<=s))) return 0;
	int l = p->size;
	if(forward) {	// Follow polygon forward
		int se=ceil(s), ee=floor(e);
		if(se>ee) ee += l;

		// Resolve first edge
		if(!data.edges.empty()) {
			if(eq(data.points.back(), p->points[se%l])) {
				data.edges.pop_back();
				data.points.pop_back();
			}
			else if(!data.edges.back()) {
				NavLink* link = p->links[(int)s];
				data.edges.back() = link; // Unresolved link
				if(link) printf("Partial (%g, %g)\n", getLinkCentre(link).x, getLinkCentre(link).y);
			}
		}

		// Follow edges
		for(int i=se; i<=ee; ++i) {
			NavLink* link = p->links[i%l];
			data.points.push_back( p->points[i%l] );
			data.edges.push_back( link );
			np->links = &data.edges[0];
			if(i!=ee) changeLink(link, p, i%l, np, data.points.size()-1);
		}
		return ee-se+1;
	}
	else {		// Follow polygon backwards
		int se=floor(s), ee=ceil(e);
		if(ee>se) se += l;
		
		// Resolve first edge
		if(!data.edges.empty()) {
			if(eq(data.points.back(), p->points[se%l])) {
				data.edges.pop_back();
				data.points.pop_back();
			}
			else if(!data.edges.back()) {
				NavLink* link = p->links[se%l];
				data.edges.back() = link; // Unresolved link
				if(link) printf("Partial (%g, %g)\n", getLinkCentre(link).x, getLinkCentre(link).y);
			}
		}

		// Follow edges
		for(int i=se; i>=ee; --i) {
			NavLink* link = p->links[(i+l-1)%l];
			data.points.push_back( p->points[i%l] );
			data.edges.push_back( link );
			np->links = &data.edges[0];
			if(i!=ee) {
				if(link) {
					int k = link->poly[0]==p? 1: 0; // FIXME internal links ?
					changeLink(link, link->poly[k], link->edge[k], np, data.points.size()-1);
				}
				else if(!isBrush) {
					createLink(const_cast<NavPoly*>(p), (i+l-1)%l, np, data.points.size()-1);
				}
			}
		}
		return se-ee+1;
	}
}

/** A list of inverted 'hole' polygons and the polygon they are in. Returns single polygon */
NavPoly* nav::joinHoles(PList& holes) {
	int count = holes.size() - 1;
	printf("\nConnect %d holes\n", count);
	int pa=0, pb=0, ea=0, eb=0;
	while(count) {
		float best=1e8f;
		// Loop through all polygon pairs
		for(uint i=0; i<holes.size(); ++i) {
			if(holes[i]) for(uint j=i+1; j<holes.size(); ++j) {
				// Loop through all point pairs
				if(holes[j]) for(int a=0; a<holes[i]->size; ++a) {
					for(int b=0; b<holes[j]->size; ++b) {
						// Is it a the shortest valid join
						const vec3& va = holes[i]->points[a];
						const vec3& vb = holes[j]->points[b];
						float d = va.distance2(vb);
						if(d<best && testLine(holes[i], va, vb)==INSIDE && testLine(holes[j], vb, va)==INSIDE) {
							pa = i; pb = j;
							ea = a; eb = b;
							best = d;
						}
					}
				}
			}
		}
		if(best==1e8f) { printf("Error: Failed to find connections\n"); return 0; }
		// Connect them
		NavPoly* a = holes[pa];
		NavPoly* b = holes[pb];
		NavPoly* n = create(a, a->size + b->size + 2);
		printf("Connect %d, %d\n", a->size, b->size);
		for(int i=0; i<=a->size; ++i) {
			int k = (ea+i) % a->size;
			n->points[i] = a->points[k];
			n->links[i]  = a->links[k];
			changeLink(a->links[k], a,k, n,i);
		}
		int s = a->size+1;
		for(int i=0; i<=b->size; ++i) {
			int k = (eb+i) % b->size;
			n->points[i+s] = b->points[k];
			n->links[i+s] = b->links[k];
			changeLink(b->links[k], b,k, n,i+s);
		}
		createLink(n, a->size, n, n->size-1);

		// Update hole list
		holes.push_back( n );

		validateAllLinks(holes);
		deletePoly( a );
		deletePoly( b );
		--count;
	}
	return holes.back(); // Last one left
}

// Get a value to use for sorting by angle between two direction vectors - Do we need an approximation for speed?
float approximateArc(const vec2& d1, const vec2& d2) {
	float a = d1.dot(d2);
	float b = d1.x*d2.y-d1.y*d2.x;
	float v = atan2(b,a);
	if(v<-1e-4) v += TWOPI;
	return v;
}


int nav::getIntersections(const NavPoly* a, const NavPoly* b, IList& list) {
	// Get all intersections of brush with poly. a is the brush.
	int count = 0;
	vec3 ae, be, bd;
	NavSect sect;
	sect.p[0] = a;
	sect.p[1] = b;
	for(int i=a->size-1,j=0; j<a->size; i=j, ++j) {
		for(int u=b->size-1, v=0; v<b->size; u=v, ++v) {
			// Points the same
			bool iu = eq(a->points[i], b->points[u]);
			bool iv = eq(a->points[i], b->points[v]);
			bool ju = eq(a->points[j], b->points[u]);
			bool jv = eq(a->points[j], b->points[v]);
			if(iu || iv || ju || jv) {
				// Need third point
				int k = (j+1)%a->size;
				int w = (v+1)%b->size;
					
				bool ku = eq(a->points[k], b->points[u]);
				bool iw = eq(a->points[i], b->points[w]);
				bool kw = eq(a->points[k], b->points[w]);

				// Matching edge
				if(jv && (ku != iw || iu != kw)) {
					// Which side?
					vec2 s0 = (a->points[iw||iu? i: k] - a->points[j]).xz(); // matched edge
					vec2 s1 = (a->points[iw||iu? k: i] - a->points[j]).xz(); // unmatched edge a
					vec2 s2 = (b->points[iw||kw? u: w] - a->points[j]).xz(); // unmatched edge b
					float v1 = approximateArc(s0, s1);
					float v2 = approximateArc(s0, s2);
					if(fabs(v1 - v2) > 1e-4) {
						sect.e[0] = j;
						sect.e[1] = v;
						sect.u[0] = 0;
						sect.u[1] = 0;
						sect.point = a->points[j];
						sect.s = (v1 < v2) != (iu!=kw)? 1: 0; // Wrong way round ?
						list.push_back(sect);
						printf("sect touch %d,%d (%g,%g) [%d]\n", j,v, sect.point.x, sect.point.z, sect.s);
					}
				}

				else if(jv && !ku && !iw && !iu && !kw) {
					// Possibly passing through a point - need to check all sides
					vec2 di = (a->points[i] - a->points[j]).xz();
					vec2 dk = (a->points[k] - a->points[j]).xz();
					vec2 du = (b->points[u] - a->points[j]).xz();
					vec2 dw = (b->points[w] - a->points[j]).xz();
					float arck = approximateArc(di, dk);
					float arcu = approximateArc(di, du);
					float arcw = approximateArc(di, dw);

					// Both lines coincident
					if(eq(arck, arcw) && eq(arcu, 0)) continue;
					if(eq(arck, arcu) && eq(arcw, 0)) continue;

					if(arck == arcw) {
						printf("Error: Lines coincident - %d,%d (%g,%g) : %g|%g|%g\n", j, v, a->points[j].x, a->points[j].z, arck, arcu, arcw);
					}
					
					if(fabs(arcw)<1e-4 && fabs(arck - arcu)<1e-4) {
						printf("skip (%g,%g)\n", a->points[j].x, a->points[j].z);
					}
					else if((arck>arcu) != (arck>arcw) || fabs(arck - arcu) < 1e-4 || fabs(arcw)<1e-4 || fabs(arcu)<1e-4) {
						sect.e[0] = j;
						sect.e[1] = v;
						sect.u[0] = 0;
						sect.u[1] = 0;
						sect.point = a->points[j];
						if(fabs(arcu<1e-4)) sect.s = arck < arcw ? 0: 1;
						else sect.s = arck <= arcu + 1e-4? 1: 0; // Wrong way round ?
						list.push_back(sect);
						printf("sect %s %d,%d (%g,%g) [%d]\n", (arck==arcu? "touch": "through"), j,v, sect.point.x, sect.point.z, sect.s);
					}
					else if((arck>arcu && arck>arcw) || (arck<arcu && arck<arcw)) {
						if(isVectorInsidePoint(b, v, a->points[i]) /*&& isVectorInsidePoint(b,v,a->points[k])*/) { // second test unnecessary
							sect.point = a->points[j];
							sect.e[0] = j;
							sect.e[1] = v;
							sect.u[0] = sect.u[1] = 0;
							sect.s = 0;
							list.push_back(sect);
							sect.s = 1;
							sect.u[0] = sect.u[1] = 1e-4;
							list.push_back(sect);
							printf("sect double %d,%d (%g,%g)\n", j,v, sect.point.x, sect.point.z);
						}
					}
				}
			}

			// Intersections
			else if(nav::intersectLines(a->points[i], a->points[j], b->points[u], b->points[v], sect.u[0], sect.u[1])) {
				
				if(sect.u[0]>0.99999) sect.u[0]=1;	// Precision errors
				if(sect.u[1]>0.99999) sect.u[1]=1;
				if(sect.u[0]<0.00001) sect.u[0]=0;
				if(sect.u[1]<0.00001) sect.u[1]=0;

				// Detect coincident lines - for floating point error
				vec3 an(a->points[i].z-a->points[j].z, 0, a->points[j].x-a->points[i].x);
				an.normalise();
				if( fabs(an.dot(b->points[u] - a->points[i])) < 1e-2 && 
					fabs(an.dot(b->points[v] - a->points[i])) < 1e-2) continue;


				bd = b->points[v] - b->points[u];
				sect.point = b->points[u] + bd*sect.u[1];
				sect.e[0] = i;
				sect.e[1] = u;
				
				// Calculate side
				if(sect.u[0]<1) ae = a->points[j];
				else ae = a->points[(j+1)%a->size];
				if(sect.u[1]<1) be = b->points[v];
				else { be = b->points[(v+1)%b->size]; bd = be - b->points[v]; }
				float dot = dot2d(ae - be, vec2(bd.z, -bd.x));
				sect.s = dot <= 0? 1: 0;

				// Normalise because of precision issues
				dot = dot2d((ae - be).setY(0).normalise(), vec2(bd.z, -bd.x).normalise());

				// Get side from the other direction if coincident
				// should only happen if sect.u[0]==1 || sect.u[1]==1
				if(eq(dot,0)) {
					assert((sect.u[0]==1) || (sect.u[1]==1));
					if(sect.u[0]==1 && sect.u[1]==1) printf("Warning: intercestion coincident\n");
					if(sect.u[0] == 1) {
						// bn.dot(ad);
						float dd = dot2d(vec2(bd.z, -bd.x), a->points[j]-a->points[i]);
						sect.s = dd < 0? 1: 0;
					}
					else { // sect.u[1] == 1
						// an.dot(bd)
						float dd = dot2d(an, bd);
						sect.s = dd < 0? 1: 0;
					}
				}

				// Skip coincident-inernal points
				if(sect.u[1]==1 && fabs(an.dot(be-b->points[v])) < 1e-2 && an.dot(b->points[u]-b->points[v]) > 0)
					continue;

				if(sect.u[1]==0 && fabs(an.dot(b->points[u]-b->points[previousIndex(b,u)])) < 1e-2 && an.dot(b->points[v]-b->points[u]) > 0)
					continue;



				printf("sect edge %d,%d (%g,%g) %c\n", i,u,sect.point.x, sect.point.z, sect.s? 'P':'B');

				// FIXME: Floating point error when crossing an internal line (HACKY)
				for(uint k=0; k<list.size(); ++k) {
					if(list[k].e[0] == sect.e[0] && fabs(list[k].u[0]-sect.u[0])<0.0001) sect.u[0] = list[k].u[0];
				}

				// replace existing intersections
				if(sect.u[0]==1) { sect.u[0]=0; sect.e[0]=j; }
				if(sect.u[1]==1) { sect.u[1]=0; sect.e[1]=v; }
				if(sect.u[0]==0 || sect.u[1]==0) {
					bool skip = false;
					for(uint k=0; k<list.size() && !skip; ++k) {
						const NavSect& s = list[k];
						if(sect.u[0]==0 && s.u[0]==0 && sect.e[0]==s.e[0] && sect.p[1]==s.p[1] && sect.s==s.s) skip = true;
						if(sect.u[1]==0 && s.u[1]==0 && sect.e[1]==s.e[1] && sect.p[1]==s.p[1] && sect.s==s.s) skip = true;
					}
					if(skip) continue;
				}

				// Do we need to double up for spikes
				if(sect.u[0]==0 && sect.s==1) {
					vec3 n(bd.z, 0, -bd.x);
					int e = sect.e[0];
					if(n.dot(a->points[previousIndex(a,e)]-be)<0 && n.dot(a->points[nextIndex(a,e)]-be)<0) {
						printf("sect double %d,%d (%g,%g) B\n", i,u,sect.point.x, sect.point.z);
						list.push_back(sect);
						list.back().s = 0;
						list.back().e[0] = previousIndex(a,e);
						list.back().u[0] = 0.9999;
						list.back().u[1] -= 1e-4;
						++count;
					}
				}

				list.push_back(sect);
				++count;
			}
		}
	}
	return list.size(); //count;
}

int nav::collect(const PList& mesh, const NavPoly* brush, PList& list) {
	// Get brush aabb - probably cant rely on its own data
	BoundingBox bounds(brush->points[0]);
	for(int i=1; i<brush->size; ++i) bounds.include(brush->points[i]);
	vec3 c = bounds.centre();
	vec3 e = c - bounds.min + 1e-4;
	PList touching;

	// Collect affected polygons : todo spacial optimisation
	for(NavPoly* poly : mesh) {
		vec3 d = poly->centre - c;
		if(fabs(d.x) > e.x+poly->extents.x) continue;
		if(fabs(d.y) > e.y+poly->extents.y) continue;
		if(fabs(d.z) > e.z+poly->extents.z) continue;

		switch(intersect(poly, brush)) {
		default: assert(false);
		case 0: break; // Not intersecting
		case 1: case 2: case 3: case 4:
			list.push_back(poly);
			break;
		case 5: // Touching - only include if connected to a collected poly
			touching.push_back(poly);
			break;
		}
	}

	// Add touching polys (recursive)
	if(!touching.empty()) {
		auto isConnected = [](const NavPoly* a, const NavPoly* b) {
			if(a->size<b->size) std::swap(a,b);
			for(int i=0; i<a->size; ++i) if(a->links[i] && (a->links[i]->poly[0]==b || a->links[i]->poly[1]==b)) return true;
			return false;
		};
		bool added = true;
		while(added) {
			added = false;
			for(NavPoly*& t: touching) {
				if(t) for(NavPoly* p: list) {
					if(isConnected(t, p)) {
						list.push_back(t);
						added = true;
						t = nullptr;
						break;
					}
				}
			}
		}
	}


	return list.size();
}

int nav::expand(PList& list, int mask) {
	uint k, s = list.size();
	for(uint i=0; i<s; ++i) {
		// Get conencted polygons
		for(int j=0; j<list[i]->size; ++j) {
			NavPoly* p = NavMesh::getLinkedPolygon( list[i], j );
			// add to list if not already there
			if(p && (mask<0 || p->typeIndex==mask)) {
				for(k=0; k<list.size(); ++k) if(list[k]==p) k=list.size()+1;
				if(k==list.size()) list.push_back(p);
			}
		}
	}
	return list.size()-s;
}


//// //// //// ////  Navmesh Edit functions - Entry point   //// //// //// ////

void NavMesh::carve(const NavPoly& sb, bool add) {
	int p = (uint)sb.typeIndex<s_typeOrder.size()? s_typeOrder[sb.typeIndex]: 0;
	carve(sb, p, add);
}

void NavMesh::carve(const NavPoly& sb, int precidence, bool add) {
	if(sb.size < 3) return; // Invalid polygon

	#ifndef DEBUGGER
	logPolygon(this, sb, precidence, add, m_mesh.empty());
	#endif

	printf("-------------------------\nCarve: %s\n", add?"Add":"Remove");
	PList in, out;
	collect(m_mesh, &sb, in);
	printf("Collect %lu\n", in.size());

	// Remove input polygons from world
	for(uint i=0; i<in.size(); ++i) removePolygon(in[i]);

	// Merge input polygons
	// We actually only want to merge edges contained by or intersecting brush
	printf("\nMerge:\n");
	merge( in );

	validateLinkMemory();


	// Get intersections
	IList sect; int pc=0;
	for(uint i=0; i<in.size(); ++i) {
		if(in[i]) { getIntersections(&sb, in[i], sect); ++pc; }
	}
	printf("\n%lu intersections in %d polygons\n", sect.size(), pc);
	std::sort(sect.begin(), sect.end(), NavSectCmp(0));
	
	for(size_t i=0; i<sect.size(); ++i) {
		printf("%d: (%f, %f) %d  - %f, %f\n", (int)i, sect[i].point.x, sect[i].point.z, sect[i].s, sect[i].e[0]+sect[i].u[0], sect[i].e[1]+sect[i].u[1]);
	}


	if(debugFlags&2) {	// MERGE ONLY
		for(uint i=0; i<in.size(); ++i) {
			if(in[i]) out.push_back(in[i]);
		}
		in.clear();
	} else {

	
	if(sect.size()==1) sect.clear();
	if(sect.empty()) {
		// Either drill or place
		if(in.empty()) {
			if(add) out.push_back( create(&sb, sb.size, sb.points) );
		} else {
			printf("\nDrill:\n");
			bool s = inside(in.back(), sb.points[0].xz());
			bool t = add && (sb.typeIndex==in.back()->typeIndex || NavMesh::getPrecidence(in.back()->typeIndex) > precidence);
			if(s && t) out.push_back(in.back());
			else if(s) {
				NavPoly* tmp = create(&sb, sb.size, sb.points);
				out.push_back( drill(in.back(), tmp, add) );
				if(add) out.push_back( tmp );
				else delete tmp;
			} else out.push_back( create(&sb, sb.size, sb.points) );
			in.clear();
		}
	}
	else {
		// Construct output polygons
		#ifdef DEBUGGER
		printf("\nConstruct:\n");
		if(debugFlags&0x10) {
			auto printPoly = [](const NavPoly* p) { for(const vec3& v: p) { printf("(%g,%g)", v.x, v.z); } printf("\n"); };
			for(NavPoly* i: in) if(i) { printf("Poly: "); printPoly(i); }
			printf("Brush: "); printPoly(&sb);
		}
		#endif

		if(construct(sect, out, add? sb.typeIndex: -1, precidence)<0) return;
	}

	// TODO: re-drill any internal polygons with higher precidence
	if(in.size()>1 && !sect.empty()) {
		NavPolyList restore;
		for(size_t i=0; i<in.size(); ++i) {
			if(in[i] && in[i]->typeIndex != sb.typeIndex && NavMesh::getPrecidence(in[i]->typeIndex) > precidence) {
				bool used = false;
				for(size_t j=0; j<sect.size(); ++j) {
					if(sect[j].p[0] == in[i] || sect[j].p[1] == in[i]) { used=true; break; }
				}
				if(!used) {
					restore.push_back(in[i]);
					in[i] = 0;
				}
			}
		}
		if(!restore.empty()) {
			printf("TODO: Redrill %lu polygons\n", restore.size());
			// This works if the polygons to drill are discrete, but what if they are two connected polygons ?
			// can probably drill the first, but the second would need a full carve
			// unless we merged the drill polygons, then moved the links back ??
			// Also if multiple discrete drills we should handle them all together for a better result.

			for(NavPoly* p: restore) {
				size_t target;
				for(target=0; target<out.size(); ++target) if(inside(out[target], p->points[0].xz())) break;
				if(target < out.size()) {
					out.push_back( drill(out[target], p, true) );
					out.push_back(p);
					deletePoly(out[target]);
					out[target] = 0;
				}
				else in.push_back(p);
			}
		}
	}


	}

	for(int i=0; i<sb.size; ++i) assert(!sb.links || sb.links[i]==0);
	validateLinkMemory();
	validateAllLinks(m_mesh);

	// Make convex
	if(~debugFlags&4) {	// DONT SPLIT
		printf("\nMakeConvex\n");
		makeConvex(out);
	}

	printf("\nClean up\n");


	validateLinkMemory();
	validateAllLinks(m_mesh);
	
	// Clean up input polygons
	for(uint i=0; i<in.size(); ++i) {
		if(in[i]) deletePoly(in[i]);
	}

	// Add output polygons to mesh
	for(uint i=0; i<out.size(); ++i) {
		if(out[i]) {
			updateCentre(out[i]);
			addPolygon(out[i]);
		}
	}

	validateLinkMemory();
	validateAllLinks(m_mesh);
}



void NavMesh::changeType(const NavPoly* p, const char* type) {
	int index = getTypeID(type);
	// Merge with adjacent polygons with the same type
	PList list;
	list.push_back(const_cast<NavPoly*>(p));
	for(int i=0; i<p->size; ++i) {
		NavPoly* c = getLinkedPolygon( p, i );
		if(c && c->typeIndex == index) list.push_back(c);
	}
	// Remove from mesh
	for(uint i=0; i<list.size(); ++i) removePolygon(list[i]);
	if(list.size()>1) {
		merge(list);
		makeConvex(list);
	}
	// Add new polygons to mesh
	for(uint i=0; i<list.size(); ++i) {
		if(list[i]) addPolygon(list[i]);
	}
}


/** Put this here as the helper function needed is in NavEdit */
bool NavMesh::isInsidePolygon(const vec3& p, const NavPoly* poly) {
	if(!poly) return false;
	return nav::inside(poly, p.xz());
}

//NavPoly* NavMesh::getPolygon(const vec2& p) const {
//	for(uint i=0; i<m_mesh.size(); ++i) {
//		if(nav::inside(m_mesh[i], p)) return m_mesh[i];
//	}
//	return 0;
//}

bool nav::validateLinks(const NavPoly* p, bool removeInvalid) {
	if(!p) return true;
	bool valid = true;
	for(int i=0; i<p->size; ++i) {
		const NavLink* link = p->links[i];
		if(!link) continue;

		assert(link->poly[0] && link->poly[1]);
		assert(link->poly[0]->links[link->edge[0]]==link);
		assert(link->poly[1]->links[link->edge[1]]==link);

		if(link->poly[0] == p && link->edge[0]==i) continue;
		if(link->poly[1] == p && link->edge[1]==i) continue;
		printf("Validation: Invalid link at (%g,%g) %d\n", getEdgeCentre(p,i).x, getEdgeCentre(p,i).y, p->id);
		if(removeInvalid) p->links[i] = 0;
		assert(false);
		valid = false;
	}
	return valid;
}

void nav::validateAllLinks(const NavPolyList& mesh) {
	#ifdef DEBUGGER
	for(const NavPoly* p: mesh) {
		validateLinks(p, true);
	}
	#endif
}



bool nav::logPolygon(const NavMesh* mesh, const NavPoly& p, int precidence, bool add, bool clean) {
	static std::vector<const NavMesh*> meshes;
	size_t index;
	for(index=0; index<meshes.size(); ++index) if(meshes[index] == mesh) break;
	FILE* fp;
	if(index == meshes.size()) meshes.push_back(mesh);
	if(index == 0) fp = fopen("navmesh-log", clean? "w": "a");
	else {
		char logFile[16];
		sprintf(logFile, "navmesh-log-%c", 'A' + (char)index);
		fp = fopen(logFile, clean? "w": "a");
	}
	if(!fp) return false;
	if(add) fprintf(fp, "ADD %s %d [", NavMesh::getTypeName(p.typeIndex), precidence);
	else fprintf(fp, "DEL %d [", precidence);
	
	fwrite(&p.size, 4, 1, fp);
	fwrite(p.points, p.size, sizeof(vec3), fp);
	fprintf(fp, "]\n");

	//for(int i=0; i<p.size; ++i) fprintf(fp, " %g,%g", p.points[i].x, p.points[i].y);
	//fprintf(fp, "\n");
	fclose(fp);
	return true;
}

