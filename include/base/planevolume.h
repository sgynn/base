#pragma once

#include <vector>
#include <base/math.h>

namespace base {

	class Plane {
		public:
		enum Side { Inside=1, Outside=2, Intersect=3 };
		vec3 normal;
		float d;

		Plane(const vec3& norm, float offset) : normal(norm.normalised()), d(offset) { }
		Plane(const vec3& norm, const vec3& point) : normal(norm.normalised()), d(normal.dot(point)) { }
		Plane(const vec3& a, const vec3& b, const vec3& c) : normal((c-a).cross(b-a).normalise()), d(normal.dot(a)) {}
		Plane flipped() const { return Plane(-normal, -d); }
		void flip() { d = -d; normal = -normal; }

		Side test(const vec3& point) const;								// Point
		Side test(const vec3& centre, float radius) const;				// Sphere
		Side test(const vec3& a, const vec3& b, float radius) const;	// Capsule
		Side test(const vec3& centre, const vec3& extent) const;		// Axis Aligned Box
		bool trace(const Ray& ray, float& t) const;
	};

	/** Plane bounded volume */
	class PlaneVolume {
		public:
		bool empty() const											{ return m_planes.empty(); }
		int getSize() const											{ return m_planes.size(); }
		const Plane& getPlane(int index) const 						{ return m_planes[index]; }
		void addPlane(const Plane& plane)							{ m_planes.push_back(plane); }
		void addPlane(const vec3& normal, float d)					{ addPlane(Plane(normal, d)); }
		void addPlane(const vec3& point, const vec3& normal)		{ addPlane(Plane(normal, point)); }
		void addPlane(const vec3& a, const vec3& b, const vec3& c)	{ addPlane(Plane(a,b,c)); }
		void clear()												{ m_planes.clear(); }

		static PlaneVolume build(const vec3* points, int size);
		std::vector<vec3> getEdges() const;
		std::vector<vec3> getPoints() const;

		std::vector<Plane>::const_iterator begin() const { return m_planes.begin(); }
		std::vector<Plane>::const_iterator end() const { return m_planes.end(); }

		public:
		Plane::Side test(const vec3& point) const;							// Collide point
		Plane::Side test(const vec3& point, float radius) const;			// Collide sphere
		Plane::Side test(const vec3& start, const vec3& end, float radius) const;	// Collide capsule
		Plane::Side test(const BoundingBox& box) const;						// Collide AABB
		Plane::Side test(const Plane& plane) const;
		bool trace(const Ray& ray, float& t) const;
		//bool trace(const Ray& ray, float radius, float& t) const;

		protected:
		std::vector<Plane> m_planes;

		template<class ...T> Plane::Side testPlanes(T...t) const {
			Plane::Side side = Plane::Inside;
			for(const Plane& p: m_planes) {
				Plane::Side s = p.test(t...);
				if(s == Plane::Outside) return s;
				side = (Plane::Side) (s|side);
			}
			return side;
		}

		bool getIntersetionPoint(int ia, int ib, int ic, vec3& out) const;
	};


	inline Plane::Side Plane::test(const vec3& p) const {
		float r = p.dot(normal);
		return r<d? Outside: r>d? Inside: Intersect;
	}
	inline Plane::Side Plane::test(const vec3& p, float r) const {
		float v = p.dot(normal);
		return v+r<d? Outside: v-r>d? Inside: Intersect;
	}
	inline Plane::Side Plane::test(const vec3& a, const vec3& b, float r) const {
		return (Plane::Side)(test(a,r) | test(b,r));
	}
	inline Plane::Side Plane::test(const vec3& centre, const vec3& extent) const {
		float dist = normal.dot(centre) - d;
		float absDot = fabs(normal.x * extent.x) + fabs(normal.y * extent.y) + fabs(normal.z * extent.z);
		if(dist > absDot) return Outside;
		else if(dist > -absDot) return Intersect;
		else return Inside;
	}


	inline Plane::Side PlaneVolume::test(const vec3& point) const {
		return testPlanes(point);
	}
	inline Plane::Side PlaneVolume::test(const vec3& centre, float radius) const {
		return testPlanes(centre, radius);
	}
	inline Plane::Side PlaneVolume::test(const vec3& a, const vec3& b, float radius) const {
		return testPlanes(a, b, radius);
	}
	inline Plane::Side PlaneVolume::test(const BoundingBox& box) const {
		const vec3 centre = box.centre();
		const vec3 extent = box.size() * 0.5;
		return testPlanes(centre, extent);
	}
	inline Plane::Side PlaneVolume::test(const Plane& p) const {
		char state = 0;
		vec3 point;
		size_t s = m_planes.size();
		for(size_t i=0; i<s; ++i) {
			for(size_t j=i+1; j<s; ++j) {
				for(size_t k=j+1; k<s; ++k) {
					if(getIntersetionPoint(i,j,k,point)) {
						switch(p.test(point)) {
						case Plane::Inside: state|=1; break;
						case Plane::Outside: state|=2; break;
						case Plane::Intersect: return Plane::Intersect;
						}
						if(state==3) return Plane::Intersect;
					}
				}
			}
		}
		return state==1? Plane::Inside: Plane::Outside;
	}

	inline bool Plane::trace(const Ray& ray, float& t) const {
		float dn = normal.dot(ray.direction);
		if(dn==0) return false;
		t = (d - normal.dot(ray.start)) / dn;
		return t>0;
	}
	inline bool PlaneVolume::trace(const Ray& ray, float& t) const {
		float value;
		bool hit = false;
		for(const Plane& p: m_planes) {
			if(ray.direction.dot(p.normal) < 0) continue;
			if(!p.trace(ray, value) || value>t) continue;
			vec3 point = ray.point(value);
			for(const Plane& o: m_planes) {
				if(&o==&p) continue;
				if(o.test(point, 1e-3) == Plane::Outside) goto nope;
			}
			t = value;
			hit = true;
			nope:;
		}
		return hit;
	}
}

