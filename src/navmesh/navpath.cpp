#include <base/navpath.h>
#include <base/navmesh.h>
#include <base/collision.h>
#include <map>
#include <algorithm>
#include <cstdio>

using namespace base;

#ifdef DEBUGGER
extern void addDebugLine(const vec3& a, const vec3& b, int colour);
#define DebugLine(a,b,c) addDebugLine(a,b,c);
#else
#define DebugLine(a,b,c)
#endif


NavFilter::NavFilter(uint64 m) : m_mask(m) {}
void NavFilter::addType(const char* name) {
	m_mask |= 1<<NavMesh::getTypeID(name);
}
void NavFilter::removeType(const char* name) {
	m_mask &= ~(1<<NavMesh::getTypeID(name));
}
bool NavFilter::hasType(const char* name) const {
	int type = NavMesh::getTypeID(name);
	return hasType(1<<type);
}

const NavFilter NavFilter::ALL = NavFilter(~0u);
const NavFilter NavFilter::NONE = NavFilter();

// ======================================================================= //

Pathfinder::Pathfinder(const NavMesh* nav) : m_navmesh(nav), m_state(PathState::None) {
	m_filter = NavFilter::ALL;
}

void Pathfinder::setNavMesh(const NavMesh* nav) {
	m_navmesh = nav;
}

void Pathfinder::setFilter(const NavFilter& f) {
	m_filter = f;
	clear();
}

void Pathfinder::clear() {
	m_path.clear();
	m_state = PathState::None;
}

PathState Pathfinder::search(const vec3& a, const vec3& b) {
	uint startPoly = m_navmesh->getPolygon(a)->id;
	uint endPoly   = m_navmesh->getPolygon(b)->id;
	return search( a, startPoly, b, endPoly);
}

PathState Pathfinder::search(uint a, uint b) {
	vec3 start, end;
	NavPoly* p;
	p = m_navmesh->getPolygon(a);
	if(!p) return PathState::Invalid;

	for(int i=0; i<p->size; ++i) start += p->points[i];
	start /= p->size;

	p = m_navmesh->getPolygon(b);
	if(!p) return PathState::Invalid;

	for(int i=0; i<p->size; ++i) end += p->points[i];
	end /= p->size;

	return search(start, a, end, b);
}


PathState Pathfinder::search(const vec3& a, uint pa, const vec3& b, uint pb) {
	clear();
	if(pa==NavPoly::Invalid || pb==NavPoly::Invalid) return PathState::Invalid;
	if(pa==pb) return (m_state=PathState::Success);

	const NavPoly* poly = m_navmesh->getPolygon(a);
	if(!poly) {
		m_state = PathState::Fail; // Invalid staring polygon
		return m_state;
	}

	struct AStarNode { const NavLink* link; int k; vec3 p; float value; AStarNode* parent; };
	auto cmp = [](const AStarNode* a, const AStarNode* b) { return a->value > b->value; };


	std::vector<AStarNode*> open, all;
	AStarNode startNode { nullptr, 0, a, 0.f, nullptr };
	AStarNode* node = &startNode;

	std::map<const NavLink*, AStarNode*> states;
	std::map<const NavLink*, AStarNode*>::iterator state;

	do {
		int added = 0;
		for(int i=0; i<poly->size; ++i) {
			NavLink* link = poly->links[i];
			if(link && m_filter.hasType(poly->typeIndex)) {	// ToDo: Clearance
				state = states.find(link);
				if(state != states.end() && state->second==0) continue; // closed

				// Invalid
				if(!link->poly[0] || !link->poly[1]) {
					printf("Path Error: Messed up link detected\n");
					continue;
				}


				// Heuristic
				vec3& u = link->poly[0]->points[ link->edge[0] ];
				vec3& v = link->poly[1]->points[ link->edge[1] ];
				vec3 p = (u + v) * 0.5;
				float h = node->value + p.distance(node->p) + p.distance(b);

				// Create / update node
				AStarNode* n = 0;
				if(state==states.end()) all.push_back((n = new AStarNode));
				else if(state->second->value<h) continue;
				else n = state->second;
				n->parent = node;
				n->link = link;
				n->k = link->poly[0]==poly? 1: 0;
				n->value = h;

				// Add to open list
				if(state==states.end()) {
					states[link] = n;
					open.push_back( n );
				}
				++added;
			}
		}
		if(open.empty()) break;
		if(added) std::sort(open.begin(), open.end(), cmp);
		node = open.back();
		open.pop_back();
		states[ node->link ] = 0; // closed
		poly = node->link->poly[ node->k ];
	} while(poly->id != pb);
	
	// Build path
	Node pathNode;;
	if(poly->id == pb) {
		while(node->link) {
			pathNode.poly = node->link->poly[ node->k^1 ]->id;
			pathNode.edge = node->link->edge[ node->k^1 ];
			m_path.push_back( pathNode );
			node = node->parent;
		}
		std::reverse(m_path.begin(), m_path.end());
		m_state = PathState::Success;
	} else m_state = PathState::Fail;

	// cleanup
	for(uint i=0; i<all.size(); ++i) delete all[i];
	return m_state;
}



