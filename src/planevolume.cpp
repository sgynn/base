#include "base/planevolume.h"

using namespace base;

// something for vector utils
template<class T, class Func>
static void modify(std::vector<T>& list, Func&& func) {
	for(size_t i=0; i<list.size();) {
		if(func(list[i])) ++i;
		else { list[i] = list.back(); list.pop_back(); }
	}
}


PlaneVolume PlaneVolume::build(const vec3* points, int size) {
	if(size < 4) return {};

	struct Used { vec3 point; std::vector<int> link; bool missing=false; };
	std::vector<Used> used;
	std::vector<Plane> planes;
	auto removePlane = [&used, &planes](int index) {
		int back = planes.size() - 1;
		modify(used, [back, index](Used& u) {
			modify(u.link, [back, index, &u](int& i) {
				if(i == index) { u.missing=true;  return false; }
				if(i == back) i = index;
				return true;
			});
			return !u.link.empty();
		});
		planes[index] = planes.back();
		planes.pop_back();
	};

	// bootstrap a sealed volume
	// then for each new point
	// 	if inside, skip
	//	remove the planes we are otside
	//	remove the points that only connect touch planes
	//	build new planes using new point, and points that touched the removed planes
	
	// Find initial points
	vec3 centre;
	Plane initialPlane(vec3(),0);
	for(int i=0; i<size && used.size() < 4; ++i) {
		vec3 p = points[i];
		bool bad = false;
		for(Used& u: used) if(p == u.point) bad = true; // skip duplicate points
		if(used.size() == 3 && initialPlane.test(p, 1e-4) == Plane::Intersect) bad = true;
		if(bad) continue;
		used.push_back({p});
		if(used.size() == 3) initialPlane = Plane(p, used[0].point, used[1].point);
		centre += p;
	}
	centre /= used.size();

	// Build initial planes
	for(int i=0; i<2; ++i) {
		for(int j=i+1; j<3; ++j) {
			for(int k=j+1; k<4; ++k) {
				Plane p(used[i].point, used[j].point, used[k].point);
				if(p.test(centre) == Plane::Outside) p.flip();
				used[i].link.push_back(planes.size());
				used[j].link.push_back(planes.size());
				used[k].link.push_back(planes.size());
				planes.push_back(p);
			}
		}
	}

	// Expand with the rest
	for(int i=0; i<size; ++i) {
		vec3 p = points[i];

		// Only need points outside the existing volume
		bool needed = false;
		for(size_t j=0; j<planes.size();) {
			if(planes[j].test(p, 1e-3) != Plane::Outside) ++j;
			else { removePlane(j); needed = true; }
		}
		if(!needed) continue;

		Used newPoint{p};

		// Connect new point to any existing planes we are on
		for(size_t i=0; i<planes.size(); ++i) {
			if(planes[i].test(p, 1e-3) == Plane::Intersect) newPoint.link.push_back(i);
		}

		// Try to add planes to all vertices flagged missing
		for(size_t j=0; j<used.size(); ++j) {
			if(!used[j].missing) continue;
			for(size_t k=j+1; k<used.size(); ++k) {
				if(!used[k].missing) continue;
				// test the other points
				Plane plane(p, used[j].point, used[k].point);
				bool in=false, out=false;
				for(size_t t=0; t<used.size() && !(in&&out); ++t) {
					if(t==j || t==k) continue;
					Plane::Side s = plane.test(used[t].point, 1e-3);
					in |= s == Plane::Inside;
					out |= s == Plane::Outside;
					if(s == Plane::Intersect && !used[t].missing) in=out=true;
				}
				if(in && out) continue;
				if(out) plane.flip();

				used[j].link.push_back(planes.size());
				used[k].link.push_back(planes.size());
				newPoint.link.push_back(planes.size());
				planes.push_back(plane);
			}
		}
		for(Used& u: used) u.missing = false;
		used.push_back(newPoint);
	};
	

	// Result
	PlaneVolume result;
	for(Plane&p: planes) result.addPlane(p);
	return result;
}


bool PlaneVolume::getIntersetionPoint(int ia, int ib, int ic, vec3& point) const {
	const Plane& a = getPlane(ia);
	const Plane& b = getPlane(ib);
	const Plane& c = getPlane(ic);

	vec3 u = b.normal.cross(c.normal);
	float denom = u.dot(a.normal);
	if(fabs(denom) < 1e-4) return false; // no intersections
	point = (u * a.d + a.normal.cross(-c.normal*b.d + b.normal*c.d)) / denom;
	for(int i=0; i<getSize(); ++i) {
		if(i==ia || i==ib || i==ic) continue;
		if(getPlane(i).test(point, 1e-3) == Plane::Outside) return false;
	}
	return true;
}

std::vector<vec3> PlaneVolume::getEdges() const {
	int s = getSize();
	std::vector<vec3> edges;
	for(int i=0; i<s-1; ++i) {
		for(int j=i+1; j<s; ++j) {
			vec3 d = getPlane(i).normal.cross(getPlane(j).normal);
			float denom = d.length2();
			if(fabs(denom) < 1e-8) continue; // No edge
			vec3 p = d.cross(getPlane(i).normal * getPlane(j).d - getPlane(j).normal * getPlane(i).d) / denom;
			
			// get limits
			float low = 1e8f, high=-1e8f;
			vec3 nlow, nhigh;
			for(int k=0; k<s; ++k) {
				if(k==i || k==j) continue;
				const Plane& plane = getPlane(k);
				float dn = plane.normal.dot(d);
				if(dn==0) continue;
				float t = (plane.d - plane.normal.dot(p)) / dn;
				if(dn < 0) {
					if(t<low) nlow = plane.normal;
					low = fmin(low, t);
				}
				else {
					if(t>high) nhigh = plane.normal;
					high = fmax(high, t);
				}
			}
			if(low < high) continue;
			edges.push_back(p + d * low);
			edges.push_back(p + d * high);
		}
	}
	return edges;
}
	
std::vector<vec3> PlaneVolume::getPoints() const {
	vec3 p;
	int s = getSize();
	std::vector<vec3> points;
	for(int i=0; i<s; ++i) {
		for(int j=i+1; j<s; ++j) {
			for(int k=j+1; k<s; ++k) {
				if(getIntersetionPoint(i,j,k,p)) points.push_back(p);
			}
		}
	}
	return points;
}


