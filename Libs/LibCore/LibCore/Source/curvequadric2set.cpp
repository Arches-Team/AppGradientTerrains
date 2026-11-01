// Curves

#include "curveset.h"

#include <QtCore/QVarLengthArray>
/*!
\brief Creates the piecewise quadric curve given a set of control points.
\param p Control vertices.
*/
QuadricCurve2Set::QuadricCurve2Set(const QVector<Vector2>& p)
{
  int n = p.size();

  if (n > 2)
  {
    QVector<Vector2> knot;
    knot.clear();

    // Starting vertices
    knot.append(p[0]);
    knot.append(p[1]);

    // Mid vertices
    for (int i = 1; i < n - 2; i++)
    {
      knot.append(0.5 * (p[i] + p[i + 1]));
      knot.append(p[i + 1]);
    }

    // End vertex
    knot.append(p[n - 1]);

    curve.clear();
    curve.resize(n - 2);

    for (int i = 0; i < n - 2; i++)
    {
      curve[i] = QuadricCurve2::Bezier(knot[2 * i], knot[2 * i + 1], knot[2 * i + 2]);
    }

    qlengths.clear();
    qlengths.resize(curve.size());
    qqlength = 0.0;
    for (int i = 0; i < curve.size(); i++)
    {
      qlengths[i] = curve[i].S(1.0, 100);
      qqlength += qlengths[i];
    }
  }
  else if (n == 1)
  {
    curve.append(QuadricCurve2(Quadric(p[0][0], 0.0, 0.0), Quadric(p[0][1], 0.0, 0.0)));

  }
  else if (n == 2)
  {
    Vector2 d = p[1] - p[0];
    curve.append(QuadricCurve2(Quadric(p[0][0], d[0], 0.0), Quadric(p[0][1], d[1], 0.0)));
  }
  UpdateBoxes();
}/*!
\brief Destroys a piecewise quadric curve.
*/
QuadricCurve2Set::~QuadricCurve2Set()
{}

/*!
\brief Recompute the bounding box of every curve, used by UV() to skip curves that cannot be the closest.

Must be called by every function that modifies the set of curves.

The boxes are enlarged by a small margin so that they are guaranteed to contain their curve:
Quadric::Range() ignores the extremum of a quadric whose second order coefficient is
below Quadric::epsilon, so the exact box of a nearly degenerate curve can be off by
about that much, and the distance to a box is computed with its own rounding.
*/
void QuadricCurve2Set::UpdateBoxes()
{
  boxes.resize(curve.size());
  for (int i = 0; i < curve.size(); i++)
  {
    boxes[i] = curve.at(i).GetBox().Extended(1.0e-6);
  }
}/*!
\brief Compute the parameter of the curve corresponding to the input length.
\param s Input length.
\param u Parameter of the i-th curve.
\return Identifier of the curve in the piecewise definition.
*/
int QuadricCurve2Set::U(const double& s, double& u) const
{
  if ((s > qqlength) || (s < 0.0))
  {
    u = 0.0;
    return -1;
  }

  double l = 0.0;
  for (int i = 0; i < curve.size(); i++)
  {
    // We found the right interval
    if (s < l + qlengths[i])
    {
      // Find the right location
      u = curve[i].U(s - l, 256);
      return i;
    }
    // Increment the length
    l += qlengths[i];
  }

  // Extreme case : we reached the end of the curve (in fact we should almost never get there)
  u = 1.0;
  return curve.size() - 1;
}/*!
\brief This function returns the squared distance between
a point and a set of quadric curves.

\sa QuadicCurve::R()

\param p Point.
\param u Parameter defining the coordinate of the projection of the argument vertex onto the curve.
\param k Index of the curve for which the minimum distance was found.
*/
double QuadricCurve2Set::R(const Vector2& p, double& u, int& k) const
{
  if (curve.size() == 0)
    return 0.0;

  k = 0;
  double a = curve.at(0).R(p, u);
  for (int i = 1; i < curve.size(); i++)
  {
    double v;
    double b = curve.at(i).R(p, v);

    if (b < a)
    {
      a = b;
      u = v;
      k = i;
    }
  }
  return a;
}