// ============================================================================================= //


bool Pathfinder::ray(const vec3& start, const vec3& end, uint polyID, const NavFilter& f) const {
	const NavPoly* p = polyID==NavPoly::Invalid? m_navmesh->getPolygon(start): m_navmesh->getPolygon(polyID);
	if(!NavMesh::isInsidePolygon(start, p)) p = m_navmesh->getPolygon(start); // Validate
	if(!p) return false;
	vec3 d = end - start;
	vec3 n(-d.z, 0, d.x);
	int last = -1;
	while(true) {
		if(NavMesh::isInsidePolygon(end, p)) return true;
		for(int i=p->size-1, j=0; j<p->size; i=j, ++j) {
			if(i!=last) {
				if(n.dot(p->points[i]-start)<=0 && n.dot(p->points[j]-start)>=0) {
					if(!p->links[i]) return false;
					last = NavMesh::getLinkedEdge( p, i );
					p = NavMesh::getLinkedPolygon( p, i );
					if(!f.hasType(p->typeIndex)) return false;
					break;
				}
			}
		}
	}
}

// ============================================================================================= //

bool Pathfinder::resolvePoint(vec3& pt, float r, float search, int iter) const {
	NavPoly* poly = m_navmesh->getPolygon(pt);
	if(!poly) {
		vec3 close;
		poly = m_navmesh->getClosestPolygon(pt, search, &close);
		if(!poly) return false;
		pt = close;
	}

	if(r==0) return m_navmesh->isInsidePolygon(pt, poly);

	vec3 close;
	uint pix, eix;
	for(int i=0; i<iter; ++i) {
		if(m_navmesh->getClosestBoundary(pt, poly->id, r, close, &pix, &eix)) {
			close.y = pt.y;
			vec3 resolve = pt - close;
			NavPoly* p = m_navmesh->getPolygon(pix);
			vec3 d = p->points[(eix+1)%p->size] - p->points[eix];
			// Use edge normal if no distance
			if(resolve.length2() < 1e-3) resolve.set(-d.z, 0, d.x);
			resolve.normalise();
			float penetration = r - pt.distance(close);
			if(resolve.dot(vec3(-d.z,0,d.x))<0) penetration -= r*2; // Wrong side?
			DebugLine(pt, pt + resolve*penetration, 0xffffff);
			pt += resolve * penetration;
		}
		else return true;
	}

	return m_navmesh->getPolygon(pt);
}


// =====================================================================================i================ //

PathFollower::PathFollower(const NavMesh* nav) : m_path(nav), m_pathIndex(0), m_polygon(NavPoly::Invalid), m_goalPoly(NavPoly::Invalid), m_radius(0) {
}

void PathFollower::setNavMesh(const NavMesh* nav) {
	m_path.setNavMesh(nav);
}

