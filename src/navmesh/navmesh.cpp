#include <base/navmesh.h>
#include <base/opengl.h>
#include <cstdio>
#include <map>

using namespace base;

//// Polygon type functions ////
base::HashMap<int> NavMesh::s_types;
std::vector<int>   NavMesh::s_typeOrder;
int NavMesh::setType(const char* n, int v) {
	if(s_types.contains(n)) {
		int i = s_types[n];
		s_typeOrder[i] = v;
		return i;
	} else {
		int i = s_types.size();
		s_types[n] = i;
		s_typeOrder.push_back(v);
		return i;
	}
}
int NavMesh::getTypeID(const char* n) {
	if(s_types.contains(n)) return s_types[n];
	else return setType(n, 0);
}

const char* NavMesh::getTypeName(int id) {
	for(auto i: s_types) if(i.value == id) return i.key;
	return 0;
}

int NavMesh::getPrecidence(int id) {
	if((uint)id<s_typeOrder.size()) return s_typeOrder[id];
	return 0;
}


// ========== //

NavPoly* NavMesh::getLinkedPolygon(const NavPoly* p, uint e) {
	if(e<(uint)p->size && p->links[e]) return p->links[e]->poly[0]==p? p->links[e]->poly[1]: p->links[e]->poly[0];
	return 0;
}
uint NavMesh::getLinkedID(const NavPoly* p, uint e) {
	NavPoly* l = getLinkedPolygon(p,e);
	return l? l->id: NavPoly::Invalid;
}
uint NavMesh::getLinkedEdge(const NavPoly* p, uint e) {
	if(p->links[e]) return p->links[e]->poly[0]==p? p->links[e]->edge[1]: p->links[e]->edge[0];
	return 0;
}

void NavMesh::getEdgePoints(const NavPoly* p, uint e, vec3& a, vec3& b) {
	if(p && e < (uint)p->size) {
		a = p->points[e];
		b = p->points[ (e+1)%p->size ];
	}
}

// ========== //

uint NavMesh::getPolygonID(const vec3& p) const {
	NavPoly* t = getPolygon(p);
	return t? t->id: NavPoly::Invalid;
}
NavPoly* NavMesh::getPolygon(const vec3& p) const {
	float vertical = 1e8f;
	NavPoly* result = nullptr;
	for(NavPoly* poly : m_mesh) {
		if(isInsidePolygon(p, poly)) {
			if(poly->centre.y == p.y) return poly; // Early out for 2d navmeshes
				
			float d = fabs(poly->centre.y - p.y);
			if(d < vertical) {
				result = poly;
				vertical = d;
			}
		}
	}
	return result;
}
NavPoly* NavMesh::getPolygon(uint id) const {
	for(uint i=0; i<m_mesh.size(); ++i) if(m_mesh[i]->id==id) return m_mesh[i]; // ToDO: Lookup
	return 0;
}
short NavMesh::getPolygonTag(uint id) const {
	NavPoly* t = getPolygon(id);
	return t? t->tag: 0;
}

uint NavMesh::getClosestPolygonID(const vec3& p, float max, vec3* out) const {
	NavPoly* t = getClosestPolygon(p, max, out);
	return t? t->id: NavPoly::Invalid;
}
NavPoly* NavMesh::getClosestPolygon(const vec3& p, float max, vec3* out) const {
	NavPoly* r = nullptr;
	vec3 point = p;
	if(max < 1e37f) max *= max;
	for(NavPoly* poly : m_mesh) {
		if(isInsidePolygon(p, poly)) {
			float d = fabs(p.y - poly->centre.y) - poly->extents.y;
			if(d <= 0) { point = p; r = poly; break; } // Early out if within height range
			d *= d;
			if(d < max) {
				point = p;
				r = poly;
				max = d;
			}
		}
		else {
			// get minimum distance from all edges
			for(int j=poly->size-1, i=0; i<poly->size; j=i, ++i) {
				if(!poly->links[j]) {
					vec3 ij = poly->points[j] - poly->points[i];
					float t = ij.dot(p - poly->points[i]) / ij.dot(ij);
					t = t<0? 0: t>1? 1: t;
					vec3 closestPoint = poly->points[i] + ij * t;
					float d = p.distance2(closestPoint);
					if(d < max) {
						max = d;
						r = poly;
						point = closestPoint;
					}
				}
			}
		}
	}
	if(r && out) *out = point;
	return r;
}


