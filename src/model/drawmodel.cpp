/** This file contains the OpenGL functions for drawing the model */

#include "base/model.h"
#include "base/opengl.h"
#include "base/shader.h"

using namespace base;
using namespace model;


void Model::draw() {
	int flags=0;
	
	//Draw all meshes
	Joint* joint = 0;
	for(int i=0; i<m_meshCount; i++) {
		if(m_meshes[i].flag==MESH_OUTPUT) {
			Mesh* mesh = m_meshes[i].mesh;
			mesh->getMaterial().bind();
			
			//transform - Need to know the joint this mesh is attached to.
			if(m_skeleton && m_meshes[i].joint) joint = m_skeleton->getJoint( m_meshes[i].joint ); else joint=0;
			if(joint) {
				glPushMatrix();
				glMultMatrixf( joint->getTransformation() );
			}
			
			drawMesh(mesh, flags);
			
			if(joint) glPopMatrix();
		}
	}
	
	if(flags&1) glDisableClientState(GL_VERTEX_ARRAY);
	if(flags&2) glDisableClientState(GL_NORMAL_ARRAY);
	if(flags&4) glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

void Model::drawMesh(const Mesh* mesh) {
	int flags=0;
	drawMesh(mesh, flags);
	//disable client states afterwards
	if(flags&1) glDisableClientState(GL_VERTEX_ARRAY);
	if(flags&2) glDisableClientState(GL_NORMAL_ARRAY);
	if(flags&4) glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

void Model::drawMesh(const Mesh* mesh, int& glFlags) {
	//enable client states
	if((glFlags&1)==0) { glEnableClientState(GL_VERTEX_ARRAY); glFlags |= 1; }
	if((glFlags&2)==0 && mesh->getFormat()!=VERTEX_SIMPLE) { glEnableClientState(GL_NORMAL_ARRAY); glFlags |= 2; }
	if((glFlags&4)==0 && mesh->getFormat()!=VERTEX_SIMPLE) { glEnableClientState(GL_TEXTURE_COORD_ARRAY); glFlags |= 4; }
	//bind material
	const_cast<Mesh*>(mesh)->getMaterial().bind();
	//set pointers
	glVertexPointer(3, GL_FLOAT, mesh->getStride(), mesh->getVertexPointer());
	glNormalPointer(GL_FLOAT, mesh->getStride(), mesh->getNormalPointer());
	glTexCoordPointer(2, GL_FLOAT, mesh->getStride(), mesh->getTexCoordPointer());
	//Tangent pointer
	if(mesh->getTangentPointer()) {
		Shader::current().AttributePointer("tangent", 3, GL_FLOAT, 0, mesh->getStride(), mesh->getTangentPointer());
	}

	//draw it!
	if(mesh->hasIndices()) glDrawElements(mesh->getMode(), mesh->getSize(), GL_UNSIGNED_SHORT, mesh->getIndexPointer() );
	else glDrawArrays(mesh->getMode(), 0, mesh->getSize());
}

void Model::drawSkeleton(const Skeleton* skeleton) {
	if(!skeleton || !skeleton->getJointCount()) return;
	
	//Joint model
	static float vx[18] = { 0,0,0,  0.06f,0.06f,0.1f, 0.06f,-0.06f,0.1f, -0.06f,-0.06f,0.1f, -0.06f,0.06f,0.1f, 0,0,1 };
	static unsigned char ix[12] = { 1,5,2, 1,0,2,  3,5,4, 3,0,4 };
	
	//Set up openGL
	SMaterial().bind();
	glDisable(GL_DEPTH_TEST);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, vx);
	
	for(int i=0; i<skeleton->getJointCount(); i++) {
		const model::Joint* joint = skeleton->getJoint(i);
		glPushMatrix();
		//get the matrix of this joint. matrix is in the model local coordinates. need to transform if we move the model.
		glMultMatrixf( joint->getTransformation() );
		glScalef(joint->getLength(), joint->getLength(), joint->getLength());
		glColor3f(0.4, 0, 1);
		glDrawElements(GL_LINE_LOOP, 12, GL_UNSIGNED_BYTE, ix);
		glPopMatrix();
	}
	
	glDisableClientState(GL_VERTEX_ARRAY);
	glEnable(GL_DEPTH_TEST);
}


