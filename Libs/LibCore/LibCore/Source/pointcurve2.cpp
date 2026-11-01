// Curves 

#include "curvepoint.h"
#include "segment.h"
/*!
\brief Creates a curve with points.

\param p Set of points.
\param closed Flag defining whether the curve is closed (true) or open (false).
*/
PointCurve2::PointCurve2(const QVector<Vector2>& p, bool closed) :VectorSet2(p), closed(closed)
{
}/*!
\brief Computes the projection of p onto the piecewise linear curve and returns the squared distance.
\param p Point.
\param u Parameter defining the coordinate of the projection of the argument vertex onto the curve.
\param k Index of the curve for which the minimum distance was found.
*/
double PointCurve2::R(const Vector2& p, double& u, int& k) const
{
  if (v.size() == 0)
    return 0.0;

  k = 0;
  Vector2 a = v.at(0);
  double d = SquaredNorm(a - p);
  for (int i = 1; i < v.size(); i++)
  {
    const Vector2 b = v.at(i);
    const Segment2 ab(a, b);
    double s;
    double di = ab.R(p, s);
    if (di < d)
    {
      d = di;
      u = s;
      k = i - 1;
    }
    a = b;
  }
  return d;
}

/*!
\brief Computes the projection of p onto the piecewise linear curve, returns the squared distance, and the side with respect to the curve
\param p Point.
\param u Parameter defining the coordinate of the projection of the argument vertex onto the curve.
\param k Index of the curve for which the minimum distance was found.
\param side Side of the point with respect to the closest segment (1 or -1)
*/
double PointCurve2::R(const Vector2& p, double& u, int& k, int& side) const
{
  if (v.size() == 0)
    return 0.0;

  k = 0;
  Vector2 a = v.at(0);
  double d = SquaredNorm(a - p);
  for (int i = 1; i < v.size(); i++)
  {
    const Vector2 b = v.at(i);
    const Segment2 ab(a, b);
    double s;
    double di = ab.R(p, s);
    if (di < d)
    {
      d = di;
      u = s;
      k = i - 1;
      if ((p - a) / (b - a) > 0.0)
        side = 1;
      else
        side = -1;
    }
    a = b;
  }
  return d;
}