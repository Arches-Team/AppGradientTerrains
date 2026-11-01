// Curves 

#include "curve.h"


/*!
\class QuadricCurve2 curve.h
\brief %Quadric curves in the plane.

Quadric curves are implemented with two quadric polynomials defining the
coordinates of the points on the curve. It is a minimum storage class in
the sense that it does not keep extra constant data which may be useful
for computing the bounding box of the quadric curve parameterized over
the unit interval [0,1] or the distance between a point an the curve.

\ingroup PlanarGroup

*/

/*!
\brief Creates a quadric curve.
\param x, y Quadric parametric equations of the corresponding x, y coordinates.
*/
QuadricCurve2::QuadricCurve2(const Quadric& x, const Quadric& y) :x(x), y(y)
{}/*!
\brief Computes the point on the curve.
\param t Parameter.
*/
Vector2 QuadricCurve2::operator() (const double& t) const
{
  return Vector2(x(t), y(t));
}

/*!
\brief Computes the point on the curve.
\param t Parameter.
*/
Vector2 QuadricCurve2::Eval(const double& t) const
{
  return Vector2(x(t), y(t));
}/*!
\brief Computes the tangent to the curve.
\param t Parameter.
*/
Vector2 QuadricCurve2::Tangent(const double& t) const
{
  return Vector2(2.0 * x[2] * t + x[1], 2.0 * y[2] * t + y[1]);
}

/*!
\brief Computes the normal vector to the quadric curve at a given position on the curve.

The normal is the obtained by evaluating the second derivative.
\param t Position (not used).
*/
#pragma warning(push)
#pragma warning(disable: 4100)  
#pragma warning(pop)
/*!
\brief Creates a quadratic Bezier curve on interval [0,1].
\param a,b,c The control points of the Bezier curve.
*/
QuadricCurve2 QuadricCurve2::Bezier(const Vector2& a, const Vector2& b, const Vector2& c)
{
  return QuadricCurve2(Quadric::Bezier(a[0], b[0], c[0]), Quadric::Bezier(a[1], b[1], c[1]));
}/*!
\brief Compute the bounding box of the quadric curve with parameter interval [0,1].
*/
Box2 QuadricCurve2::GetBox() const
{
  Box2 box;

  // Define bounding box
  x.Range(box[0][0], box[1][0]);
  y.Range(box[0][1], box[1][1]);

  return box;
}

/*!
\brief Compute the squared distance between a point and the quadric curve.

This function computes the projection of the point p onto
the curve by minimizing the distance function along the curve,
expressed as a cubic polynomial.

Most of the coefficients of this cubic polynomial are constant,
and could be pre-processed in the constructor.

\warn This function does not work if the quadric degenerates to a linear curve.

\param p Vertex.
\param u Parameter defining the coordinate of the projection of the argument vertex onto the curve.
*/
double QuadricCurve2::R(const Vector2& p, double& u) const
{
  double xy[3];
  double aa[3];

  // Quadric constant coefficients
  xy[2] = 2.0 * (x[2] * x[2] + y[2] * y[2]);
  xy[1] = 3.0 * (x[2] * x[1] + y[2] * y[1]);
  xy[0] = x[1] * x[1] + y[1] * y[1];

  // Normalize by highest coefficient
  xy[1] /= xy[2];
  xy[0] /= xy[2];

  aa[0] = xy[1] * xy[1];
  aa[1] = 2.0 / xy[2];
  aa[2] = 1.0 / xy[2];

  double t[3];

  // Normalized form
  Cubic c(
    1.0,
    xy[1],
    xy[0] + aa[1] * (x[2] * (x[0] - p[0]) + y[2] * (y[0] - p[1])),
    aa[2] * ((x[0] - p[0]) * x[1] + (y[0] - p[1]) * y[1]));

  int k = c.SolveNormalized(t);

  // Search at caps
  Vector2 e = p - Eval(0.0);
  double r = e * e;

  Vector2 f = p - Eval(1.0);
  double s = f * f;

  u = 0.0;
  if (s < r)
  {
    r = s;
    u = 1.0;
  }

  for (int i = 0; i < k; i++)
  {
    if ((t[i] > 0.0) && (t[i] < 1.0))
    {
      f = p - Vector2(x(t[i]), y(t[i]));
      s = f * f;
      if (s < r)
      {
        u = t[i];
        r = s;
      }
    }
  }
  return r;
}

/*!
\brief Compute the curvilign absisca of a point on the curve.

\param t Parameter.
\param n Discretization of the integration interval.
*/
double QuadricCurve2::S(const double& t, int n) const
{
  const double epsilon = t / n;
  double s = 0.0;
  Vector2 a = Vector2(x(0.0), y(0.0));
  for (int i = 1; i <= n; i++)
  {
    const double u = i * epsilon;
    Vector2 b = Vector2(x(u), y(u));
    s += Norm(b - a);
    a = b;
  }
  return s;
}

/*!
\brief Compute the parameter corresponding to the argument curvilign absisca of a point on the curve.

\param s Argument curvilign absisca (should be between 0.0 and the length of the curve).
\param n Discretization of the integration interval.
*/
double QuadricCurve2::U(const double& s, int n) const
{
  if (s == 0.0) return 0.0;
  const double epsilon = 1.0 / n;
  double ss = 0.0;
  Vector2 a = Vector2(x(0.0), y(0.0));
  for (double u = epsilon; u < 1.0; u += epsilon)
  {
    Vector2 b = Vector2(x(u), y(u));
    ss += Norm(b - a);
    if (ss > s)
    {
      return u - epsilon / 2.0;
    }
    a = b;
  }
  return 1.0;
}/*!
\brief Draw a quadric curve.
\param scene Graphics scene.
\param pen The pen.
\param brush Not used, the curve is drawn as a cubic Bezier path with the pen only.
*/
#pragma warning(push)
#pragma warning(disable: 4100)  
#pragma warning(pop)
