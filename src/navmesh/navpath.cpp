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

namespace nav {
	extern void updateLink(NavLink*); 
	extern void calculateTraversal(const NavPoly*, float);
}
float Pathfinder::s_maxRadius = 1.f;


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
	uint startPoly = m_navmesh->getPolygonID(a);
	uint endPoly   = m_navmesh->getPolygonID(b);
	if(startPoly == NavPoly::Invalid || endPoly == NavPoly::Invalid) return PathState::Invalid;
	return search({a, startPoly}, {b, endPoly});
}

PathState Pathfinder::search(uint a, uint b) {
	NavPoly* p;
	p = m_navmesh->getPolygon(a);
	if(!p) return PathState::Invalid;

	Location start = {0.f, a};
	for(int i=0; i<p->size; ++i) start.position += p->points[i];
	start.position /= p->size;

	p = m_navmesh->getPolygon(b);
	if(!p) return PathState::Invalid;

	Location end = {0.f, b};
	for(int i=0; i<p->size; ++i) end.position += p->points[i];
	end.position /= p->size;

	return search(start, end);
}

bool Pathfinder::checkTraversal(const NavPoly* poly, const vec3& a, const vec3& b) {
	if(!poly->traversalCalculated) nav::calculateTraversal(poly, s_maxRadius);
	for(const NavTraversalThreshold& t: poly->traversal) {
		if(t.threshold < m_radius*2) {
			if((t.normal.dot(a.xz()) < t.d) != (t.normal.dot(b.xz()) < t.d)) return false;
		}
	}
	return true;
};

