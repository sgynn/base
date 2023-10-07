#include "renderers.h"

using namespace particle;

struct ParticleVertex {
	vec3 position;
	vec2 texcoord;
	uint colour;
};

SpriteRendererQuads::SpriteRendererQuads() : RenderData(QUADS) {
	m_attributes.add(base::VA_VERTEX, base::VA_FLOAT3);
	m_attributes.add(base::VA_TEXCOORD, base::VA_FLOAT2);
	m_attributes.add(base::VA_COLOUR, base::VA_ARGB);
}
void SpriteRendererQuads::setParticleVertices(void* out, const Particle& p, const Matrix& view) const {
	vec3 x, y;
	if(p.orientation.x) {
		float s = sin(p.orientation.x);
		float c = cos(p.orientation.x);
		x = vec3(&view[0])*s + vec3(&view[4])*c;
		y = vec3(&view[0])*-c + vec3(&view[4])*s;
		x *= p.scale.x;
		y *= p.scale.y;
	}
	else {
		x = vec3(&view[0]) * p.scale.x;
		y = vec3(&view[4]) * p.scale.y;
	}


	ParticleVertex* vx = reinterpret_cast<ParticleVertex*>(out);
	vx[0].position = p.position - x - y;
	vx[0].texcoord.set(0, 0);
	vx[0].colour = p.colour;

	vx[1].position = p.position + x - y;
	vx[1].texcoord.set(1, 0);
	vx[1].colour = p.colour;

	vx[2].position = p.position + x + y;
	vx[2].texcoord.set(1, 1);
	vx[2].colour = p.colour;

	vx[3].position = p.position - x + y;
	vx[3].texcoord.set(0, 1);
	vx[3].colour = p.colour;
}

// ======================================================================================== //

SpriteRenderer::SpriteRenderer() : RenderData(TRIANGLES) {
	m_attributes.add(base::VA_VERTEX, base::VA_FLOAT3);
	m_attributes.add(base::VA_TEXCOORD, base::VA_FLOAT2);
	m_attributes.add(base::VA_COLOUR, base::VA_ARGB);
}
void SpriteRenderer::setParticleVertices(void* out, const Particle& p, const Matrix& view) const {
	vec3 x, y;
	if(p.orientation.x) {
		float s = sin(p.orientation.x);
		float c = cos(p.orientation.x);
		x = vec3(&view[0])*s + vec3(&view[4])*c;
		y = vec3(&view[0])*-c + vec3(&view[4])*s;
		x *= p.scale.x;
		y *= p.scale.y;
	}
	else {
		x = vec3(&view[0]) * p.scale.x;
		y = vec3(&view[4]) * p.scale.y;
	}


	ParticleVertex* vx = reinterpret_cast<ParticleVertex*>(out);
	vx[0].position = p.position - x - y;
	vx[0].texcoord.set(0, 0);
	vx[0].colour = p.colour;

	vx[1].position = p.position + x - y;
	vx[1].texcoord.set(1, 0);
	vx[1].colour = p.colour;

	vx[2].position = p.position - x + y;
	vx[2].texcoord.set(0, 1);
	vx[2].colour = p.colour;

	vx[3].position = p.position - x + y;
	vx[3].texcoord.set(0, 1);
	vx[3].colour = p.colour;

	vx[4].position = p.position + x - y;
	vx[4].texcoord.set(1, 0);
	vx[4].colour = p.colour;

	vx[5].position = p.position + x + y;
	vx[5].texcoord.set(1, 1);
	vx[5].colour = p.colour;
}

// ======================================================================================== //


struct ParticleInstance {
	vec3 position;
	vec3 scale;
	Quaternion orientation;
	uint colour;
};

InstanceRenderer::InstanceRenderer() : RenderData(INSTANCE), m_meshName(0) {
	m_attributes.add(base::VA_CUSTOM, base::VA_FLOAT3, "loc",   1);	// Location
	m_attributes.add(base::VA_CUSTOM, base::VA_FLOAT3, "scale", 1);	// Scale
	m_attributes.add(base::VA_CUSTOM, base::VA_FLOAT4, "rot",   1);	// quaternion
	m_attributes.add(base::VA_CUSTOM, base::VA_ARGB,   "col",   1);	// colour
}
InstanceRenderer::~InstanceRenderer() { free(m_meshName); }
void InstanceRenderer::setInstancedMesh(const char* name) {
	free(m_meshName);
	m_meshName = name? strdup(name): 0;
}
void InstanceRenderer::setParticleVertices(void* output, const Particle& p, const Matrix& view) const {
	ParticleInstance* inst = reinterpret_cast<ParticleInstance*>(output);
	inst->position = p.position;
	inst->scale = p.scale;
	inst->orientation = p.orientation;
	inst->colour = p.colour;
}


