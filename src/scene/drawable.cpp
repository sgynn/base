#include <base/opengl.h>
#include <base/shader.h>
#include <base/renderer.h>
#include <base/material.h>
#include <base/drawablemesh.h>
#include <base/autovariables.h>
#include <base/hardwarebuffer.h>
#include <base/mesh.h>
#include <base/model.h>
#include <cstdio>

using namespace base;

#ifdef WIN32
extern PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
extern PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
extern PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
extern PFNGLVERTEXATTRIBDIVISORPROC glVertexAttribDivisor;
extern PFNGLDRAWELEMENTSINSTANCEDPROC glDrawElementsInstanced;
extern PFNGLDRAWARRAYSINSTANCEDPROC glDrawArraysInstanced;
extern PFNGLACTIVETEXTUREARBPROC glActiveTexture;
extern PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays;
#endif


Drawable::Drawable(): m_transform(new Matrix), m_sharedTransform(false) {}
Drawable::~Drawable() {
	if(!m_sharedTransform) delete m_transform;
}

void Drawable::shareTransform(Matrix* m) {
	if(m) {
		if(!m_sharedTransform) delete m_transform;
		m_sharedTransform = true;
		m_transform = m;
	}
	else if(m_sharedTransform) {
		m_transform = new Matrix;
		m_sharedTransform = false;
	}
}

void Drawable::addBuffer(HardwareVertexBuffer* buffer) {
	if(!buffer) return;
	bind();
	buffer->createBuffer();
	const int stride = buffer->attributes.getStride();
	const char* base = (const char*)buffer->bind();
	for(const Attribute& a : buffer->attributes) {
		int loc = 0;
		switch(a.semantic) {
		case VA_VERTEX:	    loc = 0; break;
		case VA_NORMAL:     loc = 1; break;
		case VA_TANGENT:    loc = 2; break;
		case VA_TEXCOORD:   loc = 3; break;
		case VA_COLOUR:     loc = 4; break;
		case VA_SKININDEX:  loc = 5; break;
		case VA_SKINWEIGHT: loc = 6; break;
		case VA_CUSTOM:
			loc = m_material? m_material->getPass(0)->getShader()->getAttributeLocation(a.name): 0;
			if(loc <= 0) {
				if(!m_material) printf("Error: Custom shader attributes can't be bound before material is set.\n");
				else printf("Error: Custom shader attribute '%s' not found.\n", a.name);
				continue;
			}
			break;
		}

		unsigned dataType = HardwareVertexBuffer::getDataType(a.type);
		int      dataElem = HardwareVertexBuffer::getDataCount(a.type);
		bool integerType = a.semantic == VA_SKININDEX;
		bool normalised = a.type==VA_ARGB || a.type==VA_RGB;

		glEnableVertexAttribArray(loc);
		if(integerType) glVertexAttribIPointer(loc, dataElem, dataType, stride, base+a.offset);
		else glVertexAttribPointer(loc, dataElem, dataType, normalised, stride, base+a.offset);
		/*DOUBLE: glVertexAttribLPointer(loc, dataElem, type, stride, base+a.offset);*/
		if(a.divisor) glVertexAttribDivisor(loc, a.divisor);
		GL_CHECK_ERROR;
	}
}

void Drawable::addBuffer(HardwareIndexBuffer* buffer) {
	if(!buffer) return;
	bind();
	buffer->createBuffer();
	buffer->bind();
}

void Drawable::bind() {
	if(!m_binding) glGenVertexArrays(1, &m_binding);
	glBindVertexArray(m_binding);
}




// =============================================================== //




DrawableMesh::DrawableMesh(Mesh* m, Material* mat, int queue)
	: m_mesh(m), m_skeleton(0), m_skinMap(0), m_instances(1), m_instanceBuffer(0) 
{
	setRenderQueue(queue);
	setMaterial(mat);
	if(m) setMesh(m);
}
DrawableMesh::~DrawableMesh() {
	if(m_binding) glDeleteVertexArrays(1, &m_binding);
	if(m_instanceBuffer) m_instanceBuffer->dropReference();
	delete [] m_skinMap;
}


