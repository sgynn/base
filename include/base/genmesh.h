#ifndef _BASE_GEN_MESH_
#define _BASE_GEN_MESH_

#include "mesh.h"

namespace base {
	namespace model {
		#define DF VERTEX3 | NORMAL

		extern Mesh* createBox     (const vec3& size=vec3(1,1,1), uint format=DF);
		extern Mesh* createSphere  (float radius=1, int seg=12, int div=8, uint format=DF);
		extern Mesh* createCapsule (float radius=1, float length=1, int seg=12, int div=8, uint format=DF);
		extern Mesh* createCylinder(float radius=1, float length=1, int seg=12, uint format=DF);
		extern Mesh* createCone    (float radius=1, float length=1, int seg=12, uint format=DF);
		extern Mesh* createTorus   (float radius=1, float ring=0.25 int seg=12, int div=12, uint format=DF);

		#undef DF
	}
}

#endif