int NavMesh::getClosestBoundary(const vec3& p, uint id, float radius, vec3& out, uint* oid, uint* oedge) const {
	NavPoly* poly = id==NavPoly::Invalid? getPolygon(p): getPolygon(id);
	if(!poly) return 0;

	int result = 0;
	radius *= radius;
	std::vector<NavPoly*> stack;
	stack.push_back(poly);
	for(uint i=0; i<stack.size(); ++i) {
		poly = stack[i];
		for(EdgeInfo& edge : poly) {
			// Closest point
			vec3 dir = edge.direction();
			float t = dir.dot(p - poly->points[edge.a]) / dir.dot(dir);
			t = fclamp(t, 0, 1);
			vec3 closestPoint = edge.point() + dir * t;
			closestPoint.y = p.y;
			float d = p.distance2(closestPoint);
			if(d < radius) {
				if(edge.isConnected()) {
					// Internal edge - add next polygon to stack
					NavPoly* other = edge.connected();
					for(uint k=0; k<stack.size(); ++k) {
						if(stack[k] == other) { other = 0; break; }
					}
					if(other) stack.push_back(other);
				}
				else {
					// Boundary edge - set closest point
					out = edge.point() + dir * t;
					if(oid) *oid = poly->id;
					if(oedge) *oedge = edge.a;
					radius = d;
					result = 1;
				}
			}
		}
	}
	return result;
}



// ==== Link and polygon functions ==== //

#ifdef EMSCRIPTEN
#define sert(x) assert(x)
#else
#define sert(x) if(!(x)) asm("int $3\nnop");
#endif

std::vector<NavLink*> allLinks;
void validateLinkMemory() {
	for(NavLink* l: allLinks) {
		sert(l->poly[0] && l->poly[1]);
		sert(!l->poly[0] || l->poly[0]->links[l->edge[0]] == l);
		sert(!l->poly[1] || l->poly[1]->links[l->edge[1]] == l);
	}
}


NavLink::NavLink() : poly{0,0}, edge{0,0}, width(0), step(0), distance(0) {
	allLinks.push_back(this);
}
NavLink::~NavLink() {
	for(size_t i=0; i<allLinks.size(); ++i) if(allLinks[i]==this) { allLinks[i]=allLinks.back(); allLinks.pop_back(); break; }
	sert(!poly[0] || poly[0]->links[edge[0]] == this);
	sert(!poly[1] || poly[1]->links[edge[1]] == this);
	if(poly[0]) poly[0]->links[ edge[0] ] = 0;
	if(poly[1]) poly[1]->links[ edge[1] ] = 0;
}

NavPoly::NavPoly(const char* type, int size, const vec3* p) : size(size), tag(0), id(0) {
	typeIndex = type? NavMesh::getTypeID(type): 0;
	points = new vec3[size];
	links =  new NavLink*[size];
	memset(links, 0, size*sizeof(NavLink*));
	if(p) memcpy((char*)points, p, size*sizeof(vec3));
}

NavPoly::NavPoly(const char* type, int size, const vec2* p, float y) : NavPoly(type, size) {
	for(int i=0; i<size; ++i) points[i] = p[i].xzy(y);
}

NavPoly::~NavPoly() {
	for(int i=0; i<size; ++i) delete links[i];
	delete [] points;
	delete [] links;
}



// ==== NavMesh functions ==== //



NavMesh::NavMesh(): m_newId(0) {
}
NavMesh::~NavMesh() {
	clear();
}
void NavMesh::clear() {
	for(NavPoly* p: m_mesh) delete p;
	m_mesh.clear();
	m_newId = 0;
}

NavPoly* NavMesh::addPolygon(const NavPoly& p) {
	NavPoly* poly = new NavPoly(0, p.size, p.points);
	poly->typeIndex = p.typeIndex;
	poly->tag = p.tag;
	poly->centre = p.centre;
	poly->extents = p.extents;
	addPolygon(poly);
	return poly;
}

