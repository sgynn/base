#pragma once

#include <base/math.h>
#include <base/colour.h>
#include <base/thread.h>
#include <vector>
#include <map>

namespace base {
	class DebugGeometryManager;
	class Material;
	class Drawable;
	class SceneNode;
	class Scene;

	/** Flush method for debug geometries */
	enum DebugGeometryFlush {
		SDG_MANUAL,			// Requires calling DebugGeometry::flush() manually
		SDG_FRAME,			// Flush automatically called every frame
		SDG_ALWAYS = SDG_FRAME,
		SDG_FRAME_IF_DATA,	// Flush automatically called every frame if there is new data
		SDG_AUTOMATIC = SDG_FRAME_IF_DATA,
		SDG_APPEND			// Flush automatically, previous lines are not cleared. use reset() to clear.
	};

	/** Vertex class used in debug geometry */
	struct DebugGeometryVertex { vec3 pos; uint8 r,g,b,a; };

	/** Debug geometry instance */
	class DebugGeometry {
		friend class DebugGeometryManager;
		public:
		DebugGeometry(DebugGeometryManager*, DebugGeometryFlush mode=SDG_FRAME, bool xray=true, float lineWidth=1.0f);
		DebugGeometry(DebugGeometryFlush mode=SDG_FRAME, bool xray=true, float lineWidth=1.0f);
		~DebugGeometry();
		void flush();
		void reset();

		void line(const vec3& a, const vec3& b, int colour=0xffffff);
		void arrow(const vec3& a, const vec3& b, int colour=0xffffff);
		void vector(const vec3& start, const vec3& dir, int colour=0xffffff);
		void circle(const vec3& centre, const vec3& axis, float radius, int segments=32, int colour=0xffffff);
		void capsule(const vec3& a, const vec3& b, float radius, int seg=32, int colour=0xffffff, float cap=1);
		void arc(const vec3& centre, const vec3& dstart, const vec3& dend, int segments=32, int colour=0xffffff);
		void marker(const vec3& point, float size=1, int colour=0xffffff);
		void marker(const vec3& point, const Quaternion& orientation, float size=1, int colour=0xffffff);
		void axis(const Matrix&, float size=1); // RGB coloured axes

		void box(const BoundingBox& box, int colour=0xffffff);
		void box(const BoundingBox& box, const Matrix& transform, int colour=0xffffff);
		void box(const BoundingBox& box, const vec3& offset, const Quaternion& orientation, int colour=0xffffff);
		void box(const vec3& extents, const Matrix& transform, int colour=0xffffff);
		void box(const vec3& extents, const vec3& centre, const Quaternion& orientation, int colour=0xffffff);
		void box(const vec3& extents, const vec3& centre, int colour=0xffffff);

		void strip(int count, const vec3* points, int colour=0xffffff);
		void lines(int count, const vec3* data, int colour=0xffffff);

		protected:
		typedef std::vector<DebugGeometryVertex> GeometryList;
		DebugGeometryManager*  m_manager;
		DebugGeometryFlush     m_mode;
		GeometryList*          m_buffer;
		BoundingBox            m_bounds;
		Drawable*              m_drawable;
	};


	/** Debug geometry manager - handles auto-flushing and rendering */
	class DebugGeometryManager {
		static DebugGeometryManager* s_instance;
		public:
		DebugGeometryManager();
		~DebugGeometryManager();
		static void initialise(Scene*, int queue=10, Material* mat=0);
		static void initialise(Scene*, int queue, bool onTop);
		static DebugGeometryManager* getInstance() { return s_instance; }
		static Material* getDefaultMaterial();
		static void flush() { if(s_instance) s_instance->update(); } // Call this in render function to update drawables
		Material* getMaterial() const { return m_material; }
		void setRenderQueue(int queue);
		void setMaterial(Material*);
		void setVisible(bool);
		void add(DebugGeometry*);
		void remove(DebugGeometry*);
		void removeNode(SceneNode*);
		void update();
		protected:

		int m_renderQueue;
		Material* m_material;
		std::map<DebugGeometry*, Drawable*> m_drawables;
		std::vector<DebugGeometry*> m_addList;
		std::vector<DebugGeometry*> m_deleteList;
		std::vector<SceneNode*> m_nodes;
		Mutex m_mutex;
	};

}