void PathFollower::setRadius(float r) {
	m_radius = r;
}

void PathFollower::setPosition(const vec3& p) {
	m_position = p;
	NavPoly* poly = m_path.getNavMesh()->getPolygon(m_polygon);
	if(poly && NavMesh::isInsidePolygon(p, poly)) return; // Still in the same polygon

	// Next polygon in path?
	if(poly && m_path.state() == PathState::Success && m_pathIndex<m_path.m_path.size()) {
		uint edge = m_path.m_path[ m_pathIndex ].edge;
		NavPoly* next = NavMesh::getLinkedPolygon( poly, edge );
		for(uint i=m_pathIndex+1; next && i<m_pathIndex+4 && i<m_path.m_path.size(); ++i) {
			if(next->id != m_path.m_path[i].poly) break; // invalid path
			if(NavMesh::isInsidePolygon(p, next)) {
				m_pathIndex = i;
				m_polygon = next->id;
				return;
			}
			edge = m_path.m_path[ i ].edge;
			next = NavMesh::getLinkedPolygon( next, edge );
		}
		// goal polygon is not in path list
		NavPoly* goal = m_path.getNavMesh()->getPolygon(m_goalPoly);
		if(goal && NavMesh::isInsidePolygon(p, goal)) {
			m_pathIndex = m_path.m_path.size();
			m_polygon = m_goalPoly;
			return;
		}

		printf("Position (%g, %g) not on path\n", m_position.x, m_position.z);
		m_polygon = NavPoly::Invalid;
	}

	// Full search
	m_polygon = m_path.getNavMesh()->getPolygonID(p);
	if(m_polygon==NavPoly::Invalid) printf("Error: Positon not on navmesh\n");
	else if(getState() == PathState::Success) repath();
}

bool PathFollower::setGoal(const vec3& t) {
	m_goal = t;
	if(!m_path.resolvePoint(m_goal, m_radius)) {
		printf("Failed to resolve navmesh location (%g, %g)\n", t.x, t.z);
		stop();
		return false;
	}
	uint tp = m_path.getNavMesh()->getPolygonID(m_goal);
	if(atGoal(1e-6)) return true;
	m_goalPoly = tp;
	PathState r = repath();
	return r==PathState::Success || r==PathState::Partial;
}

uint PathFollower::getPolygonID() const {
	return m_polygon;
}



int PathFollower::findNextPolygon(int typeIndex, int skip) {
	int start = m_pathIndex + skip;

	// Cache
	if(m_findCache.size() <= (uint)typeIndex) m_findCache.resize(typeIndex+1, -1);
	if(m_findCache[typeIndex] == -2) return NavPoly::Invalid;
	if(m_findCache[typeIndex] >= start) return m_path.m_path[m_findCache[typeIndex]].poly;

	// Search
	size_t end = m_path.m_path.size();
	for(size_t i=start; i<end; ++i) {
		NavPoly* poly = m_path.m_navmesh->getPolygon(m_path.m_path[i].poly);
		if(!poly) break;
		if(poly->typeIndex==typeIndex) {
			m_findCache[typeIndex] = i;
			return poly->id;
		}
	}
	m_findCache[typeIndex] = -2;
	return NavPoly::Invalid;
}

