// Box


#include "box.h"

#include "segment.h"

/*!
\class Box2 box.h
\brief %Axis aligned box in the plane.

\sa Box
\ingroup PlanarGroup
*/

const double Box2::epsilon = 1.0e-5; //!< Epsilon value used to check intersections and some round off errors.

const Box2 Box2::Infinity(Vector2(-Math::Infinity), Vector2(Math::Infinity)); //!< Huge bounding box, which should enclose any other 

const Box2 Box2::Null(0.0); //!< Null box, equivalent to \code Box2(Vector2(0.0)); \endcode

const Box2 Box2::Unit(Vector2(0.0), Vector2(1.0)); //!< Unit box, defined as \code Box2(Vector2(0.0),Vector2(1.0)); \endcode

const int Box2::edge[8] =
{
  0,1,2,3,0,2,1,3
};/*!
\brief Extend the limits of the box by a given distance.

Note that this is the same as performing the Minkowski sum with a cubic box of size r.
\param r Range.
*/
Box2 Box2::Extended(const double& r) const
{
  return Box2(a - Vector2(r), b + Vector2(r));
}
/*!
\brief Compute the horizontal or vertical segment dividing the box into u and 1-u parts.
\param u Real.
\param horizontal Boolean, set to true for horizontal split, false for vertical.
*/
Segment2 Box2::GetSegment(const double& u, bool horizontal) const
{
  if (horizontal == true)
  {
    double y = Math::Lerp(a[1], b[1], u);
    return Segment2(Vector2(a[0], y), Vector2(b[0], y));
  }
  else
  {
    double x = Math::Lerp(a[0], b[0], u);
    return Segment2(Vector2(x, a[1]), Vector2(x, b[1]));
  }
}

/*!
\brief Test if a point is inside the box.
\param p Point.
*/
bool Box2::Inside(const Vector2& p) const
{
  if ((p[0] < a[0]) || (p[0] > b[0]) || (p[1] < a[1]) || (p[1] > b[1]))
    return false;
  else
    return true;
}/*!
\brief Convert a planar Box2 to a Box.
\param a,b Lower and upper coordinates (note that a should be smaller than b).
*/
Box Box2::ToBox(const double& a, const double& b) const
{
  return Box(Box2::a.ToVector(a), Box2::b.ToVector(b));
}/*!
\brief Create the Qt rectangle.
*/
QRectF Box2::GetQtRect() const
{
  return QRectF(a[0], a[1], b[0] - a[0], b[1] - a[1]);
}