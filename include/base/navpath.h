#pragma once

#include <base/math.h>
#include <vector>

namespace base {

	class NavMesh;
	struct NavPoly;

	enum class PathState : char { None, Success, Fail, Partial, Invalid };

	/** Filter for navpoly type traversal */
	class NavFilter {
		friend class Pathfinder;
		uint64 m_mask;
		inline bool hasType(int index) const { return (m_mask&(1<<index))>0; }
		public:
		NavFilter(uint64 m=0);
		void addType(const char* type);
		void removeType(const char* type);
		bool hasType(const char* name) const;

		static const NavFilter ALL;
		static const NavFilter NONE;
	};

	/** Navmesh pathfinder - Keep separate from navmesh */
	class Pathfinder {
		friend class PathFollower;
		public:
		struct Location { vec3 position; uint polygon=-1; };
		Pathfinder(const NavMesh* mesh);
		PathState search(uint startPoly, uint endPoly);
		PathState search(const vec3& start, const vec3& end);
		PathState search(const Location& start, const Location& goal);
		PathState search(const Location& start, const std::vector<Location>& goals, int& goalIndex);
		PathState state() const { return m_state; }
		void clear();

		void setNavMesh(const NavMesh*);
		void setFilter( const NavFilter& );

		bool ray(const vec3& start, const vec3& end, uint poly=~0u, const NavFilter& f=NavFilter::ALL) const;
		float ray(const Ray& ray, float limit=1e6f, uint poly=~0u, const NavFilter& f=NavFilter::ALL) const;

		const NavPoly* resolvePoint(vec3& point, float radius=0, float search=1, int iterations=4) const;

		const NavMesh* getNavMesh() const { return m_navmesh; }
		static void setMaxRadius(float m) { s_maxRadius = m; }

		private:
		template<class AtGoal, class Heuristic>
		PathState searchInternal(const Location& start, AtGoal&&, Heuristic&&);
		bool checkTraversal(const base::NavPoly* poly, const vec3& a, const vec3& b);

		protected:
		const NavMesh*  m_navmesh;			// Navmesh to search
		NavFilter m_filter;					// Navmesh polygon type filter
		float     m_radius;					// Character radius
		uint      m_length;					// Path distance
		PathState m_state;					// Pathfinder state

		// The path is a list of edges to traverse
		struct Node { uint poly, edge; };
		std::vector<Node> m_path;
		static float s_maxRadius;
	};

	struct VecPair {
		vec3 first, second;
		VecPair(const vec3& p) : first(p), second(p) {}
		VecPair(const vec3& a,const vec3& b) : first(a), second(b){}
		operator vec3&() { return first; }
	};

	/** Path follower class*/
	class PathFollower {
		public:
		PathFollower(const NavMesh* nav = nullptr);
		void        setNavMesh(const NavMesh*);
		void        setRadius(float r);			// Set character radius
		void        setPosition(const vec3&);	// Update the internal actor position
		bool        setGoal(const vec3&);		// Set the target
		int         setGoal(const std::vector<vec3>&); // Multi-pathfind, returns goal index or -1
		VecPair     nextPoint();				// Get the next point to head to
		PathState   repath();					// Re-generate path
		void        stop();						// stop everything
		PathState   getState() const;			// Get path state
		uint        getPolygonID() const;		// Get id of polygon follower is currently at
		bool        atGoal(float d=1) const;	// Have we reached the target?
		int         findNextPolygon(int typeIndex, int skip=0); // Find next polygon in path of type

		const NavMesh* getNavMesh() const { return m_path.getNavMesh(); }
		const vec3& getPosition() const { return m_position; }
		const vec3& getGoal() const { return m_goal; } 
		float getRadius() const { return m_radius; }

		/// Movement collision, returns 0-1 multiplier of amount of movement to be used.
		float moveCollide(const vec3& pos, const vec3& move, float radius, vec3& tangent) const;
		bool  ray(const vec3& target, const NavFilter& f=NavFilter::ALL) const;
		bool  resolvePoint(vec3& out, const vec3& target, float radius=0, float search=10) const;
		bool  setPositionAndResolve(const vec3& pos, float search=10);

		protected:
		Pathfinder m_path;	// Path object
		uint m_pathIndex;	// Position in path
		uint m_polygon;		// Current polygon
		uint m_goalPoly;	// Target polygon
		vec3 m_position;	// Current position
		vec3 m_goal;		// Goal position
		vec3 m_next;		// Next point to head for
		float m_radius;		// Character radius
		std::vector<short> m_findCache;
	};

}

