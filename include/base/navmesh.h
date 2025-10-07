#pragma once

#include <base/math.h>
#include <base/colour.h>
#include <base/hashmap.h>
#include <vector>
#include <iosfwd>

namespace base {

struct NavPoly;

/** Link between polygons */
struct NavLink {
	NavLink();
	~NavLink();

	NavPoly* poly[2];
	int      edge[2];
	float    width;
	float    step;
	float    distance;
};

struct NavTraversalThreshold { float threshold; vec2 normal; float d; };

/** Polygon class */
struct NavPoly {
	NavPoly(const char* type, int size, const vec2* p, float y=0);
	NavPoly(const char* type, int size, const vec3* p=0);
	~NavPoly();

	// data
	int         size;			// Number of points
	vec3*       points;			// Point positions
	NavLink**   links;			// Links for each edge
	short       typeIndex;		// Type index value (cached)
	short       tag;			// Additional user data
	vec3        centre;			// polygon centroid point (cached)
	vec3        extents;		// float extents from centre for AABB checks
	uint        id;				// polygon unique id for referencing external data

	mutable std::vector<NavTraversalThreshold> traversal;
	mutable bool traversalCalculated = false;

	static constexpr uint Invalid = ~0u; // Invalid polygon id
};

typedef std::vector<NavPoly*> NavPolyList;

class NavMesh {
	public:
	NavMesh();
	~NavMesh();
	NavMesh(NavMesh&& o);
	NavMesh(const NavMesh& o);
	NavMesh& operator=(NavMesh&&);
	NavMesh& operator=(const NavMesh&);
	bool empty() const { return m_mesh.empty(); }

	static int  setType(const char* name, int precidence=0); 	/// Initialise type with precidence
	static int  getTypeID(const char* type);					/// Get index of named type. Adds it if missing
	static const char* getTypeName(int typeID);					/// Get type name from type index
	static int getPrecidence(int typiID);						/// Get type carving precidence

	// Polygon traversal utilities
	static NavPoly* getLinkedPolygon(const NavPoly*, uint edge);
	static uint     getLinkedEdge(const NavPoly*, uint edge);
	static uint     getLinkedID(const NavPoly*, uint edge);
	static bool     isInsidePolygon(const vec3& p, const NavPoly* poly);
	static void     getEdgePoints(const NavPoly* poly, uint edge, vec3& a, vec3& b);

	// Polygon access
	uint     getPolygonID(const vec3& point) const;
	NavPoly* getPolygon(const vec3& point) const;
	NavPoly* getPolygon(uint id) const;
	short    getPolygonTag(uint id) const;

	uint     getClosestPolygonID(const vec3& point, float max=1e37f, vec3* out=0) const;
	NavPoly* getClosestPolygon(const vec3& point, float max=1e37f, vec3* out=0) const;
	int      getClosestBoundary(const vec3& point, uint polygon, float radius, vec3& out, uint* poly=0, uint* edge=0) const;

	static vec3 getRandomPoint(const NavPoly* poly);
	template<class F> const NavPoly* getRandomPolygon(const F& filter) const;
	template<class F> const vec3 getRandomPoint(const F& filter) const { return getRandomPoint(getRandomPolygon(filter)); }


	// Direct access for drawing
	const NavPolyList& getMeshData() const { return m_mesh; }
	
	// Editing
	void clear();
	void changeType(const NavPoly* p, const char* type);
	void carve(const NavPoly& p, bool add=true);
	void carve(const NavPoly& p, int precidence, bool add=true);

	NavPoly* addPolygon(const NavPoly& p);	/// Add unchecked polygon to the mesh


	void writeToFile(FILE* fp) const;
	void readFromFile(FILE* fp);

	protected:
	static base::HashMap<int> s_types;		// Map of types to typeindex
	static std::vector<int>   s_typeOrder;	// Type precidence values.


	NavPolyList m_mesh;		// The mesh
	uint     m_newId = 0;	// Unique polygon id counter

	void addPolygon(NavPoly* p, uint id=0);	// Add polygon to mesh
	void removePolygon(NavPoly* p);			// Remove polygon from mesh


	public:
	/** Edge Iterator system for ranged iterator. Can iterate edges or points */
	struct EdgeInfo {
		const NavPoly& poly;
		uint a, b;
		const vec3& point() const { return poly.points[a]; }
		const vec3& pointA() const { return poly.points[a]; }
		const vec3& pointB() const { return poly.points[b]; }
		operator const vec3&() const { return point(); }
		NavLink* link() { return poly.links[a]; }
		const NavLink* link() const { return poly.links[a]; }
		bool isConnected() const { return poly.links[a]; }
		NavPoly* connected() { if(NavLink* l=link()) return l->poly[l->poly[0]==&poly?1:0]; return nullptr;  }
		uint oppositeEdge() const { return NavMesh::getLinkedEdge(&poly, a); }
		vec3 direction() const { return poly.points[b] - poly.points[a]; }
		vec3 normal() const { return vec3(poly.points[b].z-poly.points[a].z, 0, poly.points[a].x-poly.points[b].x); }
	};
	class EdgeIterator {
		EdgeInfo data;
		public:
		EdgeIterator(const NavPoly* p, uint i, uint j) : data{*p, i, j} {}
		EdgeInfo& operator*() { return data; }
		EdgeInfo* operator->() { return &data; }
		EdgeIterator& operator++() { data.a=data.b; ++data.b; return *this; }
		bool operator!=(const EdgeIterator& o) const { return data.b != o.data.b; }
	};
};

inline NavMesh::EdgeIterator begin(const NavPoly* p) { return NavMesh::EdgeIterator(p, p->size-1, 0); }
inline NavMesh::EdgeIterator end(const NavPoly* p) { return NavMesh::EdgeIterator(p, p->size, p->size); }

template<class F>
const NavPoly* NavMesh::getRandomPolygon(const F& filter) const {
	float accum = 0;
	const NavPoly* result = nullptr;
	for(const NavPoly* p: m_mesh) {
		float weight = filter(p);
		if(weight <= 0) continue;
		float area = 0;
		for(auto& e: p) area += (e.pointA().x - e.pointB().x) * (e.pointA().z + e.pointB().z);
		area *= weight;
		if(randf() * (accum + area) >= accum) result = p;
		accum += area;
	}
	return result;
}
}

