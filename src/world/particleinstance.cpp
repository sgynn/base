#include <base/world/particleinstance.h>
#include <base/particles.h>
#include <base/hardwarebuffer.h>
#include <base/drawablemesh.h>
#include <base/resources.h>
#include <base/material.h>
#include <base/model.h>
#include <base/mesh.h>

using namespace base;
using namespace particle;

ParticleInstance::ParticleInstance(Manager* m, System* system, const char* name, int queue) : Instance(system), SceneNode(name), m_renderQueue(queue) {
	m->add(this);
}

ParticleInstance::~ParticleInstance() {
	destroyDrawables();
}

void ParticleInstance::setRenderQueue(int q) {
	m_renderQueue = q;
	for(Drawable* d: m_drawables) d->setRenderQueue(q);
}

void ParticleInstance::initialise() {
	destroyDrawables();
	Instance::initialise();
	for(RenderInstance& r : m_renderers) {
		r.drawable = createDrawable(r.render);
		createMaterial(r.render);
	}
}

void* ParticleInstance::createDrawable(const RenderData* data) {
	TagData* tag = new TagData{0,0,0,0,0};
	tag->buffer = new HardwareVertexBuffer();
	tag->buffer->attributes = data->getVertexAttributes();
	tag->buffer->setData(nullptr, 0, tag->buffer->attributes.getStride(), false); // set stride
	tag->stride = data->getVertexAttributes().calculateStride();

	if(data->getDataType() == RenderData::INSTANCE) {
		tag->instanced = true;
		Model* model = Resources::getInstance()->models.get(data->getInstancedMesh());
		if(model) {
			tag->drawable = new DrawableMesh(model->getMesh(0));
			tag->drawable->setMaterial(Resources::getInstance()->materials.get("particleinst.mat"));
			tag->drawable->setInstanceBuffer(tag->buffer);
			tag->drawable->setInstanceCount(0);
			tag->drawable->setRenderQueue(m_renderQueue);
		}
		else printf("Failed to load mesh '%s'\n", data->getInstancedMesh());
	}
	else {
		Mesh* mesh = new Mesh();
		mesh->setVertexBuffer(tag->buffer);
		tag->drawable = new DrawableMesh(mesh);
		tag->drawable->setRenderQueue(m_renderQueue);
		tag->instanced = false;
		switch(data->getDataType()) {
		case RenderData::QUADS: mesh->setPolygonMode(PolygonMode::QUADS); break;
		case RenderData::STRIP: mesh->setPolygonMode(PolygonMode::TRIANGLE_STRIP); break;
		case RenderData::POINTS: mesh->setPolygonMode(PolygonMode::POINTS); break;
		default: break;
		}
	}

	if(tag->drawable) attach(tag->drawable);
	else { delete tag; tag=0; }
	return tag;
}

void ParticleInstance::createMaterial(const RenderData* data) {
	TagData* tag = (TagData*)m_renderers[data->getDataIndex()].drawable;
	if(tag) {
		const char* name = data->getMaterial();
		Resources& res = *Resources::getInstance();
		Material* mat = res.materials.getIfExists(name);
		if(!mat) {
			Material* base = res.materials.get(tag->instanced? "particleinst.mat": "particle.mat");
			Texture* tex = res.textures.get(name);
			if(tex && base) {
				mat = base->clone();
				mat->getPass(0)->setTexture("diffuse", tex);
				mat->getPass(0)->compile();
				res.materials.add(name, mat);
			}
			else mat = base;
		}
		tag->drawable->setMaterial(mat);
	}
}

void ParticleInstance::destroyDrawables() {
	for(RenderInstance& r : m_renderers) {
		if(TagData* a = (TagData*)r.drawable) {
			if(a->instanced) delete a->buffer;
			r.drawable = 0;
		}
	}
	deleteAttachments();
}

void ParticleInstance::updateGeometry() {
	for(RenderInstance& r : m_renderers) {
		TagData* a = (TagData*)r.drawable;
		if(!a) continue;
		if(a->empty && r.count==0) continue;
		a->buffer->setData(r.data, r.count * r.render->getVerticesPerParticle(), a->stride, false);
		assert(a->buffer->getStride() == a->buffer->attributes.getStride());
		a->drawable->setVisible(r.count);
		if(a->instanced) a->drawable->setInstanceCount(r.count);
		a->empty = r.count == 0;
	}
}

const Matrix& ParticleInstance::getTransform() const {
	return m_derived;
}

