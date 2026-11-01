// Box

// Self include
#include "box.h"

/*!
\page RefFaq Frequently Asked Questions

\section FaqGeometry Geometry
<P><I>How do I compute the area or the volume of a geometric primitive?</I>
<BR>In general, create an instance of the geometric shape, and use aither the member functions Area() or Volume().
In some cases, static member functions have been implemented to avoid instanciation,
for instance the area of a circle can be computed directly without creating a Circle:
\code
double a = Circle::Area(r);
\endcode
<P><I>How do I compute the bounding box of a given simple geometric primitive?</I>
Most primitives have a corresponding member function. For example, see:
\code
Box Cone::GetBox() const;
\endcode
In general those member functions do not yield the tightest bounding box, but a simple to compute and efficient one. See the details of the corresponding member function.
<P><I>How do I compute the distance from a point to a geometric primitive?</I>
<BR>Most geometric primitives, including Box, Sphere, Triangle, Cylinder, Circle or
even QuadricCurve implement a specific member function.
For example, see:
\code
double Box::R(const Vector&) const;
\endcode
In general member functions compute the squared distance out of efficiency.
*/

/*!
\mainpage CoreLib
<center>
\image html core-small.png
<P>Latest stable version : 2024.01.12
</center>
<h3>License</h3>
Copyright &copy; by
<a href="mailto:eric.galin@liris.cnrs.fr">Eric Galin</a>.
No representations are made about the suitability of this software
for any purpose. It is provided as is without express or implied
warranty.

<h3>Features overview</h3>
The Core library implements several classes that are often used in graphical applications. The list includes four kinds
of classes : \ref Math such as vector, polynomial and matrix classes, \ref KernelGroup  three
dimensional geometric objects (such as boxes, triangles, spheres), \ref PlanarGroup that are two dimensional classes, frame management
classes, \ref ColorGroup classes and some specific classes involved in many geometry applications (such polytopes, k-polytopes or slabs).
*/

/*!
\defgroup KernelGroup Core geometric classes

\brief Core geometric classes include several core classes such as Box, Axis, Cylinder,
Cone, Plane and many others that are useful in many graphic applications.

Most of the time, those classes have been implemented so as to save memory.
For example, the Sphere class only stores the center and the radius, but does
not store the squared radius although this value is often needed in many algorithms
such as intersection with a ray, or point membership classification. The very reason
for this is that in that case only one multiply is needed to compute the squared radius.

Some exceptions to this rule include the implementation of the class Axis, which is used
for implementing many other classes such as the Cone or the Cylinder. This class not only
stores the end points of the axis, but also the normalized axis vector and the length of
the axis. See the details of the Axis class for further details.
*/

/*!
\defgroup ColorGroup %Color classes

\brief %Color classes include several color manipulation classes such as Color, Palette.
*/

/*!
\defgroup SystemCoreGroup Generic core classes.

\brief Generic core classes include System, Hashing and other classes.
*/


/*!
\defgroup PlanarGroup Planar geometric classes

\brief Planar core geometric classes are a subset of core classes that implement objects in the plane.

Several types of objects are implemented both in the plane and in space, such as Disc or Disc2.
*/

/*!
\defgroup StructureGroup Geometric structures

\brief Geometric structures implement several simple generic structures such as
two-dimensionnal arrays of vectors or reals.
*/

/*!
\class Box box.h
\brief An axis aligned box.

The class stores the opposite two corners as vectors.
The center and the radius (diagonal vector) are computed on the
fly by inline functions.

\image html box.png

The vertices of a box can be obtained by the Box::Vertex()
member function which returns one of the eight vertices of the box.
The two opposite corners can be obtained faster as follows:
\code
Box box(Vector(0.0,0.0,0.0),Vector(1.0,1.0,1.0)); // Unit box
Vector a=box[0]; // Lower vertex
Vector b=box[1]; // Opposite vertex
\endcode

This class provides a set of useful functions, such as the intersection
between a box and a ray. This class also implements the Minkowski sum
of boxes by overloading some operators.

Note that some intersection methods involving boxes may be implemented in other classes,
for instance Segment::Intersect(const Box&) const.

\ingroup KernelGroup

*/

const Box Box::Infinity(Math::Infinity); //!< Huge bounding box, which should enclose any other.

const Box Box::Null(0.0); //!< Null box, equivalent to: \code Box(Vector(0.0)); \endcode 

const Box Box::Unit(Vector(0.0), Vector(1.0)); //!< Unit box.

const int Box::edge[24] =
{
  0,1,2,3,4,5,6,7,
  0,2,1,3,4,6,5,7,
  0,4,1,5,2,6,3,7
};

