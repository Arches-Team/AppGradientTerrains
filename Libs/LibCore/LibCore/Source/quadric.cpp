// Quadric polynomials

#include "quadric.h"

const double Quadric::epsilon = 1.0e-10;

/*!
\class Quadric quadric.h
\brief %Quadric polynomials.

Constructors should provide the coefficients in descending order.
Here is an example of how to code the
quadric \f$ 2 x^2 - x + 1 \f$:
\code
Quadric p(2.0,-1.0,1.0);
\endcode

\ingroup MathGroup
*/

/*!
\brief This function computes the range of values taken by a quadric over a given interval.

Computes the roots of the first derivative, and evaluates the polynomial
at the roots if they are within the interval bounds.

\param a,b Interval.
\param x,y Returned range.
*/
void Quadric::Range(double& x, double& y, const double& a, const double& b) const
{
  x = (*this)(a);
  y = (*this)(b);

  Math::Sort(x, y);

  if (fabs(c[2]) < Quadric::epsilon)
    return;

  double t = -c[1] / (c[2] + c[2]);

  if ((t > a) && (t < b))
  {
    double s = (*this)(t);
    if (s < x)
    {
      x = s;
    }
    else if (s > y)
    {
      y = s;
    }
  }
}/*!
\brief Creates a quadratic Bezier polynomial on interval [0,1].
\param a,b,c The parameters of the B�zier polynomial.

Parameters are as follows:

\f$ c(t)=a (1-t)^2+2b t(1-t)+c t^2=(a-2b+c) t^2+2(b-a) t+a \f$.
*/
Quadric Quadric::Bezier(const double& a, const double& b, const double& c)
{
  return Quadric(a - 2.0 * b + c, 2.0 * (b - a), a);
}