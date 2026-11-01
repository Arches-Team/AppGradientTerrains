// Plane

#include "plane.h"

/*!
\class Plane plane.h
\brief A plane defined by its analytic equation.

This class serves as a base core class for binary space patitionning
or constructive solid geometry in ray-tracing.

\ingroup KernelGroup

*/

const double Plane::epsilon = 1e-06;

const Plane Plane::XY(Vector(0.0, 0.0, 1.0), 0.0);
const Plane Plane::YZ(Vector(1.0, 0.0, 0.0), 0.0);
const Plane Plane::ZX(Vector(0.0, 1.0, 0.0), 0.0);

/*!
\brief Creates a plane given normal and vertex.

\param n %Plane normal, should be unit.
\param p Any point on the plane.
*/
Plane::Plane(const Vector& n, const Vector& p) :n(n), cqc(p* n)
{}/*!
\brief Compute the intersection between a plane and a ray.

The intersection depth is returned if intersection occurs.
\param ray The ray.
\param t Intersection depth.
*/
bool Plane::Intersect(const Ray& ray, double& t) const
{
  // Check if parallel
  double x = n * ray.Direction();

  if ((x < epsilon) && (x > -epsilon))
    return false;

  double y = cqc - n * ray.Origin();

  // Depth
  t = y / x;

  return true;
}
