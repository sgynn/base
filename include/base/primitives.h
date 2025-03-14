
#pragma once

#include <base/vec.h>

/*
 * Mesh generators for primitive shapes.
 * Everything uses vertex format: vertex3, normal3, tangent3, uv2
 * Shapes are generated as y-up. Flat shapes are on the xz-plane
 */

namespace base {
	class Mesh;
	extern Mesh* createPlane   (const vec2& size=vec2(1,1));
	extern Mesh* createPlane   (const vec2& size, int divisions);
	extern Mesh* createBox     (const vec3& size=vec3(1,1,1));
	extern Mesh* createSphere  (float radius=0.5, int seg=12, int div=8);
	extern Mesh* createCircle  (float radius=0.5, int seg=12);
	extern Mesh* createIcoSphere(float radius=0.5, int div=1);
	extern Mesh* createCapsule (float radius=0.5, float length=1, int seg=12, int div=8);
	extern Mesh* createCylinder(float radius=0.5, float length=1, int seg=12);
	extern Mesh* createCone    (float radius=0.5, float length=1, int seg=12);
	extern Mesh* createTorus   (float radius=1, float ring=0.25, int seg=12, int div=12);
}

