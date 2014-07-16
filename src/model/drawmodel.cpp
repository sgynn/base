/** This file contains the OpenGL functions for drawing the model */

#include "base/opengl.h"
#include "base/model.h"
#include "base/texture.h"
#include "base/shader.h"

// Extensions
#ifndef GL_VERSION_2_0
#define GL_ARRAY_BUFFER          0x8892
#define GL_ELEMENT_ARRAY_BUFFER  0x8893
#define GL_STATIC_DRAW           0x88E4

typedef ptrdiff_t GLsizeiptr;
typedef void (*PFNGLBINDBUFFERPROC) (GLenum target, GLuint buffer);
typedef void (*PFNGLDELETEBUFFERSPROC) (GLsizei n, const GLuint *buffers);
typedef void (*PFNGLGENBUFFERSPROC) (GLsizei n, GLuint *buffers);
typedef void (*PFNGLBUFFERDATAPROC) (GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage);

PFNGLBINDBUFFERPROC    glBindBuffer    = (PFNGLBINDBUFFERPROC)    wglGetProcAddress("glBindBuffer");
PFNGLDELETEBUFFERSPROC glDeleteBuffers = (PFNGLDELETEBUFFERSPROC) wglGetProcAddress("glDeleteBuffers");
PFNGLGENBUFFERSPROC    glGenBuffers    = (PFNGLGENBUFFERSPROC)    wglGetProcAddress("glGenBuffers");
PFNGLBUFFERDATAPROC    glBufferData    = (PFNGLBUFFERDATAPROC)    wglGetProcAddress("glBufferData");
#endif

using namespace base;
using namespace model;


// Vertex buffer object functions

void Mesh::bindBuffer() const {
	glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer? m_vertexBuffer->bufferObject: 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer? m_indexBuffer->bufferObject: 0);
}
void Mesh::useBuffer(bool use) {
	if(use) {
		// Create Vertex Buffer
		if(m_vertexBuffer && m_vertexBuffer->bufferObject==0) {
			glGenBuffers(1, &m_vertexBuffer->bufferObject);
			updateBuffer();
		}
		// Create Index buffer
		if(m_indexBuffer && m_indexBuffer->bufferObject==0) {
			glGenBuffers(1, &m_indexBuffer->bufferObject);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer->bufferObject);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer->size*sizeof(IndexType), m_indexBuffer->data, GL_STATIC_DRAW);
		}
	} else {
		// Delete buffers
		if(m_vertexBuffer && m_vertexBuffer->ref<=1 && m_vertexBuffer->bufferObject) {
			glDeleteBuffers(1, &m_vertexBuffer->bufferObject);
			m_vertexBuffer->bufferObject = 0;
		}
		if(m_indexBuffer  && m_indexBuffer->ref<=1  && m_indexBuffer->bufferObject) {
			glDeleteBuffers(1, &m_indexBuffer->bufferObject);
			m_indexBuffer->bufferObject = 0;
		}
	}
	// Update pointers
	cachePointers();
}
void Mesh::updateBuffer() {
	if(m_vertexBuffer && m_vertexBuffer->bufferObject) {
		glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer->bufferObject);
		glBufferData(GL_ARRAY_BUFFER, m_vertexBuffer->size*sizeof(VertexType), m_vertexBuffer->data, GL_STATIC_DRAW);
	}
}


//// //// //// //// //// //// //// //// //// //// //// //// //// //// //// ////

void Model::draw() const {
	// Draw all meshes
	glEnableClientState(GL_VERTEX_ARRAY);
	int state = 1;
	for(uint i=0; i<m_meshes.size(); ++i) {
		// Transform?
		if(m_meshes[i].bone) {
			glPushMatrix();
			glMultMatrixf(m_meshes[i].bone->getAbsoluteTransformation());
			state = drawMesh(m_meshes[i].output, state);
			glPopMatrix();
		} else {
			state = drawMesh(m_meshes[i].output, state);
		}
	}
	glDisableClientState(GL_VERTEX_ARRAY);
	if(state&2) glDisableClientState(GL_NORMAL_ARRAY);
	if(state&4) glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	if(state&8) glDisableClientState(GL_COLOR_ARRAY);
}


void Model::drawMesh( const Mesh* mesh ) {
	glEnableClientState(GL_VERTEX_ARRAY);

	int state = Model::drawMesh(mesh, 1);

	glDisableClientState(GL_VERTEX_ARRAY);
	if(state&2) glDisableClientState(GL_NORMAL_ARRAY);
	if(state&4) glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	if(state&8) glDisableClientState(GL_COLOR_ARRAY);
}


int Model::drawMesh( const Mesh* m, int state) {
	// Set up gl states
	int f = m->getFormat();
	int s = 1 | (f&NORMAL?2:0) | (f&TEXCOORD?4:0) | (f&(COLOUR3|COLOUR4)?8:0);
	if(s!=state) {
		int on  = (s^state)&s;
		int off = (s^state)&state;
		if(on&2)       glEnableClientState(GL_NORMAL_ARRAY);
		else if(off&2) glDisableClientState(GL_NORMAL_ARRAY);
		if(on&4)       glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		else if(off&4) glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		if(on&8)       glEnableClientState(GL_COLOR_ARRAY);
		else if(off&8) glDisableClientState(GL_COLOR_ARRAY);
		state=s;
	}

	// Bind material
	if(m->getMaterial()) m->getMaterial()->bind();

	// Bind buffer objects
	m->bindBuffer();

	// Set pointers
	glVertexPointer(f&0xf, GL_FLOAT, m->getStride(), m->getVertexPointer());
	glNormalPointer(       GL_FLOAT, m->getStride(), m->getNormalPointer());
	glTexCoordPointer(2,   GL_FLOAT, m->getStride(), m->getTexCoordPointer());
	if(s&8) glColorPointer( (f>>16)&7, GL_FLOAT, m->getStride(), m->getColourPointer());
	// Tangent?
	if(f&TANGENT) {
		Shader::current().AttributePointer("tangent", 3, GL_FLOAT, 0, m->getStride(), m->getTangentPointer());
	}

	// Draw mesh
	if(m->hasIndices()) glDrawElements(m->getMode(), m->getSize(), GL_UNSIGNED_SHORT, m->getIndexPointer() );
	else glDrawArrays(m->getMode(), 0, m->getVertexCount());

	return state;
}


void Model::drawSkeleton(const Skeleton* s) {
	glEnableClientState(GL_VERTEX_ARRAY);
	for(int i=0; i<s->getBoneCount(); ++i) {
		drawBone( s->getBone(i) );
	}
	glDisableClientState(GL_VERTEX_ARRAY);
}



void Model::drawBone(const Bone* b) {
	static float vx[18] = { 0,0,0,  .06,.06,.1,  .06,-.06,0.1,  -.06,-.06,.1, -.06,.06,.1,  0,0,1 };
	static unsigned char ix[24] = { 0,1,2, 0,2,3, 0,3,4, 0,4,1,  1,5,2, 2,5,3, 3,5,4, 4,5,1 };
	glPushMatrix();
	glMultMatrixf(b->getAbsoluteTransformation());
	glScalef(b->getLength(), b->getLength(), b->getLength());
	glVertexPointer(3, GL_FLOAT, 0, vx);
	glDrawElements(GL_TRIANGLES, 24, GL_UNSIGNED_BYTE, ix);
	glPopMatrix();
}


