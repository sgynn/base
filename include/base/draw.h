#ifndef _BASE_DRAW_
#define _BASE_DRAW_

#include "math.h"
#include "opengl.h"

/* A static class for drawing a few primitives */
class Draw {
	public:
	/** Draw a line to the current OpenGL context */
	static void Line(const vec3& start, const vec3& end, const Colour& colour=white, float width=1) {
		if(width!=1) glLineWidth(width);
		glColor4fv(colour);
		glBegin(GL_LINES);
		glVertex3fv(&start.x);
		glVertex3fv(&end.x);
		glEnd();
		if(width!=1) glLineWidth(1);
	}
	/** Draw a point marker (intersecting lines) */
	static void Marker(const vec3& point, const Colour& colour=white, float size=1, float width=1) {
		if(width!=1) glLineWidth(width);
		glColor4fv(colour);
		glBegin(GL_LINES);
		glVertex3f(point.x+size, point.y, point.z);
		glVertex3f(point.x-size, point.y, point.z);
		glVertex3f(point.x, point.y+size, point.z);
		glVertex3f(point.x, point.y-size, point.z);
		glVertex3f(point.x, point.y, point.z+size);
		glVertex3f(point.x, point.y, point.z-size);
		glEnd();
		if(width!=1) glLineWidth(1);
	}
	/** Draw a point */
	static void Point(const vec3& point, const Colour& colour=white, float size=1) {
		if(size!=1) glPointSize(size);
		glColor4fv(colour);
		glBegin(GL_POINTS);
		glVertex3fv(&point.x);
		glEnd();
	}
	/** Draw a circle */
	static void Circle(const vec3& point, float radius=1, int segments=32, const vec3& normal=vec3(0,1,0), const Colour& colour=white) {
		static vec3* cp = 0;
		static int lseg = 0;
		static const vec3 vx(1,0,0), vz(0,0,1); //vectors to calculate orthogonal vectors
		//Recalculate vertices
		if(segments!=lseg) {
			lseg=segments;
			if(cp) delete [] cp;
			cp = new vec3[ segments ];
			float step = 6.283185f/segments;
			for(int i=0; i<segments; i++) cp[i] = vec3(sin(i*step),0,cos(i*step));
		}
		//Matrix
		vec3 a = normal.cross(normal.z>normal.x?vx:vz).normalise();
		vec3 b = a.cross(normal).normalise();
		Matrix mat(a, normal, b, point);
		glPushMatrix();
		glMultMatrixf(mat);
		glScalef(radius, radius, radius);
		//draw array
		glColor4fv(colour);
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3,GL_FLOAT,0,cp);
		glDrawArrays(GL_LINE_LOOP, 0, segments);
		glDisableClientState(GL_VERTEX_ARRAY);
		glPopMatrix();
	}
	/** Draw a wireframe AABB */
	static void Box(const aabb& box, const Colour& colour=blue) {
		static const unsigned char ix[16] = {0,1,3,2,0, 4,5,7,6,4, 5,1,3,7,6,2 };
		vec3 p[8];
		for(int i=0; i<8; i++) {
			p[i].x = i&1? box.max.x: box.min.x;
			p[i].y = i&2? box.max.y: box.min.y;
			p[i].z = i&4? box.max.z: box.min.z;
		}
		glColor4fv(colour);
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3,GL_FLOAT,0,p);
		glDrawElements(GL_LINE_STRIP, 16, GL_UNSIGNED_BYTE, ix);
		glDisableClientState(GL_VERTEX_ARRAY);
	}
	
};

#endif

