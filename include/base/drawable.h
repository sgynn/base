#ifndef _DRAWABLE_
#define _DRAWABLE_

#include <base/math.h>

namespace base {
	class HardwareVertexBuffer;
	class HardwareIndexBuffer;
	class RenderState;
	class Material;

	/** Drawable base class */
	class Drawable {
		public:
		Drawable();
		virtual ~Drawable();

		virtual void draw( RenderState& ) = 0;

		void setTransform(const Matrix& m) { *m_transform = m; }
		void setTransform(const vec3& p, const Quaternion& q) { q.toMatrix(*m_transform); m_transform->setTranslation(p); }
		void setTransform(const vec3& p, const Quaternion& q, const vec3& s) { q.toMatrix(*m_transform); m_transform->setTranslation(p); m_transform->scale(s); }
		const Matrix& getTransform() const { return *m_transform; }
		void shareTransform(Matrix* m);

		void setVisible(bool v) { m_visible = v; }
		bool isVisible() const { return m_visible; }

		void setFlags(int flags) { m_flags = flags; }
		int  getFlags() const { return m_flags; }
		void setRenderQueue(int q) { m_queue = q; }
		int  getRenderQueue() const { return m_queue; }

		virtual void setMaterial(Material* m) { m_material=m; }
		Material* getMaterial() const { return m_material; }
		void setCustom(float* data) { m_custom = data; }
		const float* getCustom() const { return m_custom; }

		const BoundingBox& getBounds() const { return m_bounds; }
		virtual void updateBounds() {}

		public:
		void addBuffer(HardwareVertexBuffer*);
		void addBuffer(HardwareIndexBuffer*);
		void bind();

		protected:
		int         m_flags   = 0;
		bool        m_visible = true;
		BoundingBox m_bounds;
		Matrix*     m_transform;
		bool        m_sharedTransform;

		// Drawables are sorted by materials for optimisation
		Material*   m_material = nullptr;
		float*      m_custom   = nullptr;
		int         m_queue    = 0;
		unsigned    m_binding  = 0;	// vaobj
	};
}


#endif

