#pragma once
#include <base/particles.h>

namespace particle {

class PointRenderer : public RenderData {
};

class SpriteRenderer : public RenderData {
	public:
	SpriteRenderer();
	void setParticleVertices(void* output, const Particle& p, const Matrix& view) const override;
};

class InstanceRenderer : public RenderData {
	public:
	InstanceRenderer();
	~InstanceRenderer();
	void setInstancedMesh(const char*);
	const char* getInstancedMesh() const override { return m_meshName; }
	void setParticleVertices(void* output, const Particle& p, const Matrix& view) const override;
	protected:
	char* m_meshName;
};

class RibbonRenderer : public RenderData {
	public:
	RibbonRenderer() : RenderData(STRIP) {}
	void setParticleVertices(void* output, const Particle& p, const Matrix& view) const override {}
};


}