void DrawableMesh::updateBounds() {
	const vec3& a = m_mesh->getBounds().min;
	const vec3& b = m_mesh->getBounds().max;

	m_bounds.setInvalid();
	m_bounds.include( *m_transform * a);
	m_bounds.include( *m_transform * b);
	m_bounds.include( *m_transform * vec3(a.x, a.y, b.z));
	m_bounds.include( *m_transform * vec3(a.x, b.y, a.z));
	m_bounds.include( *m_transform * vec3(a.x, b.y, b.z));
	m_bounds.include( *m_transform * vec3(b.x, a.y, a.z));
	m_bounds.include( *m_transform * vec3(b.x, a.y, b.z));
	m_bounds.include( *m_transform * vec3(b.x, b.y, a.z));
}

void DrawableMesh::setMesh(Mesh* m) {
	m_mesh = m;
	if(m_binding) glDeleteVertexArrays(1, &m_binding);
	m_binding = 0;

	if(!m_mesh) return;

	addBuffer(m_mesh->getVertexBuffer());
	addBuffer(m_mesh->getSkinBuffer());
	addBuffer(m_mesh->getIndexBuffer());
	addBuffer(m_instanceBuffer);
}

void DrawableMesh::setupSkinData( Skeleton* s ) {
	m_skeleton = s;
	delete [] m_skinMap;
	if(s && m_mesh->getSkinCount()) m_skinMap = Model::createSkinMap( s, m_mesh );
	else {
		m_skeleton = 0;
		m_skinMap = 0;
	}
}

void DrawableMesh::setInstanceCount(unsigned count) {
	m_instances = count;
}

void DrawableMesh::setInstanceBuffer(HardwareVertexBuffer* buf) {
	if(buf==m_instanceBuffer) return;
	if(m_instanceBuffer) m_instanceBuffer->dropReference();
	m_instanceBuffer = buf;
	m_instanceBuffer->addReference();
	addBuffer(m_instanceBuffer);
}

HardwareVertexBuffer* DrawableMesh::getInstanceBuffer() const {
	return m_instanceBuffer;
}

Mesh* DrawableMesh::getMesh() const {
	return m_mesh;
}

Skeleton* DrawableMesh::getSkeleton() const {
	return m_skeleton;
}

void DrawableMesh::draw( RenderState& state) {
	if(!m_binding) return;
	GL_CHECK_ERROR;

	updateSkeletonSource(state);
	state.setMaterial( m_material );
	GL_CHECK_ERROR;

	if(!Shader::current().isCompiled()) return;

	bind();
	GL_CHECK_ERROR;
	renderGeometry();
	GL_CHECK_ERROR;
}

void DrawableMesh::updateSkeletonSource(RenderState& r) const {
	if(m_skeleton && m_skeleton->getMatrixPtr() && r.getVariableSource()) {
		int size = m_mesh->getSkinCount();
		const Matrix* matrices = m_skeleton->getMatrixPtr();
		r.getVariableSource()->setSkinMatrices( size, matrices, m_skinMap );
	}
	else if(m_mesh->getSkinCount()) {
		static const Matrix empty[100];
		r.getVariableSource()->setSkinMatrices(m_mesh->getSkinCount(), empty);
	}
	else r.getVariableSource()->setSkinMatrices( 0, 0 );
}

void DrawableMesh::renderGeometry() const {
	static const uint modes[] = { GL_TRIANGLES, GL_QUADS, GL_LINES, GL_POINTS, GL_TRIANGLE_STRIP, GL_LINE_STRIP };
	uint mode = modes[ (int)m_mesh->getPolygonMode() ];
	if(m_instances > 1) {
		if(m_mesh->getIndexBuffer()) {
			GLenum indexType = m_mesh->getIndexBuffer()->getDataType();
			const char* ix = (char*)m_mesh->getIndexBuffer()->bind();
			glDrawElementsInstanced(mode, m_mesh->getIndexCount(), indexType, ix, m_instances);
		}
		else {
			glDrawArraysInstanced(mode, 0, m_mesh->getVertexCount(), m_instances);
		}
	}
	else {
		if(m_mesh->getIndexBuffer()) {
			m_mesh->getIndexBuffer()->bind();  // Shouldn't be needed as it should be in the VAO, but that crashes.
			GLenum indexType = m_mesh->getIndexBuffer()->getDataType();
			glDrawElements(mode, m_mesh->getIndexCount(), indexType, 0);
		}
		else {
			glDrawArrays(mode, 0, m_mesh->getVertexCount());
		}
	}
}