const Vector Box::normal[6] =
{
  Vector(-1.0,0.0,0.0),
  Vector(0.0,-1.0,0.0),
  Vector(0.0,0.0,-1.0),
  Vector(1.0,0.0,0.0),
  Vector(0.0, 1.0,0.0),
  Vector(0.0,0.0,1.0)
};/*!
\brief Create a box given two opposite corners.

Note that this constructor does not check the coordinates of the two vectors.
Therefore, the coordinates of a should be lower than those of b.

To create the axis aligned bounding box of two vectors a and b in
the general case, one should use:
\code
Box box(Vector::Min(a,b),Vector::Max(a,b));
\endcode
\param a,b End vertices.
*/
Box::Box(const Vector& a, const Vector& b) :a(a), b(b)
{}

/*!
\brief Create an empty box given one vertex.
\param v Vertex.
*/
Box::Box(const Vector& v) :a(v), b(v)
{}

/*!
\brief Create a cube centered at the origin and of given half side length.

This is equivalent to:
\code
Box box(Vector(0.0),2.0);  // Simplified constructor: Box(2.0);
\endcode
\param r Half side length.
*/
Box::Box(const double& r)
{
  a = -Vector(r);
  b = Vector(r);
}/*!
\brief Computes the intersection between a box and a ray.

Sorted intersection depths are returned if intersection occurs.
\param ray The ray
\param tmin, tmax Intersection depths
*/
int Box::Intersect(const Ray& ray, double& tmin, double& tmax) const
{
  tmin = -1e16;
  tmax = 1e16;

  Vector p = ray.Origin();
  Vector d = ray.Direction();

  double t;
  // Ox
  if (d[0] < -epsilon)
  {
    t = (a[0] - p[0]) / d[0];
    if (t < tmin)
      return 0;
    if (t <= tmax)
      tmax = t;
    t = (b[0] - p[0]) / d[0];
    if (t >= tmin)
    {
      if (t > tmax)
        return 0;
      tmin = t;
    }
  }
  else if (d[0] > epsilon)
  {
    t = (b[0] - p[0]) / d[0];
    if (t < tmin)
      return 0;
    if (t <= tmax)
      tmax = t;
    t = (a[0] - p[0]) / d[0];
    if (t >= tmin)
    {
      if (t > tmax)
        return 0;
      tmin = t;
    }
  }
  else if (p[0]<a[0] || p[0]>b[0])
    return 0;

  // Oy
  if (d[1] < -epsilon)
  {
    t = (a[1] - p[1]) / d[1];
    if (t < tmin)
      return 0;
    if (t <= tmax)
      tmax = t;
    t = (b[1] - p[1]) / d[1];
    if (t >= tmin)
    {
      if (t > tmax)
        return 0;
      tmin = t;
    }
  }
  else if (d[1] > epsilon)
  {
    t = (b[1] - p[1]) / d[1];
    if (t < tmin)
      return 0;
    if (t <= tmax)
      tmax = t;
    t = (a[1] - p[1]) / d[1];
    if (t >= tmin)
    {
      if (t > tmax)
        return 0;
      tmin = t;
    }
  }
  else if (p[1]<a[1] || p[1]>b[1])
    return 0;

  // Oz
  if (d[2] < -epsilon)
  {
    t = (a[2] - p[2]) / d[2];
    if (t < tmin)
      return 0;
    if (t <= tmax)
      tmax = t;
    t = (b[2] - p[2]) / d[2];
    if (t >= tmin)
    {
      if (t > tmax)
        return 0;
      tmin = t;
    }
  }
  else if (d[2] > epsilon)
  {
    t = (b[2] - p[2]) / d[2];
    if (t < tmin)
      return 0;
    if (t <= tmax)
      tmax = t;
    t = (a[2] - p[2]) / d[2];
    if (t >= tmin)
    {
      if (t > tmax)
        return 0;
      tmin = t;
    }
  }
  else if (p[2]<a[2] || p[2]>b[2])
    return 0;

  return 1;
}/*!
\brief Extend the limits of the box given a point.

If the point lies inside the box, the vertices of the box are unchanged.
\param p Point.
*/
void Box::Extend(const Vector& p)
{
  a = Vector::Min(a, p);
  b = Vector::Max(b, p);
}/*!
\brief Overloaded.
\param s Stream.
\param box The box.
*/
std::ostream& operator<<(std::ostream& s, const Box& box)
{
  s << "Box(" << box.a << ',' << box.b << ")";
  return s;
}