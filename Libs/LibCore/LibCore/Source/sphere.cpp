
// Sphere

#include "sphere.h"

const double Sphere::epsilon = 1.0e-6;

/*!
\class Sphere sphere.h
\brief Spheres.

Spheres are defined by their center and radius. The
squared radius is not stored in the data-structure, and is
computed on the fly whenever needed.

This class implements several high level functions to compute
the minimal bounding sphere of a set of points.
It also implements the Minkowski sum of two spheres.

The sphere-ray intersection functions assume that the ray
is normalized, i.e. has a unit direction vector.

\ingroup KernelGroup

*/

const Sphere Sphere::Null(0.0);

const Sphere Sphere::Infinity(Math::Infinity);

const Sphere Sphere::Unit(1.0);

/*!
\brief Creates a sphere centered at origin with specified radius.
\param r Radius.
*/
Sphere::Sphere(const double& r) :c(Vector::Null), r(r)
{}