VecPair PathFollower::nextPoint() {
	if(m_position.distance2(m_goal) < m_radius*m_radius) return m_goal;
	const uint end = m_path.m_path.size();
	if(m_pathIndex < end && m_path.m_path[m_pathIndex].poly != m_polygon) { //not on path
		repath();
		return m_position;
	}

	const NavPoly* p = m_path.getNavMesh()->getPolygon(m_polygon);
	if(!p) { repath(); return m_position; }

	auto insetPoint = [start=m_position](const vec3& point, float inset) {
		vec3 n(start.z - point.z, 0, point.x - start.x);
		n.normalise();

		float vv = powf(start.x-point.x,2) + powf(start.z-point.z, 2);
		float rr = inset * inset;
		float aa = vv - rr;
		float xx = vv * rr / aa;

		if(vv < rr + 1e-8) {
			//printf("Stuck on a corner\n");
			return point + (start - point).normalise() * fabs(inset);
		}

		float x = sqrt(xx);
		vec3 target = point + n * x * fsign(inset);
		return start + (target-start) * (fabs(inset)/x);
	};

	//Setup wedge
	int edgeIndex = -1;
	vec3 target[2], normal[2];
	static constexpr float mult[2] = { 1, -1 };
	if(m_pathIndex < end) {
		edgeIndex = m_path.m_path[m_pathIndex].edge;
		target[0] = insetPoint(p->points[edgeIndex], m_radius);
		target[1] = insetPoint(p->points[(edgeIndex+1)%p->size], -m_radius);
	}
	else {
		target[0] = target[1] = m_goal;
	}
	normal[0].set(target[0].z - m_position.z, 0, m_position.x - target[0].x);
	normal[1].set(m_position.z - target[1].z, 0, target[1].x - m_position.x);

	auto processPoint = [&](const vec3& p, int side) {
		vec3 insetTarget = insetPoint(p, m_radius * mult[side]);
		if(normal[side].dot(insetTarget - m_position) < 0 || normal[side].dot(p-m_position) < 0) {
			target[side] = insetTarget;
			normal[side].set(target[side].z - m_position.z, 0, m_position.x - target[side].x);
			if(side) normal[side] *= -1;
			DebugLine(p, target[side], side? 0xffa000: 0xa000ff);
			DebugLine(m_position, target[side], side? 0xffa000: 0xa000ff);
		}
	};


	// Check if there are collisions before the first edge
	struct EdgeStack { const NavPoly* poly; int from; };
	std::vector<EdgeStack> stack = {{p, edgeIndex}};
	for(size_t i=0; i<stack.size(); ++i) {
		for(auto& edge: stack[i].poly) {
			if(edge.a == stack[i].from) continue;
			if(base::closestPointOnLine(m_position, edge.poly.points[edge.a], edge.poly.points[edge.b]).distance2(m_position) < m_radius*m_radius) {
				vec3 m = edge.point() - m_position;
				if(m.dot(normal[0]) > 0) processPoint(edge.point(), 0);
				if(m.dot(normal[1]) > 0) processPoint(edge.point(), 1);

				m = edge.poly.points[edge.b] - m_position;
				if(m.dot(normal[0]) > 0) processPoint(edge.poly.points[edge.b], 0);
				if(m.dot(normal[1]) > 0) processPoint(edge.poly.points[edge.b], 1);

				if(const NavPoly* c = edge.connected()) stack.push_back({c, edge.oppositeEdge()});
			}
		}
	}

	// Collapse remaining wedge
	bool collapsed = normal[0].dot(target[1]-m_position) > 0; // Collapsed
	for(uint i=m_pathIndex+1; i<end && !collapsed; ++i) {
		p = NavMesh::getLinkedPolygon(p, edgeIndex);
		edgeIndex = m_path.m_path[i].edge;

		// Path error
		if(!p || p->id != m_path.m_path[i].poly) {
			repath();
			return m_position;
		}

		processPoint(p->points[edgeIndex], 0);
		processPoint(p->points[(edgeIndex+1)%p->size], 1);
		collapsed = normal[0].dot(target[1]-m_position) > 0; // Collapsed
	}
	
	// Test goal
	if(!collapsed) {
		vec3 m = m_goal - m_position;
		if(m.dot(normal[0])>0) return VecPair(target[0], m_goal);
		else if(m.dot(normal[1])>0) return VecPair(target[1], m_goal);
		else return m_goal;
	}
	else {
		if(target[0].distance2(m_position) < target[1].distance2(m_position)) return VecPair(target[0], target[1]);
		else return VecPair(target[1], target[0]);
	}
}

