/** This file contains the OpenGL functions for drawing the model */

#include "base/opengl.h"
#include "base/model.h"
#include "base/texture.h"
#include "base/shader.h"

using namespace base;
using namespace model;


// Vertex buffer object functions




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


