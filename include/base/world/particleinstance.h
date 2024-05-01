#pragma once
#include <base/particles.h>
#include <base/scene.h>

namespace base {
	class DrawableMesh;
	class HardwareVertexBuffer;
	class SceneNode;

	class ParticleInstance : public particle::Instance, public SceneNode {
		private:
		struct TagData { DrawableMesh* drawable; HardwareVertexBuffer* buffer; int stride; bool instanced; };
		public:
		ParticleInstance(particle::Manager* m, particle::System* system, const char* name=nullptr);
		~ParticleInstance();
		void initialise() override;
		void* createDrawable(const particle::RenderData* data);
		void createMaterial(const particle::RenderData* data) override;
		void destroyDrawables();
		void updateGeometry() override;
		const Matrix& getTransform() const override;
	};
}

