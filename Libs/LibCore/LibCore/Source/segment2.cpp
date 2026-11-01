// Segments


#include "segment.h"
/*!
\brief Compute the squared distance to the axis edge characterized by its end points.
\param p Point.
\param s Linear coefficient corresponding to the projection of argument point p onto the edge.
*/
double Segment2::R(const Vector2& p, double& s) const
{
  Vector2 n = p - b;
  // Do not normalize there
  Vector2 axis = b - a;
  s = n * axis;

  // Squared distance 
  double r = n * n;

  // Not vertex b
  if (s < 0.0)
  {
    n = p - a;
    s = n * axis;
    r = n * n;
    // Not vertex a
    if (s > 0.0)
    {
      double length = axis * axis;
      s = s * s / length;
      r -= s; // Pythagore
      s = sqrt(s / length);
      // Added an absolute value as imprecisions may cause rounding errors
      r = fabs(r);
    }
    else
    {
      s = 0.0;
    }
  }
  else
  {
    s = 1.0;
  }
  return r;
}