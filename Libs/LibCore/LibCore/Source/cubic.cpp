// Cubic polynomials

#include "cubic.h"


const double Cubic::epsilon = 1.0e-10;/*!
\brief Bicubic interpolation based on four values.
\param a,b,c,d Four input values.
\param x Interpolant.
\author Lois Paulin
*/
double Cubic::Interpolation(const double& x, const double& a, const double& b, const double& c, const double& d)
{
  double a_m1 = ((-0.5 * x + 1.0) * x - 0.5) * x;
  double a_0 = (1.5 * x - 2.5) * x * x + 1.0;
  double a_1 = ((-1.5 * x + 2.0) * x + 0.5) * x;
  double a_2 = (0.5 * x - 0.5) * x * x;

  return a_m1 * a + a_0 * b + a_1 * c + a_2 * d;
}/*!
\brief Solve a normalized cubic equation.

The highest degree coefficient is 1.0.

This function stores the sorted roots in an array and returns the number of roots.

\param y The array of roots.
*/
int Cubic::SolveNormalized(double* y) const
{
  double A2 = c[2] * c[2];
  double Q = (A2 - 3.0 * c[1]) / 9.0;
  double R = (c[2] * (A2 - 4.5 * c[1]) + 13.5 * c[0]) / 27.0;
  double Q3 = Q * Q * Q;
  double R2 = R * R;
  double d = Q3 - R2;
  double an = c[2] / 3.0;

  if (d >= 0.0)
  {
    // Three double roots
    d = R / sqrt(Q3);

    double theta = acos(d) / 3.0;
    double sQ = -2.0 * sqrt(Q);

    y[0] = sQ * cos(theta) - an;
    y[1] = sQ * cos(theta + Math::TwoPiOverThree) - an;
    y[2] = sQ * cos(theta + Math::FourPiOverThree) - an;

    return 3;
  }
  else
  {
    double sQ = pow(sqrt(R2 - Q3) + fabs(R), 1.0 / 3.0);

    if (R < 0)
    {
      y[0] = (sQ + Q / sQ) - an;
    }
    else
    {
      y[0] = -(sQ + Q / sQ) - an;
    }

    return 1;
  }
}