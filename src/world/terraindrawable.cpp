#include <base/world/terraindrawable.h>
#include <base/world/landscape.h>
#include <base/hardwarebuffer.h>
#include <base/renderer.h>
#include <base/opengl.h>
#include <base/camera.h>

using namespace base;

struct PatchTag {
	base::HardwareVertexBuffer* vertexBuffer;
	base::HardwareIndexBuffer* indexBuffer;
	uint binding;
};


TerrainDrawable::TerrainDrawable(Landscape* land) : m_land(land) {
	m_land->setPatchCallbacks( ::bind(this, &TerrainDrawable::patchCreated),
							   ::bind(this, &TerrainDrawable::patchDetroyed),
							   ::bind(this, &TerrainDrawable::patchUpdated));
}

void TerrainDrawable::draw(base::RenderState& r) {
	// This block ideally should be elsewhere
	Camera cam = *r.getCamera();
	cam.setPosition( cam.getPosition() - vec3(&getTransform()[12]));
	cam.updateFrustum();
	m_land->update( &cam );
	m_land->cull( &cam );

	r.setMaterial( m_material );
	for(const PatchGeometry* g: m_land->getGeometry()) {
		PatchTag* tag = (PatchTag*)g->tag;
		if(tag) {
			glBindVertexArray(tag->binding);
			tag->indexBuffer->bind(); // Because it is not part of the buffer for some bizarre reason
			glDrawElements(GL_TRIANGLE_STRIP, g->indexCount, tag->indexBuffer->getDataType(), 0);
		}
	}
}

void TerrainDrawable::patchCreated(PatchGeometry* patch) {
	PatchTag* tag = new PatchTag;
	patch->tag = tag;
	tag->vertexBuffer = new base::HardwareVertexBuffer();
	tag->vertexBuffer->attributes.add(base::VA_VERTEX, base::VA_FLOAT3);
	tag->vertexBuffer->attributes.add(base::VA_NORMAL, base::VA_FLOAT3);
	tag->vertexBuffer->setData(patch->vertices, patch->vertexCount, 6*sizeof(float));
	tag->vertexBuffer->createBuffer();
	tag->vertexBuffer->addReference();

	tag->indexBuffer = new base::HardwareIndexBuffer();
	tag->indexBuffer->setData(patch->indices, patch->indexCount);
	tag->indexBuffer->createBuffer();
	tag->indexBuffer->addReference();
	
	m_binding = 0;
	addBuffer(tag->vertexBuffer);
	addBuffer(tag->indexBuffer);
	tag->binding = m_binding;
	m_binding = 0;
}

void TerrainDrawable::patchUpdated(PatchGeometry* patch) {
	PatchTag* tag = (PatchTag*)patch->tag;
	if(tag) {
		tag->vertexBuffer->setData(patch->vertices, patch->vertexCount, 6*sizeof(float));
		tag->indexBuffer->setData(patch->indices, patch->indexCount);
	}
}

void TerrainDrawable::patchDetroyed(PatchGeometry* patch) {
	PatchTag* tag = (PatchTag*)patch->tag;
	if(tag) {
		glDeleteVertexArrays(1, &tag->binding);
		tag->indexBuffer->dropReference();
		tag->vertexBuffer->dropReference();
		patch->tag = nullptr;
		delete tag;
	}
}

