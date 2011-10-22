#ifndef _COLLISION_
#define _COLLISIOM_

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
	 * @param a1 Start of first line
	 * @param b1 End of forst line
	 * @param a2 Start of second line
	 * @param b2 End of second line
	 * @param p1 Output: Point on first line that is closest to the second line
	 * @param p2 Output: Point on second line that is closest to the first line
	 * @return Squared distance between the two output points
	 * */
	BASEAPI float closestPointBetweenLines(const vec3& a1, const vec3& b1, const vec3& a2, const vec3& b2, vec3& p1, vec3& p2);

	/** Intersection point between a ray and a plane
	 * @param p Start of ray
	 * @param d Normalised ray direction
	 * @param normal Plane normal
	 * @param offset Plane offset
	 * @param out Output: intersection point
	 * @return if intersects
	 */
	BASEAPI int intersectRayPlane(const vec3& p, const vec3& d,  const vec3& normal, float offset, vec3& result);
	/** Intersection between a ray and a sphere
	 * @param p Start of ray
	 * @param d Normalised ray direction
	 * @param centre Sphere centre point
	 * @param radius Sphere radius
	 * @param out Intersection point
	 * @return if intersects
	 */
	BASEAPI int intersectRaySphere(const vec3& p, const vec3& d, const vec3& centre, float radius, vec3& out);

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
	 * @param as Start of first line
	 * @param ad Direction vector of first line (end-start)
	 * @param bs Start of second line
	 * @param bd Direction of second line
	 * @param out Output: Intersection point
	 * @return if the lines intersect
	 */
	BASEAPI int intersectLines(const vec2& as, const vec2& ad, const vec2& bs, const vec2& bd, vec2& out);

};
#endif

