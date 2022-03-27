#pragma once

#include <base/drawable.h>

namespace base {
	class Skeleton;
	class Mesh;

	class DrawableMesh : public Drawable {
		public:
		DrawableMesh(base::Mesh* mesh=0, Material* mat=0, int queue=0);
		~DrawableMesh();

		void updateBounds();

		virtual void draw( RenderState& );

		void setMesh(base::Mesh* mesh);
		void setupSkinData( base::Skeleton* );

		void                        setInstanceCount(unsigned);
		void                        setInstanceBuffer(base::HardwareVertexBuffer*);
		base::HardwareVertexBuffer* getInstanceBuffer() const;
		base::Mesh* 		        getMesh() const;
		base::Skeleton*             getSkeleton() const;

		protected:
		void updateSkeletonSource(RenderState&) const;				// Updates skeleton matrices in auto variable source. Must be called before material is bound.
		void renderGeometry() const;								// Render geometry. Assumes buffers are already bound.

		base::Mesh*     			m_mesh;			// mesh data
		base::Skeleton* 			m_skeleton;		// skeleton data
		int*                    	m_skinMap;		// Map of bones to skin indices
		unsigned					m_instances;	// Instance count
		base::HardwareVertexBuffer*	m_instanceBuffer;
	};
}


