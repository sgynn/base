#ifndef _BASE_COLLISION_
#define _BASE_COLLISIOM_

#include "math.h"

#define BASEAPI extern

// Collision functions. Add to as needed.

namespace base {

	/** Get the closest point on a line to a point
	 * @param point The point to test
	 * @param p Line start point
	 * @param q Line end point
	 * @return Closest point
	 * */
	BASEAPI vec3 closestPointOnLine(const vec3& point, const vec3& p, const vec3& q);

	/** Get the point where a pait=r of 3D lines are closest
	 * @param p0 Start of first line
	 * @param q0 End of forst line
	 * @param p1 Start of second line
	 * @param q1 End of second line
	 * @param c0 Output: Point on first line that is closest to the second line
	 * @param c1 Output: Point on second line that is closest to the first line
	 * @return Squared distance between the two output points
	 * */
	BASEAPI float closestPointBetweenLines(const vec3& p0, const vec3& q0, const vec3& p1, const vec3& q1, vec3& c0, vec3& c1);
	BASEAPI float closestPointBetweenLines(const vec3& p0, const vec3& q0, const vec3& p1, const vec3& q1, float& u, float& v);

	/** Intersection point between a ray and a plane
	 * @param p Start of ray
	 * @param d Normalised ray direction
	 * @param normal Plane normal
	 * @param offset Plane offset
	 * @param out Output: intersection point
	 * @return if intersects
	 */
	BASEAPI int intersectRayPlane(const vec3& p, const vec3& d, const vec3& normal, float offset, vec3& result);
	BASEAPI int intersectRayPlane(const vec3& p, const vec3& d, const vec3& normal, float offset, float& t);

	/** Intersection between a ray and a sphere
	 * @param p Start of ray
	 * @param d Normalised ray direction
	 * @param centre Sphere centre point
	 * @param radius Sphere radius
	 * @param out Intersection point
	 * @return if intersects
	 */
	BASEAPI int intersectRaySphere(const vec3& p, const vec3& d, const vec3& centre, float radius, vec3& out);
	BASEAPI int intersectRaySphere(const vec3& p, const vec3& d, const vec3& centre, float radius, float& t);

	/** Intersection of a line and a triangle
	 * @param p Start of line
	 * @param q End of line
	 * @param a Triangle point
	 * @param b Triangle point
	 * @param c Triangle point
	 * @param out Output: Intersection point
	 * @return if intersects
	 */
	BASEAPI int intersectLineTriangle(const vec3& p, const vec3& q, const vec3& a, const vec3& b, const vec3& c, vec3& out);
	/** Same as intersectLineTriangle but gets barycentric coordinates */
	BASEAPI int intersectLineTriangleb(const vec3& p, const vec3& q, const vec3& a, const vec3& b, const vec3& c, float* barycentric);

	/** Get the intersection point of two 2D lines
	 * @param p0 Start of first line
	 * @param q0 End of first line
	 * @param p1 Start of second line
	 * @param q1 End of second line
	 * @param out Output: Intersection point
	 * @return if the lines intersect
	 */
	BASEAPI int intersectLines(const vec2& p0, const vec2& q0, const vec2& p1, const vec2& q1, vec2& out);
	BASEAPI int intersectLines(const vec2& p0, const vec2& q0, const vec2& p1, const vec2& q1, float& u, float& v);

};
#endif

