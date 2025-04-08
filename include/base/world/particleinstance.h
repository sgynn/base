#pragma once
#include <base/particles.h>
#include <base/scene.h>

namespace base {
	class DrawableMesh;
	class HardwareVertexBuffer;
	class SceneNode;

	class ParticleInstance : public particle::Instance, public SceneNode {
		private:
		struct TagData { DrawableMesh* drawable; HardwareVertexBuffer* buffer; int stride; bool instanced; bool empty; };
		int m_renderQueue;
		public:
		ParticleInstance(particle::Manager* m, particle::System* system, const char* name=nullptr, int queue=10);
		~ParticleInstance();
		void setRenderQueue(int);
		void initialise() override;
		void* createDrawable(const particle::RenderData* data);
		void createMaterial(const particle::RenderData* data) override;
		void destroyDrawables();
		void updateGeometry() override;
		const Matrix& getTransform() const override;
	};
}

