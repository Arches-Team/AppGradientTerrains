// Circle


#include "circle.h"

/*!
\class Circle2 circle.h
\brief Circles in the plane.

\sa Circle
\ingroup PlanarGroup
*/

const Circle2 Circle2::Unit(1.0);
const Circle2 Circle2::Infinite(Math::Infinity);

/*!
\brief Create a circle.
\param c Center.
\param r Radius.
*/
Circle2::Circle2(const Vector2& c, const double& r) :c(c), r(r)
{}

/*!
\brief Create a circle centered at origin.
\param r Radius.
*/
Circle2::Circle2(const double& r) :Circle2(Vector2::Null, r)
{}/*!
\brief Vertex on the circle.
\param a Angle.
*/
Vector2 Circle2::Vertex(const double& a) const
{
  return c + r * Vector2::Polar(a);
}