template<class AtGoal, class Heuristic>
PathState Pathfinder::searchInternal(const Location& start, AtGoal&& atGoal, Heuristic&& heuristic) {
	clear();

	const NavPoly* poly = m_navmesh->getPolygon(start.polygon);
	if(!poly) {
		m_state = PathState::Fail; // Invalid staring polygon
		return m_state;
	}

	struct AStarNode { const NavLink* link; int k; vec3 p; float value; AStarNode* parent; };
	static const auto cmp = [](const AStarNode* a, const AStarNode* b) { return a->value > b->value; };

	std::vector<AStarNode*> open, all;
	AStarNode startNode { nullptr, 0, start.position, 0.f, nullptr };
	AStarNode* node = &startNode;

	std::map<const NavLink*, AStarNode*> states;
	typename std::map<const NavLink*, AStarNode*>::iterator state;

	while(true) {
		int added = 0;
		if(atGoal(poly, node->p)) break;
		else for(int i=0; i<poly->size; ++i) {
			NavLink* link = poly->links[i];
			if(link && m_filter.hasType(poly->typeIndex)) {	// ToDo: Clearance
				state = states.find(link);
				if(state != states.end() && state->second==0) continue; // closed

				// Invalid
				if(!link->poly[0] || !link->poly[1]) {
					printf("Path Error: Messed up link detected\n");
					continue;
				}

				// Simple size test - not perfect
				if(link->width == 0) nav::updateLink(link);
				if(link->width < m_radius * 2) continue;

				// Centre point - used for distance heuristic
				vec3& u = link->poly[0]->points[ link->edge[0] ];
				vec3& v = link->poly[1]->points[ link->edge[1] ];
				vec3 p = (u + v) * 0.5;

				// Traversal thresholds
				if(!checkTraversal(poly, node->p, p)) continue;

				// Heuristic
				float h = node->value + p.distance(node->p) + heuristic(p);

				// Create / update node
				AStarNode* n = 0;
				if(state==states.end()) all.push_back((n = new AStarNode));
				else if(state->second->value<h) continue;
				else n = state->second;
				n->parent = node;
				n->link = link;
				n->k = link->poly[0]==poly? 1: 0;
				n->value = h;
				n->p = p;

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
	}
	
	// Build path
	Node pathNode;
	if(atGoal(poly, node->p)) {
		while(node->link) {
			pathNode.poly = node->link->poly[ node->k^1 ]->id;
			pathNode.edge = node->link->edge[ node->k^1 ];
			m_path.push_back( pathNode );
			node = node->parent;
		}
		std::reverse(m_path.begin(), m_path.end());
		m_state = PathState::Success;
	}
	else m_state = PathState::Fail;

	// cleanup
	for(uint i=0; i<all.size(); ++i) delete all[i];
	return m_state;
}

PathState Pathfinder::search(const Location& start, const Location& goal) {
	return searchInternal(
		start, 
		[goal, this](const NavPoly* poly, const vec3& pos) {
			if(poly->id != goal.polygon) return false;
			return checkTraversal(poly, pos, goal.position);
		},
		[goal](const vec3& pos) {
			return pos.distance(goal.position);
		}
	);
}

PathState Pathfinder::search(const Location& start, const std::vector<Location>& goals, int& goalIndex) {
	if(goals.empty()) return PathState::Invalid;
	return searchInternal(
		start, 
		[&goals, &goalIndex, this](const NavPoly* poly, const vec3& pos) {
			for(const Location& g: goals) {
				if(poly->id == g.polygon && checkTraversal(poly, pos, g.position)) {
					goalIndex = (int)(&g - goals.data());
					return true;
				}
			}
			return false;
		},
		[&goals](const vec3& pos) {
			float min = 1e30f;
			for(const Location& g: goals) min = fmin(min, pos.distance(g.position));
			return min;
		}
	);
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

float Pathfinder::ray(const Ray& ray, float limit, uint polyID, const NavFilter& f) const {
	const NavPoly* p = polyID==NavPoly::Invalid? m_navmesh->getPolygon(ray.start): m_navmesh->getPolygon(polyID);
	if(!NavMesh::isInsidePolygon(ray.start, p)) p = m_navmesh->getPolygon(ray.start); // Validate
	if(!p) return 0;
	vec3 n(-ray.direction.z, 0, ray.direction.x);
	int last = -1;
	float u, v;
	const vec3 end = ray.point(limit).setY(ray.start.y);
	while(true) {
		for(int i=p->size-1, j=0; j<p->size; i=j, ++j) {
			if(i!=last) {
				if(n.dot(p->points[i]-ray.start)<=0 && n.dot(p->points[j]-ray.start)>=0) {
					closestPointBetweenLines(ray.start, end, p->points[i], p->points[j], u, v);
					if(u >= 1) return limit;
					if(!p->links[i]) return u * limit;
					last = NavMesh::getLinkedEdge( p, i );
					p = NavMesh::getLinkedPolygon( p, i );
					if(!f.hasType(p->typeIndex)) return u * limit;
					break;
				}
			}
		}
	}
	return 0;
}

// ============================================================================================= //

const NavPoly* Pathfinder::resolvePoint(vec3& pt, float r, float search, int iter) const {
	NavPoly* poly = m_navmesh->getPolygon(pt);
	if(!poly || fabs(pt.y - poly->centre.y) > poly->extents.y + 0.1) {
		vec3 close;
		poly = m_navmesh->getClosestPolygon(pt, search, &close);
		if(!poly) return nullptr;
		pt = close;
	}

	if(r==0 &&  m_navmesh->isInsidePolygon(pt, poly)) return poly;

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
		else return poly;
	}

	return poly;
}


// =====================================================================================i================ //

PathFollower::PathFollower(const NavMesh* nav) : m_path(nav), m_pathIndex(0), m_polygon(NavPoly::Invalid), m_goalPoly(NavPoly::Invalid), m_radius(0) {
}

void PathFollower::setNavMesh(const NavMesh* nav) {
	m_path.setNavMesh(nav);
}

void PathFollower::setRadius(float r) {
	m_radius = r;
	m_path.m_radius = r;
}

void PathFollower::setSearchRadius(float r) {
	m_search = r;
}

void PathFollower::setPosition(const vec3& p) {
	m_position = p;
	NavPoly* poly = m_path.getNavMesh()->getPolygon(m_polygon);
	if(poly && NavMesh::isInsidePolygon(p, poly)) return; // Still in the same polygon

	// Next polygon in path?
	if(poly && m_path.state() == PathState::Success && m_pathIndex<m_path.m_path.size()) {
		uint edge = m_path.m_path[ m_pathIndex ].edge;
		NavPoly* next = NavMesh::getLinkedPolygon( poly, edge );
		for(uint i=m_pathIndex+1; next && i<m_pathIndex+6 && i<m_path.m_path.size(); ++i) {
			if(next->id != m_path.m_path[i].poly) break; // invalid path
			if(fabs(next->centre.y - poly->centre.y) > next->extents.y + poly->extents.y + 1) break;
			if(NavMesh::isInsidePolygon(p, next)) {
				m_pathIndex = i;
				m_polygon = next->id;
				return;
			}
			edge = m_path.m_path[ i ].edge;
			next = NavMesh::getLinkedPolygon( next, edge );
		}
		// goal polygon is not in path list
		if(m_pathIndex + 6 > m_path.m_path.size()) {
			NavPoly* goal = m_path.getNavMesh()->getPolygon(m_goalPoly);
			if(goal && NavMesh::isInsidePolygon(p, goal)) {
				m_pathIndex = m_path.m_path.size();
				m_polygon = m_goalPoly;
				return;
			}
		}

		// Search other adjacent polygons if we slipped off
		for(NavMesh::EdgeInfo& e: poly) {
			if(NavPoly* adjacent = e.connected()) {
				if(NavMesh::isInsidePolygon(p, adjacent)) {
					printf("Off path but probably still valid (%u -> %u)\n", m_polygon, adjacent->id);
					m_polygon = adjacent->id;
					m_path.m_path[m_pathIndex] = { m_polygon, (uint)e.oppositeEdge() };
					return;
				}
			}
		}

		printf("Position (%g, %g, %g) not on path\n", m_position.x, m_position.y, m_position.z);
		m_polygon = NavPoly::Invalid;
	}


	// Full search
	poly = m_path.m_navmesh->getPolygon(p);
	if(!poly || fabs(p.y - poly->centre.y) > poly->extents.y + 0.1) {
		vec3 close;
		poly = m_path.m_navmesh->getClosestPolygon(p, m_search, &close);
	}
	m_polygon = poly? poly->id: NavPoly::Invalid;
	if(m_polygon==NavPoly::Invalid) printf("Error: Position not on navmesh\n");
	else if(getState() == PathState::Success) repath();
}

bool PathFollower::setGoal(const vec3& t) {
	m_goal = t;
	const NavPoly* poly = m_path.resolvePoint(m_goal, m_radius, m_search);
	if(!poly) {
		printf("Failed to resolve navmesh location (%g, %g, %g)\n", t.x, t.y, t.z);
		stop();
		return false;
	}
	if(atGoal(1e-6)) return true;
	m_goalPoly = poly->id;
	PathState r = repath();
	return r==PathState::Success || r==PathState::Partial;
}

int PathFollower::setGoal(const std::vector<vec3>& goals) {
	if(goals.empty()) return -1;
	if(goals.size() == 1) return setGoal(goals[0])? 1: -1;
	std::vector<Pathfinder::Location> targets;
	targets.reserve(goals.size());
	bool atLeastOneGoalIsValid = false;
	for(vec3 g: goals) {
		const NavPoly* poly = m_path.resolvePoint(g, m_radius, m_search);

		// at this goal already
		if(poly && m_polygon == poly->id && m_position.xz().distance2(m_goal.xz()) < 1e-12) {
			m_goal = g;
			return targets.size();
		}

		targets.push_back({g, poly? poly->id: NavPoly::Invalid});
		atLeastOneGoalIsValid |= (bool)poly;
		if(!poly) printf("Failed to resolve navmesh location (%g, %g, %g)\n", g.x, g.y, g.z);
	}
	if(!atLeastOneGoalIsValid) {
		stop();
		return -1;
	}

	int goalIndex = -1;
	m_pathIndex = 0;
	PathState r = m_path.search({m_position, m_polygon}, targets, goalIndex);
	if(r != PathState::Success) return -1;
	m_goalPoly = targets[goalIndex].polygon;
	m_goal = targets[goalIndex].position;
	return goalIndex;
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
	if(atGoal(m_radius)) return m_goal;
	const uint end = m_path.m_path.size();

	auto repathAndUpdatePoly = [this]() {
		if(repath() != PathState::Success) return false;
		m_polygon = m_path.m_path[0].poly; // May have changed
		return true;
	};
	if(m_pathIndex < end && m_path.m_path[m_pathIndex].poly != m_polygon) { //not on path
		if(!repathAndUpdatePoly()) return m_position;
	}
	const NavPoly* poly = m_path.getNavMesh()->getPolygon(m_polygon);
	if(!poly) { // Current polygon no longer exists
		if(!repathAndUpdatePoly()) return m_position;
		poly = m_path.getNavMesh()->getPolygon(m_polygon);
	}

	auto insetPoint = [start=m_position.xz()](const vec3& point, float inset) {
		vec2 n(start.y - point.z, point.x - start.x);
		n.normalise();

		float vv = powf(start.x-point.x,2) + powf(start.y-point.z, 2);
		float rr = inset * inset;
		float aa = vv - rr;
		float xx = vv * rr / aa;

		if(vv < rr + 1e-8) {
			//printf("Stuck on a corner\n");
			return point.xz() + (start - point.xz()).normalise() * fabs(inset);
		}

		float x = sqrt(xx);
		vec2 target = point.xz() + n * x * fsign(inset);
		return start + (target-start) * (fabs(inset) / x);
	};

	//Setup wedge
	int edgeIndex = -1;
	bool collapsed = false;
	vec2 position = m_position.xz();
	vec2 target[2], normal[2];
	float distanceSquared[2];
	static constexpr float mult[2] = { 1, -1 };
	if(m_pathIndex < end) {
		edgeIndex = m_path.m_path[m_pathIndex].edge;
		int otherSide = (edgeIndex+1) % poly->size;
		target[0] = insetPoint(poly->points[edgeIndex], m_radius);
		target[1] = insetPoint(poly->points[otherSide], -m_radius);
		distanceSquared[0] = position.distance2(target[0]);
		distanceSquared[1] = position.distance2(target[1]);
		DebugLine(poly->points[edgeIndex], target[0].xzy(poly->points[edgeIndex].y), 0xa000ff);
		DebugLine(m_position, target[0].xzy(poly->points[edgeIndex].y), 0xa000ff);
		DebugLine(poly->points[otherSide], target[1].xzy(poly->points[otherSide].y), 0xffa000);
		DebugLine(m_position, target[1].xzy(poly->points[otherSide].y), 0xffa000);
	}
	else {
		target[0] = target[1] = m_goal.xz();
		distanceSquared[0] = distanceSquared[1] = position.distance2(target[0]);
		collapsed = true;
	}
	normal[0].set(target[0].y - position.y,  position.x - target[0].x);
	normal[1].set(position.y - target[1].y,  target[1].x - position.x);
	collapsed |= normal[0].dot(target[1]-position) > 0;

	// Inset target point and update wedges
	auto processPoint = [&](const vec3& p, int side) {
		vec2 insetTarget = insetPoint(p, m_radius * mult[side]);
		if(normal[side].dot(insetTarget - position) < 0 || normal[side].dot(p.xz()-position) < 0) {
			target[side] = insetTarget;
			normal[side].set(target[side].y - position.y, position.x - target[side].x);
			distanceSquared[side] = position.distance2(insetTarget);
			if(side) normal[side] *= -1;
			DebugLine(p, target[side].xzy(p.y), side? 0xffa000: 0xa000ff);
			DebugLine(m_position, target[side].xzy(p.y), side? 0xffa000: 0xa000ff);
			return true;
		}
		return false;
	};

	// Collapse remaining wedge
	const NavPoly* pathPoly = poly;
	if(!collapsed) {
		vec3 leftAnchor, rightAnchor;
		for(uint i=m_pathIndex+1; i<end; ++i) {
			pathPoly = NavMesh::getLinkedPolygon(pathPoly, edgeIndex);
			edgeIndex = m_path.m_path[i].edge;

			// Path error
			if(!pathPoly || pathPoly->id != m_path.m_path[i].poly) {
				repath();
				return m_position;
			}

			vec3 leftPoint = pathPoly->points[edgeIndex];
			vec3 rightPoint = pathPoly->points[(edgeIndex+1)%pathPoly->size];
			if(collapsed && (leftPoint!=leftAnchor && rightPoint!=rightAnchor)) break;

			processPoint(leftPoint, 0);
			processPoint(rightPoint, 1);
			if(!collapsed && normal[0].dot(target[1]-position) > 0) {
				leftAnchor = leftPoint;
				rightAnchor = rightPoint;
				collapsed = true;
			}
		}
	}
	
	// Collapse into goal
	if(!collapsed) {
		vec2 m = m_goal.xz() - position;
		if(m.dot(normal[0]) > 0) target[1] = m_goal.xz();
		else if(m.dot(normal[1]) > 0) target[0] = m_goal.xz();
		else target[0] = target[1] = m_goal.xz();
		normal[0].set(target[0].y - position.y, position.x - target[0].x);
		normal[1].set(position.y - target[1].y, target[1].x - position.x);
		collapsed = true;
	}

	// Handle collisions from off-path edges due to radius overlapping other polygons
	if(m_radius > 0) {
		float collapseDistSquared = 1e8f;
		auto checkCollapse = [&]() {
			// project the invalid target onto the closen direction, make sure it is not shorter
			int side = target[0].distance2(position) < target[1].distance2(position)? 1: 0;
			vec2 targetDirection = target[side^1] - position;
			float nearDistance = targetDirection.normaliseWithLength();
			float distance = targetDirection.dot(target[side] - position);
			if(distance <= nearDistance) target[side] = target[side^1];
			else target[side] = position + targetDirection * distance;
			distanceSquared[side] = powf(fmax(distance, nearDistance), 2);
			normal[side] = -normal[side^1];
			collapseDistSquared = distanceSquared[side^1];
		};

		// Need to use a 2D projection
		auto closestPointOnLine2D = [](const vec3& p, const vec3& a, const vec3& b) {
			vec2 d(b.x-a.x, b.z-a.z);
			float t = d.dot((p-a).xz()) / d.dot(d);
			return a.xz() + d * fclamp(t, 0, 1);
		};

		checkCollapse();
		struct EdgeStack { const NavPoly* poly; uint from; };
		std::vector<EdgeStack> stack = {{poly, -1u}}; 
		for(size_t i=0; i<stack.size(); ++i) {
			for(auto& edge: stack[i].poly) {
				const EdgeStack& item = stack[i];
				if(edge.a == item.from || !edge.link()) continue;
				if(closestPointOnLine2D(m_position, edge.pointA(), edge.pointB()).distance2(position) < m_radius*m_radius) {
					vec2 m = edge.pointA().xz() - position;
					if(m.dot(target[0] - position) >= 0 && m.length2() < collapseDistSquared) {
						if(processPoint(edge.pointA(), m.dot(normal[0]) > 0? 0: 1)) checkCollapse();
					}

					m = edge.pointB().xz() - position;
					if(m.dot(target[0] - position) >= 0 && m.length2() < collapseDistSquared) {
						if(processPoint(edge.pointB(), m.dot(normal[0]) > 0? 0: 1)) checkCollapse();
					}
					
					stack.push_back({edge.connected(), edge.oppositeEdge()});
				}
			}
		}
	}
	
	float y = m_position.y;
	if(distanceSquared[0] < distanceSquared[1]) return VecPair(target[0].xzy(y), target[1].xzy(y));
	else return VecPair(target[1].xzy(y), target[0].xzy(y));
}

PathState PathFollower::repath() {
	m_pathIndex = 0;
	uint goalID = m_path.getNavMesh()->getPolygonID(m_goal);
	m_goalPoly = goalID; // may have changed
	m_findCache.clear();
	return m_path.search({m_position, m_polygon}, {m_goal, goalID});
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

bool PathFollower::setPositionAndResolve(const vec3& pos, float search) {
	vec3 newPos = pos;
	const NavPoly* poly = m_path.resolvePoint(newPos, m_radius, search);
	if(poly && poly->id != m_polygon) {
		m_position = newPos;
		m_polygon = poly->id;
		if(m_pathIndex < m_path.m_path.size()) repath();
		else stop();
	}
	else setPosition(newPos);
	return poly;
}

#include <base/assert.h>
std::vector<vec3> PathFollower::getDebugPath(bool detail) const {
	std::vector<vec3> points;
	if(getState() != PathState::Success) return points;

	vec3 a, b;
	const NavPoly* poly = getNavMesh()->getPolygon(m_polygon);
	if(!detail) {
		points.push_back(m_position);
		for(uint i=m_pathIndex; i<m_path.m_path.size(); ++i) {
			const Pathfinder::Node& node = m_path.m_path[i];
			NavMesh::getEdgePoints(poly, node.edge, a, b);
			points.push_back((a+b)*0.5);
			poly = NavMesh::getLinkedPolygon(poly, node.edge);
			//if(poly->id != node.poly) break; // Error ?
		}
		points.push_back(m_goal);
	}
	else {
		struct Line { vec2 a, b; };
		struct Node { vec3 pivot; int side; uint index; const NavPoly* poly; };
		static auto getTangentNormal = [](vec2 p, vec2 c, float r, int side) { // gets direction from circle centre to line tangent
			vec2 cp = p - c;
			float d = sqrt(cp.x*cp.x + cp.y*cp.y - r*r) * side;
			float n = r*r + d*d;
			return vec2((cp.x*r + cp.y*d) / n, (cp.y*r - cp.x*d) / n);
		};
		static auto getLine = [](const Node& a, const Node& b, float radius) {
			if(a.side == b.side) {
				if(a.side == 0) return Line{a.pivot.xz(), b.pivot.xz()};
				vec2 shift = vec2(b.pivot.z-a.pivot.z, a.pivot.x-b.pivot.x).normalise() * a.side * radius;
				return Line{a.pivot.xz() - shift, b.pivot.xz() - shift};
			}
			else if(a.side == 0) {
				vec2 n = getTangentNormal(a.pivot.xz(), b.pivot.xz(), radius, b.side);
				return Line{a.pivot.xz(), b.pivot.xz() + n * radius};
			}
			else if(b.side == 0) {
				vec2 n = getTangentNormal(b.pivot.xz(), a.pivot.xz(), radius, -a.side);
				return Line{a.pivot.xz() + n * radius, b.pivot.xz()};
			}
			else {
				vec2 n = getTangentNormal(a.pivot.xz(), b.pivot.xz(), radius*2, -a.side);
				return Line{a.pivot.xz() - n * radius, b.pivot.xz() + n * radius};
			}
		};
		static auto side = [](const Line& a, const Line& b) {
			if(a.b == b.b) return 0.f;
			return vec2(a.b.y-a.a.y, a.a.x-a.b.x).dot(b.b - b.a);
		};
		static auto compareLength = [](const Line& a, const Line& b) {
			return a.a.distance2(a.b) - b.a.distance2(b.b);
		};
		static auto drawWedge = [](const Line& line, const vec3& pivot, int side) {
			addDebugLine(line.a.xzy(), line.b.xzy(), side>0? 0xff8000: 0xff0080);
			addDebugLine(pivot, line.b.xzy(), side>0? 0xff8000: 0xff0080);
		};
		// Wedge collapse
		vec3 v[2];
		std::vector<Node> nodes = {{m_position, 0, m_pathIndex, poly}};

		// Manage off-path edges overlapping start location
		auto getOffPathNode = [this](const vec2& centre, const Node& last, const Line& left, const Line& right, const NavPoly* poly, uint skipEdge) {
			int s = 0;
			for(auto& e: poly) {
				if(e.a != skipEdge && e.isConnected()) {
					vec2 edgeDir = e.direction().xz();
					float t = fclamp(edgeDir.dot(centre - e.point().xz()) / edgeDir.dot(edgeDir),  0, 1);
					if(centre.distance2(e.point().xz() + edgeDir * t) < m_radius * m_radius) {
						addDebugLine(centre.xzy(), e.point() + edgeDir.xzy() * t, 0x0080ff);
						
						for(int ei : {e.a, e.b}) {
							const vec3& p = e.poly.points[ei];
							Line k { left.a, p.xz()};
							if((left.b-left.a).dot(k.b-k.a)>0 && side(left, k)>0) s = 1;
							else if((right.b-right.a).dot(k.b-k.a)>0 && side(right, k)<0) s = -1;
							else continue;

							Node n = { p, s, 0, poly };
							k = getLine(last, n, m_radius);
							//drawWedge(k, p, s);
							if(side(k, s<0? left: right)*s > 0 && compareLength(k, s<0?left:right)<0) {
								return n;
							}
						}
					}
				}
			}
			return Node{0.f, 0, 0, 0};
		};

		if(m_pathIndex < m_path.m_path.size()) {
			for(size_t i=0; i<nodes.size(); ++i) {
				const NavPoly* p = nodes[i].poly;
				const Pathfinder::Node& n = m_path.m_path[nodes[i].index];
				NavMesh::getEdgePoints(nodes[i].poly, n.edge, v[0], v[1]);
				Node lnode = { v[1], -1, nodes[i].index, p };
				Node rnode = { v[0],  1, nodes[i].index, p };
				Line left  = getLine(nodes[i], lnode, m_radius);
				Line right = getLine(nodes[i], rnode, m_radius);

				// Off path edges at start of path
				if(i==0) {
					Node off = getOffPathNode(left.a, nodes[0], left, right, poly, m_path.m_path[m_pathIndex].edge);
					if(off.poly) {
						off.index = m_pathIndex;
						if(off.side<0) { lnode = off; left = getLine(nodes[i], off, m_radius); }
						else { rnode = off; right = getLine(nodes[i], off, m_radius); }
						drawWedge(getLine(nodes[i], off, m_radius), off.pivot, off.side);
						if(side(right, left) > 0) { // collapsed already?
							nodes.push_back(compareLength(left, right) < 0? lnode: rnode);
							continue;
						}
					}
				}

				bool collapsed = false;
				if(nodes[i].index+1 < m_path.m_path.size()) p = NavMesh::getLinkedPolygon(p, m_path.m_path[nodes[i].index].edge);
				for(uint e = nodes[i].index + 1; e<m_path.m_path.size(); p=NavMesh::getLinkedPolygon(p, m_path.m_path[e++].edge)) {
					NavMesh::getEdgePoints(p, m_path.m_path[e].edge, v[0], v[1]);
					Line r = getLine(nodes[i], {v[0], 1}, m_radius);
					Line l = getLine(nodes[i], {v[1], -1}, m_radius);
					drawWedge(left, lnode.pivot, -1);
					drawWedge(right, rnode.pivot, 1);

					if(side(r,l) > 0) {
						if(compareLength(r, l) < 0) {
							if(side(right, r) <= 0) rnode = {v[0], 1, e, p};
							nodes.push_back(rnode);
						}
						else {
							if(side(left, l) >= 0) lnode = {v[1], -1, e, p};
							nodes.push_back(lnode);
						}
						collapsed = true;
						break;
					}
					
					if(side(left, r) < 0) {
						nodes.push_back(lnode);
						collapsed = true;
						break;
					}
					else if(side(left, l) >= 0) { lnode = { v[1], -1, e, p }; left=l; }

					if(side(right, l) > 0) {
						nodes.push_back(rnode);
						collapsed = true;
						break;
					}
					else if(side(right, r) <= 0) { rnode = { v[0], 1, e, p }; right=r; }
				}
				assert(p);
				if(!collapsed) {
					Line goal = getLine(nodes[i], {m_goal, 0}, m_radius);
					Node off = getOffPathNode(goal.b, nodes[i], goal, goal, p, -1);
					if(off.poly) {
						off.index = m_path.m_path.size() - 1;
						Line line = getLine(nodes[i], off, m_radius);
						if(side(line, off.side<0? left: right)*off.side < 0) {}
						else if(off.side<0) { lnode = off; left = line; }
						else { rnode = off; right = line; }
						drawWedge(getLine(nodes[i], off, m_radius), off.pivot, off.side);
					}

					if(side(left, goal) < 0) {
						if(goal.a.distance2(goal.b) >= left.a.distance2(left.b)) nodes.push_back(lnode);
					}
					else if(side(right, goal) > 0) {
						if(goal.a.distance2(goal.b) >= right.a.distance2(right.b)) nodes.push_back(rnode);
					}
				}

				if(nodes.size() > 50) break;
			}
		}
		else {
			Line goal = { m_position.xz(), m_goal.xz() };
			Node a = getOffPathNode(goal.a, nodes.back(), goal, goal, poly, -1);
			Node b = getOffPathNode(goal.b, nodes.back(), goal, goal, poly, -1);
			if(a.poly && b.poly && a.side==b.side) {
				if(side(getLine(nodes[0], a, m_radius), getLine(nodes[0], b, m_radius)) * a.side < 0) a.poly = 0;
				else b = getOffPathNode(goal.b, a, goal, goal, poly, -1);
			}
			if(a.poly) nodes.push_back(a);
			if(b.poly) nodes.push_back(b);

			if(a.poly) drawWedge(getLine(nodes[0], a, m_radius), a.pivot, a.side);
			if(b.poly) drawWedge(getLine(a.poly? a: nodes[0], b, m_radius), b.pivot, b.side);
		}

		nodes.push_back({m_goal, 0});


		// Build path point list
		for(size_t i=1; i<nodes.size(); ++i) {
			Line line = getLine(nodes[i-1], nodes[i], m_radius);
			// Rounded corners?
			if(i>1) {
				const vec3& ctr = nodes[i-1].pivot;
				vec2 a = (points.back() - ctr).xz();
				vec2 b = line.a - ctr.xz();
				float angle = acos(a.dot(b) / (m_radius*m_radius));
				int steps = 32 * TWOPI / angle;
				angle *= -nodes[i-1].side;
				for(int j=1; j<steps; ++j) {
					float s = sin(j*angle/steps), c = cos(j*angle/steps);
					vec2 v(c*a.x - s*a.y, c*a.y + s*a.x);
					points.push_back(ctr + v.xzy());
				}
			}
			points.push_back(line.a.xzy(nodes[i-1].pivot.y));
			points.push_back(line.b.xzy(nodes[i].pivot.y));
		}
	}
	return points;
}


