#include <base/collision.h>
#include <base/mesh.h>

using namespace base;

int base::intersectRayMesh(const vec3& start, const vec3& dir, float radius, const Mesh* mesh, float& t, vec3& normal) {
	if(radius == 0) return intersectRayMesh(start, dir, mesh, t, normal);

	bool result = false;
	float limit = 1e8f;
	float value;
	vec3 n;
	vec3 s;
	vec3 edge;
	const size_t count = mesh->getIndexCount();
	for(size_t i=0; i<count; i+=3) {
		const vec3& a = mesh->getIndexedVertex(i);
		const vec3& b = mesh->getIndexedVertex(i+1);
		const vec3& c = mesh->getIndexedVertex(i+2);
		n = (b-a).cross(c-a);
		if(n.dot(dir)>0) continue;
		n.normalise();
		// Closest point on plane
		s = start - n * radius;
		float dn = n.dot(dir);
		if(dn != 0) {
			value = (n.dot(a) - n.dot(s)) / dn;
			if(value < limit) {
				s += dir * value;
				int k = base::closestPointOnTriangle(s, a, b, c, edge);
				if(k==0 && value >= 0) {
					result =  true;
					normal = n;
					t = limit = value;
				}
				// Trace backwards from potential collision point to hit the sphere
				else if(base::intersectRaySphere(edge, -dir, start, radius, value)) {
					if(value >=0 && value < limit) {
						normal = start + dir * value - edge;
						result = true;
						t = limit = value;
					}
				}
			}
		}
	}
	return result;
}

int base::intersectRayMesh(const vec3& start, const vec3& dir, const Mesh* mesh, float& t, vec3& normal) {
	bool result = false;
	float value;
	float limit = 1e8f;
	const size_t count = mesh->getIndexCount();
	for(size_t i=0; i<count; i+=3) {
		const vec3& a = mesh->getIndexedVertex(i);
		const vec3& b = mesh->getIndexedVertex(i+1);
		const vec3& c = mesh->getIndexedVertex(i+2);
		if(base::intersectRayTriangle(start, dir, a,b,c, value) && value>=0 && value<limit) {
			normal = (b-a).cross(c-a);
			result = true;
			t = limit = value;
		}
	}
	return result;
}

int base::intersectRayMesh(const vec3& start, const vec3& dir, const Mesh* mesh, float& t) {
	vec3 ignore;
	return intersectRayMesh(start, dir, mesh, t, ignore);
}