PathState PathFollower::repath() {
	m_pathIndex = 0;
	uint goalID = m_path.getNavMesh()->getPolygonID(m_goal);
	m_goalPoly = goalID; // may have changed
	m_findCache.clear();
	return m_path.search(m_position, m_polygon, m_goal, goalID);
}

void PathFollower::stop() {
	m_goal = m_position;
	m_goalPoly = m_polygon;
	m_path.clear();
	m_findCache.clear();
}

PathState PathFollower::getState() const {
	return m_path.state();
}

bool PathFollower::atGoal(float d) const {
	return m_polygon == m_goalPoly && m_position.xz().distance2(m_goal.xz()) < d*d;
}


// ============================================================================================= //

float PathFollower::moveCollide(const vec3& pos, const vec3& move, float radius, vec3& tangent) const {
	if(move.x == 0 && move.z==0) return 1;
	
	// Starting navmesh face
	const NavPoly* poly = m_path.m_navmesh->getPolygon( pos );
	if(!poly) return 1; // Not on navmesh

	std::vector<const NavPoly*> polys;
	polys.push_back(poly);

	float s, t;
	float moved = 1.0f;
	vec2 moveDirection = move.xz();
	float moveDistance = moveDirection.normaliseWithLength();
	for(size_t stackIndex=0; stackIndex<polys.size(); ++stackIndex) {
		poly = polys[stackIndex];
		for(NavMesh::EdgeInfo& edge : poly) {
			const vec2 a = poly->points[edge.a].xz();
			const vec2 b = poly->points[edge.b].xz();

			if(!edge.isConnected()) {
				vec2 normal(a.y-b.y, b.x-a.x);
				if(normal.dot(moveDirection) > -1e-3) continue; // Ignore edges facing away
				float mod = radius / a.distance(b);
				vec2 p = pos.xz() - normal * mod; // Point on circle that will hit first
				base::intersectLines(p, p+move.xz(), a, b, s, t);
				if(s<1 && t > -mod*2 && t < 1+mod*2) {
					// Get intersection point
					if(t>=0 && t<=1) p = lerp(a,b,t);
					else {
						p = t<0? a: b;
						vec2 m = p - pos.xz();
						float vb = -m.dot(moveDirection);
						float vc = m.dot(m) - radius * radius;
						float discriminant = vb * vb - vc;
						if(discriminant < 1e-4) s = moved + 1; // nope
						else s = (-vb - sqrt(discriminant)) / moveDistance;
						if(s<-1e-4) s = moved + 1;
					}
					if(s > -1 && s <= moved) {
						vec3 hitTangent;
						if(t>=0&& t<=1) hitTangent.set(normal.y, 0, -normal.x);
						else hitTangent.set(pos.z-p.y, 0, p.x-pos.x);
						if(hitTangent.dot(move)<0) hitTangent *= -1;

						if(moved < 1e-4) {
							vec3 test = pos - p.xzy(pos.y);
							hitTangent.normalise();
							tangent.normalise();
							if(test.dot(hitTangent) > test.dot(tangent)) tangent = hitTangent;
						}
						else tangent = hitTangent;
						moved = fmax(0,s);
					}
				}
			}
			else {
				// Capsule collision with this edge to determine if we use next poly
				const NavPoly* next = edge.connected();
				bool alreadyCovered = false;
				for(const NavPoly* c: polys) alreadyCovered |= next == c;
				if(!alreadyCovered && base::closestPointBetweenLines(a.xzy(pos.y), b.xzy(pos.y), pos, pos+move, s,t) < radius*radius) {
					polys.push_back(edge.connected());
				}
			}
		}
	}
	return moved;
}

bool PathFollower::ray(const vec3& goal, const NavFilter& filter) const {
	return m_path.ray(m_position, goal, m_polygon, filter);
}
bool PathFollower::resolvePoint(vec3& out, const vec3& target, float radius, float search) const {
	out = target;
	return m_path.resolvePoint(out, radius, search);
}