void NavMesh::addPolygon(NavPoly* p, uint id) {
	if(id==0) id = ++m_newId;
	else if(id>=m_newId) m_newId=id+1;
	p->id = id;
	m_mesh.push_back(p);
}
void NavMesh::removePolygon(NavPoly* p) {
	// Need to find it - if mesh was sorted by id, it would be faster
	for(uint i=0; i<m_mesh.size(); ++i) {
		if(m_mesh[i] == p) {
			m_mesh.erase(m_mesh.begin()+i);
			break;
		}
	}
}

// =========================================================== //


NavMesh::NavMesh(NavMesh&& o) : m_mesh(std::move(o.m_mesh)), m_newId(o.m_newId) { }
NavMesh::NavMesh(const NavMesh& src) { *this = src; }
NavMesh& NavMesh::operator=(NavMesh&& src) {
	clear();
	m_mesh = std::move(src.m_mesh);
	m_newId = src.m_newId;
	return  *this;
}
NavMesh& NavMesh::operator=(const NavMesh& src) {
	clear();

	// Copy polygons
	std::map<const NavPoly*, NavPoly*> lookup;
	for(const NavPoly* p: src.m_mesh) {
		NavPoly* np = addPolygon(*p);
		np->id = p->id;
		lookup[p] = np;
	}

	// Link everything
	for(auto& pair : lookup) {
		for(int i=0; i<pair.first->size; ++i) {
			if(const NavLink* link = pair.first->links[i]) {
				if(!pair.second->links[i]) {
					auto p0 = lookup.find(link->poly[0]);
					auto p1 = lookup.find(link->poly[1]);
					assert(p0 != lookup.end() && p1 != lookup.end());
					NavLink* newLink = new NavLink(*link);
					newLink->poly[0] = p0->second;
					newLink->poly[1] = p1->second;
					p0->second->links[newLink->edge[0]] = newLink;
					p1->second->links[newLink->edge[1]] = newLink;
					allLinks.push_back(newLink); // because we are using copy constructor
				}
			}
		}
	}
	validateLinkMemory();
	m_newId = src.m_newId;
	return *this;
}



// =========================================================== //

void NavMesh::writeToFile(FILE* fp) const {
	uint size = m_mesh.size();
	uint linkCount = 0;
	fwrite(&size, 4, 1, fp);
	for(NavPoly* p: m_mesh) {
		assert(p->size<256);
		fputc(p->size, fp);
		fwrite(p->points, sizeof(vec3), p->size, fp);
		fwrite(&p->typeIndex, 1, 24, fp);
		for(int i=0; i<p->size; ++i) if(p->links[i] && p->links[i]->poly[0]==p) ++linkCount;
	}
	fwrite(&linkCount, 4, 1, fp);
	for(NavPoly* p: m_mesh) {
		for(int i=0; i<p->size; ++i) if(p->links[i] && p->links[i]->poly[0]==p) {
			NavLink* link = p->links[i];
			fwrite(&link->poly[0]->id, 4, 1, fp);
			fwrite(&link->poly[1]->id, 4, 1, fp);
			fwrite(&link->edge[0], 4, 5, fp);
		}
	}
}

void NavMesh::readFromFile(FILE* fp) {
	std::map<uint, NavPoly*> idMap;
	uint count = 0;
	fread(&count, 4, 1, fp);
	while(count--) {
		NavPoly* p = new NavPoly(0, fgetc(fp));
		fread(p->points, sizeof(vec3), p->size, fp);
		fread(&p->typeIndex, 1, 24, fp);
		addPolygon(p, p->id);
		idMap[p->id] = p;
	}
	uint id[2];
	fread(&count, 4, 1, fp);
	while(count--) {
		fread(id, 4, 2, fp);
		NavLink* link = new NavLink;
		link->poly[0] = idMap[id[0]];
		link->poly[1] = idMap[id[1]];
		fread(&link->edge[0], 4, 5, fp);

		link->poly[0]->links[link->edge[0]] = link;
		link->poly[1]->links[link->edge[1]] = link;
	}
}



