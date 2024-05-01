#pragma once

#include <base/drawable.h>

namespace base {
	
	class NavMesh;
	class HardwareVertexBuffer;
	
	/** Drawable object for navmesh */
	class NavDrawable : public base::Drawable {
		public:
		NavDrawable(const NavMesh*, float offset=0, float polygonAlpha=1, base::Material* mat=0);
		~NavDrawable();
		void                draw( base::RenderState& ) override;
		void                build();
		private:
		const NavMesh* m_nav;		// Mesh to draw
		float          m_offset;	// Vertical offset
		BoundingBox    m_bounds;	// Drawable bounding box
		uint           m_lc;		// Last size - for recalculating aabb
		uint           m_alpha;		// Polygon alpha

		class SubDrawable : public base::Drawable {
			public:
			SubDrawable(int mode);
			~SubDrawable();
			void draw(base::RenderState&) override;
			base::HardwareVertexBuffer* buffer;
			int mode;
		};

		SubDrawable* m_polys;
		SubDrawable* m_lines;
	};
}

