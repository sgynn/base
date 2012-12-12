#ifndef _BASE_COLLISION_
#define _BASE_COLLISIOM_

#include "math.h"

#define BASEAPI extern

// Collision functions. Add to as needed.

namespace base {

	/** Get the closest point on a line to a point
	 * @param point The point to test
	 * @param a Line start point
	 * @param b Line end point
	 * @return Closest point
	 * */
	BASEAPI vec3 closestPointOnLine(const vec3& point, const vec3& a, const vec3& b);

	/** Get the point where a pait=r of 3D lines are closest
	 * @param a0 Start of first line
	 * @param a1 End of forst line
	 * @param b0 Start of second line
	 * @param b1 End of second line
	 * @param p0 Output: Point on first line that is closest to the second line
	 * @param p1 Output: Point on second line that is closest to the first line
	 * @return Squared distance between the two output points
	 * */
	BASEAPI float closestPointBetweenLines(const vec3& a0, const vec3& a1, const vec3& b0, const vec3& b1, vec3& pa, vec3& pb);
	BASEAPI float closestPointBetweenLines(const vec3& a0, const vec3& a1, const vec3& b0, const vec3& b1, float& u, float& v);

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

	/** Get the intersection point of two 2D lines
	 * @param a0 Start of first line
	 * @param a1 End of first line
	 * @param b0 Start of second line
	 * @param b1 End of second line
	 * @param out Output: Intersection point
	 * @return if the lines intersect
	 */
	BASEAPI int intersectLines(const vec2& a0, const vec2& a1, const vec2& b0, const vec2& b1, vec2& out);
	BASEAPI int intersectLines(const vec2& a0, const vec2& a1, const vec2& b0, const vec2& b1, float& u, float& v);

};
#endif

